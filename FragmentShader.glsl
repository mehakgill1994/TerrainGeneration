#version 410

const vec3 ambientColor = vec3(0.2, 0.2, 0.2);
const vec3 diffuseColor = vec3(0.3, 0.3, 0.4);
const vec3 lightColor = vec3(1.0, 1.0, 1.0);
const float shininess = 10.0f;

in vec3 viewNormal;
in vec3 toEye;
in vec2 uv; 

uniform bool useTexture;
uniform vec3 lightDirection;
uniform sampler2D tex;

out vec4 result;

#define saturate(x) clamp(x, 0, 1)

void main()
{
	result = vec4(0, 0, 0, 1);

	float diffuseFactor = saturate(dot(lightDirection, viewNormal));

	vec3 reflectedRay = reflect(-lightDirection, viewNormal);
	float specularFactor = pow(saturate(dot(reflectedRay, toEye)), shininess);

	if (useTexture)
	{
		result.rgb += (0.4 + diffuseFactor) * texture(tex, uv).rgb;
	}
	else
	{
		result.rgb += ambientColor * lightColor;
		result.rgb += diffuseFactor * diffuseColor * lightColor;
	}

	//result.rgb += specularFactor * lightColor;

	result = saturate(result);
}