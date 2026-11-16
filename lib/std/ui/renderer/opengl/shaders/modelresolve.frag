// The picture of a 3D model from its drawing target: uSamples x uSamples
// premultiplied texels averaged into each pixel and written with straight
// alpha, as std.gfx.scene's CPU renderer averages its samples.
#ifdef ADM_VULKAN
layout(std140, set = 0, binding = 0) uniform Resolve {
	int uSamples; // (0, to 16)
};
layout(set = 0, binding = 1) uniform sampler2D uDrawn;
layout(location = 0) out vec4 oColor;
#else
uniform sampler2D uDrawn;
uniform int uSamples;

out vec4 oColor;
#endif

void main() {
	ivec2 at = ivec2(gl_FragCoord.xy) * uSamples;
	vec4 sum = vec4(0.0);
	for (int y = 0; y < uSamples; y++) {
		for (int x = 0; x < uSamples; x++) {
			sum += texelFetch(uDrawn, at + ivec2(x, y), 0);
		}
	}
	float n = float(uSamples * uSamples);
	oColor = sum.a > 0.0 ? vec4(sum.rgb / sum.a, sum.a / n) : vec4(0.0);
}
