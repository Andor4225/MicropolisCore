/**
 * @file platform.h
 * @brief Platform abstraction layer for cross-platform compatibility
 *
 * This header provides Windows compatibility for POSIX functions and
 * conditionally includes Emscripten headers when building for WebAssembly.
 */

#ifndef MICROPOLIS_PLATFORM_H
#define MICROPOLIS_PLATFORM_H

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
    #define MICROPOLIS_WINDOWS 1
#elif defined(__EMSCRIPTEN__)
    #define MICROPOLIS_EMSCRIPTEN 1
#elif defined(__linux__)
    #define MICROPOLIS_LINUX 1
#elif defined(__APPLE__)
    #define MICROPOLIS_MACOS 1
#else
    #define MICROPOLIS_POSIX 1
#endif

// Standard includes (all platforms)
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <cstdarg>
#include <cstdint>

// C++ STL
#include <string>
#include <vector>
#include <map>

#ifdef MICROPOLIS_WINDOWS
    // Windows-specific includes
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
    #include <io.h>
    #include <direct.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <chrono>

    // POSIX compatibility macros for Windows
    #define access _access
    #define F_OK 0
    #define R_OK 4
    #define W_OK 2

    // S_ISDIR macro (not available on Windows)
    #ifndef S_ISDIR
        #define S_ISDIR(mode) (((mode) & _S_IFMT) == _S_IFDIR)
    #endif

    // POSIX timeval struct for Windows
    struct timeval {
        long tv_sec;
        long tv_usec;
    };

    // gettimeofday for Windows
    inline int gettimeofday(struct timeval* tp, void* tzp) {
        (void)tzp; // unused
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
        auto micros = std::chrono::duration_cast<std::chrono::microseconds>(duration - seconds);
        tp->tv_sec = static_cast<long>(seconds.count());
        tp->tv_usec = static_cast<long>(micros.count());
        return 0;
    }

    // Emscripten stubs for native Windows builds
    #ifndef MICROPOLIS_EMSCRIPTEN
        #define EMSCRIPTEN_KEEPALIVE

        // EM_ASM_ macro stub (does nothing on native builds)
        #define EM_ASM_(code, ...) ((void)0)
        #define EM_ASM(code) ((void)0)

        // Stub for emscripten::val
        namespace emscripten {
            class val {
            public:
                val() = default;
                val(const val&) = default;
                val& operator=(const val&) = default;

                static val null() { return val(); }
                static val undefined() { return val(); }
                static val global(const char*) { return val(); }

                template<typename T>
                val call(const char*, T) const { return val(); }

                val call(const char*) const { return val(); }

                template<typename... Args>
                val operator()(Args...) const { return val(); }

                bool isUndefined() const { return true; }
                bool isNull() const { return true; }

                template<typename T>
                T as() const { return T{}; }
            };
        }
    #endif

#elif defined(MICROPOLIS_EMSCRIPTEN)
    // Emscripten builds
    #include <sys/stat.h>
    #include <unistd.h>
    #include <sys/time.h>
    #include <sys/file.h>
    #include <sys/types.h>
    #include <emscripten.h>
    #include <emscripten/bind.h>

#else
    // Unix/Linux/macOS (POSIX)
    #include <sys/stat.h>
    #include <unistd.h>
    #include <sys/time.h>
    #include <sys/file.h>
    #include <sys/types.h>

    // Emscripten stubs for native POSIX builds
    #define EMSCRIPTEN_KEEPALIVE

    // EM_ASM_ macro stub (does nothing on native builds)
    #define EM_ASM_(code, ...) ((void)0)
    #define EM_ASM(code) ((void)0)

    namespace emscripten {
        class val {
        public:
            val() = default;
            val(const val&) = default;
            val& operator=(const val&) = default;

            static val null() { return val(); }
            static val undefined() { return val(); }
            static val global(const char*) { return val(); }

            template<typename T>
            val call(const char*, T) const { return val(); }

            val call(const char*) const { return val(); }

            template<typename... Args>
            val operator()(Args...) const { return val(); }

            bool isUndefined() const { return true; }
            bool isNull() const { return true; }

            template<typename T>
            T as() const { return T{}; }
        };
    }
#endif

// Cross-platform time utilities
#ifdef MICROPOLIS_WINDOWS
    #include <chrono>

    inline long long micropolis_get_time_ms() {
        auto now = std::chrono::steady_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        );
        return ms.count();
    }
#else
    inline long long micropolis_get_time_ms() {
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
    }
#endif

#endif // MICROPOLIS_PLATFORM_H
