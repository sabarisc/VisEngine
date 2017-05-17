//constant buffer layout, must match the layout in code
cbuffer ConstantBuffer : register( b0 )
{
	matrix World;
    matrix View;
    matrix Projection;
	float4 lightDir;
	float4 lightColor;
};

// Constant Buffer Variables
Texture2D txDiffuse : register( t0 );
SamplerState samLinear : register( s0 );

struct VS_INPUT
{
	float3 Pos : POSITION;
	float3 Normal : NORMAL;
	float2 Tex: TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 Pos : SV_POSITION;
	float3 Normal : NORMAL;
	float2 Tex : TEXCOORD0;
};