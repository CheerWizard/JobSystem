#pragma once

#include <cstdint>
#include <memory>
#include <iostream>

typedef uint32_t u32;
typedef uint64_t u64;

template<typename T>
using Scope = std::unique_ptr<T>;

template<typename T, typename ... Args>
constexpr Scope<T> createScope(Args&& ... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T>
using Ref = std::shared_ptr<T>;

template<typename T, typename ... Args>
constexpr Ref<T> createRef(Args&& ... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template<typename T>
using Weak = std::weak_ptr<T>;

template<typename T, typename ... Args>
constexpr Weak<T> createWeak(Args&& ... args) {
    return std::weak_ptr<T>(std::forward<Args>(args)...);
}

#ifdef WIN32
#define DEBUGBREAK() __debugbreak()
#elif defined(__linux__)
#include <signal.h>
		#define DEBUGBREAK() raise(SIGTRAP)
	#else
		#error "Platform doesn't support debugbreak yet!"
#endif

#define ASSERT(x, ...) \
if (!(x)) { \
    std::cout << __VA_ARGS__; \
    DEBUGBREAK(); \
}