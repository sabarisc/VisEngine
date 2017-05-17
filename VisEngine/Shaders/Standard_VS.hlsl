#include "Standard_Defines.h"

//Vertex shader
VS_OUTPUT VS( VS_INPUT input )
{
	VS_OUTPUT output = (VS_OUTPUT)0;
    output.Pos = mul( float4( input.Pos, 1 ), World );
    output.Pos = mul( output.Pos, View );
    output.Pos = mul( output.Pos, Projection );
	output.Normal = mul( float4( input.Normal, 1 ), World ).xyz;
	output.Tex = input.Tex;
    return output;
}