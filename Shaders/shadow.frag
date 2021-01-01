#version 330 core

in vec2 uv;
in vec3 nor;
in vec3 fragPos;
in vec4 fragPosLightSpace;

uniform sampler2D myTexture;
uniform sampler2D depthMap;
uniform sampler2D noise;
uniform vec3 lightPosition;
uniform float eyeX;
uniform float eyeY;
uniform float eyeZ;
uniform float dissolveThres;

out vec4 FragColor;


float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
	// Solve shadow acne
	float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

	float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(depthMap, 0);
	
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture2D(depthMap, projCoords.xy).r; 

    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

	for(int x = -1; x <= 1; ++x)
	{
		for(int y = -1; y <= 1; ++y)
		{
			float pcfDepth = texture(depthMap, projCoords.xy + vec2(x, y) * texelSize).r; 
			shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
		}    
	}
	shadow /= 9.0;

    // check whether current frag pos is in shadow
    //float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;

    return shadow;
}

void main()
{             
	vec4 noiseTex = texture2D(noise, uv).rgba;

	if(noiseTex.r < dissolveThres)
        discard;

	vec3 viewPos = vec3(eyeX, eyeY, eyeZ);
    vec3 color = texture2D(myTexture, uv).rgb;
	vec3 lightColor = vec3(1.0);
	vec3 normal = normalize(nor);

	//ambient
	vec3 ambient = 0.15 * color;

	// diffuse
	vec3 lightDir = normalize(lightPosition - fragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;

	// specular
	vec3 viewDir = normalize(viewPos - fragPos);
    float spec = 0.0;
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * lightColor;    

	float shadow = ShadowCalculation(fragPosLightSpace, normal, lightDir);
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;

    FragColor = vec4(lighting, 1.0);
}