/*
 Copyright (c) 2012, The Cinder Project, All rights reserved.

 This code is intended for use with the Cinder C++ library: http://libcinder.org

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
	the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
	the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

#if ! defined( CINDER_GL_ANGLE )
#include "cinder/app/AppImplMswRendererGl.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/Environment.h"
#include "glload/wgl_all.h"
#include "glload/wgl_load.h"
#include "cinder/app/App.h"
#include "cinder/Camera.h"
#include "cinder/Log.h"
#include <windowsx.h>

namespace cinder { namespace app {

bool sMultisampleSupported = false;
int sArbMultisampleFormat;
typedef HGLRC (__stdcall * PFNWGLCREATECONTEXTATTRIBSARB) (HDC hDC, HGLRC hShareContext, const int *attribList);
typedef BOOL (__stdcall * PFNWGLCHOOSEPIXELFORMATARBPROC)(HDC hdc, const int * piAttribIList, const FLOAT * pfAttribFList, UINT nMaxFormats, int * piFormats, UINT * nNumFormats);

AppImplMswRendererGl::AppImplMswRendererGl( App *aApp, RendererGl *aRenderer )
	: AppImplMswRenderer( aApp ), mRenderer( aRenderer )
{
	mRC = 0;
}

void AppImplMswRendererGl::prepareToggleFullScreen()
{
}

void AppImplMswRendererGl::finishToggleFullScreen()
{
}

void AppImplMswRendererGl::defaultResize() const
{
	::RECT clientRect;
	::GetClientRect( mWnd, &clientRect );
	int width = clientRect.right - clientRect.left;
	int height = clientRect.bottom - clientRect.top;

	gl::viewport( 0, 0, width, height );
	gl::setMatricesWindow( width, height );
}

void AppImplMswRendererGl::swapBuffers() const
{
	::SwapBuffers( mDC );
}

void AppImplMswRendererGl::makeCurrentContext()
{
	mCinderContext->makeCurrent();
}


// We can't use the normal mechanism for this test because we don't have a context yet
namespace {
HWND createDummyWindow()
{
	DWORD windowExStyle, windowStyle;

	WNDCLASS	wc;						// Windows Class Structure
	RECT		WindowRect;				// Grabs Rectangle Upper Left / Lower Right Values
	WindowRect.left = 0L;
	WindowRect.right = 640L;
	WindowRect.top = 0L;
	WindowRect.bottom = 480L;

	HINSTANCE instance	= ::GetModuleHandle( NULL );				// Grab An Instance For Our Window
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;	// Redraw On Size, And Own DC For Window.
	wc.lpfnWndProc		= DefWindowProc;						// WndProc Handles Messages
	wc.cbClsExtra		= 0;									// No Extra Window Data
	wc.cbWndExtra		= 0;									// No Extra Window Data
	wc.hInstance		= instance;
	wc.hIcon			= ::LoadIcon( NULL, IDI_WINLOGO );		// Load The Default Icon
	wc.hCursor			= ::LoadCursor( NULL, IDC_ARROW );		// Load The Arrow Pointer
	wc.hbrBackground	= NULL;									// No Background Required For GL
	wc.lpszMenuName		= NULL;									// We Don't Want A Menu
	wc.lpszClassName	= TEXT("FLINTTEMP");

	if( ! ::RegisterClass( &wc ) ) {								// Attempt To Register The Window Class
		DWORD err = ::GetLastError();
		return 0;											
	}
	windowExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	windowStyle = ( WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME );

	::AdjustWindowRectEx( &WindowRect, windowStyle, FALSE, windowExStyle );

	return ::CreateWindowEx( windowExStyle, TEXT("FLINTTEMP"), TEXT("FLINT"), windowStyle, 0, 0, WindowRect.right-WindowRect.left, WindowRect.bottom-WindowRect.top, NULL, NULL, instance, 0 );
}

bool getWglFunctionPointers( PFNWGLCREATECONTEXTATTRIBSARB *resultCreateContextAttribsFnPtr, PFNWGLCHOOSEPIXELFORMATARBPROC *resultChoosePixelFormatFnPtr )
{
	static PFNWGLCREATECONTEXTATTRIBSARB cachedCreateContextAttribsFnPtr = nullptr;
	static PFNWGLCHOOSEPIXELFORMATARBPROC cachedChoosePixelFormatFnPtr = nullptr;
	if( ! cachedCreateContextAttribsFnPtr || ! cachedChoosePixelFormatFnPtr ) {
		static PIXELFORMATDESCRIPTOR pfd = {
			sizeof(PIXELFORMATDESCRIPTOR),				// Size Of This Pixel Format Descriptor
			1,											// Version Number
			PFD_DRAW_TO_WINDOW |						// Format Must Support Window
			PFD_SUPPORT_OPENGL |						// Format Must Support OpenGL
			PFD_DOUBLEBUFFER,							// Must Support Double Buffering
			PFD_TYPE_RGBA,								// Request An RGBA Format
			32,											// Select Our Color Depth
			0, 0, 0, 0, 0, 0,							// Color Bits Ignored
			0,											// No Alpha Buffer
			0,											// Shift Bit Ignored
			0,											// No Accumulation Buffer
			0, 0, 0, 0,									// Accumulation Bits Ignored
			16,											// depth bits
			0,											// stencil bits
			0,											// No Auxiliary Buffer
			PFD_MAIN_PLANE,								// Main Drawing Layer
			0,											// Reserved
			0, 0, 0										// Layer Masks Ignored
		};

		HWND tempWindow = createDummyWindow();
		HDC tempDc = ::GetDC( tempWindow );
		auto pixelFormat = ::ChoosePixelFormat( tempDc, &pfd );
		if( pixelFormat == 0 ) {
			::ReleaseDC( tempWindow, tempDc );
			::DestroyWindow( tempWindow );
			::UnregisterClass( TEXT("FLINTTEMP"), ::GetModuleHandle( NULL ) );
			return false;
		}
		::SetPixelFormat( tempDc, pixelFormat, &pfd );
		auto tempCtx = ::wglCreateContext( tempDc ); 
		::wglMakeCurrent( tempDc, tempCtx );

		cachedCreateContextAttribsFnPtr = (PFNWGLCREATECONTEXTATTRIBSARB) ::wglGetProcAddress( "wglCreateContextAttribsARB" );
		cachedChoosePixelFormatFnPtr = (PFNWGLCHOOSEPIXELFORMATARBPROC) ::wglGetProcAddress( "wglChoosePixelFormatARB" );
		*resultCreateContextAttribsFnPtr = cachedCreateContextAttribsFnPtr;
		*resultChoosePixelFormatFnPtr = cachedChoosePixelFormatFnPtr;
		::wglMakeCurrent( NULL, NULL );
		::wglDeleteContext( tempCtx );

		::ReleaseDC( tempWindow, tempDc );
		::DestroyWindow( tempWindow );
		::UnregisterClass( TEXT("FLINTTEMP"), ::GetModuleHandle( NULL ) );

		if( ! cachedCreateContextAttribsFnPtr || ! cachedChoosePixelFormatFnPtr ) {
			return false;
		}
		else
			return true;
	}
	else {
		*resultCreateContextAttribsFnPtr = cachedCreateContextAttribsFnPtr;
		*resultChoosePixelFormatFnPtr = cachedChoosePixelFormatFnPtr;
		return cachedCreateContextAttribsFnPtr && cachedChoosePixelFormatFnPtr;
	}
}

HGLRC createContext( HDC dc, bool coreProfile, bool debug, int majorVersion, int minorVersion )
{
	HGLRC result = 0;
	static bool initializedLoadOGL = false;

	PFNWGLCREATECONTEXTATTRIBSARB wglCreateContextAttribsARBPtr = NULL;
	PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARBPtr = NULL;
	if( getWglFunctionPointers( &wglCreateContextAttribsARBPtr, &wglChoosePixelFormatARBPtr ) ) {
		int attribList[] = {
			WGL_CONTEXT_MAJOR_VERSION_ARB, majorVersion,
			WGL_CONTEXT_MINOR_VERSION_ARB, minorVersion,
			WGL_CONTEXT_FLAGS_ARB, (debug) ? WGL_CONTEXT_DEBUG_BIT_ARB : 0,
			WGL_CONTEXT_PROFILE_MASK_ARB, (coreProfile) ? WGL_CONTEXT_CORE_PROFILE_BIT_ARB : WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
			0, 0
		};
 
		result = (*wglCreateContextAttribsARBPtr)( dc, 0, attribList );
		return result;
	}
	else {
		throw ExcRendererAllocation( "wglCreateContextAttribsARB / wglChoosePixelFormatARB unavailable" );
	}
}

bool testPixelFormat( HDC dc, int colorSamples, int depthDepth, int msaaSamples, int *resultFormat )
{
	PFNWGLCREATECONTEXTATTRIBSARB wglCreateContextAttribsARBPtr = NULL;
	PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARBPtr = NULL;
	if( ! getWglFunctionPointers( &wglCreateContextAttribsARBPtr, &wglChoosePixelFormatARBPtr ) )
		throw ExcRendererAllocation( "wglCreateContextAttribsARB / wglChoosePixelFormatARB unavailable" );

	float fAttributes[] = {0,0};
	std::vector<int> iAttributes;
	iAttributes.push_back( WGL_DRAW_TO_WINDOW_ARB ); iAttributes.push_back( GL_TRUE );
	iAttributes.push_back( WGL_SUPPORT_OPENGL_ARB ); iAttributes.push_back( GL_TRUE );
	iAttributes.push_back( WGL_ACCELERATION_ARB ); iAttributes.push_back( WGL_FULL_ACCELERATION_ARB );
	iAttributes.push_back( WGL_RED_BITS_ARB ); iAttributes.push_back( colorSamples );
	iAttributes.push_back( WGL_GREEN_BITS_ARB ); iAttributes.push_back( colorSamples );
	iAttributes.push_back( WGL_BLUE_BITS_ARB ); iAttributes.push_back( colorSamples );
	iAttributes.push_back( WGL_DEPTH_BITS_ARB ); iAttributes.push_back( depthDepth );
	iAttributes.push_back( WGL_DOUBLE_BUFFER_ARB ); iAttributes.push_back( GL_TRUE );
	iAttributes.push_back( WGL_SAMPLES_ARB ); iAttributes.push_back( msaaSamples );
	iAttributes.push_back( 0 ); iAttributes.push_back( 0 );

	UINT numFormats;
	int valid = (*wglChoosePixelFormatARBPtr)( dc, iAttributes.data(), fAttributes, 1, resultFormat, &numFormats );
	if( valid && ( numFormats >= 1 ) )
		return true;
	else
		return false;
}

bool setPixelFormat( HDC dc, const RendererGl::Options &options )
{
	int format;
	for( int colorDepth = options.getColorChannelDepth(); colorDepth >= 8; colorDepth -= 2 ) {
		for( int depthDepth = options.getDepthBufferDepth(); depthDepth >= 16; depthDepth -= 8 ) {
			for( int msaaSamples = options.getMsaa(); msaaSamples >= 0; msaaSamples >>= 1 ) {
				if( testPixelFormat( dc, colorDepth, depthDepth, msaaSamples, &format ) )
					goto FOUND;
				if( msaaSamples == 0 )
					break;
			}
		}
	}

	return false;

FOUND:
	PIXELFORMATDESCRIPTOR pfd;
	::DescribePixelFormat( dc, format, sizeof(pfd), &pfd );
	::SetPixelFormat( dc, format, &pfd );
	return true;
}

bool initializeGl( HWND wnd, HDC dc, HGLRC sharedRC, const RendererGl::Options &options, HGLRC *resultRc )
{
	if( ! setPixelFormat( dc, options ) )
		throw ExcRendererAllocation( "Failed to find suitable WGL pixel format" );

	if( ! ( *resultRc = createContext( dc, options.getCoreProfile(), options.getDebug(), options.getVersion().first, options.getVersion().second ) ) ) {
		return false;								
	}

	if( ! ::wglMakeCurrent( dc, *resultRc ) ){					// Try To Activate The Rendering Context
		return false;								
	}

	gl::Environment::setCore();
	gl::env()->initializeFunctionPointers();

	wgl_LoadFunctions( dc );								// Initialize WGL function pointers

	::wglMakeCurrent( NULL, NULL );
	
	if( sharedRC )
		::wglShareLists( sharedRC, *resultRc );

	::wglMakeCurrent( dc, *resultRc );

	return true;
}
} // anonymous namespace

bool AppImplMswRendererGl::initialize( HWND wnd, HDC dc, RendererRef sharedRenderer )
{
	mWnd = wnd;
	mDC = dc;

	RendererGl *sharedRendererGl = dynamic_cast<RendererGl*>( sharedRenderer.get() );
	HGLRC sharedRC = ( sharedRenderer ) ? sharedRendererGl->mImpl->mRC : NULL;

	if( ! initializeGl( wnd, dc, sharedRC, mRenderer->getOptions(), &mRC ) ) {
		return false;
	}

	gl::Environment::setCore();
	auto platformData = std::shared_ptr<gl::Context::PlatformData>( new gl::PlatformDataMsw( mRC, mDC ) );
	platformData->mDebug = mRenderer->getOptions().getDebug();
	platformData->mDebugLogSeverity = mRenderer->getOptions().getDebugLogSeverity();
	platformData->mDebugBreakSeverity = mRenderer->getOptions().getDebugBreakSeverity();
	platformData->mObjectTracking = mRenderer->getOptions().getObjectTracking();
	mCinderContext = gl::Context::createFromExisting( platformData );
	mCinderContext->makeCurrent();

	return true;
}


void AppImplMswRendererGl::kill()
{
	if( mRC ) {											// Do We Have A Rendering Context?
		::wglMakeCurrent( NULL, NULL );					// release The DC And RC Contexts
		::wglDeleteContext( mRC );						// delete The RC
	}

	mRC = 0;
}

} } // namespace cinder::app
#endif // ! defined( CINDER_GL_ANGLE )
