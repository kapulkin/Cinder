#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Surface.h"
#include "cinder/gl/Texture.h"
#include "cinder/qtime/QuickTimeGl.h"
//#include "cinder/Rand.h"

#include <iostream>

using namespace ci;
using namespace ci::app;
using std::string;

class QTimeIterApp : public App {
 public:
	void prepareSettings( Settings *settings );
	void setup();

	void keyDown( KeyEvent event );
	void fileDrop( FileDropEvent event );

	void update();
	void draw();

	void loadMovieFile( const fs::path &path );

	qtime::MovieSurfaceRef	mMovie;
	SurfaceRef				mSurface;
};


void QTimeIterApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 640, 480 );
	settings->setFullScreen( false );
	settings->setResizable( true );
}

void QTimeIterApp::setup()
{
	fs::path moviePath = getOpenFilePath();
	if( ! moviePath.empty() )
		loadMovieFile( moviePath );
}

void QTimeIterApp::keyDown( KeyEvent event )
{
	if( event.getChar() == 'f' ) {
		setFullScreen( ! isFullScreen() );
	}
	else if( event.getChar() == 'o' ) {
		fs::path moviePath = getOpenFilePath();
		if( ! moviePath.empty() )
			loadMovieFile( moviePath );
	}
	
	// these keys only make sense if there is an active movie
	if( mMovie ) {
		if( event.getCode() == KeyEvent::KEY_LEFT ) {
			mMovie->stepBackward();
		}
		if( event.getCode() == KeyEvent::KEY_RIGHT ) {
			mMovie->stepForward();
		}
		else if( event.getChar() == 's' ) {
			if( mSurface ) {
				fs::path savePath = getSaveFilePath();
				if( ! savePath.empty() ) {
					writeImage( savePath, *mSurface );
				}
			}
		}
		else if( event.getChar() == 'm' ) {
			// jump to the middle frame
			mMovie->seekToTime( mMovie->getDuration() / 2 );
		}
		else if( event.getChar() == ' ' ) {
			if( mMovie->isPlaying() )
				mMovie->stop();
			else
				mMovie->play();
		}
	}
}

void QTimeIterApp::loadMovieFile( const fs::path &moviePath )
{
	try {
		mMovie = qtime::MovieSurface::create( moviePath );

		console() << "Dimensions:" << mMovie->getWidth() << " x " << mMovie->getHeight() << std::endl;
		console() << "Duration:  " << mMovie->getDuration() << " seconds" << std::endl;
		console() << "Frames:    " << mMovie->getNumFrames() << std::endl;
		console() << "Framerate: " << mMovie->getFramerate() << std::endl;
		console() << "Has audio: " << mMovie->hasAudio() << " Has visuals: " << mMovie->hasVisuals() << std::endl;
		mMovie->setLoop( true, true );
		mMovie->seekToStart();
		mMovie->play();
	}
	catch( ci::Exception &exc ) {
		console() << "Exception caught trying to load the movie from path: " << moviePath << ", what: " << exc.what() << std::endl;
	}	
}

void QTimeIterApp::fileDrop( FileDropEvent event )
{
	loadMovieFile( event.getFile( 0 ) );
}

void QTimeIterApp::update()
{
	if( mMovie )
		mSurface = mMovie->getSurface();
}

void QTimeIterApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
	gl::enableAlphaBlending( true );

	if( ( ! mMovie ) || ( ! mSurface ) )
		return;
		
	// We are using OpenGL to draw the frames here, so we'll make a texture out of the surface
	gl::draw( gl::Texture::create( *mSurface ) );
}

CINDER_APP( QTimeIterApp, RendererGl )