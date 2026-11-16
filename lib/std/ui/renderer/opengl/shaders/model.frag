// One pixel of a 3D model, as std.gfx.scene's CPU renderer shades it
// (raster/shade.adm): glTF's metallic-roughness model in linear light,
// written as sRGB, premultiplied by its alpha (the target is cleared to
// transparent and resolved to straight alpha afterwards).
//
// uMode: 0 a lit surface, 1 the base colour times the vertex colour (lines
// and points), 2 the edge colour as it is (wireframe edges).
//
// uBase is the linear base colour and alpha, uEmissive the linear emission;
// uAlphaMode is 0 opaque, 1 mask, 2 blend, 3 the alpha as computed.
// Per texture (base colour, metallic-roughness, normal, occlusion,
// emissive): uTexMove holds offset u, offset v, rotation and the coordinate
// set it is read with (below zero: the material has none), uTexScale the
// scale of u and v.
// Per light: uLightPlace holds position and kind (0 directional, 1 point,
// 2 spot), uLightAim direction and range, uLightColor colour times intensity
// and the inner cone's cosine, uLightCone the outer cone's cosine.
//
// One text for OpenGL and Vulkan (compiled after `#define ADM_VULKAN`, where
// the uniforms are two std140 blocks the renderer writes at the offsets
// noted: what a picture's draws share, and one surface's).
#ifdef ADM_VULKAN
layout(std140, set = 0, binding = 0) uniform Scene {
	vec4 uEdge;          // (0)
	vec3 uEye;           // (16)
	float uAmbient;      // (28)
	vec3 uForward;       // (32)
	int uOrtho;          // (44)
	int uLightCount;     // (48)
	vec4 uLightPlace[8]; // (64)
	vec4 uLightAim[8];   // (192)
	vec4 uLightColor[8]; // (320)
	vec4 uLightCone[8];  // (448, to 576)
};
layout(std140, set = 0, binding = 1) uniform Surface {
	vec4 uBase;          // (0)
	vec3 uEmissive;      // (16)
	float uMetallic;     // (28)
	float uRoughness;    // (32)
	float uCutoff;       // (36)
	float uNormalScale;  // (40)
	float uOcclusion;    // (44)
	int uMode;           // (48)
	int uAlphaMode;      // (52)
	int uUnlit;          // (56)
	int uHasNormal;      // (60)
	int uHasTangent;     // (64)
	int uHasColor;       // (68)
	vec4 uTexMove[5];    // (80)
	vec4 uTexScale[5];   // (160, to 240)
};
#define SAMPLER(binding_, name_) layout(set = 0, binding = binding_) uniform sampler2D name_
#define VARYING(location_, type_, name_) layout(location = location_) in type_ name_
layout(location = 0) out vec4 oColor;
#else
uniform int uMode;
uniform vec4 uEdge;
uniform vec3 uEye;
uniform vec3 uForward;
uniform int uOrtho;
uniform float uAmbient;
uniform vec4 uBase;
uniform vec3 uEmissive;
uniform float uMetallic;
uniform float uRoughness;
uniform float uCutoff;
uniform float uNormalScale;
uniform float uOcclusion;
uniform int uAlphaMode;
uniform int uUnlit;
uniform int uHasNormal;
uniform int uHasTangent;
uniform int uHasColor;
uniform vec4 uTexMove[5];
uniform vec4 uTexScale[5];
uniform int uLightCount;
uniform vec4 uLightPlace[8];
uniform vec4 uLightAim[8];
uniform vec4 uLightColor[8];
uniform vec4 uLightCone[8];
#define SAMPLER(binding_, name_) uniform sampler2D name_
#define VARYING(location_, type_, name_) in type_ name_
out vec4 oColor;
#endif

SAMPLER(3, uBaseTex);
SAMPLER(4, uRoughTex);
SAMPLER(5, uNormalTex);
SAMPLER(6, uOcclusionTex);
SAMPLER(7, uEmissiveTex);

VARYING(0, vec3, vPosition);
VARYING(1, vec3, vNormal);
VARYING(2, vec4, vTangent);
VARYING(3, vec2, vUV0);
VARYING(4, vec2, vUV1);
VARYING(5, vec4, vColor);

const float pi = 3.14159265358979323846;

vec3 encode(vec3 c) {
	c = clamp(c, 0.0, 1.0);
	vec3 low = c * 12.92;
	vec3 high = 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055;
	return mix(high, low, vec3(lessThanEqual(c, vec3(0.0031308))));
}

bool has(int k) {
	return uTexMove[k].w >= 0.0;
}

vec2 coordinate(int k) {
	vec2 uv = uTexMove[k].w < 0.5 ? vUV0 : vUV1;
	vec2 s = uv * uTexScale[k].xy;
	float cs = cos(uTexMove[k].z);
	float sn = sin(uTexMove[k].z);
	return vec2(cs * s.x - sn * s.y, sn * s.x + cs * s.y) + uTexMove[k].xy;
}

void main() {
	if (uMode == 2) {
		oColor = vec4(uEdge.rgb * uEdge.a, uEdge.a);
		return;
	}
	vec3 base = uBase.rgb;
	float alpha = uBase.a;
	if (uMode == 0 && has(0)) {
		vec4 t = texture(uBaseTex, coordinate(0));
		base *= t.rgb;
		alpha *= t.a;
	}
	if (uHasColor != 0) {
		base *= vColor.rgb;
		alpha *= vColor.a;
	}
	if (uAlphaMode == 1) {
		if (alpha < uCutoff) {
			discard;
		}
		alpha = 1.0;
	} else if (uAlphaMode == 0) {
		alpha = 1.0;
	}
	if (uMode == 1 || uUnlit != 0) {
		oColor = vec4(encode(base) * alpha, alpha);
		return;
	}
	// The view vector, and the normal on the side that is seen.
	vec3 view = uOrtho != 0 ? -uForward : normalize(uEye - vPosition);
	vec3 n;
	if (uHasNormal != 0) {
		n = normalize(vNormal);
		// Front faces are clockwise in the target (see the vertex shader).
		if (gl_FrontFacing) {
			n = -n;
		}
	} else {
		n = normalize(cross(dFdx(vPosition), dFdy(vPosition)));
		if (dot(n, view) < 0.0) {
			n = -n;
		}
	}
	if (uHasTangent != 0 && has(2)) {
		vec3 m = texture(uNormalTex, coordinate(2)).xyz * 2.0 - 1.0;
		m.xy *= uNormalScale;
		vec3 t = vTangent.xyz - n * dot(vTangent.xyz, n);
		float tl = length(t);
		if (tl > 0.0) {
			t /= tl;
			vec3 b = cross(n, t) * (vTangent.w < 0.0 ? -1.0 : 1.0);
			vec3 r = t * m.x + b * m.y + n * m.z;
			float rl = length(r);
			if (rl > 0.0) {
				n = r / rl;
			}
		}
	}
	float metallic = uMetallic;
	float roughness = uRoughness;
	if (has(1)) {
		vec4 t = texture(uRoughTex, coordinate(1));
		roughness *= t.g;
		metallic *= t.b;
	}
	float occlusion = 1.0;
	if (has(3)) {
		occlusion = 1.0 + uOcclusion * (texture(uOcclusionTex, coordinate(3)).r - 1.0);
	}
	vec3 emissive = uEmissive;
	if (has(4)) {
		emissive *= texture(uEmissiveTex, coordinate(4)).rgb;
	}
	metallic = clamp(metallic, 0.0, 1.0);
	roughness = clamp(roughness, 0.04, 1.0);
	float ndotv = max(dot(n, view), 1.0e-4);
	vec3 diffuse = base * (1.0 - metallic);
	vec3 f0 = vec3(0.04 * (1.0 - metallic)) + base * metallic;
	float alpha2 = roughness * roughness * roughness * roughness;
	vec3 lit = (diffuse + f0 * 0.3) * uAmbient * occlusion;
	for (int i = 0; i < 8; i++) {
		if (i >= uLightCount) {
			break;
		}
		vec3 l = -uLightAim[i].xyz;
		float radiance = 1.0;
		if (uLightPlace[i].w > 0.5) {
			l = uLightPlace[i].xyz - vPosition;
			float d2 = dot(l, l);
			float d = sqrt(d2);
			if (d == 0.0) {
				continue;
			}
			l /= d;
			radiance = 1.0 / max(d2, 1.0e-4);
			float range = uLightAim[i].w;
			if (range > 0.0) {
				float f = d / range;
				float window = clamp(1.0 - f * f * f * f, 0.0, 1.0);
				radiance *= window * window;
			}
			if (uLightPlace[i].w > 1.5) {
				float cd = -dot(l, uLightAim[i].xyz);
				float inner = uLightColor[i].w;
				float outer = uLightCone[i].x;
				float span = inner - outer;
				float t = span > 0.0 ? clamp((cd - outer) / span, 0.0, 1.0) : (cd >= outer ? 1.0 : 0.0);
				radiance *= t * t;
			}
		}
		if (radiance <= 0.0) {
			continue;
		}
		float ndotl = dot(n, l);
		if (ndotl <= 0.0) {
			continue;
		}
		vec3 h = l + view;
		float hl = length(h);
		if (hl == 0.0) {
			continue;
		}
		h /= hl;
		float ndoth = max(dot(n, h), 0.0);
		float vdoth = max(dot(view, h), 0.0);
		// Schlick's Fresnel, the GGX distribution and the height-correlated
		// Smith visibility (glTF's appendix B).
		float fw = pow(1.0 - vdoth, 5.0);
		vec3 fresnel = f0 + (1.0 - f0) * fw;
		float dd = ndoth * ndoth * (alpha2 - 1.0) + 1.0;
		float dist = alpha2 / (pi * dd * dd);
		float vis = 0.5 / (ndotl * sqrt(ndotv * ndotv * (1.0 - alpha2) + alpha2) + ndotv * sqrt(ndotl * ndotl * (1.0 - alpha2) + alpha2));
		lit += ((1.0 - fresnel) * diffuse / pi + fresnel * (dist * vis)) * uLightColor[i].rgb * (radiance * ndotl);
	}
	oColor = vec4(encode(lit + emissive) * alpha, alpha);
}
