#pragma once
#include <windows.h>
#include <accctrl.h>
#include <aclapi.h>
#include <type_traits>
#include <string>
#include <vector>
#include <stdexcept>

// 1. Core Obfuscation Engine (Moved from obf_macros.h for stability)
#include "oxorany.h"

// Advanced polymorphic wrapper for standard string compatibility
template<typename T>
class skCryptWrapper {
    T val;
public:
    __forceinline skCryptWrapper(T v) : val(v) {}
    __forceinline T decrypt() { return val; }
    __forceinline operator T() { return val; }
    
    __forceinline friend std::string operator+(const skCryptWrapper& lhs, const std::string& rhs) {
        return std::string(lhs.val) + rhs;
    }
    __forceinline friend std::string operator+(const std::string& lhs, const skCryptWrapper& rhs) {
        return lhs + std::string(rhs.val);
    }
    __forceinline friend std::wstring operator+(const skCryptWrapper& lhs, const std::wstring& rhs) {
        return std::wstring(lhs.val) + rhs;
    }
    __forceinline friend std::wstring operator+(const std::wstring& lhs, const skCryptWrapper& rhs) {
        return lhs + std::wstring(rhs.val);
    }
};

#define OBF(str) oxorany(str)
#define skCrypt(str) skCryptWrapper<decltype(OBF(str))>(OBF(str))
#define _xor_(str) OBF(str)

// 2. Core Protection Engine
#include "junk_engine.h"

// 3. Protection SDK Wrappers
#include "obsidium/obsidium64.h"
#include "cv/VirtualizerSDK.h"
#include "themida/ThemidaSDK.h"
#include "vmp/VMProtectSDK.h"

// 4. Triple-Layer Protection Macros
#undef PROTECT_VIRTUAL_START
#define PROTECT_VIRTUAL_START \
    VIRTUALIZER_START; \
    STR_ENCRYPT_START; \
    VM_START; \
    VMProtectBeginUltra("FatalVirtual");

#undef PROTECT_VIRTUAL_END
#define PROTECT_VIRTUAL_END \
    VMProtectEnd(); \
    VM_END; \
    STR_ENCRYPT_END; \
    VIRTUALIZER_END;

#undef PROTECT_MUTATE_START
#define PROTECT_MUTATE_START \
    MUTATE_START; \
    VMProtectBeginMutation("FatalMutation");

#undef PROTECT_MUTATE_END
#define PROTECT_MUTATE_END \
    VMProtectEnd(); \
    MUTATE_END;

// 5. Utility Macros
#ifndef SAFE_CALL
#define SAFE_CALL(FUNC) LI_FN(FUNC)
#endif
