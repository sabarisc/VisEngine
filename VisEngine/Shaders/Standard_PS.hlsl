#include "Standard_Defines.h"

float4 PS( VS_OUTPUT input ) : SV_Target
{
	float4 returnColor, diffuseColor, ambientColor;
	float4 ambientLightColor = float4( 0.1, 0.1, 0.1, 1.0 );
	
	//dot the light direction with the normal to simulate brightness
	//sample the texcoord
	//multiply with lightColor
	
	diffuseColor = saturate( dot( ( float3 )lightDir, input.Normal ) * txDiffuse.Sample( samLinear, input.Tex ) * lightColor );
	
	//add the ambient color
	ambientColor = saturate( txDiffuse.Sample( samLinear, input.Tex ) * ambientLightColor );
	
	returnColor = saturate( diffuseColor + ambientColor );
	
	return returnColor;
}