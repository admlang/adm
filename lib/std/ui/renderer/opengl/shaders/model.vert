// A vertex of a 3D model (std.gfx.scene GpuMesh), already in world
// coordinates. The picture's top row is the target's first row, as the GPU
// canvas reads its textures, so y is turned over; front faces are then
// clockwise. uLift moves a wireframe edge towards the eye, in depth units.
//
// One text for OpenGL and Vulkan (compiled after `#define ADM_VULKAN`, where
// the uniforms are a std140 block the renderer writes at the offsets noted,
// and inputs and outputs have their locations).
#ifdef ADM_VULKAN
layout(std140, set = 0, binding = 2) uniform View {
	mat4 uViewProjection; // (0)
	float uLift;          // (64, to 80)
};
#define VERTEX_IN(location_, type_, name_) layout(location = location_) in type_ name_
#define VARYING(location_, type_, name_) layout(location = location_) out type_ name_
#else
uniform mat4 uViewProjection;
uniform float uLift;
#define VERTEX_IN(location_, type_, name_) in type_ name_
#define VARYING(location_, type_, name_) out type_ name_
#endif

VERTEX_IN(0, vec3, aPosition);
VERTEX_IN(1, vec3, aNormal);
VERTEX_IN(2, vec4, aTangent);
VERTEX_IN(3, vec2, aUV0);
VERTEX_IN(4, vec2, aUV1);
VERTEX_IN(5, vec4, aColor);

VARYING(0, vec3, vPosition);
VARYING(1, vec3, vNormal);
VARYING(2, vec4, vTangent);
VARYING(3, vec2, vUV0);
VARYING(4, vec2, vUV1);
VARYING(5, vec4, vColor);

void main() {
	vPosition = aPosition;
	vNormal = aNormal;
	vTangent = aTangent;
	vUV0 = aUV0;
	vUV1 = aUV1;
	vColor = aColor;
	vec4 p = uViewProjection * vec4(aPosition, 1.0);
	p.y = -p.y;
	p.z -= 2.0 * uLift * p.w;
#ifdef ADM_VULKAN
	// Vulkan's depth runs from 0 where OpenGL's clip space runs from -1, and
	// a point has the size the shader gives it.
	p.z = (p.z + p.w) * 0.5;
	gl_PointSize = 1.0;
#endif
	gl_Position = p;
}
