#version 410

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;

uniform mat4 matWorld;
uniform mat4 matView;
uniform mat4 matProjection;

out vec3 viewNormal;
out vec3 toEye;
out vec2 uv;

void main()
{
	gl_Position = vec4(position, 1.0);
	gl_Position = matWorld * gl_Position;
	gl_Position = matView * gl_Position;
	toEye = -normalize(gl_Position.xyz);
	gl_Position = matProjection * gl_Position;

	mat3 normalTransform = mat3(matView * matWorld);
	viewNormal = normalize(normalTransform * normal);

	uv = texcoord;
}