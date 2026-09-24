#pragma once
#include <windows.h>
#include <cstdint>

inline uint64_t __ROL8__(uint64_t v, int c) { return (v << (c & 63)) | (v >> ((-c) & 63)); }

#pragma once
#define CameraLocation 0x178
#define CameraRotation + CameraLocation + 0x10 
#define ROTATION_B_OFFSET 0x20 //never changes
#define ROTATION_C_OFFSET 0x1D0 //never changes

#define BONE_ARRAY_PADDING 0x10 

#define GWORLD 0x19DE3C10         //updated
#define GAME_INSTANCE 0x248       //updated
#define LOCAL_PLAYERS 0x38        //updated
#define PLAYER_CONTROLLER 0x30    //updated
#define LOCAL_PAWN 0x318          //updated
#define ACKNOWLEDGED_PAWN 0x318   //updated
#define PAWN_PRIVATE 0x2e8        //updated
#define ROOT_COMPONENT 0x1B0      //updated
#define RELATIVE_LOCATION 0x140   //updated
#define RELATIVE_ROTATION 0x158   //updated
#define bIsABot 0x27A



#define PLAYER_STATE 0x290        //updated
#define TEAM_INDEX 0xf11         //updated
#define GAME_STATE 0x1D0          //updated
#define PLAYER_ARRAY 0x288        //updated
#define MESH 0x2F0                //updated

#define COMPONENT_TO_WORLD 0x1e0  //Double Check! is this wrong there is no ESP 
#define BONE_ARRAY 0x650          //updated
#define BONE_ARRAY_CACHE + BONE_ARRAY + 0x10    //updated
#define LAST_SUBMIT_TIME 0x2f0
#define LAST_SUBMIT_TIME_ON_SCREEN 0x33C 
#define persistent_level  0x38   //updated

#define LEVELS 0x1e8             //updated
#define RotationInput 0x4B0 

#define CURRENT_WEAPON 0x998  //updated
#define WEAPON_DATA 0x630     //updated
#define AMMO_COUNT 0x10a4     //updated
#define PROJECTILE_SPEED 0x2250 //updated
#define A_ACTORS 0x38
#define ITEM_NAME 0x38        //updated
#define ITEM_DEFINITION 0x10 //updated / almost never changes
#define RARITY 0x179 
#define ITEM_TYPE 0xa0
#define PRIMARY_PICKUP_ITEM_ENTRY 0x368 //updated / almost never changes

#define USERNAME_COMPONENT 0xA00 //idk
#define WORLD_TIME_SECONDS 0x198 //just update CameraLocation if needed
#define NET_CONNECTION 0x4a8 // idk
#define TARGETED_FORT_PAWN 0x1630 // updated
#define CURRENT_VEHICLE 0x2ad0 // updated
#define DEFAULT_FLY_TIME 0x28d // idk
#define CUSTOM_TIME_DILATION 0x68 // Never changes
#define SPAWN_SOURCE_OVERRIDE 0x748 // idk

#define HABANERO_COMPONENT 0x908 //updated
#define RANKED_PROGRESS 0xd8 // updated
#define PLATFORM 0x400 //updated
#define WEAPON_NAME = ITEM_NAME // same as ITEM_NAME
#define ISDYING 0x720 //updatedBB
#define PLAYER_ID 0x274 // idk
#define VELOCITY 0x188 // idk
#define bIsDBNO 0x849 
#define ENGINE_OFFSET 0x19DE55F8 // updated
#define VIEWPORT_OFFSET 0xb70 // updated

#define PLAYERNAME 0x9C8 // check
#define DEFAULTFOV 0x374 // updated 
#define PLAYERCAMERAMANAGER 0x328 // updated 

template<typename t>
class TArray {
public:
    TArray() : tData(nullptr), iCount(0), iMaxCount(0) {}
    t* tData;
    int iCount;
    int iMaxCount;
};
