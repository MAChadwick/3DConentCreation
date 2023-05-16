// an ultra simple hlsl vertex shader
#pragma pack_matrix(row_major) 

// an ultra simple hlsl vertex shader
// TODO: Part 1F
#pragma pack_matrix(row_major) 

struct Vertex
{
	float3 pos : SV_POSITION;
	float3 uvw : TEXCOORD0;
	float3 nrm : NORMAL;
};

// TODO: Part 2B
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

struct OutputToRasterizer
{
	float4 posH : SV_POSITION;	// Position in homogenous projection space
	float3 posW : WORLD;		// Position in world space
	float3 normW : NORMAL;		// Normal in world space
};

cbuffer SceneData : register(b1)
{
	float4 sunDirection, sunColor;
	float4x4 viewMatrix, projectionMatrix;
};

cbuffer MeshData : register(b0)
{
	float4x4 worldMatrix;
	OBJ_ATTRIBUTES material;
};

// TODO: Part 2D 
float4 Rasterize(float4 inputVertex)
{
	inputVertex = mul(inputVertex, worldMatrix);
	inputVertex = mul(inputVertex, viewMatrix);
	return mul(inputVertex, projectionMatrix);
}

// TODO: Part 4A 
// TODO: Part 4B 
OutputToRasterizer main(float3 pos : POSITION, float3 uvw : UVW, float3 nrm : NORMAL)
{
	OutputToRasterizer output;
	output.posH = Rasterize(float4(pos, 1));
	output.posW = mul(pos, worldMatrix);
	output.normW = mul(nrm, worldMatrix);

	return output;
}

