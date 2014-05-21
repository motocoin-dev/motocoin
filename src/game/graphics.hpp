// Copyright (c) 2014 The Motocoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//--------------------------------------------------------------------
// This file is part of The Game of Motocoin.
//--------------------------------------------------------------------
// Very simple graphics library on top of OpenGL.
// If you don't know how OpenGL works then you probably won't understand
// what fuctions in this file do.
//--------------------------------------------------------------------

#ifndef MOTOGAME_GRAPHICS_H
#define MOTOGAME_GRAPHICS_H

#include <initializer_list>

#include "vec2.hpp"

 // Window size in pixels.
extern vec2 g_ViewportSize;

// Initialization.
void initGraphics();

// Push attributes.
void pushAttribute(float a);
void pushAttribute(vec2 A);

// Draw anything using previously put attributes and current OpenGL program.
void drawArrays(GLenum ShapesType, int NumAttributesPerVertex);

// Compile OpenGL program consisting of fragment (pixel) and vertex shaders.
// Attributes are a list of all attributes names for vertex shader.
GLuint compileProgram(const char* pVertexCode, const char* pFragmentCode, std::initializer_list<const char*> Attributes);

// Load texture from *.BMP file.
// Returns 0 in case of error.
GLuint loadTexture(const char* pName);

// Set 2D vector uniform for current OpenGL program.
void setUniformVec2(GLint iLocation, vec2 V);

// Set clip region. ScreenPos coordinates are in [-1, 1] range.
void setScissor(const vec2 ScreenPos[2]);
void unsetScissor();

#endif // MOTOGAME_GRAPHICS_H