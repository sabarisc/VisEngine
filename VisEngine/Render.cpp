#include "stdafx.h"
#include "Render.h"
#include "Model.h"
#include "Camera.h"
#include <stdio.h>
#include <stdlib.h>

//-------------------------------------------------------------------------------------- 
// Global Variables 
//-------------------------------------------------------------------------------------- 
D3D_DRIVER_TYPE         g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL       g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*           g_pd3dDevice = nullptr;
ID3D11Device1*          g_pd3dDevice1 = nullptr;
ID3D11DeviceContext*    g_pImmediateContext = nullptr;
ID3D11DeviceContext1*   g_pImmediateContext1 = nullptr;
IDXGISwapChain*         g_pSwapChain = nullptr;
IDXGISwapChain1*        g_pSwapChain1 = nullptr;
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;
ID3D11Texture2D*        g_pDepthStencil = nullptr;
ID3D11DepthStencilView* g_pDepthStencilView = nullptr;
ID3D11VertexShader*     g_pVertexShader = nullptr;
ID3D11PixelShader*      g_pPixelShader = nullptr;
ID3D11InputLayout*      g_pVertexLayout = nullptr;
ID3D11Buffer*           g_pVertexBuffer = nullptr;
ID3D11Buffer*			g_pIndexBuffer = nullptr;
ID3D11Buffer*			g_pConstantBuffer = nullptr;
ID3D11ShaderResourceView*           g_pTextureRV = nullptr;
ID3D11SamplerState*                 g_pSamplerLinear = nullptr;
XMMATRIX                g_World;
XMMATRIX                g_View;
XMMATRIX                g_Projection;
static Camera			clCamera;
//-------------------------------------------------------------------------------------- 
// Helper for compiling shaders with D3DCompile 
// 
// With VS 11, we could load up prebuilt .cso files instead... 
//-------------------------------------------------------------------------------------- 
HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG 
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders. 
	// Setting this flag improves the shader debugging experience, but still allows  
	// the shaders to be optimized and to run exactly the way they will run in  
	// the release configuration of this program. 
	dwShaderFlags |= D3DCOMPILE_DEBUG;

	// Disable optimizations to further improve shader debugging 
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif 

	ID3DBlob* pErrorBlob = nullptr;
	hr = D3DCompileFromFile(szFileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, szEntryPoint, szShaderModel,
		dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
			pErrorBlob->Release();
		}
		return hr;
	}
	if (pErrorBlob) pErrorBlob->Release();

	return S_OK;
}

//-------------------------------------------------------------------------------------- 
// Create Direct3D device and swap chain 
//-------------------------------------------------------------------------------------- 
HRESULT InitDevice()
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect( g_hWnd, &rc );
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG 
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif 

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		g_driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDevice(nullptr, g_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
			D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);

		if (hr == E_INVALIDARG)
		{
			// DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it 
			hr = D3D11CreateDevice(nullptr, g_driverType, nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
				D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);
		}

		if (SUCCEEDED(hr))
			break;
	}
	if (FAILED(hr))
		return hr;

	// Obtain DXGI factory from device (since we used nullptr for pAdapter above) 
	IDXGIFactory1* dxgiFactory = nullptr;
	{
		IDXGIDevice* dxgiDevice = nullptr;
		hr = g_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
		if (SUCCEEDED(hr))
		{
			IDXGIAdapter* adapter = nullptr;
			hr = dxgiDevice->GetAdapter(&adapter);
			if (SUCCEEDED(hr))
			{
				hr = adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory));
				adapter->Release();
			}
			dxgiDevice->Release();
		}
	}
	if (FAILED(hr))
		return hr;

	// Create swap chain 
	IDXGIFactory2* dxgiFactory2 = nullptr;
	hr = dxgiFactory->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2));
	if (dxgiFactory2)
	{
		// DirectX 11.1 or later 
		hr = g_pd3dDevice->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void**>(&g_pd3dDevice1));
		if (SUCCEEDED(hr))
		{
			(void)g_pImmediateContext->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&g_pImmediateContext1));
		}

		DXGI_SWAP_CHAIN_DESC1 sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.Width = width;
		sd.Height = height;
		sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = 1;

		hr = dxgiFactory2->CreateSwapChainForHwnd(g_pd3dDevice, g_hWnd, &sd, nullptr, nullptr, &g_pSwapChain1);
		if (SUCCEEDED(hr))
		{
			hr = g_pSwapChain1->QueryInterface(__uuidof(IDXGISwapChain), reinterpret_cast<void**>(&g_pSwapChain));
		}

		dxgiFactory2->Release();
	}
	else
	{
		// DirectX 11.0 systems 
		DXGI_SWAP_CHAIN_DESC sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.BufferCount = 1;
		sd.BufferDesc.Width = width;
		sd.BufferDesc.Height = height;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = g_hWnd;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.Windowed = TRUE;

		hr = dxgiFactory->CreateSwapChain(g_pd3dDevice, &sd, &g_pSwapChain);
	}

	dxgiFactory->MakeWindowAssociation(g_hWnd, DXGI_MWA_NO_ALT_ENTER);

	dxgiFactory->Release();

	if (FAILED(hr))
	{
		return hr;
	}

	// Create a render target view 
	ID3D11Texture2D* pBackBuffer = nullptr;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
	if (FAILED(hr))
	{
		return hr;
	}

	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr))
	{
		return hr;
	}

	// Create depth stencil texture 
	D3D11_TEXTURE2D_DESC descDepth;
	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	hr = g_pd3dDevice->CreateTexture2D(&descDepth, nullptr, &g_pDepthStencil);
	if (FAILED(hr))
	{
		return hr;
	}
	// Create the depth stencil view 
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencil, &descDSV, &g_pDepthStencilView);
	if (FAILED(hr))
	{
		return hr;
	}
	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

	// Setup the viewport 
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pImmediateContext->RSSetViewports(1, &vp);

	// Compile the vertex shader 
	ID3DBlob* pVSBlob = nullptr;
	hr = CompileShaderFromFile(L"Shaders/Standard_VS.hlsl", "VS", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The VS hlsl file cannot be compiled.  Please run this executable from the directory that contains the VS hlsl file.", L"Error", MB_OK);
		return hr;
	}

	// Create the vertex shader 
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pVertexShader);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}

	// Define the input layout 
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	UINT numElements = ARRAYSIZE(layout);

	// Create the input layout 
	hr = g_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(), &g_pVertexLayout);
	pVSBlob->Release();
	if (FAILED(hr))
	{
		return hr;
	}

	// Set the input layout 
	g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

	// Compile the pixel shader 
	ID3DBlob* pPSBlob = nullptr;
	hr = CompileShaderFromFile(L"Shaders/Standard_PS.hlsl", "PS", "ps_4_0", &pPSBlob);
	if( FAILED( hr ) )
	{
		MessageBox(nullptr,
			L"The PS hlsl file cannot be compiled.  Please run this executable from the directory that contains the PS hlsl file.", L"Error", MB_OK);
		return hr;
	}

	// Create the pixel shader 
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader);
	pPSBlob->Release();
	if( FAILED( hr ) )
	{
		return hr;
	}
	//just some initialization data
	SimpleVertex initVertex;
	memset( &initVertex, 0, sizeof( SimpleVertex ) );

	//create the vertex buffer
	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof( bd ) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof( SimpleVertex ) * MAX_NUM_VERTICES;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory( &InitData, sizeof( InitData ) );
	InitData.pSysMem = &initVertex;
	hr = g_pd3dDevice->CreateBuffer( &bd, &InitData, &g_pVertexBuffer );
	if( FAILED( hr ) )
	{
		return hr;
	}

	// Set vertex buffer 
	UINT stride = sizeof( SimpleVertex );
	UINT offset = 0;
	g_pImmediateContext->IASetVertexBuffers( 0, 1, &g_pVertexBuffer, &stride, &offset );

	//some initialization data
	WORD initIndex;
	memset( &initIndex, 0, sizeof( WORD ) );

	//create the index buffer
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof( WORD ) * MAX_NUM_INDICES; 
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	InitData.pSysMem = &initIndex;
	hr = g_pd3dDevice->CreateBuffer( &bd, &InitData, &g_pIndexBuffer );
	if( FAILED( hr ) )
	{
		return hr;
	}

	// Set index buffer 
	g_pImmediateContext->IASetIndexBuffer( g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0 );

	// Create the constant buffer 
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof( ConstantBuffer );
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = g_pd3dDevice->CreateBuffer( &bd, nullptr, &g_pConstantBuffer );
	if( FAILED( hr ) )
	{
		return hr;
	}

	//Create the texture resource view
	hr = CreateDDSTextureFromFile( g_pd3dDevice, L"Textures/seafloor.dds", nullptr, &g_pTextureRV );
	if( FAILED( hr ) )
	{
		return hr;
	}

	// Create the sample state 
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory( &sampDesc, sizeof( sampDesc ) );
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = g_pd3dDevice->CreateSamplerState( &sampDesc, &g_pSamplerLinear );
	if( FAILED( hr ) )
	{
		return hr;
	}

	// Set primitive topology 
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Initialize the projection matrix 
	g_Projection = XMMatrixPerspectiveFovLH( XM_PIDIV2, width / ( FLOAT )height, 0.01f, 100.0f ); //FOV Y is set to 90 degrees

	//setup some starting data for our primitive camera
	Vector3 origin = { 0.0f, 3.0f, -6.0f };
	//Vector3ScaledAdd( origin, -10.0f, clCamera.world.m_4[2] );
	Vector3Copy( origin, clCamera.world.m_4[3] );

	return S_OK;
}

//test function to create a model
void SetupModel( Model &model )
{
	memset( &model, 0, sizeof( Model ) );
	model.world.SetIdentity();
	model.numVerts = 24;
	model.numIndices = 36;

		float x[24][3] =
	{
		{ -1.0f, 1.0f, -1.0f },
		{ 1.0f, 1.0f, -1.0f },
		{1.0f, 1.0f, 1.0f },
		{-1.0f, 1.0f, 1.0f },
		
		{ -1.0f, -1.0f, -1.0f },
		{ 1.0f, -1.0f, -1.0f },
		{ 1.0f, -1.0f, 1.0f },
		{ -1.0f, -1.0f, 1.0f },

		{ -1.0f, -1.0f, 1.0f },
		{ -1.0f, -1.0f, -1.0f },
		{ -1.0f, 1.0f, -1.0f },
		{ -1.0f, 1.0f, 1.0f },

		{ 1.0f, -1.0f, 1.0f },
		{ 1.0f, -1.0f, -1.0f },
		{ 1.0f, 1.0f, -1.0f },
		{ 1.0f, 1.0f, 1.0f },

		{ -1.0f, -1.0f, -1.0f },
		{ 1.0f, -1.0f, -1.0f },
		{ 1.0f, 1.0f, -1.0f },
		{ -1.0f, 1.0f, -1.0f },

		{ -1.0f, -1.0f, 1.0f },
		{ 1.0f, -1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ -1.0f, 1.0f, 1.0f }

	};

	memcpy( model.vertices, x, sizeof( float ) * model.numVerts * 3 );

	float y[24][3] = 
	{
		{0.0f, 1.0f, 0.0f},
		{0.0f, 1.0f, 0.0f},
		{0.0f, 1.0f, 0.0f},
		{0.0f, 1.0f, 0.0f},
		{0.0f, 1.0f, 0.0f},
		{0.0f, 1.0f, 0.0f},
		{0.0f, 1.0f, 0.0f},
		{0.0f, 1.0f, 0.0f},
		{0.0f, 1.0f, 0.0f},
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, -1.0f, 0.0f },
		{ 0.0f, -1.0f, 0.0f },
		{ 0.0f, -1.0f, 0.0f },
		{ 0.0f, -1.0f, 0.0f },
		{ 0.0f, -1.0f, 0.0f },
		{ 0.0f, -1.0f, 0.0f },
		{ 0.0f, -1.0f, 0.0f },
		{ 0.0f, -1.0f, 0.0f },
		{ 0.0f, -1.0f, 0.0f },
		{ 0.0f, -1.0f, 0.0f },
		{ 0.0f, -1.0f, 0.0f },
		{ 0.0f, -1.0f, 0.0f }
	};

	memcpy( model.normals, y, sizeof( float ) * model.numVerts * 3 );

	float z[24][2] = 
	{
		{1.0f, 0.0f},
		{0.0f, 0.0f},
		{0.0f, 1.0f},
		{1.0f, 1.0f},

		{0.0f, 0.0f},
		{1.0f, 0.0f},
		{1.0f, 1.0f},
		{0.0f, 1.0f},

		{0.0f, 1.0f},
		{1.0f, 1.0f},
		{1.0f, 0.0f},
		{0.0f, 0.0f},

		{1.0f, 1.0f},
		{0.0f, 1.0f},
		{0.0f, 0.0f},
		{1.0f, 0.0f},

		{0.0f, 1.0f},
		{1.0f, 1.0f},
		{1.0f, 0.0f},
		{0.0f, 0.0f},

		{1.0f, 1.0f},
		{0.0f, 1.0f},
		{0.0f, 0.0f},
		{1.0f, 0.0f},

	};

	memcpy( model.texcoords, z, sizeof( float ) * model.numVerts * 2 );

	WORD indices[] =
	{
		3,1,0,
		2,1,3,

		6,4,5,
		7,4,6,

		11,9,8,
		10,9,11,

		14,12,13,
		15,12,14,

		19,17,16,
		18,17,19,

		22,20,21,
		23,20,22
	};

	memcpy( model.indices, indices, sizeof( indices ) );
}

//-------------------------------------------------------------------------------------- 
// Setup the buffers for this drawcall 
//--------------------------------------------------------------------------------------
HRESULT SetupDrawCall()
{
	HRESULT hr = S_OK;
	
	Model model;

	SetupModel( model );
	
	// Copy into vertex struct to feed into GPU 
	SimpleVertex vertices[MAX_NUM_VERTICES];
	
	for( int i = 0; i < model.numVerts; i++ )
	{
		vertices[i].Pos = XMFLOAT3( model.vertices[i].x, model.vertices[i].y, model.vertices[i].z );
		vertices[i].Normal = XMFLOAT3( model.normals[i].x, model.normals[i].y, model.normals[i].z );
		vertices[i].Tex = XMFLOAT2( model.texcoords[i].x, model.texcoords[i].y );
	}

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	ZeroMemory( &mappedResource, sizeof( D3D11_MAPPED_SUBRESOURCE ) );

	//	Disable GPU access to the vertex buffer data.
	g_pImmediateContext->Map( g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
	//	Update the vertex buffer here.
	memcpy( mappedResource.pData, vertices, sizeof( vertices ) );
	//	Reenable GPU access to the vertex buffer data.
	g_pImmediateContext->Unmap( g_pVertexBuffer, 0 );

	ZeroMemory( &mappedResource, sizeof( D3D11_MAPPED_SUBRESOURCE ) );

	//	Disable GPU access to the index buffer data.
	g_pImmediateContext->Map( g_pIndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
	//	Update the index buffer here.
	memcpy( mappedResource.pData, model.indices, sizeof( model.indices ) );
	//	Reenable GPU access to the index buffer data.
	g_pImmediateContext->Unmap( g_pIndexBuffer, 0 );

	// Initialize the world matrix 
	g_World = XMMatrixIdentity();

	// Initialize the view matrix 
	XMVECTOR Eye = XMVectorSet( clCamera.world.m_4[3].x, clCamera.world.m_4[3].y, clCamera.world.m_4[3].z, 0.0f );
	XMVECTOR To = XMVectorSet( clCamera.world.m_4[2].x, clCamera.world.m_4[2].y, clCamera.world.m_4[2].z, 0.0f );
	XMVECTOR Up = XMVectorSet( clCamera.world.m_4[1].x, clCamera.world.m_4[1].y, clCamera.world.m_4[1].z, 0.0f );
	g_View = XMMatrixLookToLH( Eye, To, Up );

	return S_OK;
}

//-------------------------------------------------------------------------------------- 
// Cleanup the drawcall
//--------------------------------------------------------------------------------------

void CleanupDrawCall()
{
}

//-------------------------------------------------------------------------------------- 
// Render the frame 
//-------------------------------------------------------------------------------------- 
HRESULT Render()
{
	HRESULT hr = S_OK;
	//Setup the buffers for this drawcall
	if( FAILED( hr = SetupDrawCall() ) )
	{
		return hr;
	}

	// Just clear the backbuffer 
	g_pImmediateContext->ClearRenderTargetView( g_pRenderTargetView, Colors::MidnightBlue );
	// Clear the depth buffer to 1.0 (max depth)  
	g_pImmediateContext->ClearDepthStencilView( g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0 );

	ConstantBuffer cb;
	cb.mWorld = XMMatrixTranspose( g_World );
	cb.mView = XMMatrixTranspose( g_View );
	cb.mProjection = XMMatrixTranspose( g_Projection );
	cb.lightDir = XMFLOAT4( -0.577f, 0.577f, -0.577f, 1.0f );
	cb.lightColor = XMFLOAT4( 1.0f, 1.0f, 1.0f, 1.0f );
	g_pImmediateContext->UpdateSubresource( g_pConstantBuffer, 0, nullptr, &cb, 0, 0 );

	//Draw the triangles
	g_pImmediateContext->VSSetShader( g_pVertexShader, NULL, 0 );
	g_pImmediateContext->VSSetConstantBuffers( 0, 1, &g_pConstantBuffer );
	g_pImmediateContext->PSSetShader( g_pPixelShader, NULL, 0 );
	g_pImmediateContext->PSSetConstantBuffers( 0, 1, &g_pConstantBuffer );
	g_pImmediateContext->PSSetShaderResources( 0, 1, &g_pTextureRV );
	g_pImmediateContext->PSSetSamplers( 0, 1, &g_pSamplerLinear );
	g_pImmediateContext->DrawIndexed( 36, 0, 0 );

	g_pSwapChain->Present( 0, 0 );

	//Do any drawcall cleanup
	CleanupDrawCall();

	return S_OK;
}

//-------------------------------------------------------------------------------------- 
// Clean up the objects we've created 
//-------------------------------------------------------------------------------------- 
void CleanupDevice()
{
	if (g_pImmediateContext) g_pImmediateContext->ClearState();

	if (g_pSamplerLinear) g_pSamplerLinear->Release();
	if (g_pTextureRV) g_pTextureRV->Release();
	if (g_pConstantBuffer) g_pConstantBuffer->Release();
	if (g_pVertexBuffer) g_pVertexBuffer->Release();
	if (g_pIndexBuffer) g_pIndexBuffer->Release();
	if (g_pVertexLayout) g_pVertexLayout->Release();
	if (g_pVertexShader) g_pVertexShader->Release();
	if (g_pPixelShader) g_pPixelShader->Release();
	if (g_pRenderTargetView) g_pRenderTargetView->Release();
	if (g_pDepthStencil) g_pDepthStencil->Release();
	if (g_pDepthStencilView) g_pDepthStencilView->Release();
	if (g_pSwapChain1) g_pSwapChain1->Release();
	if (g_pSwapChain) g_pSwapChain->Release();
	if (g_pImmediateContext1) g_pImmediateContext1->Release();
	if (g_pImmediateContext) g_pImmediateContext->Release();
	if (g_pd3dDevice1) g_pd3dDevice1->Release();
	if (g_pd3dDevice) g_pd3dDevice->Release();
}