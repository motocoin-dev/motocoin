// Copyright (c) 2014 The Motocoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//--------------------------------------------------------------------
// This file is part of The Game of Motocoin.
//--------------------------------------------------------------------
// Game rendering.
//--------------------------------------------------------------------

#ifndef MOTOGAME_RENDER_H
#define MOTOGAME_RENDER_H

#include "../moto-engine.h"
#include "vec2.hpp"

// World viewport on the screen.
class CView
{
public:
	vec2 m_ScreenPos[2];
	vec2 m_WorldPos[2];

	CView(const vec2 ScreenPos[2], const vec2 WorldPos[2]);

	// Map from world coordinates to screen coordinates.
	vec2 operator()(vec2 P) const;
};

// Our world is wrapped and repeated infinitely.
// This function draws something in several instances to ensure that it will draw all visible copies.
template<typename TFunc>
inline void renderCyclic(const CView& View, TFunc&& Func)
{
	for (int p = -2; p < 3; p++) // This should be enough
	{
		vec2 Shift = vec2((float)p, 0.0f)*MOTO_MAP_SIZE*MOTO_SCALE;
		vec2 WorldPos[2] = { View.m_WorldPos[0] + Shift, View.m_WorldPos[1] + Shift };
		Func(CView(View.m_ScreenPos, WorldPos));
	}
}

void initRenderer();

// Initialize necessary data on GPU for rendering this world.
void prepareWorldRendering(const MotoWorld& World);

// Drawing functions.
// Schematic means uniform color, i.e. no texture.
void drawWorldAndCoin(const CView& View, const MotoState& Frame, bool Schematic, bool IsMap, vec2 SkyShift = vec2());
void drawMoto(const CView& View, const MotoState& Frame, bool MotoDir);
void drawSchematicMoto(const CView& View, const MotoState& Frame);

// Draw some text.
void drawText(const char* pText, int iLine, int iPos, float Size, int iColor = 0, float Alpha = 1.0f);

#endif // MOTOGAME_RENDER_H