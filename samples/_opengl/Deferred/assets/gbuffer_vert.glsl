#version 430 core

uniform mat4 	ciModelView;
uniform mat3 	ciNormalMatrix;
uniform mat4 	ciModelViewProjection;
uniform float	uDepthScale;

in vec4 		ciPosition;
in vec3 		ciNormal;
in vec2 		ciTexCoord0;
in vec4 		ciColor;

out Vertex
{
	vec4 		color;
	float 		depth;
	vec3 		normal;
	vec3 		position;
	vec2 		uv;
} vertex;

void main()
{
	vertex.color 	= ciColor;
	vertex.uv 		= ciTexCoord0;
	vertex.position = ( ciModelView * ciPosition ).xyz;
	vec3 n			= ciNormal;
	vertex.normal 	= normalize( ciNormalMatrix * n );
	vertex.depth 	= -vertex.position.z * uDepthScale;

	gl_Position 	= ciModelViewProjection * ciPosition;
}
