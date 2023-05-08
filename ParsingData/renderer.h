#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib") //needed for runtime shader compilation. Consider compiling shaders before runtime 

void PrintLabeledDebugString(const char* label, const char* toPrint)
{
	std::cout << label << toPrint << std::endl;
#if defined WIN32 //OutputDebugStringA is a windows-only function 
	OutputDebugStringA(label);
	OutputDebugStringA(toPrint);
#endif
}

// TODO: Part 1C 
struct VERTEX
{
	float x, y, z, w;
};

// TODO: Part 2B
struct SHADER_VARS
{
	GW::MATH::GMATRIXF world;
	GW::MATH::GMATRIXF view;
	GW::MATH::GMATRIXF projection;
};

// TODO: Part 2G 

class Renderer
{
	// proxy handles
	GW::SYSTEM::GWindow win;
	GW::GRAPHICS::GDirectX11Surface d3d;

	// what we need at a minimum to draw a triangle
	Microsoft::WRL::ComPtr<ID3D11Buffer>		vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11VertexShader>	vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>	pixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>	vertexFormat;

	// TODO: Part 2A 
	GW::MATH::GMatrix matrixMath;
	GW::MATH::GMATRIXF worldMatrix = GW::MATH::GIdentityMatrixF;

	// TODO: Part 2C 
	SHADER_VARS shaderVars;

	// TODO: Part 2D
	Microsoft::WRL::ComPtr<ID3D11Buffer>		shaderBuffer;

	// TODO: Part 2G
	GW::MATH::GMATRIXF viewMatrix = GW::MATH::GIdentityMatrixF;

	// TODO: Part 3A 
	GW::MATH::GMATRIXF projectionMatrix = GW::MATH::GIdentityMatrixF;

	// TODO: Part 3C
	GW::MATH::GMATRIXF worldMatricies[6] = { GW::MATH::GIdentityMatrixF, GW::MATH::GIdentityMatrixF, GW::MATH::GIdentityMatrixF, GW::MATH::GIdentityMatrixF, GW::MATH::GIdentityMatrixF, GW::MATH::GIdentityMatrixF };

	// TODO: Part 4A
	GW::INPUT::GInput input;
	GW::INPUT::GController controller;

public:
	Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GDirectX11Surface _d3d)
	{
		win = _win;
		d3d = _d3d;
		// TODO: Part 2A
		matrixMath.Create();

		GW::MATH::GVECTORF translation = { 0, -0.5f, 0, 0 };
		worldMatrix = InitializeWorldMatrix(worldMatrix, 1.5708f, 0, 0, translation);

		// TODO: Part 2C
		shaderVars.world = worldMatrix;

		// TODO: Part 2G
		translation = { 0.25f, -0.125f, -0.25f };
		viewMatrix = InitializeViewMatrix(viewMatrix, translation);
		shaderVars.view = viewMatrix;

		// TODO: Part 3A
		projectionMatrix = InitialzieProjectionMatrix(projectionMatrix, 65.0f, 0.1f, 100.0f);

		// TODO: Part 3B
		shaderVars.projection = projectionMatrix;

		// TODO: Part 3C
		worldMatricies[0] = worldMatrix;

		// Ceiling
		translation = { 0, 0.5f, 0, 0 };
		worldMatricies[1] = InitializeWorldMatrix(worldMatricies[1], 1.5708f, 0, 0, translation);

		// Back wall
		translation = { 0, 0, -0.5f, 0 };
		worldMatricies[2] = InitializeWorldMatrix(worldMatricies[2], 0, 0, 0, translation);

		// Front wall
		translation = { 0, 0, 0.5f, 0 };
		worldMatricies[3] = InitializeWorldMatrix(worldMatricies[3], 0, 0, 0, translation);

		// Left wall
		translation = { -0.5f, 0, 0, 0 };
		worldMatricies[4] = InitializeWorldMatrix(worldMatricies[4], 0, 1.5708f, 0, translation);

		// Right wall
		translation = { 0.5f, 0, 0, 0 };
		worldMatricies[5] = InitializeWorldMatrix(worldMatricies[5], 0, 1.5708f, 0, translation);

		// TODO: Part 4A
		input.Create(win);
		controller.Create();

		InitializeGraphics();
	}


private:
	//Constructor helper functions 
	GW::MATH::GMATRIXF InitializeWorldMatrix(GW::MATH::GMATRIXF worldMatrix, float rotX, float rotY, float rotZ, GW::MATH::GVECTORF translation)
	{
		matrixMath.RotateXGlobalF(worldMatrix, rotX, worldMatrix);
		matrixMath.RotateYGlobalF(worldMatrix, rotY, worldMatrix);
		matrixMath.RotateZGlobalF(worldMatrix, rotZ, worldMatrix);

		matrixMath.TranslateGlobalF(worldMatrix, translation, worldMatrix);

		return worldMatrix;
	}

	GW::MATH::GMATRIXF InitializeViewMatrix(GW::MATH::GMATRIXF viewMatrix, GW::MATH::GVECTORF translation)
	{
		matrixMath.TranslateLocalF(viewMatrix, translation, viewMatrix);

		GW::MATH::GVECTORF center = { worldMatrix.row4.x, worldMatrix.row4.y, worldMatrix.row4.z, worldMatrix.row4.w };
		GW::MATH::GVECTORF up = { 0, 1, 0, 1 };

		matrixMath.LookAtLHF(translation, center, up, viewMatrix);

		return viewMatrix;
	}

	GW::MATH::GMATRIXF InitialzieProjectionMatrix(GW::MATH::GMATRIXF projecitonMatrix, float vFOV, float zNear, float zFar)
	{
		float aspectRatio = 0;
		d3d.GetAspectRatio(aspectRatio);

		matrixMath.ProjectionDirectXLHF(vFOV, aspectRatio, zNear, zFar, projectionMatrix);

		return projectionMatrix;
	}

	void InitializeGraphics()
	{
		ID3D11Device* creator;
		d3d.GetDevice((void**)&creator);

		InitializeVertexBuffer(creator);

		//TODO: Part 2D
		CreateShaderBuffer(creator, &shaderVars);

		InitializePipeline(creator);

		// free temporary handle
		creator->Release();
	}

	void InitializeVertexBuffer(ID3D11Device* creator)
	{
		// TODO: Part 1B 
		// TODO: Part 1C 
		// TODO: Part 1D 
		VERTEX verts[104];

		int posMod = 0;

		for (int i = 0; i < 104; i += 4)
		{
			verts[i] = VERTEX{ 0.5f, 0.5f - ((1.0f / 25.0f) * posMod), 0, 1.0f };
			verts[i + 1] = VERTEX{ -0.5f, 0.5f - ((1.0f / 25.0f) * posMod), 0, 1.0f };
			verts[i + 2] = VERTEX{ 0.5f - ((1.0f / 25.0f) * posMod), 0.5f, 0, 1.0f };
			verts[i + 3] = VERTEX{ 0.5f - ((1.0f / 25.0f) * posMod), -0.5f, 0, 1.0f };
			posMod++;
		}
		

		CreateVertexBuffer(creator, &verts[0], sizeof(verts));
	}

	void CreateVertexBuffer(ID3D11Device* creator, const void* data, unsigned int sizeInBytes)
	{
		D3D11_SUBRESOURCE_DATA bData = { data, 0, 0 };
		CD3D11_BUFFER_DESC bDesc(sizeInBytes, D3D11_BIND_VERTEX_BUFFER);
		creator->CreateBuffer(&bDesc, &bData, vertexBuffer.GetAddressOf());
	}

	// Part 2D
	void CreateShaderBuffer(ID3D11Device* creator, const void* data)
	{
		D3D11_SUBRESOURCE_DATA bData = { data, 0, 0 };
		CD3D11_BUFFER_DESC bDesc(sizeof(shaderVars), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
		HRESULT err = creator->CreateBuffer(&bDesc, &bData, shaderBuffer.GetAddressOf());
		if (FAILED(err))
		{
			PrintLabeledDebugString("Shadder Buffer Error:\n", " Oops");
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
		// TODO: Part 1C 
		D3D11_INPUT_ELEMENT_DESC attributes[1];

		attributes[0].SemanticName = "POSITION";
		attributes[0].SemanticIndex = 0;
		attributes[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		attributes[0].InputSlot = 0;
		attributes[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		attributes[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		attributes[0].InstanceDataStepRate = 0;

		creator->CreateInputLayout(attributes, ARRAYSIZE(attributes),
			vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
			vertexFormat.GetAddressOf());
	}


public:
	void Render()
	{
		PipelineHandles curHandles = GetCurrentPipelineHandles();
		SetUpPipeline(curHandles);
		// TODO: Part 1B 
		// TODO: Part 1D 
		// TODO: Part 3D

		D3D11_MAPPED_SUBRESOURCE subResource;
		subResource.pData = shaderBuffer.Get();
		
		for (int i = 0; i < 6; i++)
		{
			shaderVars.world = worldMatricies[i];
			curHandles.context->Map(shaderBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subResource);
			memcpy(subResource.pData, &shaderVars, sizeof(shaderVars));
			curHandles.context->Unmap(shaderBuffer.Get(), 0);
			curHandles.context->Draw(104, 0);
		}

		ReleasePipelineHandles(curHandles);
	}

	// TODO: Part 4B
	void UpdateCamera()
	{
		static std::chrono::steady_clock::time_point last;
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

		float deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(now - last).count() / 1000000.0f;
		last = now;

		// TODO: Part 4C 
		// Get Temp copy of view and invert it
		GW::MATH::GMATRIXF temp;
		matrixMath.InverseF(viewMatrix, temp);

		// TODO: Part 4D
		// Get user input for camera
		float totalXChange = 0;
		float totalYChange = 0;
		float totalZChange = 0;
		float totalYawChange = 0;
		float totalPitchChange = 0;
		const float cameraSpeed = 0.3f;

		float keyboardInputs[6] = { 0, 0, 0, 0, 0, 0 }; // space, shift, w, a, s, d
		float mouseInputs[4] = { 0, 0, 0, 0 }; // Left Click, Right Click, x-axis, y-axis
		float controllerInputs[6] = { 0, 0, 0, 0}; // rt, lt, lsx, lsy, rsx, rsy

		input.GetState(23, keyboardInputs[0]); // space
		input.GetState(14, keyboardInputs[1]); // lshift
		input.GetState(60, keyboardInputs[2]); // w
		input.GetState(38, keyboardInputs[3]); // a
		input.GetState(56, keyboardInputs[4]); // s
		input.GetState(41, keyboardInputs[5]); // d
		input.GetMouseDelta(mouseInputs[2], mouseInputs[3]); // Mouse Axis Delta

		controller.GetState(controller, 7, controllerInputs[0]); // rt
		controller.GetState(controller, 6, controllerInputs[1]); // lt
		controller.GetState(controller, 16, controllerInputs[2]); // lsx
		controller.GetState(controller, 17, controllerInputs[3]); // lsy
		controller.GetState(controller, 18, controllerInputs[4]); // rsx
		controller.GetState(controller, 19, controllerInputs[5]); // rsy

		// TODO: Part 4E 
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


		// Invert and reassign the view matrix
		matrixMath.InverseF(temp, viewMatrix);

		shaderVars.view = viewMatrix;
		
	}
	 
	// TODO: Part 4F 
	// TODO: Part 4G 

private:
	struct PipelineHandles
	{
		ID3D11DeviceContext* context;
		ID3D11RenderTargetView* targetView;
		ID3D11DepthStencilView* depthStencil;
	};
	//Render helper functions
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
		SetShaders(handles);
		//TODO: Part 2E
		ID3D11Buffer* const buffs[] = { shaderBuffer.Get() };
		handles.context->VSSetConstantBuffers(0, ARRAYSIZE(buffs), buffs);
		handles.context->IASetInputLayout(vertexFormat.Get());
		handles.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST); //TODO: Part 1B 
	}

	void SetRenderTargets(PipelineHandles handles)
	{
		ID3D11RenderTargetView* const views[] = { handles.targetView };
		handles.context->OMSetRenderTargets(ARRAYSIZE(views), views, handles.depthStencil);
	}

	void SetVertexBuffers(PipelineHandles handles)
	{
		// TODO: Part 1C 
		const UINT strides[] = { sizeof(float) * 4 };
		const UINT offsets[] = { 0 };
		ID3D11Buffer* const buffs[] = { vertexBuffer.Get() };
		handles.context->IASetVertexBuffers(0, ARRAYSIZE(buffs), buffs, strides, offsets);
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
