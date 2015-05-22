/*
 Copyright (c) 2014, The Cinder Project, All rights reserved.
 
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

#include "cinder/Cinder.h"
#include "cinder/gl/Scoped.h"

// This path is not used on 64-bit Mac or Windows. On the Mac we only use this path for >=Mac OS 10.8
#if ( defined( CINDER_MAC ) && ( MAC_OS_X_VERSION_MIN_REQUIRED >= 1080 ) ) || ( defined( CINDER_MSW ) && ( ! defined( _WIN64 ) ) ) || defined( CINDER_COCOA_TOUCH )

#include "cinder/qtime/QuickTimeGlImplAvf.h"
#include "cinder/app/AppBase.h"
#include "cinder/app/RendererGl.h"

#include <CoreVideo/CoreVideo.h>
#include <CoreVideo/CVBase.h>
#if defined( CINDER_MAC )
	#include <CoreVideo/CVOpenGLTextureCache.h>
	#include <CoreVideo/CVOpenGLTexture.h>
#else
	#include <CoreVideo/CVOpenGLESTextureCache.h>
#endif

namespace cinder { namespace qtime {
/////////////////////////////////////////////////////////////////////////////////
// MovieGl
MovieGl::MovieGl()
#if defined( CINDER_COCOA_TOUCH )
	: mVideoTextureRef( nullptr ), mVideoTextureCacheRef( nullptr )
#endif
{
}

MovieGl::MovieGl( const Url& url )
#if defined( CINDER_COCOA_TOUCH )
	: mVideoTextureRef( nullptr ), mVideoTextureCacheRef( nullptr )
#endif
{
	MovieBase::initFromUrl( url );
}

MovieGl::MovieGl( const fs::path& path )
#if defined( CINDER_COCOA_TOUCH )
	: mVideoTextureRef( nullptr ), mVideoTextureCacheRef( nullptr )
#endif
{
	MovieBase::initFromPath( path );
}
	
MovieGl::MovieGl( const MovieLoader &loader )
#if defined( CINDER_COCOA_TOUCH )
	: mVideoTextureRef( nullptr ), mVideoTextureCacheRef( nullptr )
#endif
{
	MovieBase::initFromLoader( loader );
}
		
MovieGl::~MovieGl()
{
	deallocateVisualContext();
}
	
bool MovieGl::hasAlpha() const
{
/*	if( ! mVideoTextureRef )
		return false;
	
	::CVPixelBufferLockBaseAddress( mVideoTextureRef, 0 );
	OSType type = ::CVPixelBufferGetPixelFormatType(mVideoTextureRef);
	::CVPixelBufferUnlockBaseAddress( mVideoTextureRef, 0 );
#if defined ( CINDER_COCOA_TOUCH)
	return (type == kCVPixelFormatType_32ARGB ||
			type == kCVPixelFormatType_32BGRA ||
			type == kCVPixelFormatType_32ABGR ||
			type == kCVPixelFormatType_32RGBA ||
			type == kCVPixelFormatType_64ARGB);
#elif defined ( CINDER_COCOA )
	return (type == k32ARGBPixelFormat || type == k32BGRAPixelFormat);
#endif
	
	/*
	CGColorSpaceRef color_space = CVImageBufferGetColorSpace(mVideoTextureRef);
	size_t components = CGColorSpaceGetNumberOfComponents(color_space);
	return components > 3;
	*/
return false;
}

gl::TextureRef MovieGl::getTexture()
{
	updateFrame();
	
	lock();
	gl::TextureRef result = mTexture;
	unlock();
	
	return result;
}
	
void MovieGl::allocateVisualContext()
{
#if defined( CINDER_COCOA_TOUCH )
	if( ! mVideoTextureCacheRef ) {
		CVReturn err = 0;

		app::RendererGl *renderer = dynamic_cast<app::RendererGl*>( app::AppBase::get()->getRenderer().get() );
		if( renderer ) {
			EAGLContext* context = renderer->getEaglContext();
			err = ::CVOpenGLESTextureCacheCreate( kCFAllocatorDefault, NULL, context, NULL, &mVideoTextureCacheRef );
		}
		else {
			throw AvfTextureErrorExc();
		}
	}	
#endif
}

void MovieGl::deallocateVisualContext()
{
#if defined( CINDER_COCOA_TOUCH )
	if( mVideoTextureRef ) {
		::CFRelease( mVideoTextureRef );
		mVideoTextureRef = nullptr;
	}
	
	if( mVideoTextureCacheRef ) {
		::CVOpenGLESTextureCacheFlush( mVideoTextureCacheRef, 0 );
		::CFRelease( mVideoTextureCacheRef );
		mVideoTextureCacheRef = nullptr;
	}
#endif
}

void MovieGl::newFrame( CVImageBufferRef cvImage )
{
#if defined( CINDER_COCOA_TOUCH )
	CVReturn err = 0;
	::CVPixelBufferLockBaseAddress( cvImage, kCVPixelBufferLock_ReadOnly );

	::CVOpenGLESTextureCacheFlush( mVideoTextureCacheRef, 0 ); // Periodic texture cache flush every frame

	CVOpenGLESTextureRef videoTextureRef;
	err = ::CVOpenGLESTextureCacheCreateTextureFromImage( kCFAllocatorDefault,		 // CFAllocatorRef allocator
															mVideoTextureCacheRef,   // CVOpenGLESTextureCacheRef textureCache
															cvImage,                 // CVImageBufferRef sourceImage
															NULL,                    // CFDictionaryRef textureAttributes
															GL_TEXTURE_2D,           // GLenum target
															GL_RGBA,                 // GLint internalFormat
															mWidth,                  // GLsizei width
															mHeight,                 // GLsizei height
															GL_BGRA,                 // GLenum format
															GL_UNSIGNED_BYTE,        // GLenum type
															0,                       // size_t planeIndex
															&videoTextureRef );      // CVOpenGLESTextureRef *textureOut
	
	if( err )
		throw AvfTextureErrorExc();
	
	GLenum target = ::CVOpenGLESTextureGetTarget( videoTextureRef );
	GLuint name = ::CVOpenGLESTextureGetName( videoTextureRef );
	bool topDown = ::CVOpenGLESTextureIsFlipped( videoTextureRef );
	
	// custom deleter fires when last reference to Texture goes out of scope
	auto deleter = [cvImage, videoTextureRef] ( gl::Texture *texture ) {
		::CVPixelBufferUnlockBaseAddress( cvImage, kCVPixelBufferLock_ReadOnly );
		::CVBufferRelease( cvImage );
		::CFRelease( videoTextureRef );
		delete texture;
	};
	
	mTexture = gl::Texture2d::create( target, name, mWidth, mHeight, true, deleter );
	
//	vec2 t0, lowerRight, t2, upperLeft;
//	::CVOpenGLESTextureGetCleanTexCoords( videoTextureRef, &t0.x, &lowerRight.x, &t2.x, &upperLeft.x );
	mTexture->setWrap( GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE );
	mTexture->setCleanSize( mWidth, mHeight );
	mTexture->setTopDown( topDown );
#elif defined( CINDER_MAC )
	IOSurfaceRef ioSurface = ::CVPixelBufferGetIOSurface( cvImage );

	::IOSurfaceIncrementUseCount( ioSurface );
	GLsizei texWidth = (GLsizei)::IOSurfaceGetWidth( ioSurface );
	GLsizei texHeight = (GLsizei)::IOSurfaceGetHeight( ioSurface );

	if( mAvailableTextures.empty() ) {
		GLuint newName;
		glGenTextures( 1, &newName );
		mAvailableTextures.push_back( newName );
	}

	auto deleter = [cvImage, ioSurface, this] ( gl::Texture *texture ) {
		::IOSurfaceDecrementUseCount( ioSurface );
		mAvailableTextures.push_back( texture->getId() );
		delete texture;
		::CVPixelBufferRelease( cvImage );
	};
	
	GLuint textureName = mAvailableTextures.back();
	mAvailableTextures.pop_back();
	mTexture = gl::Texture2d::create( GL_TEXTURE_RECTANGLE, textureName, texWidth, texHeight, true, deleter );
	mTexture->setTopDown( true );

	gl::ScopedTextureBind bind( mTexture );
	CGLTexImageIOSurface2D( app::AppBase::get()->getRenderer()->getCglContext(), GL_TEXTURE_RECTANGLE, GL_RGBA, texWidth, texHeight, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, ioSurface, 0);        
#endif
}

void MovieGl::releaseFrame()
{
	mTexture.reset();
}
	
} } // namespace cinder::qtime

#endif