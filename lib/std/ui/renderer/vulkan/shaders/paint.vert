// The vertex stage of a `@fragment` paint pass (builtin.Fragment.spirv is the
// fragment stage): one triangle over the box's pixels image, giving the
// fragment stage the pixel centre mapped to the paint's space, as the canvas
// shades a fragment paint at (canvas.frag `shaded`). Binding 0 is the
// fragment's own uniform block.
#version 450
layout(std140, set = 0, binding = 1) uniform Placement {
	vec2 uInvX;    // device to paint space: x' = a x + c y + tx  (0)
	vec2 uInvY;    //                        y' = b x + d y + ty  (8)
	vec2 uInvT;    //                                             (16)
	vec2 uOrigin;  // the box's corner on the target               (24)
	vec2 uSize;    // the box's size                               (32, to 48)
};
layout(location = 0) out vec2 vPos;

void main() {
	vec2 p = vec2(float((gl_VertexIndex << 1) & 2), float(gl_VertexIndex & 2));
	gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
	// Interpolated, this is gl_FragCoord.xy plus the corner: the device pixel centre.
	vec2 device = p * uSize + uOrigin;
	vPos = vec2(uInvX.x * device.x + uInvY.x * device.y + uInvT.x, uInvX.y * device.x + uInvY.y * device.y + uInvT.y);
}
