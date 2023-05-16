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

	// Vectors for holding object data
	std::vector<H2B::VERTEX> vertices;
	std::vector<unsigned> indices;
	std::vector<H2B::MATERIAL> materials;
	std::vector<H2B::BATCH> batches;
	std::vector<H2B::MESH> meshes;
	std::vector<GW::MATH::GMATRIXF> worldMatrices;
	// TODO: Part 2B 
	SceneData sceneData;
	MeshData meshData;

public:
	Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GDirectX11Surface _d3d)
	{
		// Parse level data from GameLevel.txt
		lParse.ParseGameData("../GameLevel.txt");

		// Parse h2b data and add to local vectors
		for (std::vector<LevelDataParser::Mesh>::iterator itr = lParse.meshes.begin() + 1; itr != lParse.meshes.end(); itr++)
		{
			bool success = hParse.Parse(itr->meshModel.c_str());

			// Since hParse isn't saving each parsed mesh, I am adding them to a local vector to save them.
			if (success)
			{
				vertices.insert(vertices.end(), hParse.vertices.begin(), hParse.vertices.end());
				indices.insert(indices.end(), hParse.indices.begin(), hParse.indices.end());
				materials.insert(materials.end(), hParse.materials.begin(), hParse.materials.end());
				batches.insert(batches.end(), hParse.batches.begin(), hParse.batches.end());
				meshes.push_back(*(hParse.meshes.end() - 1));
				worldMatrices.push_back(itr->world);

				// Subtract this mesh's indexCount from the total indexs to get the index offset (Maybe?)
				//(meshes.end() - 1)->drawInfo.indexOffset = indices.size() - (meshes.end() - 1)->drawInfo.indexCount;
			}
		}

		win = _win;
		d3d = _d3d;

		GW::MATH::GVector::NormalizeF(lightDirection, lightDirection);

		GW::GReturn result = matrixMath.Create();

		// TODO: Part 2A
		GW::MATH::GVECTORF position = { 0, 0.75f, -1.5f, 1};
		GW::MATH::GVECTORF lookAt = {0, 0, 0, 1};

		worldMatrix = InitializeWorldMatrix(GW::MATH::GVECTORF{ 0, -10, 0.25f , 1});

		viewMatrix = InitializeViewMatrix(viewMatrix, position, lookAt);

		projectionMatrix = InitialzieProjectionMatrix(projectionMatrix, 65.0f, 0.1f, 100.0f);

		sceneData = { lightDirection, lightColor, viewMatrix, projectionMatrix };
		meshData.worldMatrix = worldMatrix;

		IntializeGraphics();
	}

private:
	//constructor helper functions
	void IntializeGraphics()
	{
		ID3D11Device* creator;
		d3d.GetDevice((void**)&creator);

		// TODO: Uncomment these once they are reworked
		InitializeVertexBuffer(creator);
		InitializeIndexBuffer(creator);
		InitializeSceneBuffer(creator);
		InitializeMeshBuffer(creator);
		InitializePipeline(creator);

		// free temporary handle
		creator->Release();
	}

	GW::MATH::GMATRIXF InitializeWorldMatrix(GW::MATH::GVECTORF position, GW::MATH::GVECTORF rotation = { 0, 0, 0, 0 })
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

	void InitializeVertexBuffer(ID3D11Device* creator)
	{
		CreateVertexBuffer(creator, &vertices[0], sizeof(H2B::VERTEX) * vertices.size());
	}

	void InitializeIndexBuffer(ID3D11Device* creator)
	{
		int empty[5000] = {};
		CreateIndexBuffer(creator, &indices[0], sizeof(unsigned) * indices.size());
	}

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
		CD3D11_BUFFER_DESC bDesc(sizeInBytes, D3D11_BIND_INDEX_BUFFER);
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

		float indexCount = 0;
		float indexOffset = 0;

		// Iterate through parsed meshes to parse h2b data
		
		// TODO: Redo this to work with multiple meshes
		for (int i = 0; i < meshes.size(); i++)
		{
			float x = (i % 10);
			float y = 0;
			float z = (i % 10);
			MapMeshBufferMatrix(curHandles, worldMatrices[i]);
			MapMeshBufferMaterial(curHandles, materials.at(meshes[i].materialIndex));
			curHandles.context->DrawIndexed(meshes[i].drawInfo.indexCount, meshes[i].drawInfo.indexOffset, 0);
		}

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

	void MapVertexBufferData(PipelineHandles curHandles)
	{
		D3D11_MAPPED_SUBRESOURCE subResource;

		subResource.pData = vertexBuffer.Get();

		curHandles.context->Map(vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subResource);
		memcpy(subResource.pData, &hParse.vertices, sizeof(hParse.vertices));
		curHandles.context->Unmap(vertexBuffer.Get(), 0);
	}

	void MapIndexBufferData(PipelineHandles curHandles)
	{
		D3D11_MAPPED_SUBRESOURCE subResource;

		subResource.pData = indexBuffer.Get();

		curHandles.context->Map(indexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subResource);
		memcpy(subResource.pData, &hParse.indices, sizeof(hParse.indices));
		curHandles.context->Unmap(indexBuffer.Get(), 0);
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
