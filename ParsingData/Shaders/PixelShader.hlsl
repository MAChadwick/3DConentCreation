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
	//int lightType;	// 1 dir, 2 area, 3 spot
	float4 lightDirection, lightColor;
	float4x4 viewMatrix, projectionMatrix;
};

cbuffer MeshData : register(b0)
{
	float4x4 worldMatrix;
	OBJ_ATTRIBUTES material;
};


// TODO: Part 4B 
// TODO: Part 4C 
// TODO: Part 4F 
float4 main(float4 posH : SV_POSITION, float3 posW : WORLD, float3 normW : NORMAL) : SV_TARGET
{
	float3 sunAmbient = (0.5, 0.5, 0.5);

	// Calculate directional light + ambient light
	float3 lightRatio = clamp(dot(-lightDirection, normalize(normW)), 0, 1);
	lightRatio = clamp(lightRatio + sunAmbient, 0, 1);
	float3 lightResult = lightColor * lightRatio;

	// Calculate reflected light
	float3 viewDir = normalize(posH - posW);
	float3 halfVector = normalize((-lightDirection) + viewDir);
	float3 intensity = dot(halfVector, normW); // Broke this up because it was hurting my brain to look at it
	intensity = clamp(intensity, 0, 1);
	lightResult += lightColor * pow(intensity, material.Ns) * material.Ks;

	// Add ambient light + diffuse light to reflected light

	return float4(lightResult * material.Kd, 1); // TODO: Part 1A (optional) 

	return float4(material.Kd, 1);
}