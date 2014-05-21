// Copyright (c) 2014 The Motocoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//--------------------------------------------------------------------
// This file is part of The Game of Motocoin and Motocoin-Qt.
//--------------------------------------------------------------------
// Protocol for communication between Motocoin-Qt and motogame.
// Currently all communication is done over stdio.
//--------------------------------------------------------------------

#ifndef MOTOCOIN_MOTOPROTOCOL_H
#define MOTOCOIN_MOTOPROTOCOL_H

#include <string>
#include "moto-engine.h"

std::string motoMessage(const MotoWork& Work);
std::string motoMessage(const MotoWork& Work, const MotoPoW& PoW);

bool motoParseMessage(const char* pMsg, MotoWork& Work);
bool motoParseMessage(const char* pMsg, MotoWork& Work,  MotoPoW& PoW);

#endif // MOTOCOIN_MOTOPROTOCOL_H