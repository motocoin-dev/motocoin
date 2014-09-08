// Copyright (c) 2014 The Motocoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//--------------------------------------------------------------------
// This file is part of The Game of Motocoin, Motocoin-Qt and motocoind.
//--------------------------------------------------------------------
// Physics engine for motogame.
//--------------------------------------------------------------------

#ifndef MOTOCOIN_MOTOENGINE_H
#define MOTOCOIN_MOTOENGINE_H

#include <stdint.h>
#ifndef __cplusplus
    #include <stdbool.h>
#endif

/** Maximum number of user inputs for one play. */
#define MOTO_MAX_INPUTS 60

/** World map size (number of grid cells for perlin noise). */
#define MOTO_MAP_SIZE 16

/** Number of bytes in block description. */
#define MOTO_WORK_SIZE 76

#define MOTO_TARGET_MASK 0x3FFF

/** World scale coefficient. */
#define MOTO_SCALE 5

/**
World is generated as countour line f(x, y) == MOTO_LEVEL,
where f(x, y) is perlin noise function generated from block data.
Values that are higher than this are ground and values that are lower are sky.
Entire range is [-8192; 8192].
*/
#define MOTO_LEVEL 650

/** Maximum number of frames in one play. This is maximum time target. */
#define MOTO_MAX_FRAMES 500*30

/* Start and finish positions (in integer coordinates). */
#ifdef TESTMODE
static const int32_t g_MotoStart[2]  = {          0, -300000000 }; /**< Position of start  in world map (in integer coordinates). */
static const int32_t g_MotoFinish[2] = {          0, -4000000000 }; /**< Position of finish in world map (in integer coordinates). */
static const int64_t g_MotoStartL[2]  = {          0, -300000000+((long)1<<32) }; /**< Position of start  in world map (in integer coordinates). */
static const int64_t g_MotoFinishL[2] = {          0, -4000000000 }; /**< Position of finish in world map (in integer coordinates). */
#else
static const int32_t g_MotoStart[2]  = {          0, -300000000 }; /**< Position of start  in world map (in integer coordinates). */
static const int32_t g_MotoFinish[2] = { 2140000000,  400000000 }; /**< Position of finish in world map (in integer coordinates). */
static const int64_t g_MotoStartL[2]  = {          0, -300000000+((int64_t)1<<32) }; /**< Position of start  in world map (in integer coordinates). */
static const int64_t g_MotoFinishL[2] = { 2140000000,  400000000 }; /**< Position of finish in world map (in integer coordinates). */
#endif

/**
Coefficient for converting integer coordinates to real ones.
Each coordinate is stored as int32_t. When integer overflows it automatically wraps,
this is used for looping world in horizontal direction.
Just multiply any coordinate stored in int32_t by g_MotoPosK and you will get real coordinate.
*/
static const float g_MotoPosK = MOTO_MAP_SIZE*MOTO_SCALE/4294967296.0f;

/**
Coefficient for converting integer angles to real ones (radians).
Each angle is represented by int32_t which represents part of full turn.
Just multiply any angle stored in int32_t by g_MotoAngPosK and you will get radians.
+-------------------+-------------+--------+-------------+
|               0∙τ |          0° |      0 |           0 |
|           (1/4)∙τ |         90° |   2^30 |  1073741824 |
|          ±(1/2)∙τ |       ±180° |  -2^31 | -2147483648 |
| (3/4)∙τ, -(1/4)∙τ |  270°, -90° |  -2^30 | -1073741824 |
+-------------------+-------------+--------+-------------+
*/
static const float g_MotoAngPosK = (float)(6.2831853071795864769/4294967296.0);

/** Wheel radius in real cordinates. */
#define MOTO_WHEEL_R 0.4

/** Bike acceleration/braking control. */
typedef enum
{
	MOTO_IDLE,      /**< Player does no acceleration nor braking. */
	MOTO_GAS_LEFT,  /**< Player is currently accelerating left wheel. */
	MOTO_GAS_RIGHT, /**< Player is currently accelerating right wheel. */
	MOTO_BRAKE,     /**< Player is currently braking. */
} EMotoAccel;

/** Bike rotation control. */
typedef enum
{
	MOTO_NO_ROTATION,
	MOTO_ROTATE_CW,  /**< Player is currently rotating clockwise. */
	MOTO_ROTATE_CCW, /**< Player is currently rotating counter-clockwise. */
} EMotoRot;

enum Filter
{
	FILTER_NONE,
	FILTER_BASIC,
	FILTER_DOUBLE,

	FILTER_COUNT
};

extern Filter g_Filter;

/** \brief Pseudo-randomly generated world.
*
* Contains no state.
*/
typedef struct
{
	/** Values for Perlin-noise. */
	int8_t Map[MOTO_MAP_SIZE][MOTO_MAP_SIZE][2];
} MotoWorld;

/** \brief Physical body.
*
* Structure describing physical body: position and velocity (both linear and angular).
* Used to describe bike itself and each wheel.
*/
typedef struct
{
	int32_t Pos[2]; /**< Position in integer coordinates. */
	int32_t Vel[2]; /**< Velocity in integer coordinates. */
	int32_t AngPos; /**< Angular position stored as integer angle. */
	int32_t AngVel; /**< Angular velocity stored as integer angle. */
} MotoBody;

/** \brief Instant state of the game.
*
* Dynamic game state (not including static world). At each frame new state is generated.
*/
typedef struct
{
	int16_t iFrame;      /**< Current frame index. */
	int16_t iLastRotate; /**< Frame index when last rotation was initiated. */
	EMotoAccel Accel;    /**< Acceleration/braking control. */
	EMotoRot   Rotation; /**< Rotation control. */
	MotoBody Wheels[2];  /**< Wheels states (position and velocity). */
	MotoBody Bike;       /**< Bike state (position and velocity). */
	int32_t  HeadPos[2]; /**< Head position in integer coordinates. */
	int32_t  HeadVel[2]; /**< Head velocity in integer coordinates. */
	bool     Dead;
} MotoState;

/** \brief Result of frame evaluation.
*
* Value of this type is generated at the end of each frame.
*/
typedef enum
{
	MOTO_CONTINUE, /**< Game is not over, player can continue to play. */
	MOTO_FAILURE,  /**< Game ended with failure. */
	MOTO_SUCCESS,  /**< Player successfully completed game. */
} EMotoResult;

/** \brief Analog of Bitcoin nonce.
*
* Nonce + player input. This is proof of work.
* Can be used to completly replay game and check that it was successfull.
*/
typedef struct
{
	uint32_t Nonce; /**< Nonce that was used to generate map. */
	uint16_t NumFrames; /**< Number of frames used to complete level. */
	uint16_t NumUpdates; /**< Number of changes in player input. */
	uint16_t Updates[MOTO_MAX_INPUTS]; /**< iFrameDelta*12 + Rotation*4 + Accel. */
} MotoPoW;

/** Structure passed between Motocoin-Qt and Motogame. */
typedef struct
{
	uint16_t IsNew; /**< All previous blocks are now useless. */
	int16_t  TimeTarget;
	int32_t height;
	char     Msg[128]; /**< Message for user. Showed at lower left corner of the screen. */
	uint8_t  Block[MOTO_WORK_SIZE];
} MotoWork;

typedef struct
{
  uint8_t  Block[80]; //full normal header less sha padding
} MotoGetWork;

#ifdef NO_OPENSSL_SHA
  extern void *SHA512(uint8_t *buffer, size_t len, void *resblock);
#else
  #include <openssl/sha.h>
#endif

void initTables();

/** Initialize proof-of-work to zero. */
void motoInitPoW(MotoPoW* pPoW);

/** \brief Check proof-of-work (proof-of-play).
*
* Complete check. Suitable for motocoin client, but not for game.
*
* @param pBlock (in) - Random data (block data) to generate world.
* @param pPoW (in) - All player input + selected nonce.
*
* @return true if game was completed in less than TimeTarget frames.
*/
bool motoCheck(const uint8_t* pBlock, MotoPoW* pPoW);

/** \brief Generate pseudo-random world.
*
* @param pWorld (out) - Variable to store generated world.
* @param pState (out) - Variable to store initial game state.
* @param pBlock (in) - Random data (block data) to generate world.
* @param Nonce  (in) - Selected nonce.
*
* @return true if generated world is well-formed and false otherwise. Ill-formed world is one that seems impossible to complete but not necessarily so.
*/
bool motoGenerateWorld(MotoWorld* pWorld, MotoState* pState, const uint8_t* pBlock, uint32_t Nonce);
bool motoGenerateGoodWorld(MotoWorld* pWorld, MotoState* pState, const MotoWork* pWork, MotoPoW* pow);
/** \brief Evaluate several game frames.
*
* @param pState (in/out) - Game state that will be modified.
* @param pPoW (in/out) - Variable that accumulates all player input. If game is completed successfully then data in this variable can be saved as proof-of-work.
* @param pWorld (in) - World previously generated with motoGenerateWorld.
* @param Accel - Current input from player.
* @param Rotation - Current input from player.
* @param NumFrames - Number of frames to evaluate.
*
* @return Result of evaluation: continue, failure or success.
*/
EMotoResult motoAdvance(MotoState* pState, MotoPoW* pPoW, const MotoWorld* pWorld, EMotoAccel Accel, EMotoRot Rotation, int16_t NumFrames);

bool motoReplay(MotoState* pState, MotoPoW* pPoW, const MotoWorld* pWorld, int16_t iToFrame);

void motoF(float Fdxdy[3], float x, float y, const MotoWorld* pWorld);

void motoCutPoW(MotoPoW* pPoW, int16_t iToFrame);

#endif /* MOTOCOIN_MOTOENGINE_H */
