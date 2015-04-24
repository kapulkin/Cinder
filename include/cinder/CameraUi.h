/*
 Copyright (c) 2010, The Cinder Project, All rights reserved.
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
#pragma once

#include "cinder/Vector.h"
#include "cinder/Camera.h"
#include "cinder/app/MouseEvent.h"
#include "cinder/app/Window.h"

namespace cinder {

//! Enables user interaction with a CameraPersp via the mouse
class CameraUi {
 public:
	CameraUi()
		: mCamera( nullptr ), mWindowSize( 640, 480 ), mMouseWheelMultiplier( 1.2f )
	{}
	CameraUi( CameraPersp *camera, const app::WindowRef &window = nullptr, int signalPriority = 0 )
		: mCamera( camera ), mWindowSize( 640, 480 ), mMouseWheelMultiplier( 1.2f )
	{
		connect( window, signalPriority );
	}

	CameraUi( const CameraUi &rhs )
		: mCamera( rhs.mCamera ), mWindowSize( rhs.mWindowSize ),
			mWindow( rhs.mWindow ), mMouseWheelMultiplier( rhs.mMouseWheelMultiplier ),
			mSignalPriority( rhs.mSignalPriority )
	{
		connect( mWindow, mSignalPriority );
	}
	
	~CameraUi()
	{
		disconnect();
	}

	CameraUi& operator=( const CameraUi &rhs )
	{
		mCamera = rhs.mCamera;
		mWindowSize = rhs.mWindowSize;
		mMouseWheelMultiplier = rhs.mMouseWheelMultiplier;
		mWindow = rhs.mWindow;
		mSignalPriority = rhs.mSignalPriority;
		connect( mWindow, mSignalPriority );
		return *this;
	}

	//! Connects to mouseDown, mouseDrag, mouseWheel and resize signals of \a window, with optional priority \a signalPriority
	void connect( const app::WindowRef &window, int signalPriority = 0 )
	{
		mWindow = window;
		mSignalPriority = signalPriority;
		if( window ) {
			mMouseDownConnection = window->getSignalMouseDown().connect( signalPriority,
				[this]( app::MouseEvent &event ) { mouseDown( event ); } );
			mMouseDragConnection = window->getSignalMouseDrag().connect( signalPriority,
				[this]( app::MouseEvent &event ) { mouseDrag( event ); } );
			mMouseWheelConnection = window->getSignalMouseWheel().connect( signalPriority,
				[this]( app::MouseEvent &event ) { mouseWheel( event ); } );
			mWindowResizeConnection = window->getSignalResize().connect( signalPriority,
				[this]() {
					setWindowSize( mWindow->getSize() );
					if( mCamera )
						mCamera->setAspectRatio( mWindow->getAspectRatio() );
				}
			);
		}
		else
			disconnect();
	}

	//! Disconnects all signal handlers
	void disconnect()
	{
		mMouseDownConnection.disconnect();
		mMouseDragConnection.disconnect();
		mMouseWheelConnection.disconnect();
		mWindowResizeConnection.disconnect();
		mWindow.reset();
	}

	//! Signal emitted whenever the user modifies the camera
	signals::Signal<void()>&	getSignalCameraChange()
	{
		return mSignalCameraChange;
	}

	void mouseDown( app::MouseEvent &event )
	{
		mouseDown( event.getPos() );
		event.setHandled();
	}

	void mouseWheel( app::MouseEvent &event )
	{
		mouseWheel( event.getWheelIncrement() );
		event.setHandled();
	}

	void mouseDown( const ivec2 &mousePos )
	{
		if( ! mCamera )
			return;

		mInitialMousePos = mousePos;
		mInitialCam = *mCamera;
		mInitialPivotDistance = mCamera->getPivotDistance();
		mLastAction = ACTION_NONE;
	}

	void mouseDrag( app::MouseEvent &event )
	{
		bool isLeftDown = event.isLeftDown();
		bool isMiddleDown = event.isMiddleDown() || event.isAltDown();
		bool isRightDown = event.isRightDown() || event.isControlDown();

		if( isMiddleDown )
			isLeftDown = false;

		mouseDrag( event.getPos(), isLeftDown, isMiddleDown, isRightDown );
		event.setHandled();
	}

	void mouseDrag( const ivec2 &mousePos, bool leftDown, bool middleDown, bool rightDown )
	{
		if( ! mCamera )
			return;

		int action;
		if( rightDown || ( leftDown && middleDown ) )
			action = ACTION_ZOOM;
		else if( middleDown )
			action = ACTION_PAN;
		else if( leftDown )
			action = ACTION_TUMBLE;
		else
			return;
		
		if( action != mLastAction ) {
			mInitialCam = *mCamera;
			mInitialPivotDistance = mCamera->getPivotDistance();
			mInitialMousePos = mousePos;
		}
		
		mLastAction = action;
	
		if( action == ACTION_ZOOM ) { // zooming
			int mouseDelta = ( mousePos.x - mInitialMousePos.x ) + ( mousePos.y - mInitialMousePos.y );

			float newPivotDistance = powf( 2.71828183f, -mouseDelta / length( vec2( getWindowSize() ) ) ) * mInitialPivotDistance;
			vec3 oldTarget = mInitialCam.getEyePoint() + mInitialCam.getViewDirection() * mInitialPivotDistance;
			vec3 newEye = oldTarget - mInitialCam.getViewDirection() * newPivotDistance;
			mCamera->setEyePoint( newEye );
			mCamera->setPivotDistance( newPivotDistance );
		}
		else if( action == ACTION_PAN ) { // panning
			float deltaX = ( mousePos.x - mInitialMousePos.x ) / (float)getWindowSize().x * mInitialPivotDistance;
			float deltaY = ( mousePos.y - mInitialMousePos.y ) / (float)getWindowSize().y * mInitialPivotDistance;
			vec3 right, up;
			mInitialCam.getBillboardVectors( &right, &up );
			mCamera->setEyePoint( mInitialCam.getEyePoint() - right * deltaX + up * deltaY );
		}
		else { // tumbling
			float deltaX = ( mousePos.x - mInitialMousePos.x ) / -100.0f;
			float deltaY = ( mousePos.y - mInitialMousePos.y ) / 100.0f;
			vec3 mW = normalize( mInitialCam.getViewDirection() );
			bool invertMotion = ( mInitialCam.getOrientation() * glm::vec3( 0, 1, 0 ) ).y < 0.0f;
			vec3 mU = normalize( cross( vec3( 0, 1, 0 ), mW ) );

			if( invertMotion ) {
				deltaX = -deltaX;
				deltaY = -deltaY;
			}

			glm::vec3 rotatedVec = glm::angleAxis( deltaY, mU ) * ( -mInitialCam.getViewDirection() * mInitialPivotDistance );
			rotatedVec = glm::angleAxis( deltaX, glm::vec3( 0, 1, 0 ) ) * rotatedVec;

			mCamera->setEyePoint( mInitialCam.getEyePoint() + mInitialCam.getViewDirection() * mInitialPivotDistance + rotatedVec );
			mCamera->setOrientation( glm::angleAxis( deltaX, glm::vec3( 0, 1, 0 ) ) * glm::angleAxis( deltaY, mU ) * mInitialCam.getOrientation() );
		}
		
		mSignalCameraChange.emit();
	}
	
	void mouseWheel( float increment )
	{
		if( ! mCamera )
			return;

		float multiplier;
		if( mMouseWheelMultiplier > 0 )
			multiplier = powf( mMouseWheelMultiplier, increment );
		else
			multiplier = powf( -mMouseWheelMultiplier, -increment );
		vec3 newEye = mCamera->getEyePoint() + mCamera->getViewDirection() * ( mCamera->getPivotDistance() * ( 1 - multiplier ) );
		mCamera->setEyePoint( newEye );
		mCamera->setPivotDistance( mCamera->getPivotDistance() * multiplier );
		
		mSignalCameraChange.emit();
	}
	
	//! Returns a reference to the currently controlled CameraPersp
	const	CameraPersp& getCamera() const		{ return *mCamera; }
	//! Specifices which CameraPersp should be modified
	void	setCamera( CameraPersp *camera )	{ mCamera = camera; }

	//! Sets the size of the window in pixels when no WindowRef is supplied with connect()
	void	setWindowSize( const ivec2 &windowSizePixels ) { mWindowSize = windowSizePixels; }

	//! Sets the multiplier on mouse wheel zooming. Larger values zoom faster. Negative values invert the direction. Default is \c 1.2
	void	setMouseWheelMultiplier( float multiplier ) { mMouseWheelMultiplier = multiplier; }
	//! Returns the multiplier on mouse wheel zooming. Default is \c 1.2.
	float	getMouseWheelMultiplier() const { return mMouseWheelMultiplier; }
	
 private:
	enum		{ ACTION_NONE, ACTION_ZOOM, ACTION_PAN, ACTION_TUMBLE };

	ivec2		getWindowSize()
	{
		if( mWindow )
			return mWindow->getSize();
		else
			return mWindowSize;
	}
 
	ivec2				mInitialMousePos;
	CameraPersp			mInitialCam;
	CameraPersp			*mCamera;
	float				mInitialPivotDistance;
	float				mMouseWheelMultiplier;
	int					mLastAction;
	
	ivec2					mWindowSize; // used when mWindow is null
	app::WindowRef			mWindow;
	int						mSignalPriority;
	signals::Connection		mMouseDownConnection, mMouseDragConnection, mMouseWheelConnection;
	signals::Connection		mWindowResizeConnection;
	signals::Signal<void()>	mSignalCameraChange;
};

}; // namespace cinder
