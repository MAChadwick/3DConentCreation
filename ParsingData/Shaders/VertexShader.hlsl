// an ultra simple hlsl vertex shader
#pragma pack_matrix(row_major) 

// TODO: Part 1C 
// TODO: Part 2B 

cbuffer SHADERS_VARS
{
	float4x4 world;
	float4x4 view;
	float4x4 projection;
};

// TODO: Part 2F 
// TODO: Part 2G 
// TODO: Part 3B 

float4 main(float4 inputVertex : POSITION) : SV_POSITION
{
	inputVertex = mul(inputVertex, world);
	inputVertex = mul(inputVertex, view);
	return mul(inputVertex, projection);
}