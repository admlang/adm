// One triangle over the whole level image; vUV runs 0..1 across it, its first
// row at the image's first row.
#version 450
layout(location = 0) out vec2 vUV;

void main() {
	vec2 p = vec2(float((gl_VertexIndex << 1) & 2), float(gl_VertexIndex & 2));
	gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
	vUV = p;
}
