#include "cinder/app/AppBasic.h"
#include "cinder/ImageIo.h"
#include "cinder/dx/dx.h"
#include "cinder/dx/DxTexture.h"
#include "cinder/Font.h"

#include "cinder/dx/HlslProg.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class BasicApp : public AppBasic {
  public:
	void setup();
	void keyDown( KeyEvent event);
	void update();
	void draw();
	
	dx::Texture		mTexture;
	dx::HlslProg	mShader;
	float			mAngle;

	Font		mFont;
};

void BasicApp::setup()
{
	mFont = Font( "Arial", 24.0f );

	mTexture = dx::Texture( loadImage ( loadAsset( "testPattern.png" ) ) );

	try 
	{
		mShader = dx::HlslProg( loadAsset("vert.hlsl"), loadAsset("pixel.hlsl") );
	} catch ( dx::HlslProgCompileExc &exc ) {
		std::cout << "Shader Compile Error: " << std::endl;
		std::cout << exc.what();
	} catch ( ... ) {
		std::cout << "Unable to load shader" << std::endl;
	}

	mAngle = 0.0f;

}

void BasicApp::keyDown( KeyEvent event )
{

}

void BasicApp::update()
{
	mAngle += 0.05f;
}

void BasicApp::draw()
{
	dx::clear( Color( 0, 0, 0 ), true );
	dx::enableAlphaBlending();

	mTexture.bind();
	mShader.bind();

	// *************************************
	// HOW DO I BIND STUFF TO HLSL???
	// *************************************
	// BELOW IS FROM THE GLSL CINDER SAMPLE. I'M USING SIMPLE SHADERS FOR TESTING
	// BUT WHAT ABOUT THE WORLDVIEWPROJECTION MATRIX AND mTexture? HOW DO I KNOW IN THE 
	// SHADER WHAT PARAMETER mTexture.bind() HAS ACTUALLY BOUND TO?

	// mShader.uniform( "tex0", 0 );
	// mShader.uniform( "sampleOffset", Vec2f( cos(mAngle ), sin( mAngle ) ) * ( 3.0f / getWindowWidth() ) );


	dx::drawSolidRect( Rectf(100,100,800,800 ));

	mTexture.unbind();

	std::stringstream s;
	s << "Framerate:" << getAverageFps();
	dx::drawString(s.str(),Vec2f(10.0f,10.0f),Color::white(),mFont);
}

// This line tells Cinder to actually create the application
CINDER_APP_BASIC( BasicApp, RendererDx )