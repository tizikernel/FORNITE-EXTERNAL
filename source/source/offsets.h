#pragma once
#include <cstdint>

// ============================================================
//  Fortnite Offsets — v37.10 (Chapter 6, Season 4)
//  Fuente: itsunderscores/Fortnite-Offsets
//  Última actualización: Sep 7, 2026
//  NOTA: GWorld y PlayerName están ENCRIPTADOS en v37.x
// ============================================================

namespace offsets {

    // --------------------------------------------------------
    // CORE / GLOBALS
    // Relativos al base del módulo (FortniteClient-Win64-Shipping.exe)
    // --------------------------------------------------------
    namespace core {
        constexpr uintptr_t GWorld            = 0x1A9E6268; // ENCRIPTADO — usar decrypt abajo
        constexpr uintptr_t gEngine           = 0x1A9E7BD8;
        constexpr uintptr_t GameViewport      = 0xB70;      // UEngine::GameViewport
        constexpr uintptr_t GameInstance      = 0x240;      // UWorld::OwningGameInstance
        constexpr uintptr_t GameState         = 0x1C8;      // UWorld::GameState
        constexpr uintptr_t PersistentLevel   = 0x38;       // UWorld::PersistentLevel
        constexpr uintptr_t Levels            = 0x1E0;      // UWorld::Levels
        constexpr uintptr_t Actors            = 0x218;      // ULevelActorContainer::Actors
        constexpr uintptr_t PlayerArray       = 0x288;      // AGameStateBase::PlayerArray
        constexpr uintptr_t ServerWorldTime   = 0x2A0;      // AGameStateBase::ServerWorldTimeSecondsDelta
        constexpr uintptr_t Seconds           = 0x188;      // UWorld::TimeSeconds (via RotationPointer+0x10)
    }

    // --------------------------------------------------------
    // PLAYER
    // --------------------------------------------------------
    namespace player {
        constexpr uintptr_t Mesh              = 0x2F0;  // ACharacter::Mesh
        constexpr uintptr_t LocalPlayers      = 0x38;   // UGameInstance::LocalPlayers
        constexpr uintptr_t PlayerController  = 0x30;   // UPlayer::PlayerController
        constexpr uintptr_t LocalPawn         = 0x318;  // APlayerController::AcknowledgedPawn
        constexpr uintptr_t PawnPrivate       = 0x2E8;  // APlayerState::PawnPrivate
        constexpr uintptr_t PlayerState       = 0x290;  // APawn::PlayerState
        constexpr uintptr_t TeamIndex         = 0xF61;  // AFortPlayerStateAthena::TeamIndex
        constexpr uintptr_t PlayerName        = 0x9E8;  // AFortPlayerState username (FText, ENCRIPTADO)
        constexpr uintptr_t KillScore         = 0xF78;  // AFortPlayerStateAthena::KillScore
        constexpr uintptr_t Platform          = 0x400;  // AFortPlayerState::Platform
        constexpr uintptr_t HabaneroComponent = 0x918;  // AFortPlayerState::HabaneroComponent (ranked)
        constexpr uintptr_t bIsDying          = 0x728;  // AFortPawn::bIsDying (bit 5)
        constexpr uintptr_t bIsDBNO           = 0x881;  // AFortPawn::bIsDBNO (bit 0)
        constexpr uintptr_t bIsABot           = 0x27A;  // APlayerState::bIsABot (bit 3)
        constexpr uintptr_t bIsCrouched       = 0x430;  // ACharacter::bIsCrouched (bit 0)
        constexpr uintptr_t ReviveFromDBNOTime = 0x4AC8; // AFortPlayerPawnAthena
    }

    // --------------------------------------------------------
    // WEAPON
    // --------------------------------------------------------
    namespace weapon {
        constexpr uintptr_t CurrentWeapon        = 0x9D0;  // AFortPawn::CurrentWeapon
        constexpr uintptr_t WeaponData           = 0x638;  // AFortWeapon::WeaponData
        constexpr uintptr_t ItemName             = 0x38;   // UItemDefinitionBase::ItemName
        constexpr uintptr_t AmmoCount            = 0x1100; // AFortWeapon::AmmoCount
        constexpr uintptr_t bIsReloading         = 0x381;  // AFortWeapon::bIsReloadingWeapon (bit 0)
        constexpr uintptr_t LastFireTime         = 0xFFC;  // AFortWeapon::LastFireTime
        constexpr uintptr_t LastFireTimeVerified = 0x1004; // AFortWeapon::LastFireTimeVerified
        constexpr uintptr_t LastDamagedTime      = 0xDE8;  // AFortPawn::LastDamagedTime
        constexpr uintptr_t ProjectileSpeed      = 0x210C; // AFortWeapon projectile speed
        constexpr uintptr_t ProjectileGravity    = 0x2110; // AFortWeapon::ProjectileGravityScale
        constexpr uintptr_t ComponentVelocity    = 0x188;  // USceneComponent::ComponentVelocity
    }

    // --------------------------------------------------------
    // MESH / BONES
    // --------------------------------------------------------
    namespace mesh {
        constexpr uintptr_t BoneArray            = 0x660; // USkeletalMeshComponent bone TArray (primary)
        constexpr uintptr_t BoneArray_cache      = 0x670; // bone TArray secondary (siempre BoneArray+0x10)
        constexpr uintptr_t ReadComponentIndex   = 0x48;  // active bone buffer index
        constexpr uintptr_t CachedSpaceTransforms = 0x9E8;
        constexpr uintptr_t LastRenderTime       = 0x290; // UPrimitiveComponent::LastRenderTimeOnScreen
        constexpr uintptr_t RootComponent        = 0x1B0; // AActor::RootComponent
        constexpr uintptr_t RelativeLocation     = 0x140; // USceneComponent::RelativeLocation
        constexpr uintptr_t ComponentToWorld     = 0x1E0; // USceneComponent::ComponentToWorld
    }

    // --------------------------------------------------------
    // CAMERA / VIEW
    // --------------------------------------------------------
    namespace camera {
        constexpr uintptr_t LocationPointer  = 0x168; // UWorld camera location
        constexpr uintptr_t RotationPointer  = 0x178; // UWorld camera rotation
        constexpr uintptr_t FOV              = 0x374; // APlayerController FOV scalar
    }

    // --------------------------------------------------------
    // AIM
    // --------------------------------------------------------
    namespace aim {
        constexpr uintptr_t TargetedFortPawn       = 0x16C0; // AFortPlayerController::TargetedFortPawn
        constexpr uintptr_t LocationUnderReticle   = 0x2188; // AFortPlayerController::LocationUnderReticle
        constexpr uintptr_t NetConnection          = 0x4A8;  // APlayerController::NetConnection
        constexpr uintptr_t RotationInput          = 0x4B0;  // via NetConnection+0x08
        constexpr uintptr_t WeaponOffsetCorrection = 0x2340;
        constexpr uintptr_t WeaponRecoilOffset     = 0x2328;
        constexpr uintptr_t PlayerAimOffset        = 0x2310;
    }

    // --------------------------------------------------------
    // LOOT (chests / ammo boxes)
    // --------------------------------------------------------
    namespace loot {
        constexpr uintptr_t SpawnSourceOverride = 0xB78;
        constexpr uintptr_t SearchedFlags       = 0xCE2;
        constexpr uintptr_t SearchText          = 0xD38;
        constexpr uintptr_t ChosenRandomUpgrade = 0xBC4;
    }

    // --------------------------------------------------------
    // PICKUP (floor loot)
    // --------------------------------------------------------
    namespace pickup {
        constexpr uintptr_t PrimaryPickupItemEntry   = 0x368;
        constexpr uintptr_t PickupFlags              = 0x28C;
        constexpr uintptr_t ItemEntryItemDefinition  = 0x10;
        constexpr uintptr_t ItemDefinitionName       = 0x38;
        constexpr uintptr_t WeaponDisplayTier        = 0x296;
    }

} // namespace offsets

// ============================================================
//  GWorld DECRYPT — OBLIGATORIO en v37.x
//  GWorld ya no es un puntero raw, está encriptado
// ============================================================
//
//  #include <intrin.h>
//  constexpr uintptr_t uworld_encrypted_rva = offsets::core::GWorld; // 0x1A9E6268
//
//  auto encrypted = Read<uint64_t>(base + uworld_encrypted_rva);
//  auto uworld = static_cast<uintptr_t>(
//      _byteswap_uint64(encrypted ^ 0x0000000001047ADE) - 0x0803A0EA
//  );
//
// ============================================================
//  ALTERNATE — vía gEngine (si el decrypt falla)
// ============================================================
//
//  auto gengine  = Read<uintptr_t>(base + offsets::core::gEngine);
//  auto viewport = Read<uintptr_t>(gengine + offsets::core::GameViewport);  // 0xB70
//  auto uworld   = Read<uintptr_t>(viewport + 0x78);
//
// ============================================================
