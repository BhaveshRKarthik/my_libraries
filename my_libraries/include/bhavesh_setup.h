#pragma once

// MSVC does not give language version in __cplusplus by default; we need _MSVC_LANG for that
#ifdef _MSC_VER
#define BHAVESH_CXX_VER _MSVC_LANG
#else
#define BHAVESH_CXX_VER __cplusplus
#endif

// constexpr new is amazing
#if BHAVESH_CXX_VER >= 202002L
#define BHAVESH_CXX20 true
#define BHAVESH_CXX20_CONSTEXPR constexpr
#define BHAVESH_USE_IF_CXX20(...) __VA_ARGS__
#else
#define BHAVESH_CXX20 false
#define BHAVESH_CXX20_CONSTEXPR
#define BHAVESH_USE_IF_CXX20(...)
#endif

// many things like if-constexpr are there since c++17
#if BHAVESH_CXX_VER >= 201703L
#define BHAVESH_CXX17 true
#define BHAVESH_CXX17_CONSTEXPR constexpr
#define BHAVESH_USE_IF_CXX17(...) __VA_ARGS__
#else
#define BHAVESH_CXX17 false
#define BHAVESH_CXX17_CONSTEXPR
#define BHAVESH_USE_IF_CXX17(...)
#endif

#if BHAVESH_CXX_VER > 202002L && !defined(_MSC_VER)
#define BHAVESH_IS_CONSTANT_EVALUATED consteval
#elif BHAVESH_CXX_VER >= 202002L
#define BHAVESH_IS_CONSTANT_EVALUATED (std::is_constant_evaluated())
#if BHAVESH_CXX_VER != 202002L
#pragma message("WARNING: MSVC does not support if-consteval in c++23 mode as of writing the code; this may need to get updated later")
#endif
#elif BHAVESH_CXX_VER >= 201703L
#define BHAVESH_IS_CONSTANT_EVALUATED constexpr(false)
#else
#define BHAVESH_IS_CONSTANT_EVALUATED (false)
#endif

// if not provided externally
#ifndef BHAVESH_DEBUG

// NDEBUG is defined by both MSVC and cmake while making release projects
// and NDEBUG is required by the standard to disable the assert macro in C; so most consistent would be that
#if !defined(NDEBUG)
#define BHAVESH_DEBUG true
#else
#define BHAVESH_DEBUG false
#endif

#endif

// relatively useful helper macro
#if BHAVESH_DEBUG
#define BHAVESH_USE_IF_DEBUG(...) __VA_ARGS__
#else
#define BHAVESH_USE_IF_DEBUG(...) 
#endif