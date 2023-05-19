// an ultra simple hlsl pixel shader
// TODO: Part 3A



struct OBJ_ATTRIBUTES
{
	float3       Kd; // diffuse reflectivity
	float	     d; // dissolve (transparency) 
	float3       Ks; // specular reflectivity
	float        Ns; // specular exponent
	float3       Ka; // ambient reflectivity
	float        sharpness; // local reflection map sharpness
	float3       Tf; // transmission filter
	float        Ni; // optical density (index of refraction)
	float3       Ke; // emissive reflectivity
	unsigned int illum; // illumination model
};

cbuffer SceneData : register(b1)
{
	// int			lightType;		// 0 = direction, 1 = area, 2 = spot
	float4		lightRotation;
	float4		lightPosition;
	float3		lightColor;
	float4		cameraPos;
	float4x4	viewMatrix, projectionMatrix;
};

cbuffer MeshData : register(b0)
{
	float4x4	 worldMatrix;
	OBJ_ATTRIBUTES material;
};

float3 CalculateDirectionalLight(float3 normW, float3 posW)
{
	float3 sunAmbient = (0.25, 0.25, 0.35);

	// Calculate directional light + ambient light
	float3 lightRatio = clamp(dot(normalize(-lightPosition), normalize(normW)), 0, 1);
	lightRatio = clamp(lightRatio + sunAmbient, 0, 1);
	return lightColor * lightRatio;
}

float3 CalculatePointLight(float3 normW, float3 posW)
{
	float3 sunAmbient = (0.25, 0.25, 0.25);

	float3 lightDir = normalize(normalize(lightPosition) - posW);

	float3 lightRatio = clamp(dot(lightDir, normalize(normW)), 0, 1);

	lightRatio = clamp(lightRatio + sunAmbient, 0, 1);

	return lightColor * lightRatio;
}


float4 main(float4 posH : SV_POSITION, float3 posW : WORLD, float3 normW : NORMAL) : SV_TARGET
{
	float3 lightResult = CalculateDirectionalLight(normW, posW);

	// Calculate reflected light
	float3 viewDir = normalize(cameraPos - posW);
	float3 halfVector = normalize(normalize(-lightPosition) + viewDir);
	float3 intensity = dot(halfVector, normW); // Broke this up because it was hurting my brain to look at it
	intensity = clamp(intensity, 0, 1); 
	intensity = pow(intensity, material.Ns);
	intensity = max(intensity, 0);

	// Add ambient light + diffuse light to reflected light
	float3 reflectedLight = lightColor * material.Ks * intensity;

	float3 totalLight = (lightResult * material.Kd) + reflectedLight + material.Ke;

	return float4(totalLight, material.d); // TODO: Part 1A (optional) 
}