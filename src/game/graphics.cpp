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

#include <GL/glew.h>

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
using namespace std;

#include "vec2.hpp"
#include "graphics.hpp"

vec2 g_ViewportSize;

// OpenGL buffer object for vertex attributes.
// We use only one buffer that is always bound.
static GLuint g_glBuffer;

// This is hard-coded limit on buffer size.
// We do just very simple drawings and therefore can compute its size in advance.
static const int g_BufferSize = 10240;

// Attributes that aren't yet coppied to GPU memory are stored temporarily in this buffer.
static int g_NumAttributes = 0;
static float g_Attributes[g_BufferSize];

void initGraphics()
{
	// Print some OpenGL info.
	cout << "GL venndor: "    << glGetString(GL_VENDOR)   << endl;
	cout << "GL renderer: "   << glGetString(GL_RENDERER) << endl;
	cout << "GL version: "    << glGetString(GL_VERSION)  << endl;
	cout << "GLSL version: "  << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	//cout << "GL extensions: " << GL::GetString(GL_EXTENSIONS) << endl;

	// Use standard alpha-blending.
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	// Initialize buffer object.
	glGenBuffers(1, &g_glBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, g_glBuffer); // Bind it forever.
	glBufferData(GL_ARRAY_BUFFER, g_BufferSize*sizeof(float), nullptr, GL_STREAM_DRAW); // Allocate buffer in GPU memory.
}

void pushAttribute(float a)
{
	// If you didn't change the code this condition will never be true.
	if (g_NumAttributes > g_BufferSize)
	{
		cout << "Error: g_NumAttributes = " << g_NumAttributes << " > g_BufferSize = " << g_BufferSize << ";" << endl;
		cout << "Dear programmer, there are too many attributes, please increase g_BufferSize!" << endl;
		exit(-1);
	}

	// Add attribute to the end of array.
	g_Attributes[g_NumAttributes++] = a;
}

void pushAttribute(vec2 A)
{
	pushAttribute(A.x);
	pushAttribute(A.y);
}

void drawArrays(GLenum ShapesType, int NumAttributesPerVertex)
{
#if 0 // code for computing value of g_BufferSize
	static int MaxAttributes = 0;
	if (g_NumAttributes > MaxAttributes)
	{
		MaxAttributes = g_NumAttributes;
		cout << "MaxAttributes = " << MaxAttributes << "!!!!!!!!!!\n";
	}
#endif
	// Our only buffer is already bound, so we just fill it with data.
	glBufferSubData(GL_ARRAY_BUFFER, 0, g_NumAttributes*sizeof(float), g_Attributes);

	// Enable and pass each attribute pointer.
	for (int i = 0; i < NumAttributesPerVertex; i++)
	{
		glEnableVertexAttribArray(i);
		glVertexAttribPointer(i, 2, GL_FLOAT, 0, 2*NumAttributesPerVertex*sizeof(float), (GLvoid*)(2*i*sizeof(float)));
	}

	// Main draw call.
	glDrawArrays(ShapesType, 0, g_NumAttributes/(2*NumAttributesPerVertex));

	// Disable all attribute pointers.
	for (int i = 0; i < NumAttributesPerVertex; i++)
		glDisableVertexAttribArray(i);

	// Clear array.
	g_NumAttributes = 0;
}

void setUniformVec2(GLint iLocation, vec2 V)
{
	glUniform2f(iLocation, V.x, V.y);
}

static GLuint compileShader(GLenum Type, const char* pCode)
{
	GLuint iShader = glCreateShader(Type);
	glShaderSource(iShader, 1, &pCode, nullptr);
	glCompileShader(iShader);

	// Output compilation log.
	int Len;
	char Buffer[1024];
	glGetShaderInfoLog(iShader, 1024, &Len, Buffer);
	cout << "OpenGL " << ((Type == GL_VERTEX_SHADER)? "vertex" : "fragment") << " shader compilaton log:" << endl;
	cout << Buffer;

	return iShader;
}

GLuint compileProgram(const char* pVertexCode, const char* pFragmentCode, initializer_list<const char*> Attributes)
{
	cout << "------------------------------------" << endl;
	cout << "Compiling OpenGL program..." << endl;
	GLuint iProgram = glCreateProgram();
	glAttachShader(iProgram, compileShader(GL_VERTEX_SHADER, pVertexCode));
	for (unsigned int i = 0; i < Attributes.size(); i++)
		glBindAttribLocation(iProgram, i, Attributes.begin()[i]);
	glAttachShader(iProgram, compileShader(GL_FRAGMENT_SHADER, pFragmentCode));
	glLinkProgram(iProgram);

	GLint Status;
	glGetProgramiv(iProgram, GL_LINK_STATUS, &Status);
	cout << "OpenGL program link status: " << Status << endl;
	cout << "------------------------------------" << endl;
	return iProgram;
}

GLuint loadTexture(const char* pName)
{
	const char* pPrefixes[4] = { "./", "./images/", "../images/", "../../images/" };

	cout << "Loading texture " << pName << "..." << endl;

	// Try to open file.
	FILE* pFile = nullptr;
	for (const char* pPrefix : pPrefixes)
	{
		string Name = string(pPrefix) + pName + ".bmp";
		pFile = fopen(Name.c_str(), "rb");
		if (pFile)
			break;
	}
	if (!pFile)
	{
		cout << "Error: Couldn't open file " << pName << endl;
		return 0;
	}

	GLuint iTexture = 0;

	// Values from BMP header
	char BM[2];
	uint32_t PixelOffset;
	uint32_t Size[2];
	uint16_t BitsPerPixel;
	uint32_t CompressionType;

	// Try to read everything necessary from file header.
	bool Read = fread(&BM, sizeof(char), 2, pFile) == 2 &&
		fseek(pFile, 10, SEEK_SET) == 0 && fread(&PixelOffset, sizeof(uint32_t), 1, pFile) == 1 &&
		fseek(pFile, 18, SEEK_SET) == 0 && fread(Size, sizeof(uint32_t), 2, pFile) == 2 &&
		fseek(pFile, 28, SEEK_SET) == 0 && fread(&BitsPerPixel, sizeof(uint16_t), 1, pFile) == 1 &&
		fseek(pFile, 30, SEEK_SET) == 0 && fread(&CompressionType, sizeof(uint32_t), 1, pFile) == 1;

	// Check that everything is correct.
	if (!Read)
		cout << "Error: couldn't read file " << pName << "." << endl;
	else if (BM[0] != 'B' || BM[1] != 'M')
		cout << "Error: File " << pName << " is not BMP." << endl;
	else if (!Size[0] || Size[0] > 1024 || !Size[1] || Size[1] > 1024)
	{
		cout << "Error: Image " << pName << " has resolution " << Size[0] << 'x' << Size[1] << endl;
		cout << "(max resoultion is 1024x1024)." << endl;
	}
	else if (BitsPerPixel != 8 && BitsPerPixel != 24 && BitsPerPixel != 32)
	{
		cout << "Error: Image " << pName << " has " << BitsPerPixel << " bits per pixel" << endl;
		cout << "(only 8, 24 and 32 bits per pixel images are supported)." << endl;
	}
	else if (CompressionType != 0)
	{
		cout << "Error: Image " << pName << " is compressed" << endl;
		cout << "(only uncompressed images are supported)." << endl;
	}
	else if (fseek(pFile, PixelOffset, SEEK_SET) != 0)
		cout << "Error: Seeking to " << PixelOffset << " in file " << pName << " failed." << endl;
	else
	{
		const int BytesPerPixel = BitsPerPixel/8;
		unique_ptr<uint8_t[]> pData(new uint8_t[Size[0]*Size[1]*BytesPerPixel]);

		for (uint32_t i = 0; i < Size[1]; i++) // Iterate over rows.
		{
			// Read pixel row from file to buffer.
			if (fread(&pData[i*Size[0]*BytesPerPixel], BytesPerPixel, Size[0], pFile) != Size[0])
				break;

			// Each row in BMP is aligned to 4 bytes.
			if ((Size[0]*BytesPerPixel) % 4 != 0)
			    if (fseek(pFile, 4 - (Size[0]*BytesPerPixel) % 4, SEEK_CUR) != 0)
				    break;

			// Premultiple alpha
			if (BytesPerPixel == 4)
			{
				for (uint32_t j = 0; j < Size[0]; j++)
				{
					uint32_t Offset = (i*Size[0] + j)*BytesPerPixel;
					uint8_t Alpha = pData[Offset + 3];
					pData[Offset + 0] = pData[Offset + 0]*Alpha / 255;
					pData[Offset + 1] = pData[Offset + 1]*Alpha / 255;
					pData[Offset + 2] = pData[Offset + 2]*Alpha / 255;
				}
			}
		}

		// Generate and bind OpenGL texture.
		glGenTextures(1, &iTexture);
		glBindTexture(GL_TEXTURE_2D, iTexture);

		// Fill it with data and release data buffer.
		GLint InternalFormat = (BytesPerPixel == 1) ? GL_RED : (BytesPerPixel == 3) ? GL_RGB : GL_RGBA;
		GLenum Format = (BytesPerPixel == 1) ? GL_RED : (BytesPerPixel == 3) ? GL_BGR : GL_BGRA;
		glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, Size[0], Size[1], 0, Format, GL_UNSIGNED_BYTE, pData.get());

		// Use the best possible filtering.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

		// Wrap texture.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glGenerateMipmap(GL_TEXTURE_2D);
	}
	fclose(pFile);

	return iTexture; // This will return 0 in case of an error.
}

void setScissor(const vec2 ScreenPos[2])
{
	vec2 Pos = (ScreenPos[0] + 1.0f)*g_ViewportSize/2.0f;
	vec2 Size = (ScreenPos[1] - ScreenPos[0])*g_ViewportSize/2.0f;

	glEnable(GL_SCISSOR_TEST);
	glScissor((int)round(Pos.x), (int)round(Pos.y), (int)round(Size.x), (int)round(Size.y));
}

void unsetScissor()
{
	glDisable(GL_SCISSOR_TEST);
}
