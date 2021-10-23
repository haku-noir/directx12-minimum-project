struct Vertex {
	float4 svpos : SV_POSITION;
	float2 uv : TEXCOORD;
};

cbuffer cbuff0: register(b0) {
	matrix mat;
};

Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);
