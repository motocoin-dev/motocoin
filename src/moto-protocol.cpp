// Copyright (c) 2014 The Motocoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//--------------------------------------------------------------------
// This file is part of The Game of Motocoin and Motocoin-Qt.
//--------------------------------------------------------------------
// Protocol for communication between Motocoin-Qt and motogame.
// Currently all communication is done over stdio.
//--------------------------------------------------------------------

#include <cstring>
#include <string>
#include "moto-protocol.h"

using namespace std;

static const char* g_pMsgWork = "***Work:";
static const char* g_pMsgGetWork = "***GetWork:";
static const char* g_pMsgWorkAndPoW = "***WPoW:";

template<typename T>
inline string toHexString(const T& Object)
{
	union
	{
		T ObjectData;
		uint8_t Data[sizeof(T)];
	};
	ObjectData = Object;

	string HexString;
	for (unsigned int i = 0; i < sizeof(T); i++)
	{
		HexString += "0123456789abcdef"[Data[i] / 16];
		HexString += "0123456789abcdef"[Data[i] % 16];
	}
	return HexString;
}

string motoMessage(const MotoWork& Work)
{
	return string("\n") + g_pMsgWork + toHexString(Work) + "\n";
}

string motoMessage(const MotoWork& Work, const MotoPoW& PoW)
{
	return string("\n") + g_pMsgWorkAndPoW + toHexString(Work) + toHexString(PoW) + "\n";
}


static int fromHex(char C)
{
	if (C >= '0' && C <= '9')
		return C - '0';
	if (C >= 'a' && C <= 'z')
		return C - 'a' + 10;
	if (C >= 'A' && C <= 'Z')
		return C - 'A' + 10;
	return 0;
}

template<typename T>
static bool readObject(const char*& pMsg, T& Out)
{
	union
	{
		T ObjectData;
		uint8_t Data[sizeof(T)];
	};

    for (unsigned int i = 0; i < sizeof(T); i++)
	{
		if (!pMsg[0] || !pMsg[1])
			return false;
		Data[i] = fromHex(pMsg[0])*16 + fromHex(pMsg[1]);
		pMsg += 2;
	}
	Out = ObjectData;

	return true;
}

bool motoParseMessage(const char* pMsg, MotoWork& Work)
{
	if (strncmp(pMsg, g_pMsgWork, strlen(g_pMsgWork)) != 0)
		return false;

	pMsg += strlen(g_pMsgWork);
	return readObject(pMsg, Work);
}

bool motoParseMessage(const char* pMsg, MotoGetWork& Work)
{
  if (strncmp(pMsg, g_pMsgGetWork, strlen(g_pMsgGetWork)) != 0)
    return false;
  
  pMsg += strlen(g_pMsgGetWork);
  MotoGetWork gw;
  if(readObject(pMsg, gw)) {
    for(int i=0;i<80;i+=4) {
      Work.Block[i] = gw.Block[i+3];
      Work.Block[i+1] = gw.Block[i+2];
      Work.Block[i+2] = gw.Block[i+1];
      Work.Block[i+3] = gw.Block[i];
    }
    return true;
  }
  return false;
}

bool motoParseMessage(const char* pMsg, MotoWork& Work, MotoPoW& PoW)
{
	if (strncmp(pMsg, g_pMsgWorkAndPoW, strlen(g_pMsgWork)) != 0)
		return false;

	pMsg += strlen(g_pMsgWorkAndPoW);
	return readObject(pMsg, Work) && readObject(pMsg, PoW);
}
