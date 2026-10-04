// The canvas's group effects (std.gfx.canvas closeGroup and its plane
// helpers) as passes over float planes of pw x h, the effect region's size
// rounded up to whole vectors. Each pass writes one plane (or, for colour
// blurs, four channels) from others; the comments name the canvas function.

uniform int uMode;
uniform sampler2D uA;
uniform sampler2D uB;
uniform sampler2D uC;
uniform sampler2D uImage;      // RGBA8 pixels (the group, or the target below)
uniform ivec2 uOrigin;         // the region's corner on uImage
uniform int uW;                // the region's width
uniform int uH;                // its height
uniform int uPW;               // the plane's width
uniform vec4 uF;               // float parameters of the pass
uniform ivec4 uI;              // int parameters of the pass
uniform double uD;             // a double parameter
uniform sampler2D uLinear;     // 256 x 1: srgbToLinearTable
uniform sampler2D uSrgb;       // 4096 x 1: linearToSrgbTable / 255
uniform sampler2D uOld;        // the region before a store (within blends)

out vec4 fragColor;

const int Alpha = 0, Covered = 1, Copy = 2, Inverted = 3, Multiply = 4, MultiplyInverse = 5,
	Shifted = 6, Box = 7, Grown = 8, Ring = 9, Signed = 10, DistanceInit = 11, DistanceColumns = 12,
	DistanceRows = 13, Linear = 14, Store = 15, Halve = 16;

vec4 at(sampler2D s, int x, int y) {
	return texelFetch(s, ivec2(x, y), 0);
}

uint byteOf(float v) {
	return uint(v * 255.0 + 0.5);
}

// sampleAt: the plane outside the region is `outside`.
float sampleAt(int x, int y, float outside) {
	if (x < 0 || y < 0 || x >= uW || y >= uH) return outside;
	return at(uA, x, y).r;
}

// distance1D's value at q: the lowest parabola (q - p)² + f(p) over the
// line of n samples, with `edge` one sample past each end.
float lowest(int q, int n, bool columns, int fixedAxis, float edge) {
	double best = 1e300lf;
	for (int r = 0; r < n; r++) {
		if (double(r * r) >= best) break;
		for (int side = 0; side < 2; side++) {
			int p = side == 0 ? q - r : q + r;
			if (p < 0 || p >= n || (side == 1 && r == 0)) continue;
			float f = edge;
			if (p > 0 && p < n - 1) {
				f = columns ? at(uA, fixedAxis, p - 1).r : at(uA, p - 1, fixedAxis).r;
			}
			best = min(best, double(r * r) + double(f));
		}
	}
	return float(best);
}

void main() {
	int x = int(gl_FragCoord.x);
	int y = int(gl_FragCoord.y);
	vec4 a = at(uA, x, y);
	if (uMode == Alpha) {
		float v = 0.0;
		if (x < uW) {
			v = float(byteOf(texelFetch(uImage, uOrigin + ivec2(x, y), 0).a)) * (1.0 / 255.0);
		}
		fragColor = vec4(v);
	} else if (uMode == Covered) {
		fragColor = vec4(min(a.r * 255.0, 1.0));
	} else if (uMode == Copy) {
		fragColor = a;
	} else if (uMode == Inverted) {
		fragColor = vec4(1.0 - a.r);
	} else if (uMode == Multiply) {
		precise float v = a.r * at(uB, x, y).r;
		fragColor = vec4(v);
	} else if (uMode == MultiplyInverse) {
		precise float v = a.r * (1.0 - at(uB, x, y).r);
		fragColor = vec4(v);
	} else if (uMode == Shifted) {
		// uI.xy: the whole shift; uF.xy: its fraction; uF.z: what comes in.
		float v = 0.0;
		if (x < uW) {
			int sx = x - uI.x - 1;
			int sy = y - uI.y - 1;
			float tx = uF.x, ty = uF.y, edge = uF.z;
			float p00 = sampleAt(sx, sy, edge);
			float p10 = sampleAt(sx + 1, sy, edge);
			float p01 = sampleAt(sx, sy + 1, edge);
			float p11 = sampleAt(sx + 1, sy + 1, edge);
			precise float top = p00 * tx + p10 * (1.0 - tx);
			precise float bottom = p01 * tx + p11 * (1.0 - tx);
			precise float mixed = top * ty + bottom * (1.0 - ty);
			v = mixed;
		}
		fragColor = vec4(v);
	} else if (uMode == Box) {
		// boxDown: radius uI.x, down the columns (uI.y 0) or across the rows
		// (1); nothing past the plane's rows or columns.
		int br = uI.x;
		bool down = uI.y == 0;
		int n = down ? uH : uPW;
		int i = down ? y : x;
		precise vec4 sum = vec4(0.0);
		for (int k = i - br; k <= i + br; k++) {
			if (k < 0 || k >= n) continue;
			sum = sum + (down ? at(uA, x, k) : at(uA, k, y));
		}
		precise vec4 v = sum * uF.x;
		fragColor = v;
	} else if (uMode == Grown) {
		fragColor = vec4(clamp(uF.x - a.r, 0.0, 1.0));
	} else if (uMode == Ring) {
		fragColor = vec4(max(a.r - at(uB, x, y).r, 0.0));
	} else if (uMode == Signed) {
		// signedDistance from the shape (uA) and the squared distances to
		// the inside (uB) and the outside (uC).
		float v = 0.0;
		if (x < uW) {
			float s = a.r;
			if (s > 0.0 && s < 1.0) {
				v = 0.5 - s;
			} else if (s >= 0.5) {
				v = 0.5 - float(sqrt(double(at(uC, x, y).r)));
			} else {
				v = float(sqrt(double(at(uB, x, y).r))) - 0.5;
			}
		}
		fragColor = vec4(v);
	} else if (uMode == DistanceInit) {
		// distanceTo's start: 0 on the side asked for (uI.x 1: inside).
		float v = 0.0;
		if (x < uW) {
			bool isIn = a.r >= 0.5;
			v = isIn == (uI.x == 1) ? 0.0 : 1e20;
		}
		fragColor = vec4(v);
	} else if (uMode == DistanceColumns || uMode == DistanceRows) {
		// uF.x: what lies one sample past each end.
		float v = a.r;
		if (x < uW) {
			v = uMode == DistanceColumns ? lowest(y + 1, uH + 2, true, x, uF.x) : lowest(x + 1, uW + 2, false, y, uF.x);
		}
		fragColor = vec4(v);
	} else if (uMode == Linear) {
		// blurWhere's planes: premultiplied linear light.
		vec4 v = vec4(0.0);
		if (x < uW) {
			uvec4 p = uvec4(texelFetch(uImage, uOrigin + ivec2(x, y), 0) * 255.0 + 0.5);
			float al = float(p.a) / 255.0;
			v = vec4(at(uLinear, int(p.r), 0).r * al, at(uLinear, int(p.g), 0).r * al, at(uLinear, int(p.b), 0).r * al, al);
		}
		fragColor = v;
	} else if (uMode == Halve) {
		// halveLevel: the 2x2 box average of the level below (uImage, uW x uH
		// texels from its corner) in premultiplied linear light, written as
		// straight sRGB bytes.
		precise vec4 sum = vec4(0.0);
		precise float alpha = 0.0;
		for (int k = 0; k < 4; k++) {
			int sx = min(x * 2 + (k & 1), uW - 1);
			int sy = min(y * 2 + (k >> 1), uH - 1);
			uvec4 p = uvec4(texelFetch(uImage, ivec2(sx, sy), 0) * 255.0 + 0.5);
			float al = float(p.a) / 255.0;
			sum = sum + vec4(at(uLinear, int(p.r), 0).r, at(uLinear, int(p.g), 0).r, at(uLinear, int(p.b), 0).r, 0.0) * al;
			alpha += al;
		}
		vec4 c = sum * 0.25;
		float al = alpha * 0.25;
		uvec4 o = uvec4(0u);
		if (al > 0.0) {
			vec3 k = clamp(c.rgb / al, 0.0, 1.0) * 4095.0 + 0.5;
			ivec3 idx = ivec3(k);
			o = uvec4(byteOf(at(uSrgb, idx.r, 0).r), byteOf(at(uSrgb, idx.g, 0).r), byteOf(at(uSrgb, idx.b, 0).r), uint(min(al, 1.0) * 255.0 + 0.5));
		}
		fragColor = vec4(o) / 255.0;
	} else if (uMode == Store) {
		// storeLinear back to straight sRGB bytes at the region's pixel; with
		// a `within` plane (uB, uI.x 1) only as far as it covers.
		ivec2 here = ivec2(gl_FragCoord.xy) - uOrigin;
		vec4 c = at(uA, here.x, here.y);
		uvec4 old = uvec4(texelFetch(uOld, here, 0) * 255.0 + 0.5);
		uvec4 o = uvec4(0u);
		float al = c.a;
		if (al > 0.0) {
			vec3 k = clamp(c.rgb / al, 0.0, 1.0) * 4095.0 + 0.5;
			ivec3 idx = ivec3(k);
			o = uvec4(byteOf(at(uSrgb, idx.r, 0).r), byteOf(at(uSrgb, idx.g, 0).r), byteOf(at(uSrgb, idx.b, 0).r), uint(min(al, 1.0) * 255.0 + 0.5));
		}
		if (uI.x == 1) {
			uint t = uint(floor(clamp(double(at(uB, here.x, here.y).r), 0.0lf, 1.0lf) * 255.0lf + 0.5lf));
			if (t == 0u) {
				o = old;
			} else if (t < 255u) {
				uvec4 y4 = o * t + old * (255u - t) + 128u;
				o = (y4 + (y4 >> 8)) >> 8;
			}
		}
		fragColor = vec4(o) / 255.0;
	}
}
