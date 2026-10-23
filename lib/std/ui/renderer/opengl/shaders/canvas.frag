// The canvas's compositing (std.gfx.canvas fillCoverage, blendPixel,
// composePixel, the blend tables, compositeRowWith and blit) for one pixel of
// a GpuFrame operation. Pixels are straight sRGB RGBA8; every path rounds as
// the canvas does, in the precision it computes in (float32 for composePixel,
// double for the blend formulas and blits).
//
// Prepended: `#version`, and for a shader paint `#define ADM_FRAGMENT`, the
// fragment's GLSL and `#define ADM_ENTRY <entry>`; for Vulkan `#define
// ADM_VULKAN`, which puts the uniforms below in one std140 block (set 0,
// binding 0; std.ui.renderer.vulkan writes it at the offsets noted) and the
// samplers at the bindings named, so the one text serves both APIs.
#ifdef ADM_VULKAN
#define UNIFORMS_BEGIN layout(std140, set = 0, binding = 0) uniform Uniforms {
#define UNIFORM
#define UNIFORMS_END };
#define SAMPLER(binding_, type_, name_) layout(set = 0, binding = binding_) uniform type_ name_
#define FRAG_OUT layout(location = 0) out vec4 fragColor
#else
#define UNIFORMS_BEGIN
#define UNIFORM uniform
#define UNIFORMS_END
#define SAMPLER(binding_, type_, name_) uniform type_ name_
#define FRAG_OUT out vec4 fragColor
#endif

UNIFORMS_BEGIN
UNIFORM ivec4 uBox;             // x, y, w, h on the target                                  (0)
UNIFORM uvec4 uTint;            //                                                           (16)
UNIFORM ivec2 uPixelsOrigin;    // target pixel of uPixels's texel (0, 0)                     (32)
UNIFORM ivec2 uImageSize;       // uImage's texels                                            (40)
UNIFORM ivec2 uImageSpread;     // spread across and down                                     (48)
UNIFORM vec2 uInvX;             // device to paint space: x' = a x + c y + tx                 (56)
UNIFORM vec2 uInvY;             //                        y' = b x + d y + ty                 (64)
UNIFORM vec2 uInvT;             //                                                           (72)
UNIFORM uvec3 uSolid;           //                                                           (80)
UNIFORM uint uSolidA;           // the solid's alpha where it is shaded (atop, plane layers)  (92)
UNIFORM uint uAlpha;            //                                                           (96)
UNIFORM int uComposite;         //                                                           (100)
UNIFORM int uBlend;             //                                                           (104)
UNIFORM int uPaired;            // shaded source-over through the pair table                  (108)
UNIFORM int uOp;                // 0 fill, 1 layer, 2 blit copy, 3 blit, 4 atop, 5 colour matrix (112)
UNIFORM int uPaintKind;         // 0 solid, 1 pixels, 2 fragment, 3 gradient, 4 noise, 5 solid by plane, 6 image (116)
UNIFORM int uHasCoverage;       //                                                           (120)
UNIFORM int uHasClip;           //                                                           (124)
UNIFORM int uCoverageFromPlane; //                                                           (128)
UNIFORM int uHasQuad;           // coverage from a convex quadrilateral                       (132)
UNIFORM int uHardQuad;          // ... or the path's, without antialias                       (136)
UNIFORM int uEdgeBands;         // coverage from the path's edges: their bands, 0 for none    (140)
UNIFORM int uEvenOdd;           // ... filled even-odd, else nonzero                          (144)
UNIFORM int uGradKind;          // 0 linear, 1 radial, 2 conic                                (148)
UNIFORM int uSpread;            // 0 pad, 1 repeat, 2 reflect                                 (152)
UNIFORM int uOctaves;           //                                                           (156)
UNIFORM int uSampling;          // 0 nearest, 1 bilinear, 2 bicubic                           (160)
UNIFORM int uTinted;            //                                                           (164)
UNIFORM double uQuad[8];        // std140: 16 bytes an element                                (176)
UNIFORM double uParams[8];      //                                                           (304)
UNIFORM vec4 uMatrix[5];        // colour matrix columns                                      (432, to 512)
UNIFORMS_END

// The target's pixel before this operation: a copy of its box, or under
// Vulkan's ADM_LOCAL_READ the attachment itself read back within the render
// pass (one pass for a run of draws, a barrier between them).
#ifdef ADM_LOCAL_READ
layout(input_attachment_index = 0, set = 0, binding = 1) uniform subpassInput uDst;
#define DST_PIXEL subpassLoad(uDst)
#else
SAMPLER(1, sampler2D, uDst);
#define DST_PIXEL texelFetch(uDst, local, 0)
#endif
SAMPLER(2, sampler2D, uCoverage);
SAMPLER(3, sampler2D, uClip);
SAMPLER(4, sampler2D, uPixels);
SAMPLER(5, sampler2D, uBlueNoise);  // 64 x 64 dither bytes
SAMPLER(6, sampler2D, uPlane);      // an effect plane over the box (R32F)
SAMPLER(7, sampler2D, uImage);      // an image level, straight sRGB RGBA8
SAMPLER(8, sampler2D, uToLinear);   // 256 x 1: srgbToLinearTable
SAMPLER(9, sampler2D, uToSrgb);     // 4096 x 1: linearToSrgbTable / 255
SAMPLER(10, usampler2D, uTable);    // gradient colours, 8.8 fixed point
SAMPLER(11, isampler2D, uPerm);     // noise hashes
SAMPLER(12, sampler2D, uEdges);     // a path's edges (RGBA32F, EdgeRow texels a row): per band of
                                    // EdgeBandRows rows its first edge and count, then x0 y0 x1 y1

FRAG_OUT;

// Division of doubles as IEEE 754 rounds it. A driver's double division may
// be off by an ulp (NVIDIA's Vulkan compiler's is, within what Vulkan asks of
// 64-bit floats), which flips the canvas's roundings at ties (HardMix's
// threshold, Average's halves); the quotient, its exact fused remainder and
// one fused correction give the rounded quotient, which the suites check
// against the CPU canvas. OpenGL's drivers divide exactly.
#ifdef ADM_VULKAN
double ddiv(double a, double b) {
	double q = a / b;
	if (b == 0.0lf) return q;
	double r = fma(-q, b, a);
	return fma(r, 1.0lf / b, q);
}

dvec3 ddiv(dvec3 a, double b) {
	return dvec3(ddiv(a.x, b), ddiv(a.y, b), ddiv(a.z, b));
}
#else
#define ddiv(a, b) ((a) / (b))
#endif

const int SourceOver = 0, SourceIn = 1, SourceOut = 2, SourceAtop = 3, DestinationOver = 4,
	DestinationIn = 5, DestinationOut = 6, DestinationAtop = 7, XorOp = 8, Copy = 9, Clear = 10, Plus = 11;

const int Normal = 0, Dissolve = 1, Darken = 2, Multiply = 3, ColorBurn = 4, LinearBurn = 5,
	DarkerColor = 6, Lighten = 7, Screen = 8, ColorDodge = 9, LinearDodge = 10, LighterColor = 11,
	Overlay = 12, SoftLight = 13, HardLight = 14, VividLight = 15, LinearLight = 16, PinLight = 17,
	HardMix = 18, Difference = 19, Exclusion = 20, Subtract = 21, Divide = 22, XorMode = 23,
	Average = 24, Negation = 25, Hue = 26, Saturation = 27, ColorMode = 28, Luminosity = 29;

uint div255(uint x) {
	uint y = x + 128u;
	return (y + (y >> 8)) >> 8;
}

uvec3 div255(uvec3 x) {
	uvec3 y = x + 128u;
	return (y + (y >> 8)) >> 8;
}

uint byteOf(float v) {
	return uint(v * 255.0 + 0.5);
}

uvec4 bytesOf(vec4 v) {
	return uvec4(v * 255.0 + 0.5);
}

// colorspace.clampU8: limited to 0..255, halves up.
uint clampU8(double x) {
	return uint(clamp(x, 0.0lf, 255.0lf) + 0.5lf);
}

// blendPixel: source-over of a straight colour with alpha a.
uvec4 blendPixel(uvec4 d, uvec3 c, uint a) {
	uint da = d.a;
	if (a >= 255u || da == 0u) {
		if (a == 0u) {
			return d;
		}
		return uvec4(c, a);
	}
	if (a == 0u) {
		return d;
	}
	uint inv = 255u - a;
	if (da == 255u) {
		return uvec4(div255(c * a + d.rgb * inv), 255u);
	}
	uint dw = da * inv;
	uint total = a * 255u + dw;
	uint halfTotal = total / 2u;
	return uvec4((c * a * 255u + d.rgb * dw + halfTotal) / total, (total + 127u) / 255u);
}

double separable(int mode, double cs, double cd) {
	if (mode == Darken) return cs < cd ? cs : cd;
	if (mode == Lighten) return cs > cd ? cs : cd;
	if (mode == Multiply) return cs * cd;
	if (mode == Screen) return 1.0lf - (1.0lf - cs) * (1.0lf - cd);
	if (mode == Overlay) return cd <= 0.5lf ? 2.0lf * cs * cd : 1.0lf - 2.0lf * (1.0lf - cs) * (1.0lf - cd);
	if (mode == HardLight) return cs <= 0.5lf ? 2.0lf * cs * cd : 1.0lf - 2.0lf * (1.0lf - cs) * (1.0lf - cd);
	if (mode == SoftLight) {
		if (cs <= 0.5lf) return cd - (1.0lf - 2.0lf * cs) * cd * (1.0lf - cd);
		double g = cd <= 0.25lf ? ((16.0lf * cd - 12.0lf) * cd + 4.0lf) * cd : sqrt(cd);
		return cd + (2.0lf * cs - 1.0lf) * (g - cd);
	}
	if (mode == ColorDodge || (mode == VividLight && cs >= 0.5lf) || (mode == HardMix && cs >= 0.5lf)) {
		double s = mode == ColorDodge ? cs : 2.0lf * cs - 1.0lf;
		double v = (s >= 1.0lf || cd >= 1.0lf) ? 1.0lf : min(ddiv(cd, 1.0lf - s), 1.0lf);
		if (mode == HardMix) return v < 0.5lf ? 0.0lf : 1.0lf;
		return v;
	}
	if (mode == ColorBurn || mode == VividLight || mode == HardMix) {
		double s = mode == ColorBurn ? cs : 2.0lf * cs;
		double v = (s <= 0.0lf || cd <= 0.0lf) ? 0.0lf : max(1.0lf - ddiv(1.0lf - cd, s), 0.0lf);
		if (mode == HardMix) return v < 0.5lf ? 0.0lf : 1.0lf;
		return v;
	}
	if (mode == LinearDodge) return min(cs + cd, 1.0lf);
	if (mode == LinearBurn) return max(cs + cd - 1.0lf, 0.0lf);
	if (mode == LinearLight) return clamp(cd + 2.0lf * cs - 1.0lf, 0.0lf, 1.0lf);
	if (mode == PinLight) return cs < 0.5lf ? min(2.0lf * cs, cd) : max(2.0lf * cs - 1.0lf, cd);
	if (mode == Difference) return abs(cd - cs);
	if (mode == Exclusion) return cd + cs - 2.0lf * cd * cs;
	if (mode == Subtract) return max(cd - cs, 0.0lf);
	if (mode == Divide) return cs <= 0.0lf ? 1.0lf : min(ddiv(cd, cs), 1.0lf);
	if (mode == Average) return ddiv(cs + cd, 2.0lf);
	if (mode == Negation) return 1.0lf - abs(1.0lf - cd - cs);
	return cs;
}

// colorspace.blendRGB for the separable and whole-colour modes.
uvec3 blendRGB(int mode, uvec3 s, uvec3 d) {
	uint ss = s.r + s.g + s.b;
	uint ds = d.r + d.g + d.b;
	if (mode == DarkerColor) return ss < ds ? s : d;
	if (mode == LighterColor) return ss > ds ? s : d;
	if (mode == XorMode) return s ^ d;
	dvec3 sf = ddiv(dvec3(s), 255.0lf);
	dvec3 df = ddiv(dvec3(d), 255.0lf);
	return uvec3(clampU8(separable(mode, sf.r, df.r) * 255.0lf), clampU8(separable(mode, sf.g, df.g) * 255.0lf), clampU8(separable(mode, sf.b, df.b) * 255.0lf));
}

double lumOf(dvec3 c) {
	return 0.3lf * c.r + 0.59lf * c.g + 0.11lf * c.b;
}

double satOf(dvec3 c) {
	return max(max(c.r, c.g), c.b) - min(min(c.r, c.g), c.b);
}

double clipToLum(double v, double lum, double n, double x) {
	if (n < 0.0lf && lum - n > 0.0lf) v = lum + ddiv((v - lum) * lum, lum - n);
	if (x > 1.0lf && x - lum > 0.0lf) v = lum + ddiv((v - lum) * (1.0lf - lum), x - lum);
	return v;
}

dvec3 setLum(dvec3 c, double l) {
	double d = l - lumOf(c);
	dvec3 o = c + d;
	double lum = lumOf(o);
	double n = min(min(o.r, o.g), o.b);
	double x = max(max(o.r, o.g), o.b);
	return dvec3(clipToLum(o.r, lum, n, x), clipToLum(o.g, lum, n, x), clipToLum(o.b, lum, n, x));
}

dvec3 setSat(dvec3 c, double s) {
	// Channel indices from the smallest to the largest; ties keep their order.
	double v[3] = double[3](c.r, c.g, c.b);
	int lo = 0, mid = 1, hi = 2;
	if (v[mid] < v[lo]) { int t = lo; lo = mid; mid = t; }
	if (v[hi] < v[mid]) { int t = mid; mid = hi; hi = t; }
	if (v[mid] < v[lo]) { int t = lo; lo = mid; mid = t; }
	if (v[hi] <= v[lo]) return dvec3(0.0lf);
	double o[3] = double[3](0.0lf, 0.0lf, 0.0lf);
	o[mid] = (v[mid] - v[lo]) * s / (v[hi] - v[lo]);
	o[hi] = s;
	return dvec3(o[0], o[1], o[2]);
}

uvec3 blendNonSeparable(int mode, uvec3 s, uvec3 d) {
	dvec3 src = ddiv(dvec3(s), 255.0lf);
	dvec3 dst = ddiv(dvec3(d), 255.0lf);
	dvec3 o;
	if (mode == Hue) o = setLum(setSat(src, satOf(dst)), lumOf(dst));
	else if (mode == Saturation) o = setLum(setSat(dst, satOf(src)), lumOf(dst));
	else if (mode == ColorMode) o = setLum(src, lumOf(dst));
	else if (mode == Luminosity) o = setLum(dst, lumOf(src));
	else return s;
	return uvec3(clampU8(o.r * 255.0lf + 0.5lf), clampU8(o.g * 255.0lf + 0.5lf), clampU8(o.b * 255.0lf + 0.5lf));
}

bool nonSeparable(int mode) {
	return mode == Hue || mode == Saturation || mode == ColorMode || mode == Luminosity;
}

bool perChannel(int mode) {
	return !(mode == Normal || mode == DarkerColor || mode == LighterColor || nonSeparable(mode));
}

vec2 porterDuff(int op, float sa, float da) {
	if (op == SourceIn) return vec2(da, 0.0);
	if (op == SourceOut) return vec2(1.0 - da, 0.0);
	if (op == SourceAtop) return vec2(da, 1.0 - sa);
	if (op == DestinationOver) return vec2(1.0 - da, 1.0);
	if (op == DestinationIn) return vec2(0.0, sa);
	if (op == DestinationOut) return vec2(0.0, 1.0 - sa);
	if (op == DestinationAtop) return vec2(1.0 - da, sa);
	if (op == XorOp) return vec2(1.0 - da, 1.0 - sa);
	if (op == Copy) return vec2(1.0, 0.0);
	if (op == Clear) return vec2(0.0, 0.0);
	if (op == Plus) return vec2(1.0, 1.0);
	return vec2(1.0, 1.0 - sa);
}

// composePixel: any operator and blend mode, m the clip's byte.
uvec4 composePixel(uvec4 d, uvec3 s, uint sa8, int op, int mode, uint m) {
	float sa = float(sa8) / 255.0;
	float da = float(d.a) / 255.0;
	vec4 cd = vec4(d) / 255.0;
	vec4 cs = vec4(vec3(s), 255.0) / 255.0;
	if (mode != Normal && da > 0.0) {
		uvec3 b = nonSeparable(mode) ? blendNonSeparable(mode, s, d.rgb) : blendRGB(mode, s, d.rgb);
		vec4 mixed = vec4(vec3(b), 255.0) / 255.0;
		cs = cs * (1.0 - da) + mixed * da;
	}
	vec2 f = porterDuff(op, sa, da);
	float ao = min(sa * f.x + da * f.y, 1.0);
	vec4 co = cs * (sa * f.x) + cd * (da * f.y);
	if (m < 255u) {
		float t = float(m) / 255.0;
		co = co * t + cd * (da * (1.0 - t));
		ao = ao * t + da * (1.0 - t);
	}
	if (ao <= 0.0) {
		return uvec4(0u);
	}
	uvec4 o = uvec4(min(co / ao, vec4(1.0)) * 255.0 + 0.5);
	o.a = uint(ao * 255.0 + 0.5);
	return o;
}

// canvas blit: source-over, or blendBytes for the other modes, in double as
// the canvas computes them.
uvec4 blitPixel(uvec4 d, uvec4 s, int mode) {
	if (mode == Normal) {
		if (s.a == 255u || (s.a > 0u && d.a == 0u)) return s;
		if (s.a == 0u) return d;
		double sa = ddiv(double(s.a), 255.0lf);
		double da = ddiv(double(d.a), 255.0lf);
		double ao = sa + da - sa * da;
		dvec3 cs = ddiv(dvec3(s.rgb), 255.0lf);
		dvec3 cd = ddiv(dvec3(d.rgb), 255.0lf);
		dvec3 p = cs * sa * (1.0lf - da) + cd * da * (1.0lf - sa) + cs * (sa * da);
		return uvec4(uvec3(floor(clamp(ddiv(p, ao), 0.0lf, 1.0lf) * 255.0lf + 0.5lf)), clampU8(ao * 255.0lf));
	}
	if (s.a == 0u) return d;
	double sa = ddiv(double(s.a), 255.0lf);
	double da = ddiv(double(d.a), 255.0lf);
	if (da <= 0.0lf && mode != Dissolve) return s;
	if (mode == Dissolve) return sa < 0.5lf ? d : s;
	uvec3 b = nonSeparable(mode) ? blendNonSeparable(mode, s.rgb, d.rgb) : blendRGB(mode, s.rgb, d.rgb);
	double ao = sa + da - sa * da;
	if (ao <= 0.0lf) return uvec4(0u);
	dvec3 o = (ddiv(dvec3(s.rgb), 255.0lf) * sa) * (1.0lf - da) + (ddiv(dvec3(d.rgb), 255.0lf) * da) * (1.0lf - sa) + ddiv(dvec3(b), 255.0lf) * (sa * da);
	dvec3 c = clamp(ddiv(o, ao), 0.0lf, 1.0lf) * 255.0lf;
	return uvec4(clampU8(c.r), clampU8(c.g), clampU8(c.b), clampU8(ao * 255.0lf));
}

// spreadToIndex: the table entry for t.
int tableIndex(float t, int spread) {
	if (spread == 1) {
		t = t - floor(t);
	} else if (spread == 2) {
		float u = t * 0.5;
		u = (u - floor(u)) * 2.0;
		t = 1.0 - abs(u - 1.0);
	}
	t = clamp(t, 0.0, 1.0);
	return int(t * 1023.0 + 0.5);
}

// ditherRow: the entry plus the blue noise at the pixel, high bytes.
uvec4 tableColor(int index, ivec2 at) {
	if (index < 0) return uvec4(0u);
	uint d = byteOf(texelFetch(uBlueNoise, ivec2(at.x & 63, at.y & 63), 0).r);
	return (texelFetch(uTable, ivec2(index, 0), 0) + d) >> 8;
}

dvec2 paintSpace(dvec2 p) {
	return dvec2(double(uInvX.x) * p.x + double(uInvY.x) * p.y + double(uInvT.x), double(uInvX.y) * p.x + double(uInvY.y) * p.y + double(uInvT.y));
}

uvec4 gradient(ivec2 at) {
	dvec2 p = dvec2(at) + 0.5lf;
	if (uGradKind == 0) {
		float t = float(uParams[0] + uParams[1] * p.x + uParams[2] * p.y);
		return tableColor(tableIndex(t, uSpread), at);
	}
	dvec2 u = paintSpace(p);
	if (uGradKind == 2) {
		double a = double(atan(float(u.y - uParams[1]), float(u.x - uParams[0]))) - uParams[2];
		return tableColor(tableIndex(float(ddiv(a, 6.283185307179586lf)), 1), at);
	}
	double fx = uParams[0], fy = uParams[1], r0 = uParams[2], cdx = uParams[3], cdy = uParams[4], dr = uParams[5], qa = uParams[6];
	double px = u.x - fx, py = u.y - fy;
	if (uParams[7] != 0.0lf) {
		float t = float(sqrt(float(px * px + py * py))) * float(ddiv(1.0lf, r0 + dr));
		return tableColor(tableIndex(t, uSpread), at);
	}
	double b = px * cdx + py * cdy + r0 * dr;
	double c = px * px + py * py - r0 * r0;
	double t = -1.0lf;
	bool ok = false;
	if (abs(qa) < 1e-12lf) {
		if (abs(b) > 1e-12lf) {
			t = ddiv(c, 2.0lf * b);
			ok = r0 + t * dr >= 0.0lf;
		}
	} else {
		double disc = b * b - qa * c;
		if (disc >= 0.0lf) {
			double root = sqrt(disc);
			double t1 = ddiv(b + root, qa);
			double t2 = ddiv(b - root, qa);
			double hi = max(t1, t2);
			double lo = min(t1, t2);
			if (r0 + hi * dr >= 0.0lf) {
				t = hi;
				ok = true;
			} else if (r0 + lo * dr >= 0.0lf) {
				t = lo;
				ok = true;
			}
		}
	}
	return tableColor(ok ? tableIndex(float(t), uSpread) : -1, at);
}

// NoiseShader: improved gradient noise, octaves summed.
uvec4 noise(ivec2 at) {
	const float gx[8] = float[8](1.0, -1.0, 1.0, -1.0, 1.0, -1.0, 0.0, 0.0);
	const float gy[8] = float[8](1.0, 1.0, -1.0, -1.0, 0.0, 0.0, 1.0, -1.0);
	double py = double(at.y) + 0.5lf;
	float pxf = float(at.x) + 0.5;
	precise float ux = pxf * float(uInvX.x) + float(double(uInvY.x) * py + double(uInvT.x));
	precise float uy = pxf * float(uInvX.y) + float(double(uInvY.y) * py + double(uInvT.y));
	float peak = 0.0, amp = 1.0;
	for (int o = 0; o < uOctaves; o++) {
		peak += amp;
		amp *= 0.5;
	}
	float norm = float(ddiv(0.5lf, double(peak)));
	precise float sum = 0.0;
	precise float a = 1.0;
	for (int o = 0; o < uOctaves; o++) {
		float fx = floor(ux), fy = floor(uy);
		precise float tx = ux - fx;
		precise float ty = uy - fy;
		int xi = int(fx) & 255, yi = int(fy) & 255;
		int x1 = (xi + 1) & 255, y1 = (yi + 1) & 255;
		int px0 = texelFetch(uPerm, ivec2(xi, 0), 0).r;
		int px1 = texelFetch(uPerm, ivec2(x1, 0), 0).r;
		int h00 = texelFetch(uPerm, ivec2(px0 + yi, 0), 0).r & 7;
		int h10 = texelFetch(uPerm, ivec2(px1 + yi, 0), 0).r & 7;
		int h01 = texelFetch(uPerm, ivec2(px0 + y1, 0), 0).r & 7;
		int h11 = texelFetch(uPerm, ivec2(px1 + y1, 0), 0).r & 7;
		precise float d00 = gx[h00] * tx + gy[h00] * ty;
		precise float d10 = gx[h10] * (tx - 1.0) + gy[h10] * ty;
		precise float d01 = gx[h01] * tx + gy[h01] * (ty - 1.0);
		precise float d11 = gx[h11] * (tx - 1.0) + gy[h11] * (ty - 1.0);
		precise float sx = tx * tx * tx * (tx * (tx * 6.0 - 15.0) + 10.0);
		precise float sy = ty * ty * ty * (ty * (ty * 6.0 - 15.0) + 10.0);
		precise float top = d00 + (d10 - d00) * sx;
		precise float bottom = d01 + (d11 - d01) * sx;
		sum = sum + (top + (bottom - top) * sy) * a;
		a = a * 0.5;
		ux = ux * 2.0 + 17.3;
		uy = uy * 2.0 + 31.7;
	}
	precise float t = clamp(sum * norm * 1.4142135 + 0.5, 0.0, 1.0);
	return tableColor(tableIndex(t, 0), at);
}

// The layout of uEdges (FrameRenderer.bindEdges).
const int EdgeRow = 1024;
const int EdgeBandRows = 16;

vec4 edgeTexel(int i) {
	return texelFetch(uEdges, ivec2(i % EdgeRow, i / EdgeRow), 0);
}

// The share of a unit of height an edge crossing a pixel's row covers to
// its right inside the pixel, the edge running from x = a to x = b
// (relative to the pixel's left side) over that height: the mean of
// clamp(1 - x, 0, 1) along it.
float rightOf(float a, float b) {
	float lo = min(a, b), hi = max(a, b);
	float span = hi - lo;
	if (span < 1e-6) return clamp(1.0 - 0.5 * (lo + hi), 0.0, 1.0);
	float left = max(min(hi, 0.0) - lo, 0.0);
	float m0 = clamp(lo, 0.0, 1.0), m1 = clamp(hi, 0.0, 1.0);
	return (left + (m1 - m0) * (1.0 - 0.5 * (m0 + m1))) / span;
}

// The coverage byte of the pixel at `at` by the path whose edges are in
// uEdges, as the canvas's rasterizer finds it: the winding-weighted area
// of the pixel square right of each edge of its band, summed, then folded
// by the rule and rounded.
uint pathCoverage(ivec2 at) {
	vec4 band = edgeTexel((at.y - uBox.y) / EdgeBandRows);
	int first = int(band.x), count = int(band.y);
	float px = float(at.x), py = float(at.y);
	float acc = 0.0;
	for (int i = 0; i < count; i++) {
		vec4 e = edgeTexel(first + i);
		float dir = 1.0;
		if (e.y > e.w) {
			e = e.zwxy;
			dir = -1.0;
		}
		float ya = max(e.y, py), yb = min(e.w, py + 1.0);
		if (ya >= yb) continue;
		float dxdy = (e.z - e.x) / (e.w - e.y);
		float xa = e.x + (ya - e.y) * dxdy - px;
		float xb = e.x + (yb - e.y) * dxdy - px;
		acc += dir * (yb - ya) * rightOf(xa, xb);
	}
	float a = abs(acc);
	if (uEvenOdd != 0) {
		a = a - 2.0 * floor(a * 0.5);
		a = min(a, 2.0 - a);
	} else {
		a = min(a, 1.0);
	}
	return uint(a * 255.0 + 0.5);
}

// The exact area of the pixel square at `at` inside the convex quad:
// the quad clipped to the square's four sides, then the shoelace sum.
double quadArea(ivec2 at) {
	double ax[12], ay[12], bx[12], by[12];
	for (int i = 0; i < 4; i++) {
		ax[i] = uQuad[i * 2];
		ay[i] = uQuad[i * 2 + 1];
	}
	int n = 4;
	for (int side = 0; side < 4; side++) {
		int m = 0;
		for (int i = 0; i < n; i++) {
			int j = i + 1 == n ? 0 : i + 1;
			double dp, dq;
			if (side == 0) { dp = ax[i] - double(at.x); dq = ax[j] - double(at.x); }
			else if (side == 1) { dp = double(at.x + 1) - ax[i]; dq = double(at.x + 1) - ax[j]; }
			else if (side == 2) { dp = ay[i] - double(at.y); dq = ay[j] - double(at.y); }
			else { dp = double(at.y + 1) - ay[i]; dq = double(at.y + 1) - ay[j]; }
			if (dp >= 0.0lf) {
				bx[m] = ax[i];
				by[m] = ay[i];
				m++;
			}
			if ((dp >= 0.0lf) != (dq >= 0.0lf)) {
				double t = ddiv(dp, dp - dq);
				bx[m] = ax[i] + (ax[j] - ax[i]) * t;
				by[m] = ay[i] + (ay[j] - ay[i]) * t;
				m++;
			}
		}
		n = m;
		if (n == 0) return 0.0lf;
		for (int i = 0; i < n; i++) {
			ax[i] = bx[i];
			ay[i] = by[i];
		}
	}
	double sum = 0.0lf;
	for (int i = 0; i < n; i++) {
		int j = i + 1 == n ? 0 : i + 1;
		sum += ax[i] * ay[j] - ax[j] * ay[i];
	}
	return abs(sum) * 0.5lf;
}

// spreadIndex: a texel index along an axis of n under the spread.
int spreadIndex(int i, int n, int spread) {
	if (spread == 1) {
		return i - n * int(floor(ddiv(double(i), double(n))));
	}
	if (spread == 2) {
		int p = 2 * n;
		int m = i - p * int(floor(ddiv(double(i), double(p))));
		return m < n ? m : p - 1 - m;
	}
	return clamp(i, 0, n - 1);
}

// texelAt: premultiplied linear r, g, b and the alpha.
vec4 linearTexel(int x, int y) {
	uvec4 t = bytesOf(texelFetch(uImage, ivec2(x, y), 0));
	float a = float(t.a) / 255.0;
	return vec4(texelFetch(uToLinear, ivec2(int(t.r), 0), 0).r * a, texelFetch(uToLinear, ivec2(int(t.g), 0), 0).r * a, texelFetch(uToLinear, ivec2(int(t.b), 0), 0).r * a, a);
}

// storeLinear: premultiplied linear colour back to straight sRGB bytes.
uvec4 storeLinear(vec3 c, float a) {
	if (a <= 0.0) return uvec4(0u);
	ivec3 k = ivec3(clamp(c / a, 0.0, 1.0) * 4095.0 + 0.5);
	return uvec4(byteOf(texelFetch(uToSrgb, ivec2(k.r, 0), 0).r), byteOf(texelFetch(uToSrgb, ivec2(k.g, 0), 0).r), byteOf(texelFetch(uToSrgb, ivec2(k.b, 0), 0).r), uint(min(a, 1.0) * 255.0 + 0.5));
}

vec4 catmullRom(float t) {
	precise float t2 = t * t;
	precise float t3 = t2 * t;
	precise vec4 w = 0.5 * vec4(-t3 + 2.0 * t2 - t, 3.0 * t3 - 5.0 * t2 + 2.0, -3.0 * t3 + 4.0 * t2 + t, t3 - t2);
	return w;
}

// PatternShader: the image level sampled at the pixel centre.
uvec4 image(ivec2 at) {
	dvec2 p = dvec2(at) + 0.5lf;
	double u = uParams[0] * p.x + uParams[2] * p.y + uParams[4];
	double v = uParams[1] * p.x + uParams[3] * p.y + uParams[5];
	int w = uImageSize.x, h = uImageSize.y;
	uvec4 o;
	if (uSampling == 0) {
		int sx = spreadIndex(int(floor(u)), w, uImageSpread.x);
		int sy = spreadIndex(int(floor(v)), h, uImageSpread.y);
		o = bytesOf(texelFetch(uImage, ivec2(sx, sy), 0));
	} else if (uSampling == 1) {
		u -= 0.5lf;
		v -= 0.5lf;
		double fx = floor(u), fy = floor(v);
		float tx = float(u - fx), ty = float(v - fy);
		int xa = spreadIndex(int(fx), w, uImageSpread.x), xb = spreadIndex(int(fx) + 1, w, uImageSpread.x);
		int ya = spreadIndex(int(fy), h, uImageSpread.y), yb = spreadIndex(int(fy) + 1, h, uImageSpread.y);
		vec4 t00 = linearTexel(xa, ya), t10 = linearTexel(xb, ya), t01 = linearTexel(xa, yb), t11 = linearTexel(xb, yb);
		precise float w00 = (1.0 - tx) * (1.0 - ty);
		precise float w10 = tx * (1.0 - ty);
		precise float w01 = (1.0 - tx) * ty;
		precise float w11 = tx * ty;
		precise vec3 c = t00.rgb * w00 + t10.rgb * w10 + t01.rgb * w01 + t11.rgb * w11;
		precise float a = t00.a * w00 + t10.a * w10 + t01.a * w01 + t11.a * w11;
		o = storeLinear(c, a);
	} else {
		u -= 0.5lf;
		v -= 0.5lf;
		double fx = floor(u), fy = floor(v);
		int x0 = int(fx) - 1, y0 = int(fy) - 1;
		vec4 wx = catmullRom(float(u - fx)), wy = catmullRom(float(v - fy));
		precise vec3 c = vec3(0.0);
		precise float a = 0.0;
		for (int j = 0; j < 4; j++) {
			int sy = spreadIndex(y0 + j, h, uImageSpread.y);
			for (int i = 0; i < 4; i++) {
				precise float wt = wx[i] * wy[j];
				vec4 t = linearTexel(spreadIndex(x0 + i, w, uImageSpread.x), sy);
				c = c + t.rgb * wt;
				a += t.a * wt;
			}
		}
		a = clamp(a, 0.0, 1.0);
		c = clamp(c, vec3(0.0), vec3(a));
		o = storeLinear(c, a);
	}
	if (uTinted != 0) {
		uvec4 q = o * uTint + 128u;
		o = (q + (q >> 8)) >> 8;
	}
	return o;
}

#ifdef ADM_FRAGMENT
uvec4 shaded(vec2 device) {
	vec2 p = vec2(uInvX.x * device.x + uInvY.x * device.y + uInvT.x, uInvX.y * device.x + uInvY.y * device.y + uInvT.y);
	vec4 c = ADM_ENTRY(p);
	return uvec4(clamp(c, 0.0, 1.0) * 255.0 + 0.5);
}
#endif

void main() {
	ivec2 at = ivec2(gl_FragCoord.xy);
	ivec2 local = at - uBox.xy;
	uvec4 d = bytesOf(DST_PIXEL);
	uvec4 s = uvec4(uSolid, 255u);
	if (uPaintKind == 1) {
		s = bytesOf(texelFetch(uPixels, at - uPixelsOrigin, 0));
	} else if (uPaintKind == 5) {
		// compositeLayer: the colour, its alpha scaled by the plane.
		double pv = clamp(double(texelFetch(uPlane, local, 0).r), 0.0lf, 1.0lf);
		s = uvec4(uSolid, uint(floor(pv * double(uSolidA) + 0.5lf)));
	}
	if (uPaintKind == 3) {
		s = gradient(at);
	} else if (uPaintKind == 4) {
		s = noise(at);
	} else if (uPaintKind == 6) {
		s = image(at);
	}
#ifdef ADM_FRAGMENT
	if (uPaintKind == 2) {
		s = shaded(gl_FragCoord.xy);
	}
#endif
	uvec4 o = d;
	if (uOp == 2) {
		o = s;
	} else if (uOp == 4) {
		// paintAtop: the colour over the group's pixels, as far as the plane.
		float k = texelFetch(uPlane, local, 0).r;
		if (k > 0.0) {
			uint sa = min(uint(floor(double(k) * double(uSolidA) + 0.5lf)), 255u);
			if (sa > 0u) o = composePixel(d, uSolid, sa, SourceAtop, uBlend, 255u);
		}
	} else if (uOp == 5) {
		// applyMatrix on straight colours 0 to 1.
		if (d.a != 0u) {
			vec4 v = vec4(d) / 255.0;
			precise vec4 m = uMatrix[0] * v.r + uMatrix[1] * v.g + uMatrix[2] * v.b + uMatrix[3] * v.a + uMatrix[4];
			o = uvec4(clamp(m, 0.0, 1.0) * 255.0 + 0.5);
		}
	} else if (uOp == 3) {
		o = blitPixel(d, s, uBlend);
	} else {
		uint k = uHasCoverage != 0 ? byteOf(texelFetch(uCoverage, local, 0).r) : 255u;
		if (uHasQuad != 0) {
			k = uint(float(min(quadArea(at), 1.0lf)) * 255.0 + 0.5);
			if (uHardQuad != 0) k = k >= 128u ? 255u : 0u;
		}
		if (uCoverageFromPlane != 0) {
			// paintOver's coverage: the ring plane rounded to a byte.
			k = uint(floor(clamp(double(texelFetch(uPlane, local, 0).r), 0.0lf, 1.0lf) * 255.0lf + 0.5lf));
		}
		uint a = uAlpha;
		bool unbounded = uComposite == SourceIn || uComposite == SourceOut || uComposite == DestinationIn || uComposite == DestinationAtop || uComposite == Copy;
		if (uEdgeBands != 0) {
			k = pathCoverage(at);
			if (uHardQuad != 0) k = k >= 128u ? 255u : 0u;
			// lowerFill's clipCoverage, for an operator that keeps what lies outside.
			if (uHasClip != 0 && !unbounded) k = div255(k * byteOf(texelFetch(uClip, local, 0).r));
		}
		uint m = (unbounded && uHasClip != 0) ? byteOf(texelFetch(uClip, local, 0).r) : 255u;
		bool shadedPaint = uPaintKind != 0;
		if (uOp == 1) {
			// compositeRowWith.
			if (uComposite != SourceOver) {
				if (k != 0u) o = composePixel(d, s.rgb, div255(s.a * a), uComposite, uBlend, k);
			} else if (k != 0u && s.a != 0u) {
				uint e = div255(div255(s.a * k) * a);
				o = uBlend == Normal ? blendPixel(d, s.rgb, e) : composePixel(d, s.rgb, e, SourceOver, uBlend, 255u);
			}
		} else if (!shadedPaint) {
			if (uComposite == SourceOver && uBlend == Normal) {
				if (k != 0u) o = blendPixel(d, s.rgb, div255(a * k));
			} else if (uComposite == SourceOver && perChannel(uBlend)) {
				if (k != 0u) {
					uvec3 bl = blendRGB(uBlend, s.rgb, d.rgb);
					uvec3 mixed = div255(s.rgb * (255u - d.a) + bl * d.a);
					o = blendPixel(d, mixed, div255(a * k));
				}
			} else if ((k != 0u || unbounded) && m != 0u) {
				o = composePixel(d, s.rgb, div255(a * k), uComposite, uBlend, m);
			}
		} else {
			uvec3 rgb = s.rgb;
			if (uPaired != 0) {
				uvec3 bl = uBlend == XorMode ? (s.rgb ^ d.rgb) : uvec3(clampU8(separable(uBlend, ddiv(double(s.r), 255.0lf), ddiv(double(d.r), 255.0lf)) * 255.0lf), clampU8(separable(uBlend, ddiv(double(s.g), 255.0lf), ddiv(double(d.g), 255.0lf)) * 255.0lf), clampU8(separable(uBlend, ddiv(double(s.b), 255.0lf), ddiv(double(d.b), 255.0lf)) * 255.0lf));
				rgb = div255(s.rgb * (255u - d.a) + bl * d.a);
			}
			if ((uComposite == SourceOver && uBlend == Normal) || uPaired != 0) {
				if (k != 0u) o = blendPixel(d, rgb, div255(div255(s.a * k) * a));
			} else if ((k != 0u || unbounded) && m != 0u) {
				o = composePixel(d, rgb, div255(div255(s.a * k) * a), uComposite, uBlend, m);
			}
		}
	}
	fragColor = vec4(o) / 255.0;
}
