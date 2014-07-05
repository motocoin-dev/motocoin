// Copyright (c) 2014 The Motocoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//--------------------------------------------------------------------
// This file is part of The Game of Motocoin.
//--------------------------------------------------------------------

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
    #include <unistd.h>
#endif

#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <future>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <thread>
#include <mutex>
#include <string>
#include <vector>
#include <exception>
#include <sstream>
using namespace std;
using namespace chrono;

#include "../moto-engine.h"
#include "../moto-protocol.h"
#include "vec2.hpp"
#include "graphics.hpp"
#include "render.hpp"

static thread g_InputThread;
static vector<string> g_InputLines;
static mutex g_InputMutex;

// View properties.
static float g_RenderScale = 0.1f;
static float g_MapSize = 0.4f;
static bool g_ShowTimer = true;
static bool g_ShowHelp = false;
static bool g_ShowMap = true;
static bool g_OverallView = false;
static int g_WaitControl = -1;
static double g_Speed = 1.0;
static const float g_ControlsLetterSize = 0.018f;

// Sky shift.
static int32_t g_PrevIntPosition[2]; // used in sky shifting calculation
static vec2 g_SkyShift;

static MotoWork  g_Work;
static MotoWork  g_NextWork;
static MotoWorld g_World;
static MotoState g_FirstFrame;
static MotoState g_Frame;
static MotoPoW   g_PoW;

static bool g_HasNextWork = false;
static bool g_PlayingForFun = true;
static bool g_SchematicMainView = false;

static enum
{
	STATE_PLAYING,
	STATE_REPLAYING,
	STATE_DEAD,
	STATE_SUCCESS
} g_State;

static bool g_MotoDir = false;

static double g_PrevTime;
static double g_PlayTime;
static double g_ProgramStartTime;

static GLFWwindow* g_pWindow;

enum EAction
{
	ACTION_SHOW_CONTROLS,
	ACTION_RESTART_EVEL,
	ACTION_NEXT_LEVEL,
	ACTION_ROTATE_CCW,
	ACTION_ROTATE_CW,
	ACTION_GAS,
	ACTION_BRAKE,
	ACTION_TURNAROUND,
	ACTION_INC_TIME_SPEED,
	ACTION_DEC_TIME_SPEED,
	ACTION_REWIND,
	ACTION_SWITCH_VIEW,
	ACTION_INC_SCALE,
	ACTION_DEC_SCALE,
	ACTION_MAP,
	ACTION_INC_MAP,
	ACTION_DEC_MAP,

	ACTION_COUNT
};

string getKeyName(int Key)
{
	if (Key >= 33 && Key <= 96)
	{
		char Str[2] = { (char)Key, 0};
		return Str;
	}
	if (Key >= 290 && Key <= 298)
	{
		char Str[3] = { 'F', char('0' + Key - 289), 0 };
		return Str;
	}
	if (Key >= 299 && Key <= 314)
	{
		char Str[4] = { 'F', char('0' + (Key - 299) / 10), char('0' + char(Key - 299) % 10), 0 };
		return Str;
	}
	if (Key >= 320 && Key <= 329)
	{
		char Str[2] = { char('0' + Key - 320), 0 };
		return string("Keypad ") + Str;
	}
	if (Key < -10)
		return  "Press new key...";

	/* Printable keys */
	switch (Key)
	{
	case GLFW_KEY_SPACE: return "Space";
	case GLFW_KEY_WORLD_1: return "World 1"; /* non-US #1 */
	case GLFW_KEY_WORLD_2: return "World 2"; /* non-US #2 */

	/* Function keys */
	case GLFW_KEY_ESCAPE: return "Escape";
	case GLFW_KEY_ENTER: return "Enter";
	case GLFW_KEY_TAB: return "Tab";
	case GLFW_KEY_BACKSPACE: return "Backspace";
	case GLFW_KEY_INSERT: return "Insert";
	case GLFW_KEY_DELETE: return "Delete";
	case GLFW_KEY_RIGHT: return "\x3";
	case GLFW_KEY_LEFT: return "\x1";
	case GLFW_KEY_DOWN: return "\x4";
	case GLFW_KEY_UP: return "\x2";
	case GLFW_KEY_PAGE_UP: return "Page Up";
	case GLFW_KEY_PAGE_DOWN: return "Page Down";
	case GLFW_KEY_HOME: return "Home";
	case GLFW_KEY_END: return "End";
	case GLFW_KEY_CAPS_LOCK: return "Caps Lock";
	case GLFW_KEY_SCROLL_LOCK: return "Scroll Lock";
	case GLFW_KEY_NUM_LOCK: return "Num Lock";
	case GLFW_KEY_PRINT_SCREEN: return "Print Screen";
	case GLFW_KEY_PAUSE: return "Pause";
	case GLFW_KEY_KP_DECIMAL: return "Keypad .";
	case GLFW_KEY_KP_DIVIDE: return "Keypad /";
	case GLFW_KEY_KP_MULTIPLY: return "Keypad *";
	case GLFW_KEY_KP_SUBTRACT: return "Keypad -";
	case GLFW_KEY_KP_ADD: return "Keypad +";
	case GLFW_KEY_KP_ENTER: return "Keypad Enter";
	case GLFW_KEY_KP_EQUAL: return "Keypad =";
	case GLFW_KEY_LEFT_SHIFT: return "Left Shift";
	case GLFW_KEY_LEFT_CONTROL: return "Left Control";
	case GLFW_KEY_LEFT_ALT: return "Left Alt";
	case GLFW_KEY_LEFT_SUPER: return "Left Super";
	case GLFW_KEY_RIGHT_SHIFT: return "Right Shift";
	case GLFW_KEY_RIGHT_CONTROL: return "Right Control";
	case GLFW_KEY_RIGHT_ALT: return "Right Alt";
	case GLFW_KEY_RIGHT_SUPER: return "Right Super";
	case GLFW_KEY_MENU: return "Menu";
	}

	return "Unknown";
}

struct Control
{
	const char* pName;
	int Key;

	string toString()
	{
		return string(pName) + " - " + getKeyName(Key);
	}
};

static Control g_Controls[ACTION_COUNT] =
{
	{ "Show/Hide controls", GLFW_KEY_F1 },
	{ "Restart current level", GLFW_KEY_F5 },
	{ "Go to next level", GLFW_KEY_F6 },

	{ "Rotate counter-clockwise", GLFW_KEY_LEFT },
	{ "Rotate clockwise", GLFW_KEY_RIGHT },
	{ "Accelerate", GLFW_KEY_UP },
	{ "Brake", GLFW_KEY_DOWN },
	{ "Turnaround", GLFW_KEY_SPACE },

	{ "Decrease time speed", GLFW_KEY_Z },
	{ "Increase time speed", GLFW_KEY_X },
	{ "Rewind", GLFW_KEY_R },

	{ "Switch view", GLFW_KEY_S },
	{ "Increase scale", GLFW_KEY_KP_ADD },
	{ "Decrease scale", GLFW_KEY_KP_SUBTRACT },
	{ "Show/Hide map", GLFW_KEY_M },
	{ "Magnify map", GLFW_KEY_KP_MULTIPLY },
	{ "Reduce map", GLFW_KEY_KP_DIVIDE }
};

static string printControls(int iLine, bool Exclude)
{
	ostringstream Controls;
	auto print = [&](int i)
	{
		if ((i == iLine) ^ Exclude)
		    Controls << "    " << g_Controls[i].toString();
		Controls << '\n';
	};
	if (Exclude)
		Controls << "General:\n";
	else
		Controls << "\n";
	for (int i = 0; i < 3; i++)
		print(i);
	if (Exclude)
		Controls << "\nGameplay:\n";
	else
		Controls << "\n\n";
	for (int i = 3; i < 8; i++)
		print(i);
	if (Exclude)
		Controls << "\nTime:\n";
	else
		Controls << "\n\n";
	for (int i = 8; i < 11; i++)
		print(i);
	if (Exclude)
		Controls << "\nView:\n";
	else
		Controls << "\n\n";
	for (int i = 11; i < 17; i++)
		print(i);
	return Controls.str();
}

static int getHoveredControl()
{
	double MouseX, MouseY;
	glfwGetCursorPos(g_pWindow, &MouseX, &MouseY);
	if (MouseX/g_ViewportSize.x > 0.5)
		return -1;
	int iHoverLine = MouseY/(g_ViewportSize.x*g_ControlsLetterSize);
	switch (iHoverLine)
	{
	case 0:
	case 4:
	case 5:
	case 11:
	case 12:
	case 16:
	case 17:
		iHoverLine = -1;
	}
	iHoverLine -= 1;
	if (iHoverLine > 3)
		iHoverLine -= 2;
	if (iHoverLine > 8)
		iHoverLine -= 2;
	if (iHoverLine > 11)
		iHoverLine -= 2;
	return iHoverLine;
}

static CView getBigView()
{
	float Size = g_RenderScale*sqrt(g_ViewportSize.area());

	vec2 Position = vec2((float)g_Frame.Bike.Pos[0], (float)uint32_t(g_Frame.Bike.Pos[1]))*g_MotoPosK;

	vec2 p0 = Position - g_ViewportSize/Size;
	vec2 p1 = Position + g_ViewportSize/Size;
	vec2 ScreenPos[2] = { vec2(-1.0f, -1.0f), vec2(1.0f, 1.0f) };
	vec2 WorldPos[2] = { p0, p1 };

	return CView(ScreenPos, WorldPos);
}

static CView getMapView()
{
	float kxy = g_ViewportSize.x/g_ViewportSize.y;
	float kyx = g_ViewportSize.y/g_ViewportSize.x;
	vec2 ScreenPos[2] = { vec2(1.0f - g_MapSize*sqrt(kyx), -1.0f), vec2(1.0f, -1.0f + (1/1.2f)*g_MapSize*sqrt(kxy)) };

	float WorldSize = MOTO_MAP_SIZE*MOTO_SCALE;
	vec2 WorldPos[2] = { vec2(-0.6f, -0.0f)*WorldSize, vec2(0.6f, 1.0f)*WorldSize };
	return CView(ScreenPos, WorldPos);
}

static CView getOverallView()
{
	vec2 ScreenPos[2] = { vec2(-1.0f, -1.0f), vec2(1.0f, 1.0f) };

	vec2 Scale(1.0f, 1.0f);
	if (g_ViewportSize.y < g_ViewportSize.x)
		Scale.x /= g_ViewportSize.y/g_ViewportSize.x;
	else
		Scale.y /= g_ViewportSize.x/g_ViewportSize.y;

	vec2 WorldSize = Scale*MOTO_MAP_SIZE*MOTO_SCALE;
	vec2 WorldPos[2] = { vec2(-0.5f, 0.0f)*WorldSize, vec2(0.5f, 1.0f)*WorldSize };
	return CView(ScreenPos, WorldPos);
}

// Called each frame.
static void draw()
{
	//glClear(GL_COLOR_BUFFER_BIT);

	// Shift sky.
	g_SkyShift.x += 0.3f*(g_Frame.Bike.Pos[0] - g_PrevIntPosition[0])*g_MotoPosK/MOTO_SCALE;
	g_SkyShift.y += 0.3f*(g_Frame.Bike.Pos[1] - g_PrevIntPosition[1])*g_MotoPosK/MOTO_SCALE;
	g_PrevIntPosition[0] = g_Frame.Bike.Pos[0];
	g_PrevIntPosition[1] = g_Frame.Bike.Pos[1];
	
	// Render main view.
	if (!g_OverallView)
	{
		CView View = getBigView();
		drawWorldAndCoin(View, g_Frame, g_SchematicMainView, false, g_SkyShift);
		drawMoto(View, g_Frame, g_MotoDir);
	}

	// Render map view.
	if (g_ShowMap && !g_OverallView)
	{
		CView View = getMapView();
		setScissor(View.m_ScreenPos);
		drawWorldAndCoin(View, g_Frame, true, true);
		renderCyclic(View, bind(drawSchematicMoto, placeholders::_1, g_Frame));
		unsetScissor();
	}

	// Render overall view
	if (g_OverallView)
	{
		CView View = getOverallView();
		setScissor(View.m_ScreenPos);
		drawWorldAndCoin(View, g_Frame, g_SchematicMainView, false, g_SkyShift);
		renderCyclic(View, bind(drawMoto, placeholders::_1, g_Frame, g_MotoDir));
		unsetScissor();
	}
	
	if (g_ShowTimer)
	{
		int TimeLeft = g_Work.TimeTarget - g_Frame.iFrame;
		int Sec = TimeLeft / 250;
		int MilliSec = 4*(TimeLeft % 250);
		char Buffer[16];
		float LetterSize = 0.03f;
		sprintf(Buffer, "%02i.%03i", Sec, MilliSec);
		drawText(Buffer, 1, -7, LetterSize, (TimeLeft == 0) ? 1 : 0);
		sprintf(Buffer, "%i", MOTO_MAX_INPUTS - g_PoW.NumUpdates);
		drawText(Buffer, 2, -7, LetterSize, (MOTO_MAX_INPUTS - g_PoW.NumUpdates == 0) ? 1 : 0);
	}

	float LetterSize = 0.02f;
	if (g_ShowHelp)
	{
		float LetterSize = g_ControlsLetterSize;
		int iHoverLine = (g_WaitControl == -1) ? getHoveredControl() : g_WaitControl;
		string Str = printControls(iHoverLine, true);
		drawText(Str.c_str(), 1, 1, LetterSize, 0);
		Str = printControls(iHoverLine, false);
		drawText(Str.c_str(), 1, 1, LetterSize, 3);
	}
	else
		drawText("Press F1 to show/change controls", 1, 1, LetterSize, 0, 1.0f - pow(float(glfwGetTime() - g_ProgramStartTime), 3.0f)*0.005f);

	if (g_Frame.Dead)
	{
		const char* pMsg = "Press F5 to restart or R to rewind";
		drawText(pMsg, 0, 0, 1.5f*LetterSize, 1);
	}
	if (g_State == STATE_SUCCESS)
	{
		const char* pMsg = "Congratulations!!!";
		drawText(pMsg, 0, 0, 1.5f*LetterSize, 2);
	}

	drawText(g_Work.Msg, -1, 1, LetterSize);
}

// Restart current world.
static void restart()
{
	if (g_State == STATE_SUCCESS)
		return;

	if (g_State == STATE_DEAD)
		g_State = STATE_PLAYING;

	g_PrevTime = glfwGetTime();
	g_PlayTime = 0.0f;
	g_Frame = g_FirstFrame;
	if (g_State != STATE_REPLAYING)
		g_PoW.NumUpdates = 0;

	g_PrevIntPosition[0] = g_Frame.Bike.Pos[0];
	g_PrevIntPosition[1] = g_Frame.Bike.Pos[1];
}

// Inform Motocoin-Qt that we abandoned this work.
static void releaseWork(const MotoWork& Work)
{
	cout << motoMessage(Work);
}

static void parseInput();

static void goToNextWorld()
{
	if (g_State == STATE_REPLAYING || g_State == STATE_SUCCESS)
		return;

	float LetterSize = 0.02f;
	const char* pMsg = "Generating next world...";
    char BUF[64];

    sprintf(BUF, pMsg, g_PoW.Nonce);
    glClear(GL_COLOR_BUFFER_BIT);
    drawText(BUF, 0, 0, 1.5f*LetterSize, 1);
    glfwSwapBuffers(g_pWindow);
    
	// Find next good world (some worlds are ill-formed).
	do {
        // If there is new work then switch to it.
        if (g_HasNextWork)
        {
          releaseWork(g_Work);
          g_Work = g_NextWork;
          g_PlayingForFun = false;
          g_HasNextWork = false;
          g_PoW.Nonce=0;
        }      
	    g_PoW.Nonce++;
        this_thread::sleep_for(milliseconds(1));
        parseInput();
    }
	while (!motoGenerateGoodWorld(&g_World, &g_FirstFrame, g_Work.Block, &g_PoW));
		
	prepareWorldRendering(g_World);
	restart();
}

static void readStdIn()
{
	while (true)
	{
		if (!cin.good())
			return;

		string Line;
		getline(cin, Line);

		if (cin.fail())
			return;

		unique_lock<mutex> Lock(g_InputMutex);
		g_InputLines.push_back(move(Line));
	}
}

static void parseInput()
{
	unique_lock<mutex> Lock(g_InputMutex);

	for (const string& Line : g_InputLines)
	{
		MotoWork Work;
		MotoPoW PoW;
		if (motoParseMessage(Line.c_str(), Work))
		{
			if (g_HasNextWork)
			{
				if (g_NextWork.IsNew)
					Work.IsNew = true;
				releaseWork(g_NextWork);
			}
			Work.TimeTarget &= MOTO_TARGET_MASK;
			g_NextWork = Work;
			g_HasNextWork = true;
		}
		if (motoParseMessage(Line.c_str(), Work, PoW))
		{
			g_State = STATE_REPLAYING;
			g_PlayingForFun = false;
            Work.TimeTarget &= MOTO_TARGET_MASK;
			g_Work = Work;
			g_PoW = PoW;
			motoGenerateWorld(&g_World, &g_FirstFrame, g_Work.Block, g_PoW.Nonce);
			prepareWorldRendering(g_World);
			restart();
		}
	}

	g_InputLines.clear();
}

static bool processSolution()
{
	bool Result = g_PoW.NumFrames < g_Work.TimeTarget && motoCheck(g_Work.Block, &g_PoW);
	if (!Result)
	{
		cout << "Error: Rechecking solution failed!" << endl;
		return false;
	}

	// Print solution, it will be parsed by Motocoin-Qt.
	cout << motoMessage(g_Work, g_PoW);
	return true;
}

static bool isPressed(EAction Action)
{
	int Key = g_Controls[Action].Key;
	return Key > 0 && glfwGetKey(g_pWindow, g_Controls[Action].Key) == GLFW_PRESS;
}

static void playWithInput(int NextFrame)
{
	NextFrame = min((int)g_Work.TimeTarget, NextFrame);

	// Get user input.
	static bool TurnWasAroundPressed = false;
	bool TurnAroundPressed = isPressed(ACTION_TURNAROUND) == GLFW_PRESS;
	if (TurnAroundPressed && !TurnWasAroundPressed)
		g_MotoDir = !g_MotoDir;
	TurnWasAroundPressed = TurnAroundPressed;

	EMotoAccel Accel = MOTO_IDLE;
	if (isPressed(ACTION_BRAKE) == GLFW_PRESS)
		Accel = MOTO_BRAKE;
	else if (isPressed(ACTION_GAS) == GLFW_PRESS)
		Accel = g_MotoDir ? MOTO_GAS_LEFT : MOTO_GAS_RIGHT;

	EMotoRot Rotation = MOTO_NO_ROTATION;
	if (isPressed(ACTION_ROTATE_CW) == GLFW_PRESS)
		Rotation = MOTO_ROTATE_CW;
	if (isPressed(ACTION_ROTATE_CCW) == GLFW_PRESS)
		Rotation = MOTO_ROTATE_CCW;

	switch (motoAdvance(&g_Frame, &g_PoW, &g_World, Accel, Rotation, NextFrame - g_Frame.iFrame))
	{
	case MOTO_FAILURE:
		g_State = STATE_DEAD;
		break;

	case MOTO_SUCCESS:
		if (g_PlayingForFun)
		    goToNextWorld();
		else
			g_State = processSolution() ? STATE_SUCCESS : STATE_DEAD;
		break;

	case MOTO_CONTINUE:
		break;
	}

	if (g_State != STATE_SUCCESS && g_Frame.iFrame >= g_Work.TimeTarget)
		g_Frame.Dead = true;
}

// Called each frame.
static void play()
{
	double Time = glfwGetTime();
	static int numfr = 0;
	static double lasttime = 0;
	numfr++;
	if (Time - lasttime > 1)
	{
		printf("%i\n", numfr);
		numfr = 0;
		lasttime = Time;
	}
	
	double TimeDelta = Time - g_PrevTime;
	g_PrevTime = Time;
	if (isPressed(ACTION_REWIND) == GLFW_PRESS)
		TimeDelta = -TimeDelta;
	if ((g_Frame.Dead && TimeDelta > 0) || g_State == STATE_SUCCESS)
		TimeDelta = 0.0;
	g_PlayTime += TimeDelta*g_Speed;
	if (g_PlayTime < 0.0f)
		g_PlayTime = 0.0f;

	int NextFrame = int(g_PlayTime/0.004);
	if (NextFrame == g_Frame.iFrame)
		return;

	if (NextFrame < g_Frame.iFrame)
	{
		g_Frame = g_FirstFrame;
		if (g_State != STATE_REPLAYING)
		    motoCutPoW(&g_PoW, NextFrame);
		if (g_State == STATE_DEAD)
			g_State = STATE_PLAYING;
		motoReplay(&g_Frame, &g_PoW, &g_World, NextFrame);
		return;
	}

	if (g_State == STATE_REPLAYING)
	{
		g_Frame = g_FirstFrame;
		motoReplay(&g_Frame, &g_PoW, &g_World, NextFrame);
		if (NextFrame >= g_PoW.NumFrames)
			restart();

		if (g_Frame.Accel == MOTO_GAS_LEFT)
			g_MotoDir = true;
		if (g_Frame.Accel == MOTO_GAS_RIGHT)
			g_MotoDir = false;
	}
	else if (g_State == STATE_PLAYING)
		playWithInput(NextFrame);
}

static MotoWork getWorkForFun()
{
	MotoWork Work;
	srand((unsigned int)system_clock::now().time_since_epoch().count());
	for (int i = 0; i < MOTO_WORK_SIZE; i++)
		Work.Block[i] = rand() % 256;
    Work.Block[MOTO_WORK_SIZE-1] = 0x00; //0x20;
    Work.Block[MOTO_WORK_SIZE-2] = 0x00; //0x70;
    Work.Block[MOTO_WORK_SIZE-3] = 0x0a; // | (3 << 6);
    Work.Block[MOTO_WORK_SIZE-4] = 0xF1;
	Work.IsNew = false;
	Work.TimeTarget = 250*60;
	//sprintf(Work.Msg, "Block %i, Reward %f MTC, Target %.3f", Work.BlockHeight, 5000000000/100000000.0, Work.TimeTarget/250.0);
	strncpy(Work.Msg, "No work available, you may play just for fun.", sizeof(Work.Msg));

	return Work;
}

static void onWindowResize(GLFWwindow *pWindow, int Width, int Height)
{
	glViewport(0, 0, Width, Height);
	g_ViewportSize.x = (float)Width;
	g_ViewportSize.y = (float)Height;
}

static void changeScale(float K)
{
	if (g_OverallView || g_RenderScale*K > 0.5f || g_RenderScale*K < 0.01f)
		return;

	g_SkyShift = g_SkyShift/K;
	g_RenderScale *= K;
}

// We can't save anything to file from this app. That's because of encoding hell it
// would be hard to do it reliable on all platforms. So we will let qt app to do it.
static void loadConfig(const char* pConfig)
{
	istringstream Str(pConfig);
	for (int i = 0; i < ACTION_COUNT; i++)
	{
		Str >> g_Controls[i].Key;
		char c;
		Str >> c;
	}
}

static void printConfig()
{
	cout << "***Config:*";
	for (int i = 0; i < ACTION_COUNT; i++)
		cout << g_Controls[i].Key << '-';
	cout << '\n';
}

static void onKeyPress(GLFWwindow* pWindow, int Key, int Scancode, int Action, int Mods)
{
	if (Action != GLFW_PRESS)
		return;

	if (Key == GLFW_KEY_ESCAPE && !g_ShowHelp)
	{
		glfwSetWindowShouldClose(g_pWindow, 1);
		return;
	}
	if (Key == GLFW_KEY_ESCAPE)
		Key = GLFW_KEY_F1;

	if (g_WaitControl != -1)
	{
		if (Key == GLFW_KEY_F1)
		{
			g_Controls[g_WaitControl].Key = -g_Controls[g_WaitControl].Key;
			g_WaitControl = -1;
			return;
		}
		g_Controls[g_WaitControl].Key = Key;
		g_WaitControl = -1;
		printConfig();
		return;
	}

	EAction Act = ACTION_COUNT;
	for (int i = 0; i < ACTION_COUNT; i++)
		if (g_Controls[i].Key == Key)
		{
			Act = (EAction)i;
			break;
		}
	if (Act == ACTION_COUNT)
		return;

	switch (Act)
	{
	case ACTION_SHOW_CONTROLS:
		g_ShowHelp = !g_ShowHelp;
		break;

	case ACTION_RESTART_EVEL:
		restart();
		break;

	case ACTION_NEXT_LEVEL:
		goToNextWorld();
		break;

	case ACTION_SWITCH_VIEW:
		g_OverallView = !g_OverallView;
		break;

	case ACTION_MAP:
		g_ShowMap = !g_ShowMap;
		break;

	case ACTION_INC_SCALE:
		changeScale(1.1f);
		break;

	case ACTION_DEC_SCALE:
		changeScale(1/1.1f);
		break;

	case ACTION_INC_MAP:
		g_MapSize *= 1.1f;
		break;

	case ACTION_DEC_MAP:
		g_MapSize /= 1.1f;
		break;

	case ACTION_INC_TIME_SPEED:
		g_Speed = max(g_Speed/1.1, 0.0003);
		break;

	case ACTION_DEC_TIME_SPEED:
		g_Speed = min(g_Speed*1.1, 3000.0);
		break;
	}
}

static void onMouseButtonPress(GLFWwindow *pWindow, int Button, int Action, int Mods)
{
	if (Button == GLFW_MOUSE_BUTTON_LEFT && Action == GLFW_PRESS && g_ShowHelp)
	{
		int iControl = getHoveredControl();
		if (iControl < 0 || iControl == ACTION_SHOW_CONTROLS)
			return;
		g_Controls[iControl].Key = -g_Controls[iControl].Key;
		g_WaitControl = iControl;
	}
}

void showError(const string& Error)
{
	cout << Error << endl;
	string Error2 = Error + "\nTry to use software rendering.";
#ifdef _WIN32
	MessageBoxA(NULL, Error2.c_str(), "Motogame Error", MB_ICONERROR);
#endif
}

static void GLFW_ErrorCallback(int iError, const char* pString)
{
	stringstream Stream;
	Stream << "GLFW Error #" << iError << ": " << pString;
	showError(Stream.str());
}

int main(int argc, char** argv)
{
	bool NoFun = false;
	bool Fullscreen = false;
	for (int i = 0; i < argc; i++)
	{
		if (strcmp(argv[i], "-nofun") == 0)
			NoFun = true;
		if (strcmp(argv[i], "-schematic") == 0)
			g_SchematicMainView = true;
		if (strcmp(argv[i], "-fullscreen") == 0)
			Fullscreen = true;
		if (strcmp(argv[i], "-config") == 0 && i + 1 < argc)
		{
			i++;
			loadConfig(argv[i]);
		}
	}
	
	// Initialize GLFW library 
	glfwSetErrorCallback(GLFW_ErrorCallback);
	if (!glfwInit())
		return -1;

	// Create a OpenGL window
	int Width = 800;
	int Height = 600;
	if (Fullscreen)
	{
		GLFWmonitor* pMonitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* pMode = glfwGetVideoMode(pMonitor);
		Width = pMode->width;
		Height = pMode->height;
		g_pWindow = glfwCreateWindow(Width, Height, "The Game of Motocoin", pMonitor, NULL);
	}
	else
		g_pWindow = glfwCreateWindow(Width, Height, "The Game of Motocoin", NULL, NULL);
	if (!g_pWindow)
	{
		showError ("glfwCreateWindow failed");
		glfwTerminate();
		return -1;
	}

	// Make the window's context current
	glfwMakeContextCurrent(g_pWindow);

	// Set window callbacks
	glfwSetKeyCallback(g_pWindow, onKeyPress);
	glfwSetMouseButtonCallback(g_pWindow, onMouseButtonPress);
	glfwSetWindowSizeCallback(g_pWindow, onWindowResize);

	// GLFW doesn't call window resize callback when window is created so we call it.
	onWindowResize(g_pWindow, Width, Height);

	// Enable vsync. Doesn't work on Windows.
	glfwSwapInterval(1);

	// Initialize our world rendering stuff.
	initRenderer();

	g_ProgramStartTime = glfwGetTime();

	motoInitPoW(&g_PoW);

	// Let's start to play.
	g_Work = getWorkForFun();
	goToNextWorld();
	
	g_State = STATE_PLAYING;

	g_InputThread = thread(readStdIn);

#if 0
	auto PrevTime = steady_clock::now();
#endif
	int PrevTime = int(glfwGetTime()*1000);

	// Loop until the user closes the window.
	while (!glfwWindowShouldClose(g_pWindow))
	{
		parseInput();

		if (g_HasNextWork && (g_PlayingForFun || g_NextWork.IsNew)) // New block was found, our current work was useless, switch to new work.
		{
			g_State = STATE_PLAYING;
			goToNextWorld();
		}

		if (!(g_PlayingForFun && NoFun))
		{
			play();
			draw();
		}

		glfwSwapBuffers(g_pWindow);
		glfwPollEvents();

		// 1. glfwSwapInterval doesn't work on Windows.
		// 2. It may also not work on other platforms for some reasons (e.g. GPU settings).
		// So we are limiting FPS to 100 to not overuse CPU and GPU.
#if 0
		auto Time = steady_clock::now();
		auto TimeToWait = milliseconds(10) - (Time - PrevTime);
		if (TimeToWait.count() > 0)
			this_thread::sleep_for(TimeToWait);
		PrevTime = steady_clock::now();
#endif
		int Time = int(glfwGetTime()*1000);
		int TimeToWait = 10 - (Time - PrevTime);
		if (TimeToWait > 0)
			this_thread::sleep_for(milliseconds(TimeToWait));
		PrevTime = int(glfwGetTime()*1000);
	}

	glfwTerminate();

	printConfig();

	// g_InputThread is still running and there is no way to terminate it.
#ifdef _WIN32
	TerminateProcess(GetCurrentProcess(), 0);
#else
	_exit(0);
#endif

	return 0;
}