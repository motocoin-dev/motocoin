// Copyright (c) 2014 The Motocoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//--------------------------------------------------------------------
// This file is part of The Game of Motocoin.
//--------------------------------------------------------------------
// Game rendering.
//--------------------------------------------------------------------

#include <GL/glew.h>

#include <cmath>
#include <iostream>
#include <algorithm>
#include <memory>
#include <cstring>
using namespace std;

#include "../moto-engine.h"
#include "vec2.hpp"
#include "graphics.hpp"
#include "render.hpp"

void showError(const string& Error);

// Circumference of a unit circle.
static const double Tau = 6.2831853071795864769;

// OpenGL shader program that draws with uniform color.
static GLuint g_glSimpleProgram;
static GLint g_glColorUniform;

// OpenGL shader program that draws texture.
static GLuint g_glTextureProgram;
static GLint g_glTextureUniform;

// OpenGL shader program that draws text.
static GLuint g_glTextProgram;
static GLint g_glTextFontUniform;
static GLint g_glTextColorUniform;

// OpenGL shader program that draws world with textures.
static GLuint g_glPerlinProgram;
static GLuint g_glPerlinGroundUniform;
static GLuint g_glPerlinSkyUniform;
static GLuint g_glPerlinGrassUniform;
static GLuint g_glPerlinMapUniform;
static GLuint g_glPerlinShiftUniform;
static GLuint g_glPerlinScale2Uniform;

// OpenGL shader program that draws world with just two colors.
static GLuint g_glSimplePerlinProgram;
static GLuint g_glSimplePerlinGroundUniform;
static GLuint g_glSimplePerlinSkyUniform;
static GLuint g_glSimplePerlinMapUniform;

// Textures.
static GLuint g_glGroundTexture;
static GLuint g_glSkyTexture;
static GLuint g_glGrassTexture;
static GLuint g_glBikeTexture;
static GLuint g_glCoinTexture;
static GLuint g_glFontTexture; // Characters
static GLuint g_glMapTexture; // World data for perlin noise.

// Colors.
static float g_SchematicGround[3] = { 0.314f, 0.209f, 0.104f };
static float g_SchematicSky[3] = { 0.674f, 0.782f, 0.999f };
static float g_SchematicMapSky[3] = { 0.674f*0.6f, 0.782f*0.75f, 0.999f*0.75f };
static float g_SchematicCoin[3] = { 1.0f, 1.0f, 0.0f };
static float g_SchematicMoto[3] = { 0.0f, 0.0f, 0.0f };
static float g_TextColors[3][3] = { { 0.5f, 1.0f, 0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } };

// Description of sub-texture stored in motocycle.bmp.
struct CImage
{
	float horz[2];
	float vert[2];
	float shift[2];
	float size;
	float angle;
};

// Descriptions of sub-textures stored in motocycle.bmp.
const CImage g_ImgWheel = { { 0.5f, 1.0f }, { 0.5f, 1.0f }, { 0.0f, 0.0f }, 0.9f, 0.0f };
const CImage g_ImgBike = { { 0.25f, 1.0f }, { 0.0f, 0.5f }, { -0.22f, -0.15f }, 1.2f, 0.2f };
const CImage g_ImgSuspensions[2] = { { { 0.0f, 0.5f }, { 0.625f, 0.75f }, { 0.00f, 0.0f }, 1.0f, 0.0f }, { { 0.5f, 0.0f }, { 0.5f, 0.625f }, { 0.07f, 0.0f }, 1.0f, 0.0f } };
const CImage g_ImgBody = { { 0.25f, 0.0f }, { 0.0f, 0.5f }, { -0.0f, -0.3f }, 1.2f, 0.6f };
const CImage g_ImgLimbs[2][2] =
{
	{ { { 0.25f, 0.5f }, { 0.875f, 1.0f }, { 0.0f, 0.0f }, 1.0f, 0.0f }, { { 0.25f, 0.5f }, { 0.75f, 0.875f }, { 0.0f, 0.0f }, 1.0f, 0.0f } },
	{ { { 0.0f, 0.25f }, { 0.875f, 1.0f }, { 0.0f, 0.0f }, 1.3f, 0.0f }, { { 0.0f, 0.25f }, { 0.75f, 0.875f }, { 0.0f, 0.0f }, 1.3f, 0.0f } }
};

CView::CView(const vec2 ScreenPos[2], const vec2 WorldPos[2])
{
	m_ScreenPos[0] = ScreenPos[0];
	m_ScreenPos[1] = ScreenPos[1];
	m_WorldPos[0] = WorldPos[0];
	m_WorldPos[1] = WorldPos[1];
}

vec2 CView::operator()(vec2 P) const
{
	return m_ScreenPos[0] + (P - m_WorldPos[0])*(m_ScreenPos[1] - m_ScreenPos[0])/(m_WorldPos[1] - m_WorldPos[0]);
}

static void setUniformColor(GLint iLocation, const float Color[3], float Alpha)
{
	glUniform4f(iLocation, Color[0], Color[1], Color[2], Alpha);
}

static void initGLEW()
{
	cout << "Initializing GLEW..." << endl;
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		// Problem: glewInit failed, something is seriously wrong.
		showError(string("GLEW Error: ") + (const char*)glewGetErrorString(err));
		exit(-1);
	}
	cout << "GLEW version: " << glewGetString(GLEW_VERSION) << endl;
}

#define STRINGIFY2(str) #str
#define STRINGIFY(str) STRINGIFY2(str)

void initRenderer()
{
	initGLEW();
	initGraphics();
	
	glGenTextures(1, &g_glMapTexture);	

	// Load image textures from files
	g_glGroundTexture = loadTexture("ground");
	g_glSkyTexture = loadTexture("sky");
	g_glGrassTexture = loadTexture("grass");
	g_glBikeTexture = loadTexture("motocykle");
	g_glCoinTexture = loadTexture("coin");
	g_glFontTexture = loadTexture("font");

	// FIXME: is there any better way of storing shader sources?
	const char* pSimpleVertexCode =
		"#version 130\n\
		 in vec2 v;\n\
		 void main() { gl_Position = vec4(v, 0.0, 1.0); }";

	const char* pSimpleFragmentCode =
		"#version 130\n\
		 uniform vec4 c;\n\
		 out vec4 FragColor;\n\
		 void main() { FragColor = c; }";

	const char* pTextureVertexCode =
		"#version 130\n\
		 in vec2 v;\n\
		 in vec2 at;\n\
		 out vec2 t;\n\
		 void main() {  t = at; gl_Position = vec4(v, 0.0, 1.0); }";

	const char* pTextureFragmentCode =
		"#version 130\n\
		 in vec2 t;\n\
		 out vec4 FragColor;\n\
		 uniform sampler2D image;\n\
		 void main() { FragColor = texture(image, t); }";
	
	const char* pTextFragmentCode =
		"#version 130\n\
		 in vec2 t;\n\
		 out vec4 FragColor;\n\
		 uniform sampler2D image;\n\
		 uniform vec4 color;\n\
		 void main()\n\
	     {\n\
            float contrast = clamp(0.04/dFdx(t.x), 15.0, 30.0);\n\
            float glow = contrast/5.0;\n\
			float c = texture(image, t).x;\n\
			vec4 c1 = vec4(color.rgb, 1.0 - clamp((c-0.5)*contrast,0.0,1.0));\n\
			vec4 c2 = vec4(0.0, 0.0, 0.0, clamp(1.0 - (c-0.5)*glow, 0.0, 1.0));\n\
	        vec4 res = mix(c1, c2, clamp((c-0.5)*contrast, 0.0, 1.0));\n\
	        FragColor = vec4(res.rgb, 1.0)*color.a*res.a;\n\
	     }";

	const char* pPerlinVertexCode = 
		"#version 130\n\
		 in vec2 v;\n\
		 in vec2 ar;\n\
		 out vec2 r;\n\
		 void main()\n\
		 {\n\
		     r = ar;\n\
			 gl_Position = vec4(v, 0.0, 1.0);\n\
		 }\n\
		";

	const char* pPerlinFragmentCode =
		"#version 130\n\
		 uniform sampler2D ground;\n\
		 uniform sampler2D sky;\n\
		 uniform sampler2D grass;\n\
		 uniform sampler2D map;\n\
		 uniform vec2 shift;\n\
		 uniform float scale2;\n\
		 in vec2 r;\n\
		 out vec4 FragColor;\n\
		 void main()\n\
		 {\n\
		 	 vec4 fxyt = texture(map, r); \n\
			 float level = " STRINGIFY(MOTO_LEVEL/8192.0) ";\n\
			 if (fxyt.z > level)\n\
			 {\n\
			     vec3 n = vec3(fxyt.xy/(fxyt.z - level), -1.0);\n\
				 vec4 C = texture(ground, r*16);\n\
				 FragColor = vec4(C.rgb*(0.6 + 0.8*dot(normalize(n), normalize(vec3(1.0, -1.0, -1.0)))), C.a);\n\
			 } else if (fxyt.w > 0.4 || fxyt.w < 0.0)\n\
			     FragColor = texture(sky, 1.5*scale2*(r*16 + shift)); \n\
			 else\n\
			     FragColor = texture(grass, r*16);\n\
		 }\n\
		 ";
	
	const char* pSimplePerlinFragmentCode =
		"#version 130\n\
		 in vec2 r;\n\
		 uniform vec4 ground;\n\
		 uniform vec4 sky;\n\
		 uniform sampler2D map;\n\
		 out vec4 FragColor;\n\
		 void main()\n\
		 {\n\
		 	 vec4 fxyt = texture(map, r); \n\
			 float level = " STRINGIFY(MOTO_LEVEL/8192.0) ";\n\
			 if (fxyt.z > level)\n\
				 FragColor = ground;\n\
			 else if (fxyt.w > 0.4 || fxyt.w < 0.0)\n\
			     FragColor = sky; \n\
			 else\n\
			     FragColor = ground;\n\
		 }\n\
		 ";
#undef Fdxdy

	g_glSimpleProgram = compileProgram(pSimpleVertexCode, pSimpleFragmentCode, {"v"});
	g_glColorUniform = glGetUniformLocation(g_glSimpleProgram, "c");

	g_glPerlinProgram = compileProgram(pPerlinVertexCode, pPerlinFragmentCode, {"v", "ar"});
	g_glPerlinGroundUniform = glGetUniformLocation(g_glPerlinProgram, "ground");
	g_glPerlinSkyUniform = glGetUniformLocation(g_glPerlinProgram, "sky");
	g_glPerlinGrassUniform = glGetUniformLocation(g_glPerlinProgram, "grass");
	g_glPerlinMapUniform = glGetUniformLocation(g_glPerlinProgram, "map");
	g_glPerlinShiftUniform = glGetUniformLocation(g_glPerlinProgram, "shift");
	g_glPerlinScale2Uniform = glGetUniformLocation(g_glPerlinProgram, "scale2");

	g_glSimplePerlinProgram = compileProgram(pPerlinVertexCode, pSimplePerlinFragmentCode, { "v", "ar" });
	g_glSimplePerlinGroundUniform = glGetUniformLocation(g_glSimplePerlinProgram, "ground");
	g_glSimplePerlinSkyUniform = glGetUniformLocation(g_glSimplePerlinProgram, "sky");
	g_glSimplePerlinMapUniform = glGetUniformLocation(g_glSimplePerlinProgram, "map");

	g_glTextureProgram = compileProgram(pTextureVertexCode, pTextureFragmentCode, {"v", "at"});
	g_glTextureUniform = glGetUniformLocation(g_glTextureProgram, "image");

	g_glTextProgram = compileProgram(pTextureVertexCode, pTextFragmentCode, { "v", "at" });
	g_glTextFontUniform = glGetUniformLocation(g_glTextProgram, "image");
	g_glTextColorUniform = glGetUniformLocation(g_glTextProgram, "color");
}

float interpLin(vec2 P, float Map[][4], int n, int N)
{
	int i = (int)floor(P.y) + N;
	int j = (int)floor(P.x) + N;
	float rx = P.x - floor(P.x);
	float ry = P.y - floor(P.y);
	float a00 = Map[i%N*N + j%N][n];
	float a01 = Map[i%N*N + (j+1)%N][n];
	float a11 = Map[(i+1)%N*N + (j+1)%N][n];
	float a10 = Map[(i+1)%N*N + j%N][n];
	float b0 = a00*(1-rx) + a01*rx;
	float b1 = a10*(1-rx) + a11*rx;
	float c = b0*(1-ry) + b1*ry;
	return c;
}

// Load world map into texture.
void prepareWorldRendering(const MotoWorld& World)
{
	glBindTexture(GL_TEXTURE_2D, g_glMapTexture);

	static int N = 0;

	// Determine maximum supported texture size but don't use texture larger than 2048.
	if (N == 0)
	{
		for (N = 2048; N > 128; N /= 2)
		{
			glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_RGB32F, N, N, 0, GL_RGB, GL_FLOAT, nullptr);

			// The queried width will not be 0, if the texture format is supported.
			GLint Width;
			glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &Width);
			if (Width != 0)
				break;
		}
		cout << "Using " << N << 'x' << N << " texture for world map\n";
	}

	unique_ptr<float[][4]> Map(new float[N*N][4]);

	// This loop causes sensible delay when switching to new level.
	// We try to parallelize it with OpenMP if possible.
    #pragma omp parallel for
	for (int i = 0; i < N; i++)
		for (int j = 0; j < N; j++)
		{
			motoF(Map[i*N + j], float(j)/N, float(i)/N, &World);
			Map[i*N + j][0] /= MOTO_SCALE;
			Map[i*N + j][1] /= MOTO_SCALE;
		}

	const int i0 = 20*N/2048;
	const int i1 = 40*N/2048;
	for (int i = i0; i < i1; i++)
		for (int j = 0; j < N; j++)
		{
			Map[i*N + j][0] *= float(i - i0)/(i1 - i0);
			Map[i*N + j][1] *= float(i - i0)/(i1 - i0);
		}
	for (int i = 0; i < i0; i++)
		for (int j = 0; j < N; j++)
			Map[i*N + j][2] = 1.0f;

    #pragma omp parallel for
	for (int i = 0; i < N; i++)
		for (int j = 0; j < N; j++)
		{
			vec2 graddir(Map[i*N + j][0], Map[i*N + j][1]);
			graddir = graddir/(graddir.length() + 0.00001);
			vec2 P2 = vec2((float)j, (float)i) - graddir*float(N*MOTO_WHEEL_R/(MOTO_SCALE*MOTO_MAP_SIZE));
			float f2 = interpLin(P2, Map.get(), 2, N);
			float dx2 = interpLin(P2, Map.get(), 0, N);
			float dy2 = interpLin(P2, Map.get(), 1, N);
			float level = MOTO_LEVEL/8192.0;
			Map[i*N + j][3] = (level - f2)/(dx2*graddir.x + dy2*graddir.y);
		}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, N, N, 0, GL_RGBA, GL_FLOAT, Map.get());

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

static void drawCircle(const CView& View, vec2 pos, float R)
{
	const int NumSecions = 17;
	for (int j = 0; j < NumSecions; j++)
		pushAttribute(View(pos + R*ang(j*Tau/NumSecions)));
	drawArrays(GL_TRIANGLE_FAN, 1);
}

static void formRect(vec2 Rect[4], const vec2 OppositeVertices[2])
{
	Rect[0] = OppositeVertices[0];
	Rect[1] = vec2(OppositeVertices[0].x, OppositeVertices[1].y);
	Rect[2] = OppositeVertices[1];
	Rect[3] = vec2(OppositeVertices[1].x, OppositeVertices[0].y);
}

static void drawWorld(const CView& View, bool Schematic, bool IsMap, vec2 SkyShift)
{
	vec2 ScreenRect[4];
	vec2 WorldRect[4];
	formRect(ScreenRect, View.m_ScreenPos);
	formRect(WorldRect, View.m_WorldPos);

	for (int i = 0; i < 4; i++)
	{
		pushAttribute(ScreenRect[i]);
		pushAttribute(WorldRect[i]/(MOTO_SCALE*MOTO_MAP_SIZE));
	}

	if (!Schematic)
	{
		float Scale = ((View.m_ScreenPos[1] - View.m_ScreenPos[0])/(View.m_WorldPos[1] - View.m_WorldPos[0])).x;
		vec2 Position = 0.5*(View.m_WorldPos[0] + View.m_WorldPos[1]);

		glUseProgram(g_glPerlinProgram);

		setUniformVec2(g_glPerlinShiftUniform, SkyShift - Position/MOTO_SCALE);
		glUniform1f(g_glPerlinScale2Uniform, Scale);

		glBindTexture(GL_TEXTURE_2D, g_glGroundTexture);
		glUniform1i(g_glPerlinGroundUniform, 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, g_glSkyTexture);
		glUniform1i(g_glPerlinSkyUniform, 1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, g_glGrassTexture);
		glUniform1i(g_glPerlinGrassUniform, 2);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, g_glMapTexture);
		glUniform1i(g_glPerlinMapUniform, 3);
		glActiveTexture(GL_TEXTURE0);

		drawArrays(GL_TRIANGLE_FAN, 2);
	}
	else
	{
		glUseProgram(g_glSimplePerlinProgram);

		setUniformColor(g_glSimplePerlinGroundUniform, g_SchematicGround, 1.0f);
		setUniformColor(g_glSimplePerlinSkyUniform, IsMap ? g_SchematicMapSky : g_SchematicSky, 1.0f);

		glBindTexture(GL_TEXTURE_2D, g_glMapTexture);
		glUniform1i(g_glSimplePerlinMapUniform, 0);

		drawArrays(GL_TRIANGLE_FAN, 2);
	}
}

static void pushTexturePart(const vec2 Pos[2], int x, int y, int w, int h)
{
	vec2 Rect[4];
	formRect(Rect, Pos);

	int indices[6] = { 0, 1, 2, 0, 2, 3 };
	for (int j = 0; j < 6; j++)
	{
		int i = indices[j];
		pushAttribute(Rect[i]);
		pushAttribute(((i >= 2) + x)/float(w));
		pushAttribute(((i == 1 || i == 2) - y - 1)/float(h));
	}
}

static void pushCoin(const CView& View, const vec2 Pos[2], int iFrame)
{
	iFrame /= 13;
	vec2 PosView[2] = { View(Pos[0]), View(Pos[1]) };
	pushTexturePart(PosView, iFrame % 8, iFrame / 8, 8, 8);
}

static void flushCoins()
{
	glUseProgram(g_glTextureProgram);
	glBindTexture(GL_TEXTURE_2D, g_glCoinTexture);
	glUniform1i(g_glTextureUniform, 0);
	drawArrays(GL_TRIANGLES, 2);
}

static void drawSchematicCoin(const CView& View, const vec2 Pos[2], bool IsMap)
{
	glUseProgram(g_glSimpleProgram);
	setUniformColor(g_glColorUniform, g_SchematicCoin, 1.0f);

	vec2 C = 0.5f*(Pos[0] + Pos[1]);
	drawCircle(View, C, (IsMap? 1.3f : 1.0f)*0.5f*(Pos[1] - Pos[0]).x);
}

void drawWorldAndCoin(const CView& View, const MotoState& Frame, bool Schematic, bool IsMap, vec2 SkyShift)
{
	drawWorld(View, Schematic, IsMap, SkyShift);

	float k = sqrt((View.m_ScreenPos[1] - View.m_ScreenPos[0]).area()/(View.m_WorldPos[1] - View.m_WorldPos[0]).area());
	float R = float(MOTO_WHEEL_R)*max(1.0f, 0.007f/k);
	vec2 Finish((float)g_MotoFinish[0], (float)(uint32_t)g_MotoFinish[1]);
	vec2 Pos[2] = { Finish*g_MotoPosK - R, Finish*g_MotoPosK + R };
	renderCyclic(View, [&](const CView& View)
	{
		if (Schematic)
			drawSchematicCoin(View, Pos, IsMap);
		else
			pushCoin(View, Pos, Frame.iFrame);
	});
	if (!Schematic)
		flushCoins();
}

static void transform(vec2& P, vec2 v, float a)
{
	float x = P.x;
	float y = P.y;
	P = v + vec2(x*cos(a) - y*sin(a), x*sin(a) + y*cos(a));
}

static void pushImage(const CView& View, const CImage& Image, vec2 Pos, float a, bool Invert = false, float SizeX = 1.0f, bool InvertY = false)
{
	vec2 v[4];

	a += Invert ? -Image.angle : Image.angle;
	float sx = Image.size*fabs(Image.horz[1] - Image.horz[0]);
	float sy = Image.size*fabs(Image.vert[1] - Image.vert[0]);
	for (int i = 0; i < 4; i++)
	{
		int s0 = 2*(i % 2) - 1;
		int s1 = 2*(i / 2) - 1;
		v[i].x = SizeX*(s0*sx + Image.shift[0]);
		v[i].y = s1*sy + Image.shift[1];
		transform(v[i], Pos, a);
	}

	// Draw as 2 triangles
	int indices[6] = { 0, 1, 2, 1, 2, 3 };
	for (int i = 0; i < 6; i++)
	{
		pushAttribute(View(v[indices[i]]));
		pushAttribute(Image.horz[(!!(indices[i] % 2))]);
		pushAttribute(Image.vert[(!!(indices[i] / 2)) ^ InvertY]);
	}
}

static void pushImage(const CView& View, const CImage& Image, vec2 A, vec2 B, bool Invert = false, float SizeX = 1.0f, bool InvertY = false)
{
	vec2 AB = B - A;
	vec2 P = 0.5f*(A + B);
	float Size = SizeX*AB.length();
	pushImage(View, Image, P, arc(AB), Invert, Size, InvertY);
}

static vec2 computePosition(const int32_t iP[2], const int32_t iBase[2])
{
	vec2 fP;
	fP.x = iBase[0]*g_MotoPosK + (iP[0] - iBase[0])*g_MotoPosK;
	fP.y = uint32_t(iBase[1])*g_MotoPosK + (iP[1] - iBase[1])*g_MotoPosK;
	return fP;
}

void drawMoto(const CView& View, const MotoState& Frame, bool MotoDir)
{
	vec2 BikePos = computePosition(Frame.Bike.Pos, Frame.Bike.Pos);
	vec2 HeadPos = computePosition(Frame.HeadPos, Frame.Bike.Pos);
	vec2 WheelPos[2];
	WheelPos[0] = computePosition(Frame.Wheels[0].Pos, Frame.Bike.Pos);
	WheelPos[1] = computePosition(Frame.Wheels[1].Pos, Frame.Bike.Pos);

	vec2 A[2] = { vec2(-0.42f, 0.29f), vec2(0.0f, -0.4f) };
	for (int i = 0; i < 2; i++)
	{
		if (MotoDir)
			A[i].x = -A[i].x;
		bool b = (!!i) ^ MotoDir;
		pushImage(View, g_ImgWheel, WheelPos[b], Frame.Wheels[b].AngPos*g_MotoAngPosK);
		transform(A[i], BikePos, Frame.Bike.AngPos*g_MotoAngPosK);
		pushImage(View, g_ImgSuspensions[i], A[i], WheelPos[b], MotoDir, 1.1f);
	}
	pushImage(View, g_ImgBike, BikePos, Frame.Bike.AngPos*g_MotoAngPosK, MotoDir, MotoDir ? 1.0f : -1.0f);

	pushImage(View, g_ImgBody, HeadPos, Frame.Bike.AngPos*g_MotoAngPosK, MotoDir, MotoDir ? 1.0f : -1.0f);

	float r[2][2] = { { 0.54f, 0.46f }, { 0.5f, 0.6f } };
	vec2 B[2] = { vec2(-0.5f, 0.3f), vec2(0.1f, -0.45f) };
	vec2 C[2] = { vec2(0.15f, -0.2f), vec2(0.55f, -0.55f) };
	float S[2][2] = { { 2.6f, 2.5f }, { 2.2f, 1.6f } };
	for (int i = 0; i < 2; i++)
	{
		if (MotoDir)
		{
			B[i].x = -B[i].x;
			C[i].x = -C[i].x;
		}
		transform(B[i], BikePos, Frame.Bike.AngPos*g_MotoAngPosK);
		transform(C[i], HeadPos, Frame.Bike.AngPos*g_MotoAngPosK);
		vec2 d = B[i] - C[i];
		float p = 2*d.x;
		float q = 2*d.y;
		float dsq = d.x*d.x + d.y*d.y;
		float pqsq = 4*dsq;
		float r02 = r[i][0]*r[i][0];
		float s = dsq + r02 - r[i][1]*r[i][1];
		float D = q*q*(pqsq*r02 - s*s);
		vec2 P;
		if (D > 0)
		{
			D = sqrt(D);
			if ((d.y > 0) ^ i ^ (!MotoDir))
				D = -D;
			P.x = B[i].x + (-p*s - D)/pqsq;
			P.y = B[i].y + (-q*q*s + p*D)/(q*pqsq);
		}
		else
		{
			float k = r[i][0]/(r[i][0] + r[i][1]);
			P = B[i] + k*(C[i] - B[i]);
		}
		pushImage(View, g_ImgLimbs[i][1], C[i], P, MotoDir, S[i][1], !MotoDir);
		pushImage(View, g_ImgLimbs[i][0], P, B[i], MotoDir, S[i][0], !MotoDir);
	}

	glUseProgram(g_glTextureProgram);
	glBindTexture(GL_TEXTURE_2D, g_glBikeTexture);
	glUniform1i(g_glTextureUniform, 0);
	drawArrays(GL_TRIANGLES, 2);
}

void drawSchematicMoto(const CView& View, const MotoState& Frame)
{
	vec2 BikePos = computePosition(Frame.Bike.Pos, Frame.Bike.Pos);
	vec2 HeadPos = computePosition(Frame.HeadPos, Frame.Bike.Pos);
	vec2 WheelPos[2];
	WheelPos[0] = computePosition(Frame.Wheels[0].Pos, Frame.Bike.Pos);
	WheelPos[1] = computePosition(Frame.Wheels[1].Pos, Frame.Bike.Pos);

	glUseProgram(g_glSimpleProgram);
	setUniformColor(g_glColorUniform, g_SchematicMoto, 1.0f);

	float k = 1.4f;
	drawCircle(View, WheelPos[0], k*(float)MOTO_WHEEL_R);
	drawCircle(View, WheelPos[1], k*(float)MOTO_WHEEL_R);
	drawCircle(View, HeadPos, k*(float)MOTO_WHEEL_R*0.6f);
}

void drawText(const char* pText, int iLine, int iPos, float Size, int iColor, float Alpha)
{
	if (Alpha <= 0)
		return;

	float HSize = Size*2.0f*g_ViewportSize.x/g_ViewportSize.y;

	vec2 P;
	if (iLine > 0)
		P.y = 0.99f - iLine*HSize;
	else if (iLine < 0)
		P.y = -0.99f - (iLine + 1)*HSize;
	else
		P.y = -0.5f*HSize;

	if (iPos > 0)
		P.x = -0.99f + (iPos - 1)*Size;
	else if (iPos < 0)
		P.x = 0.99f + iPos*Size;
	else
		P.x = -0.5f*Size*strlen(pText);

	float SizeY = 2*Size*g_ViewportSize.x/g_ViewportSize.y;
	int j = 0;
	for (int i = 0; pText[i]; i++)
	{
		if (pText[i] == '\n')
		{
			P.y -= SizeY;
			j = 0;
			continue;
		}
		vec2 Ps[2];
		Ps[0] = P + vec2(Size*j, -0.0f/3.0f*SizeY) - 0.5*vec2(Size, SizeY);
		Ps[1] = Ps[0] + 2*vec2(Size, SizeY);
		pushTexturePart(Ps, pText[i] % 16, pText[i] / 16, 16, 8);
		j++;
	}

	glUseProgram(g_glTextProgram);
	glBindTexture(GL_TEXTURE_2D, g_glFontTexture);
	glUniform1i(g_glTextureUniform, 0);
	setUniformColor(g_glTextColorUniform, g_TextColors[iColor], Alpha);
	drawArrays(GL_TRIANGLES, 2);
}