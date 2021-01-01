#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 textureUV;												//uv

uniform mat4 ModelMatrix;
uniform mat4 ViewMatrix;
uniform mat4 Projection;

uniform mat4 lightViewMatrix;
uniform mat4 lightProjection;

out vec2 uv;
out vec3 nor;
out vec3 fragPos;
out vec4 fragPosLightSpace;

void main()
{
	uv = textureUV;
	nor = transpose(inverse(mat3(ModelMatrix))) * normal;								//normal matrix

	fragPos = vec3(ModelMatrix * vec4(position.x, position.y, position.z, 1.0));
	mat4 lightSpaceMatrix = lightProjection * lightViewMatrix;
	fragPosLightSpace = lightSpaceMatrix * vec4(fragPos, 1.0);

	mat4 MVP = Projection * ViewMatrix * ModelMatrix;
	gl_Position = MVP * vec4(position.x, position.y, position.z, 1.0);
}