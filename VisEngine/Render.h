#pragma once

#include <d3d11_1.h> 
#include <directxcolors.h>
#include <d3dcompiler.h> 
#include <directxmath.h>
#include "DDSTextureLoader.h"
#include "VisEngine.h"

using namespace DirectX;

//Vertex definition struct
struct SimpleVertex
{
	XMFLOAT3 Pos;
	XMFLOAT3 Normal;
	XMFLOAT2 Tex;
};

//Constant buffer struct
struct ConstantBuffer
{
	XMMATRIX mWorld;
	XMMATRIX mView;
	XMMATRIX mProjection;
	XMFLOAT4 lightDir;
	XMFLOAT4 lightColor;
};

//Function defines
HRESULT Render();
HRESULT InitDevice();
void CleanupDevice();
