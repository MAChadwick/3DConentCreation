#include <d3dcompiler.h>	// required for compiling shaders on the fly, consider pre-compiling instead
#include "h2bParser.h"
#include "LevelDataParser.h"
#include "ObjectData.h"
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
	GW::MATH::GQUATERNIONF lightRotation;
	GW::MATH::GVECTORF lightPosition;
	GW::MATH::GVECTORF lightColor;
	GW::MATH::GVECTORF cameraPos;
	GW::MATH::GMATRIXF viewMatrix, projectionMatrix;
};

// Information that is unique for each mesh
struct MeshData
{
	GW::MATH::GMATRIXF worldMatrix;
	H2B::MATERIAL material;
};


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
	GW::MATH::GVECTORF lightDirection = { -2, 2, 2, 0 };
	GW::MATH::GVECTORF lightColor = { 0.9f, 0.9f, 1.0f, 1.0f }; // rgba

	// Vectors for holding object data
	std::vector<std::string> meshNames;
	std::vector<H2B::VERTEX> vertices;
	std::vector<unsigned> indices;
	std::vector<ObjectData> objectData;
	std::vector<GW::MATH::GMATRIXF> worldMatrices;

	// Shader data
	SceneData sceneData;
	MeshData meshData;

	// User input
	GW::INPUT::GInput input;
	GW::INPUT::GController controller;

public:
	Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GDirectX11Surface _d3d)
	{
		// Parse level data from GameLevel.txt
		lParse.ParseGameData("../GameLevel.txt");

		// Parse h2b data and add to local vectors
		for (std::vector<LevelDataParser::Mesh>::iterator itr = lParse.meshes.begin(); itr != lParse.meshes.end(); itr++)
		{
			bool success = hParse.Parse(itr->meshModel.c_str());

			// Since hParse isn't saving each parsed mesh, I am adding them to a local vector to save them.
			if (success)
			{
				// Basically creating instances of each object instead of just pasting all of the vertices into one vector
				if (!std::count(meshNames.begin(), meshNames.end(), itr->meshModel))
				{
					vertices.insert(vertices.end(), hParse.vertices.begin(), hParse.vertices.end());
					indices.insert(indices.end(), hParse.indices.begin(), hParse.indices.end());

					meshNames.push_back(itr->meshModel);
				}

				// Individual instances of objects
				ObjectData newObject = { hParse.meshes, hParse.vertices, hParse.indices, hParse.materials, itr->world };

				objectData.push_back(newObject);
			}
		}

		win = _win;
		d3d = _d3d;

		GW::MATH::GVector::NormalizeF(lightDirection, lightDirection);

		GW::GReturn result = matrixMath.Create();

		// TODO: Part 2A
		GW::MATH::GVECTORF position = { 6, 3.5f, 8, 1};
		GW::MATH::GVECTORF lookAt = {1.75f, 0, 3, 1};

		worldMatrix = InitializeWorldMatrix(GW::MATH::GVECTORF{ 0, -10, 0.25f , 1});

		viewMatrix = InitializeViewMatrix(viewMatrix, lParse.camera.world.row4, lookAt);

		projectionMatrix = InitialzieProjectionMatrix(projectionMatrix, 65.0f, 0.1f, 100.0f);

		LevelDataParser::Light newLight = lParse.lights[0];

		sceneData = { newLight.rotation, newLight.position.row4, newLight.color, lParse.camera.world.row4, viewMatrix, projectionMatrix };
		meshData.worldMatrix = worldMatrix;

		// Create input proxies
		input.Create(win);
		controller.Create();

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
		GW::MATH::GVECTORF up = { 0, 1, 0, 1 };

		matrixMath.LookAtLHF(translation, lookAt, up, viewMatrix);

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
		CD3D11_BUFFER_DESC bDesc(sizeInBytes, D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
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

		float indexCount = 0;
		float indexOffset = 0;

		// Render lighting first
		for (int i = 0; i < lParse.lights.size(); i++)
		{
			sceneData.lightColor = lParse.lights[i].color;
			sceneData.lightPosition = lParse.lights[i].position.row4;
			sceneData.lightRotation = lParse.lights[i].rotation;
			// ViewMatrix and CameraPos is updated in UpdateCamera

			MapSceneBufferData(curHandles);
			curHandles.context->Draw(0, 0);
		}

		for (int i = 0; i < objectData.size(); i++)
		{

			MapMeshBufferMatrix(curHandles, objectData[i].objectWorldMatrix);
			MapVertexBufferData(curHandles, objectData[i].objectVerts);
			MapIndexBufferData(curHandles, objectData[i].objectIndices);

			for (int x = 0; x < objectData[i].objectMeshes.size(); x++)
			{
				MapMeshBufferMaterial(curHandles, objectData[i].objectMaterials.at(objectData[i].objectMeshes[x].materialIndex));
				curHandles.context->DrawIndexed(objectData[i].objectMeshes[x].drawInfo.indexCount, objectData[i].objectMeshes[x].drawInfo.indexOffset, 0);
			}
		}

		ReleasePipelineHandles(curHandles);
	}

	void UpdateCamera()
	{
		static std::chrono::steady_clock::time_point last;
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

		float deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(now - last).count() / 1000000.0f;
		last = now;
		// Get Temp copy of view and invert it
		GW::MATH::GMATRIXF temp;
		matrixMath.InverseF(viewMatrix, temp);

		// Get user input for camera
		float totalXChange = 0;
		float totalYChange = 0;
		float totalZChange = 0;
		float totalYawChange = 0;
		float totalPitchChange = 0;
		const float cameraSpeed = 3.0f;

		float keyboardInputs[6] = { 0, 0, 0, 0, 0, 0 }; // space, shift, w, a, s, d
		float mouseInputs[4] = { 0, 0, 0, 0 }; // Left Click, Right Click, x-axis, y-axis
		float controllerInputs[6] = { 0, 0, 0, 0 }; // rt, lt, lsx, lsy, rsx, rsy

		input.GetState(23, keyboardInputs[0]); // space
		input.GetState(14, keyboardInputs[1]); // lshift
		input.GetState(60, keyboardInputs[2]); // w
		input.GetState(38, keyboardInputs[3]); // a
		input.GetState(56, keyboardInputs[4]); // s
		input.GetState(41, keyboardInputs[5]); // d
		input.GetMouseDelta(mouseInputs[2], mouseInputs[3]); // Mouse Axis Delta

		if (mouseInputs[2] >= -1 && mouseInputs[2] <= 1) mouseInputs[2] = 0;
		if (mouseInputs[3] >= -1 && mouseInputs[3] <= 1) mouseInputs[3] = 0;


		controller.GetState(controller, 7, controllerInputs[0]); // rt
		controller.GetState(controller, 6, controllerInputs[1]); // lt
		controller.GetState(controller, 16, controllerInputs[2]); // lsx
		controller.GetState(controller, 17, controllerInputs[3]); // lsy
		controller.GetState(controller, 18, controllerInputs[4]); // rsx
		controller.GetState(controller, 19, controllerInputs[5]); // rsy

		float perFrameSpeed = cameraSpeed * deltaTime;
		float rotationSpeed = deltaTime;
		float thumbstickSpeed = 3.14f * deltaTime;
		float aspectRatio = 0;
		d3d.GetAspectRatio(aspectRatio);

		// Total all input
		totalXChange = (-keyboardInputs[3] + keyboardInputs[5]) - controllerInputs[2];
		totalYChange = (keyboardInputs[0] - keyboardInputs[1]) + (controllerInputs[0] - controllerInputs[1]);
		totalZChange = (keyboardInputs[2] - keyboardInputs[4]) + controllerInputs[3];
		totalPitchChange = (65.0f * (mouseInputs[3] / 600.0f) + (controllerInputs[5] * thumbstickSpeed));
		totalYawChange = (65.0f * aspectRatio * (mouseInputs[2] / 800.0f) + controllerInputs[4] * thumbstickSpeed);

		// Translate camera based on inputs
		matrixMath.TranslateLocalF(temp, GW::MATH::GVECTORF{ totalXChange * perFrameSpeed, totalYChange * perFrameSpeed, totalZChange * perFrameSpeed, 0 }, temp);

		// Rotate camera based on inputs
		GW::MATH::GMATRIXF pitchMatrix = GW::MATH::GIdentityMatrixF;
		GW::MATH::GMATRIXF yawMatrix = GW::MATH::GIdentityMatrixF;
		matrixMath.RotateXLocalF(pitchMatrix, totalPitchChange * rotationSpeed, pitchMatrix);
		matrixMath.RotateYGlobalF(yawMatrix, totalYawChange * rotationSpeed, yawMatrix);

		matrixMath.MultiplyMatrixF(pitchMatrix, temp, temp);

		// Save camera position
		GW::MATH::GVECTORF tempPos = { temp.row4.x, temp.row4.y, temp.row4.z, temp.row4.w };
		matrixMath.MultiplyMatrixF(yawMatrix, temp, temp);

		// Restore camera position
		temp.row4.x = tempPos.x;
		temp.row4.y = tempPos.y;
		temp.row4.z = tempPos.z;
		temp.row4.w = tempPos.w;

		// Set CameraPos in SceneData
		sceneData.cameraPos = tempPos;

		// Invert and reassign the view matrix
		matrixMath.InverseF(temp, viewMatrix);

		sceneData.viewMatrix = viewMatrix;
		sceneData.cameraPos = viewMatrix.row4;
	}

private:
	struct PipelineHandles
	{
		ID3D11DeviceContext* context;
		ID3D11RenderTargetView* targetView;
		ID3D11DepthStencilView* depthStencil;
	};

	void MapSceneBufferData(PipelineHandles curHandles)
	{
		D3D11_MAPPED_SUBRESOURCE subResource;

		subResource.pData = sceneBuffer.Get();

		curHandles.context->Map(sceneBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subResource);
		memcpy(subResource.pData, &sceneData, sizeof(sceneData));
		curHandles.context->Unmap(sceneBuffer.Get(), 0);
	}


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

	void MapVertexBufferData(PipelineHandles curHandles, std::vector<H2B::VERTEX> newVerts)
	{
		D3D11_MAPPED_SUBRESOURCE subResource;

		subResource.pData = vertexBuffer.Get();

		curHandles.context->Map(vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subResource);
		memcpy(subResource.pData, &newVerts[0], sizeof(H2B::VERTEX) * newVerts.size());
		curHandles.context->Unmap(vertexBuffer.Get(), 0);
	}

	void MapIndexBufferData(PipelineHandles curHandles, std::vector<unsigned> newIndices)
	{
		D3D11_MAPPED_SUBRESOURCE subResource;

		subResource.pData = indexBuffer.Get();

		curHandles.context->Map(indexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subResource);
		memcpy(subResource.pData, &newIndices[0], sizeof(unsigned) * newIndices.size());
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
