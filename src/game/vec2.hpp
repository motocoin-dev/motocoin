// Copyright (c) 2014 The Motocoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//--------------------------------------------------------------------
// This file is part of The Game of Motocoin.
//--------------------------------------------------------------------
// 2D vector class.
//--------------------------------------------------------------------

#ifndef MOTOGAME_VEC2D_H
#define MOTOGAME_VEC2D_H

#include <cmath>

// 2D floating point vector.
class vec2
{
public:
	float x, y;

	vec2()
		: x(0.0f), y(0.0f)
	{}

	vec2(float X, float Y)
		: x(X), y(Y)
	{}

	vec2 operator-() const { return vec2(-x, -y); }

	// component-wise vector-vector operators
	vec2 operator+(vec2 B) const { return vec2(x + B.x, y + B.y); }
	vec2 operator-(vec2 B) const { return vec2(x - B.x, y - B.y); }
	vec2 operator*(vec2 B) const { return vec2(x * B.x, y * B.y); }
	vec2 operator/(vec2 B) const { return vec2(x / B.x, y / B.y); }

	// vector-scalar operators
	vec2 operator+(float b) const { return vec2(x + b, y + b); }
	vec2 operator-(float b) const { return vec2(x - b, y - b); }
	vec2 operator*(float b) const { return vec2(x * b, y * b); }
	vec2 operator/(float b) const { return vec2(x / b, y / b); }

	// properties
	float area()   const { return x*y; }
	float length() const { return sqrt(x*x + y*y); }
};

// scalar-vector operators
inline vec2 operator*(float a, vec2 B) { return vec2(a * B.x, a * B.y); }
inline vec2 operator/(float a, vec2 B) { return vec2(a / B.x, a / B.y); }

// trigonometry
inline vec2 ang(double a) { return vec2((float)cos(a), (float)sin(a)); } // get unit vector with specific angle to X-axis
inline float arc(vec2 V) { return atan2(V.y, V.x); } // get angle between vector and X-axis

#endif // MOTOGAME_VEC2D_H