#include <d3dcompiler.h>	// required for compiling shaders on the fly, consider pre-compiling instead
#include "h2bParser.h"
#include "LevelDataParser.h"
#pragma comment(lib, "d3dcompiler.lib") 

void PrintLabeledDebugString(const char* label, const char* toPrint)
{
	std::cout << label << toPrint << std::endl;
#if defined WIN32 //OutputDebugStringA is a windows-only function 
	OutputDebugStringA(label);
	OutputDebugStringA(toPrint);
#endif
}

// TODO: Part 2B

// Globally shared scene data that is only needed once per scene
struct SceneData
{
	GW::MATH::GVECTORF sunDirection, sunColor;
	GW::MATH::GMATRIXF viewMatrix, projectionMatrix;
};


// Information that is unique for each mesh
struct MeshData
{
	GW::MATH::GMATRIXF worldMatrix;
	H2B::MATERIAL material;
};

// TODO: Part 4E 

// Creation, Rendering & Cleanup
class Renderer
{

	// proxy handles
	GW::SYSTEM::GWindow win;
	GW::GRAPHICS::GDirectX11Surface d3d;

	// h2b parser
	H2B::Parser hParse = H2B::Parser();
	std::vector<H2B::MESH> meshes = std::vector<H2B::MESH>();

	// Level Data Parser
	LevelDataParser lParse;

	// what we need at a minimum to draw a triangle
	Microsoft::WRL::ComPtr<ID3D11Buffer>		sceneBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer>		meshBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer>		vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer>		indexBuffer;
	Microsoft::WRL::ComPtr<ID3D11VertexShader>	vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>	pixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>	vertexFormat;

	// TODO: Part 2A
	GW::MATH::GMatrix matrixMath; // Proxy for math functions

	GW::MATH::GMATRIXF worldMatrix = GW::MATH::GIdentityMatrixF;
	GW::MATH::GMATRIXF viewMatrix = GW::MATH::GIdentityMatrixF;
	GW::MATH::GMATRIXF projectionMatrix = GW::MATH::GIdentityMatrixF;
	GW::MATH::GVECTORF lightDirection = { -1, -1, 2, 0 };
	GW::MATH::GVECTORF lightColor = { 0.9f, 0.9f, 1.0f, 1.0f }; // rgba


	// TODO: Part 2B 
	SceneData sceneData;
	MeshData meshData;

public:
	Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GDirectX11Surface _d3d)
	{
		// Parse level data from GameLevel.txt
		lParse.ParseGameData("../GameLevel.txt");

		// Parse h2b file data
		for (std::vector<LevelDataParser::Mesh>::iterator mesh = lParse.meshes.begin(); mesh != lParse.meshes.end(); mesh++)
		{
			bool succeed = hParse.Parse(mesh->meshModel.c_str());
			if (succeed)
			{
				meshes.push_back(hParse.meshes[0]);
				std::cout << hParse.meshes[0].drawInfo.indexOffset << std::endl;
			}
		}

		win = _win;
		d3d = _d3d;


		GW::MATH::GVector::NormalizeF(lightDirection, lightDirection);

		GW::GReturn result = matrixMath.Create();

		// TODO: Part 2A
		GW::MATH::GVECTORF position = { 0.75f, 0.25f, -1.5f, 1 };
		GW::MATH::GVECTORF lookAt = { 0.15f, 0.75f, 0, 1 };
		viewMatrix = InitializeViewMatrix(viewMatrix, position, lookAt);

		projectionMatrix = InitialzieProjectionMatrix(projectionMatrix, 65.0f, 0.1f, 100.0f);

		// TODO: Part 2B 
		sceneData = { lightDirection, lightColor, viewMatrix, projectionMatrix };
		meshData.worldMatrix = worldMatrix;

		// TODO: Part 4E 
		IntializeGraphics();
	}

private:
	//constructor helper functions
	void IntializeGraphics()
	{
		ID3D11Device* creator;
		d3d.GetDevice((void**)&creator);

		// TODO: Uncomment these once they are reworked
		//InitializeVertexBuffer(creator);
		//// TODO: Part 1G
		//InitializeIndexBuffer(creator);
		// TODO: Part 2C 
		InitializeSceneBuffer(creator);
		InitializeMeshBuffer(creator);
		InitializePipeline(creator);

		// free temporary handle
		creator->Release();
	}

	GW::MATH::GMATRIXF InitializeWorldMatrix(GW::MATH::GVECTORF position, GW::MATH::GVECTORF rotation)
	{
		GW::MATH::GMATRIXF newWorld = GW::MATH::GIdentityMatrixF;

		matrixMath.TranslateLocalF(newWorld, position, newWorld);

		matrixMath.RotateXGlobalF(newWorld, rotation.x, newWorld);
		matrixMath.RotateYGlobalF(newWorld, rotation.y, newWorld);
		matrixMath.RotateZGlobalF(newWorld, rotation.z, newWorld);

		return newWorld;
	}

	GW::MATH::GMATRIXF InitializeViewMatrix(GW::MATH::GMATRIXF viewMatrix, GW::MATH::GVECTORF translation, GW::MATH::GVECTORF lookAt)
	{
		GW::GReturn greturn = GW::MATH::GMatrix::TranslateLocalF(viewMatrix, translation, viewMatrix);

		GW::MATH::GVECTORF up = { 0, 1, 0, 1 };

		greturn = matrixMath.LookAtLHF(translation, lookAt, up, viewMatrix);

		return viewMatrix;
	}

	GW::MATH::GMATRIXF InitialzieProjectionMatrix(GW::MATH::GMATRIXF projecitonMatrix, float vFOV, float zNear, float zFar)
	{
		float aspectRatio = 0;
		d3d.GetAspectRatio(aspectRatio);

		matrixMath.ProjectionDirectXLHF(vFOV, aspectRatio, zNear, zFar, projectionMatrix);

		return projectionMatrix;
	}

	// TODO: Rework these to be more ambiguous
	//void InitializeVertexBuffer(ID3D11Device* creator)
	//{
	//	// TODO: Part 1C 
	//	CreateVertexBuffer(creator, &FSLogo_vertices[0], sizeof(FSLogo_vertices));
	//}

	//void InitializeIndexBuffer(ID3D11Device* creator)
	//{
	//	CreateIndexBuffer(creator, &FSLogo_indices[0], sizeof(FSLogo_indices));
	//}

	void InitializeMeshBuffer(ID3D11Device* creator)
	{
		CreateMeshBuffer(creator, &meshData, sizeof(meshData));
	}

	void InitializeSceneBuffer(ID3D11Device* creator)
	{
		CreateSceneBuffer(creator, &sceneData, sizeof(sceneData));
	}

	void CreateVertexBuffer(ID3D11Device* creator, const void* data, unsigned int sizeInBytes)
	{
		D3D11_SUBRESOURCE_DATA bData = { data, 0, 0 };
		CD3D11_BUFFER_DESC bDesc(sizeInBytes, D3D11_BIND_VERTEX_BUFFER);
		creator->CreateBuffer(&bDesc, &bData, vertexBuffer.GetAddressOf());
	}

	void CreateIndexBuffer(ID3D11Device* creator, const void* data, unsigned int sizeInBytes)
	{
		D3D11_SUBRESOURCE_DATA bData = { data, 0, 0 };
		CD3D11_BUFFER_DESC bDesc(sizeInBytes, D3D11_BIND_INDEX_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
		creator->CreateBuffer(&bDesc, &bData, indexBuffer.GetAddressOf());
	}

	void CreateMeshBuffer(ID3D11Device* creator, const void* data, unsigned int sizeInBytes)
	{
		D3D11_SUBRESOURCE_DATA bData = { data, 0, 0 };
		CD3D11_BUFFER_DESC bDesc(sizeInBytes, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
		HRESULT err = creator->CreateBuffer(&bDesc, &bData, meshBuffer.GetAddressOf());

		if (FAILED(err))
		{
			std::cout << "Oopsie Poopsie, Mesh bad\n";
			abort();
		}
	}

	void CreateSceneBuffer(ID3D11Device* creator, const void* data, unsigned int sizeInBytes)
	{
		D3D11_SUBRESOURCE_DATA bData = { data, 0, 0 };
		CD3D11_BUFFER_DESC bDesc(sizeInBytes, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
		HRESULT err = creator->CreateBuffer(&bDesc, &bData, sceneBuffer.GetAddressOf());

		if (FAILED(err))
		{
			std::cout << "Oopsie Poopsie, Scene Bad\n";
			abort();
		}
	}

	void InitializePipeline(ID3D11Device* creator)
	{
		UINT compilerFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if _DEBUG
		compilerFlags |= D3DCOMPILE_DEBUG;
#endif
		Microsoft::WRL::ComPtr<ID3DBlob> vsBlob = CompileVertexShader(creator, compilerFlags);
		Microsoft::WRL::ComPtr<ID3DBlob> psBlob = CompilePixelShader(creator, compilerFlags);

		CreateVertexInputLayout(creator, vsBlob);
	}

	Microsoft::WRL::ComPtr<ID3DBlob> CompileVertexShader(ID3D11Device* creator, UINT compilerFlags)
	{
		std::string vertexShaderSource = ReadFileIntoString("../Shaders/VertexShader.hlsl");

		Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, errors;

		HRESULT compilationResult =
			D3DCompile(vertexShaderSource.c_str(), vertexShaderSource.length(),
				nullptr, nullptr, nullptr, "main", "vs_4_0", compilerFlags, 0,
				vsBlob.GetAddressOf(), errors.GetAddressOf());

		if (SUCCEEDED(compilationResult))
		{
			creator->CreateVertexShader(vsBlob->GetBufferPointer(),
				vsBlob->GetBufferSize(), nullptr, vertexShader.GetAddressOf());
		}
		else
		{
			PrintLabeledDebugString("Vertex Shader Errors:\n", (char*)errors->GetBufferPointer());
			abort();
			return nullptr;
		}

		return vsBlob;
	}

	Microsoft::WRL::ComPtr<ID3DBlob> CompilePixelShader(ID3D11Device* creator, UINT compilerFlags)
	{
		std::string pixelShaderSource = ReadFileIntoString("../Shaders/PixelShader.hlsl");

		Microsoft::WRL::ComPtr<ID3DBlob> psBlob, errors;

		HRESULT compilationResult =
			D3DCompile(pixelShaderSource.c_str(), pixelShaderSource.length(),
				nullptr, nullptr, nullptr, "main", "ps_4_0", compilerFlags, 0,
				psBlob.GetAddressOf(), errors.GetAddressOf());

		if (SUCCEEDED(compilationResult))
		{
			creator->CreatePixelShader(psBlob->GetBufferPointer(),
				psBlob->GetBufferSize(), nullptr, pixelShader.GetAddressOf());
		}
		else
		{
			PrintLabeledDebugString("Pixel Shader Errors:\n", (char*)errors->GetBufferPointer());
			abort();
			return nullptr;
		}

		return psBlob;
	}

	void CreateVertexInputLayout(ID3D11Device* creator, Microsoft::WRL::ComPtr<ID3DBlob>& vsBlob)
	{
		// TODO: Part 1E 
		D3D11_INPUT_ELEMENT_DESC attributes[3];

		attributes[0].SemanticName = "POSITION";
		attributes[0].SemanticIndex = 0;
		attributes[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		attributes[0].InputSlot = 0;
		attributes[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		attributes[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		attributes[0].InstanceDataStepRate = 0;

		attributes[1].SemanticName = "UVW";
		attributes[1].SemanticIndex = 0;
		attributes[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		attributes[1].InputSlot = 0;
		attributes[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		attributes[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		attributes[1].InstanceDataStepRate = 0;

		attributes[2].SemanticName = "NORMAL";
		attributes[2].SemanticIndex = 0;
		attributes[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		attributes[2].InputSlot = 0;
		attributes[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		attributes[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		attributes[2].InstanceDataStepRate = 0;

		creator->CreateInputLayout(attributes, ARRAYSIZE(attributes),
			vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
			vertexFormat.GetAddressOf());
	}


public:
	void Render()
	{
		PipelineHandles curHandles = GetCurrentPipelineHandles();

		SetUpPipeline(curHandles);
		// TODO: Part 1H 
		// TODO: Part 3B
		float indexCount = 0;
		float indexOffset = 0;

		// TODO: Part 3C 
		// TODO: Part 4D

		// TODO: Redo this to work with multiple meshes
		//for (int i = 0; i < 2; i++)
		//{
		//	MapMeshBufferMaterial(curHandles, FSLogo_materials[i].attrib);

		//	if (i == 0)
		//	{
		//		MapMeshBufferMatrix(curHandles, GW::MATH::GIdentityMatrixF);
		//	}
		//	else
		//	{
		//		matrixMath.RotateYLocalF(worldMatrix, -0.01f, worldMatrix);
		//		MapMeshBufferMatrix(curHandles, worldMatrix);
		//	}

		//	curHandles.context->DrawIndexed(FSLogo_meshes[i].indexCount, FSLogo_meshes[i].indexOffset, 0);
		//}

		// TODO: Part 1D 2
		ReleasePipelineHandles(curHandles);
	}

private:
	struct PipelineHandles
	{
		ID3D11DeviceContext* context;
		ID3D11RenderTargetView* targetView;
		ID3D11DepthStencilView* depthStencil;
	};

	void MapMeshBufferMatrix(PipelineHandles curHandles, GW::MATH::GMATRIXF newWorld)
	{
		D3D11_MAPPED_SUBRESOURCE subResource;

		subResource.pData = meshBuffer.Get();
		meshData.worldMatrix = newWorld;

		curHandles.context->Map(meshBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subResource);
		memcpy(subResource.pData, &meshData, sizeof(meshData));
		curHandles.context->Unmap(meshBuffer.Get(), 0);
	}

	void MapMeshBufferMaterial(PipelineHandles curHandles, H2B::MATERIAL newMaterial)
	{
		D3D11_MAPPED_SUBRESOURCE subResource;

		subResource.pData = meshBuffer.Get();
		meshData.material = newMaterial;

		curHandles.context->Map(meshBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subResource);
		memcpy(subResource.pData, &meshData, sizeof(meshData));
		curHandles.context->Unmap(meshBuffer.Get(), 0);
	}


	PipelineHandles GetCurrentPipelineHandles()
	{
		PipelineHandles retval;
		d3d.GetImmediateContext((void**)&retval.context);
		d3d.GetRenderTargetView((void**)&retval.targetView);
		d3d.GetDepthStencilView((void**)&retval.depthStencil);
		return retval;
	}

	void SetUpPipeline(PipelineHandles handles)
	{
		SetRenderTargets(handles);
		SetVertexBuffers(handles);
		SetIndexBuffers(handles);
		SetConstantBuffers(handles);
		SetShaders(handles);

		handles.context->IASetInputLayout(vertexFormat.Get());
		handles.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	void SetRenderTargets(PipelineHandles handles)
	{
		ID3D11RenderTargetView* const views[] = { handles.targetView };
		handles.context->OMSetRenderTargets(ARRAYSIZE(views), views, handles.depthStencil);
	}

	void SetConstantBuffers(PipelineHandles handles)
	{
		ID3D11Buffer* const buffs[] = { meshBuffer.Get(), sceneBuffer.Get() };
		handles.context->VSSetConstantBuffers(0, ARRAYSIZE(buffs), buffs);
		handles.context->PSSetConstantBuffers(0, ARRAYSIZE(buffs), buffs);
	}

	void SetVertexBuffers(PipelineHandles handles)
	{
		const UINT strides[] = { sizeof(H2B::VERTEX) }; // TODO: Part 1E 
		const UINT offsets[] = { 0 };
		ID3D11Buffer* const buffs[] = { vertexBuffer.Get() };
		handles.context->IASetVertexBuffers(0, ARRAYSIZE(buffs), buffs, strides, offsets);
	}

	void SetIndexBuffers(PipelineHandles handles)
	{
		handles.context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	}

	void SetShaders(PipelineHandles handles)
	{
		handles.context->VSSetShader(vertexShader.Get(), nullptr, 0);
		handles.context->PSSetShader(pixelShader.Get(), nullptr, 0);
	}

	void ReleasePipelineHandles(PipelineHandles toRelease)
	{
		toRelease.depthStencil->Release();
		toRelease.targetView->Release();
		toRelease.context->Release();
	}


public:
	~Renderer()
	{
		// ComPtr will auto release so nothing to do here yet 
	}
};
