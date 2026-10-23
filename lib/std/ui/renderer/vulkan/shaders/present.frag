// Shows a frame in the window: the frame's texel under the pixel, its colour
// multiplied by its alpha (what a desktop that blends windows reads; an
// opaque frame is unchanged), and nothing where the window reaches past the
// frame (resized since the frame began: the next frame fills it).
#version 450
layout(std140, set = 0, binding = 0) uniform Uniforms {
	ivec2 uSize; // the frame's size in pixels
};
layout(set = 0, binding = 1) uniform sampler2D uFrame;
layout(location = 0) out vec4 fragColor;

void main() {
	ivec2 at = ivec2(gl_FragCoord.xy);
	if (at.x >= uSize.x || at.y >= uSize.y) {
		fragColor = vec4(0.0);
		return;
	}
	vec4 c = texelFetch(uFrame, at, 0);
	fragColor = vec4(c.rgb * c.a, c.a);
}
