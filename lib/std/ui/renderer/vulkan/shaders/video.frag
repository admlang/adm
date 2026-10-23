// Y'CbCr to RGB: BT.601 or BT.709, studio or full range. The formulas are
// std.ui.renderer.opengl's video program's (video.adm, videoFragmentSource),
// which the desktop's OpenGL compiles as GLSL 1.10; the two must stay equal
// so a frame converts the same on both renderers.
#version 450
layout(std140, set = 0, binding = 0) uniform Uniforms {
	float uLumaScale;   // (0)
	float uChromaScale; // (4)
	int uNv12;          // (8)
	int uFull;          // (12)
	int uBt709;         // (16, to 32)
};
layout(set = 0, binding = 1) uniform sampler2D uY;
layout(set = 0, binding = 2) uniform sampler2D uCb;
layout(set = 0, binding = 3) uniform sampler2D uCr;
layout(set = 0, binding = 4) uniform sampler2D uCbCr;
layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 fragColor;

void main() {
	float y = texture(uY, vUV).r * uLumaScale;
	float cb;
	float cr;
	if (uNv12 == 1) {
		vec2 c = texture(uCbCr, vUV).rg;
		cb = c.r;
		cr = c.g;
	} else {
		cb = texture(uCb, vUV).r * uChromaScale;
		cr = texture(uCr, vUV).r * uChromaScale;
	}
	if (uFull == 1) {
		cb = cb - 0.5019608;
		cr = cr - 0.5019608;
	} else {
		y = (y - 0.0627451) * 1.1643836;
		cb = (cb - 0.5019608) * 1.1383929;
		cr = (cr - 0.5019608) * 1.1383929;
	}
	vec3 rgb;
	if (uBt709 == 1) {
		rgb = vec3(y + 1.5748 * cr, y - 0.187324 * cb - 0.468124 * cr, y + 1.8556 * cb);
	} else {
		rgb = vec3(y + 1.402 * cr, y - 0.344136 * cb - 0.714136 * cr, y + 1.772 * cb);
	}
	fragColor = vec4(clamp(rgb, 0.0, 1.0), 1.0);
}
