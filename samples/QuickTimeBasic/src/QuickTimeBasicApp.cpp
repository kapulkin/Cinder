#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Surface.h"
#include "cinder/gl/Texture.h"
#include "cinder/qtime/QuickTimeGl.h"
#include "cinder/Text.h"
#include "cinder/Utilities.h"
#include "cinder/ImageIo.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class QuickTimeSampleApp : public App {
 public:
	void setup();

	void keyDown( KeyEvent event );
	void fileDrop( FileDropEvent event );

	void update();
	void draw();

	void loadMovieFile( const fs::path &path );

	gl::TextureRef			mFrameTexture, mInfoTexture, mGrabbedFrame;
	qtime::MovieGlRef		mMovie;
};

void QuickTimeSampleApp::setup()
{
	fs::path moviePath = getOpenFilePath();
	if( ! moviePath.empty() )
		loadMovieFile( moviePath );
}

void QuickTimeSampleApp::keyDown( KeyEvent event )
{
	if( event.getChar() == 'f' ) {
		setFullScreen( ! isFullScreen() );
	}
	else if( event.getChar() == 'o' ) {
		fs::path moviePath = getOpenFilePath();
		if( ! moviePath.empty() )
			loadMovieFile( moviePath );
	}
	else if( event.getChar() == '1' )
		mMovie->setRate( 0.5f );
	else if( event.getChar() == '2' )
		mMovie->setRate( 2 );
	else if( event.getChar() == 'x' ) {
		mMovie.reset();
		mFrameTexture.reset();
	}
	else if( event.getChar() == 'g' ) {
		if( mGrabbedFrame )
			mGrabbedFrame.reset();
		else
			mGrabbedFrame = mMovie->getTexture();
	}
}

void QuickTimeSampleApp::loadMovieFile( const fs::path &moviePath )
{
	try {
		// load up the movie, set it to loop, and begin playing
		mMovie = qtime::MovieGl::create( moviePath );
		mMovie->setLoop();
		mMovie->play();
		
		// create a texture for showing some info about the movie
		TextLayout infoText;
		infoText.clear( ColorA( 0.2f, 0.2f, 0.2f, 0.5f ) );
		infoText.setColor( Color::white() );
		infoText.addCenteredLine( moviePath.filename().string() );
		infoText.addLine( toString( mMovie->getWidth() ) + " x " + toString( mMovie->getHeight() ) + " pixels" );
		infoText.addLine( toString( mMovie->getDuration() ) + " seconds" );
		infoText.addLine( toString( mMovie->getNumFrames() ) + " frames" );
		infoText.addLine( toString( mMovie->getFramerate() ) + " fps" );
		infoText.setBorder( 4, 2 );
		mInfoTexture = gl::Texture::create( infoText.render( true ) );
	}
	catch( ci::Exception &exc ) {
		console() << "Exception caught trying to load the movie from path: " << moviePath << ", what: " << exc.what() << std::endl;
		mMovie.reset();
		mInfoTexture.reset();
	}

	mFrameTexture.reset();
}

void QuickTimeSampleApp::fileDrop( FileDropEvent event )
{
	loadMovieFile( event.getFile( 0 ) );
}

void QuickTimeSampleApp::update()
{
	if( mMovie )
		mFrameTexture = mMovie->getTexture();
}

void QuickTimeSampleApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
	gl::enableAlphaBlending();

	if( mGrabbedFrame )
		gl::draw( mGrabbedFrame );
	else if( mFrameTexture ) {
		Rectf centeredRect = Rectf( mFrameTexture->getBounds() ).getCenteredFit( getWindowBounds(), true );
		gl::draw( mFrameTexture, centeredRect );
	}

	if( mInfoTexture ) {
		gl::draw( mInfoTexture, vec2( 20, getWindowHeight() - 20 - mInfoTexture->getHeight() ) );
	}
}

CINDER_APP( QuickTimeSampleApp, RendererGl );