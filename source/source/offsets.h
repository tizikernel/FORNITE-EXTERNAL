#pragma once
#include <cstdint>

// Fortnite Offsets — Oct 2026 (legacy macros for sdk.h / actorloop.h)

#define CameraLocation 0x168
#define CameraRotation 0x178
#define ROTATION_B_OFFSET 0x20
#define ROTATION_C_OFFSET 0x1D0
#define BONE_ARRAY_PADDING 0x10

#define GWORLD 0x1B2C5BA0
#define GAME_INSTANCE 0x238
#define LOCAL_PLAYERS 0x38
#define PLAYER_CONTROLLER 0x30
#define LOCAL_PAWN 0x318
#define ACKNOWLEDGED_PAWN 0x318
#define PAWN_PRIVATE 0x2E8
#define ROOT_COMPONENT 0x1B0
#define RELATIVE_LOCATION 0x140
#define RELATIVE_ROTATION 0x158

#define PLAYER_STATE 0x290
#define TEAM_INDEX 0xF69
#define GAME_STATE 0x1C0
#define PLAYER_ARRAY 0x288
#define MESH 0x2F0

#define COMPONENT_TO_WORLD 0x1E0
#define BONE_ARRAY 0x660
#define BONE_ARRAY_CACHE 0x670
#define LAST_SUBMIT_TIME 0x2F0
#define LAST_SUBMIT_TIME_ON_SCREEN 0x530
#define persistent_level 0x38

#define LEVELS 0x1D8
#define RotationInput 0x4B0

#define CURRENT_WEAPON 0x9D0
#define WEAPON_DATA 0x628
#define AMMO_COUNT 0x1100
#define PROJECTILE_SPEED 0x2608
#define A_ACTORS 0x1A0
#define ITEM_NAME 0x38
#define ITEM_DEFINITION 0x10
#define RARITY 0x179
#define ITEM_TYPE 0xA0
#define PRIMARY_PICKUP_ITEM_ENTRY 0x368

#define USERNAME_COMPONENT 0x9E8
#define WORLD_TIME_SECONDS 0x188
#define NET_CONNECTION 0x4A8
#define TARGETED_FORT_PAWN 0x16C0
#define CURRENT_VEHICLE 0x2AD0
#define DEFAULT_FLY_TIME 0x28D
#define CUSTOM_TIME_DILATION 0x68
#define SPAWN_SOURCE_OVERRIDE 0xB78

#define HABANERO_COMPONENT 0x918
#define RANKED_PROGRESS 0xD8
#define PLATFORM 0x400
#define WEAPON_NAME ITEM_NAME
#define ISDYING 0x728
#define PLAYER_ID 0x274
#define VELOCITY 0x188
#define ENGINE_OFFSET 0x1B2C7518
#define VIEWPORT_OFFSET 0xB70

#define PLAYERNAME 0x9E8
#define DEFAULTFOV 0x374
#define PLAYERCAMERAMANAGER 0x328

// Keep bIsABot / bIsDBNO as macros only if not colliding
#ifndef bIsABot
#define bIsABot 0x27A
#endif
#ifndef bIsDBNO
#define bIsDBNO 0x881
#endif
