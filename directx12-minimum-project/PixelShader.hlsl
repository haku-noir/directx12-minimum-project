#include "Header.hlsli"

float4 main(Vertex input) : SV_TARGET
{
	return float4(tex.Sample(smp, input.uv));
}