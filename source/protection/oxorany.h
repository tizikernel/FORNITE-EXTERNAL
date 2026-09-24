#pragma once
#include <stddef.h>

// Ultra-stable Compile-time XOR Obfuscator
template <size_t N, char K>
struct ox_a {
    char data[N];
    bool done;

    __forceinline constexpr ox_a(const char(&s)[N]) : data{}, done(false) {
        for (size_t i = 0; i < N; ++i)
            data[i] = s[i] ^ K;
    }

    __forceinline const char* d() {
        if (!done) {
            for (size_t i = 0; i < N; ++i)
                data[i] ^= K;
            done = true;
        }
        return data;
    }
};

template <size_t N, wchar_t K>
struct ox_w {
    wchar_t data[N];
    bool done;

    __forceinline constexpr ox_w(const wchar_t(&s)[N]) : data{}, done(false) {
        for (size_t i = 0; i < N; ++i)
            data[i] = s[i] ^ K;
    }

    __forceinline const wchar_t* d() {
        if (!done) {
            for (size_t i = 0; i < N; ++i)
                data[i] ^= K;
            done = true;
        }
        return data;
    }
};

namespace oxorany_detail {
    template <size_t N>
    struct size_tag {};

    template <typename Provider, size_t N>
    __forceinline const char* decrypt_literal(Provider provider, size_tag<N>, const char*) {
        static ::ox_a<N, 0x5A> o(provider());
        return o.d();
    }

    template <typename Provider, size_t N>
    __forceinline const wchar_t* decrypt_literal(Provider provider, size_tag<N>, const wchar_t*) {
        static ::ox_w<N, 0x5A5A> o(provider());
        return o.d();
    }
}

// Macros create a unique static instance at each call site
#ifndef OXORANY_MACROS_DEFINED
#define OXORANY_MACROS_DEFINED

#define oxorany(s) (::oxorany_detail::decrypt_literal( \
    []() -> decltype(auto) { return (s); }, \
    ::oxorany_detail::size_tag<sizeof(s) / sizeof((s)[0])>{}, \
    &(s)[0]))

#define oxorany_w(s) oxorany(s)

#define oxorany_flt(x) (x)
#define oxorany_ptr(x) (x)
#endif
