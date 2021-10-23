struct Vertex {
	float4 svpos : SV_POSITION;
};

cbuffer cbuff0: register(b0) {
	matrix mat;
};