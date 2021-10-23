#include "Header.hlsli"

Vertex main(float4 pos : POSITION)
{
	Vertex output;
	output.svpos = mul(mat, pos);
	return output;
}