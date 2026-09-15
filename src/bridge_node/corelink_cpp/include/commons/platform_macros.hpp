#pragma once

#include "typedefs.hpp"

/**
 * @def CORELINK_EXPORT
 * @brief alias declaration primarily for win32 systems for exporting items to beyond DLL boundaries
 */

/**
 * @def CORELINK_IMPORT
 * @brief alias declaration primarily for win32 systems for importing items from beyond DLL boundaries
 */


#ifdef _WIN32
/**
 * @def CORELINK_EXPORT
 * @brief alias declaration primarily for win32 systems for exporting items to beyond DLL boundaries
 */
#define CORELINK_EXPORT __declspec(dllexport)
/**
 * @def CORELINK_IMPORT
 * @brief alias declaration primarily for win32 systems for importing items from beyond DLL boundaries
 */
#define CORELINK_IMPORT __declspec(dllimport)
#else
#define CORELINK_EXPORT
#define CORELINK_IMPORT
#endif

/**
 * Care for
 * https://docs.microsoft.com/en-us/cpp/overview/visual-cpp-language-conformance?view=msvc-170
 * https://docs.microsoft.com/en-us/cpp/preprocessor/predefined-macros?view=msvc-170
 * The idea is that to select appropriate language features for C++ 11, 14 and 17, we need to able to define cross platform, a way
 * to select or leave out features.
 * The list can grow in the future to support more versions or platforms
 */


/**
 * @def CORELINK_CPP_PLATFORM_IDENTIFIER
 * @brief corelink C++ platform macro
 * @details This macro is defined per platform. Corelink lib internally defines this currently for most popular C++
 * compilers like MSVC, Clang++ and G++. Certain platform libraries and C++ conformance versions are determine using
 * this macro.
 */

/**
 * @def CORELINK_CPP_LEAST_CONFORMANCE_VERSION_CPP17
 * @brief Version number for the C++ standard based on library conformance for C++ 17
 */
/**
 * @def CORELINK_CPP_LEAST_CONFORMANCE_VERSION_CPP14
 * @brief Version number for the C++ standard based on library conformance for C++ 14
 */
/**
 * @def CORELINK_CPP_LEAST_CONFORMANCE_VERSION_CPP11
 * @brief Version number for the C++ standard based on library conformance for C++ 11
 */

/**
 * @def CORELINK_CPP17_SUPPORTED
 * @details Enabled when the current platform and project settings enable the use of C++ 17 features.
 * This is derived based on the value of @ref CORELINK_CPP_LEAST_CONFORMANCE_VERSION_CPP17 for the platform.
 */

/**
 * @def CORELINK_CPP17_SUPPORTED
 * @details Enabled when the current platform and project settings enable the use of C++ 17 features.
 * This is derived based on the value of @ref CORELINK_CPP_LEAST_CONFORMANCE_VERSION_CPP17 for the platform.
 */

/**
 * @def CORELINK_CPP14_SUPPORTED
 * @details Enabled when the current platform and project settings enable the use of C++ 14 features.
 * This is derived based on the value of @ref CORELINK_CPP_LEAST_CONFORMANCE_VERSION_CPP14 for the platform.
 */

/**
 * @def CORELINK_CPP11_SUPPORTED
 * @details Enabled when the current platform and project settings enable the use of C++ 11 features.
 * This is derived based on the value of @ref CORELINK_CPP_LEAST_CONFORMANCE_VERSION_CPP11 for the platform.
 */


#if defined(_WIN32)
#define CORELINK_CPP_PLATFORM_IDENTIFIER _MSC_VER
#define CORELINK_CPP_LEAST_CONFORMANCE_VERSION_CPP17 1927
#define CORELINK_CPP_LEAST_CONFORMANCE_VERSION_CPP14 1911
#define CORELINK_CPP_LEAST_CONFORMANCE_VERSION_CPP11 1800
#elif defined (__cplusplus)
#define CORELINK_CPP_PLATFORM_IDENTIFIER __cplusplus
#define CORELINK_CPP_LEAST_CONFORMANCE_VERSION_CPP17 201703
#define CORELINK_CPP_LEAST_CONFORMANCE_VERSION_CPP14 201402
#define CORELINK_CPP_LEAST_CONFORMANCE_VERSION_CPP11 201103
#else
#error "Cannot determine a platform macro to determine CPP version! Please contact the corelink development team"
#endif

/**
 * @brief Macro to evaluate and allow selection of appropriate C++ platform version
 */
#define CORELINK_CHECK_CPP_VERSION_CONFORMANCE(version) (CORELINK_CPP_PLATFORM_IDENTIFIER >= version)

#if CORELINK_CHECK_CPP_VERSION_CONFORMANCE(CORELINK_CPP_LEAST_CONFORMANCE_VERSION_CPP17)
#define CORELINK_CPP17_SUPPORTED
#elif CORELINK_CHECK_CPP_VERSION_CONFORMANCE(CORELINK_CPP_LEAST_CONFORMANCE_VERSION_CPP14)
#define CORELINK_CPP14_SUPPORTED
#elif CORELINK_CHECK_CPP_VERSION_CONFORMANCE(CORELINK_CPP_LEAST_CONFORMANCE_VERSION_CPP11)
#define CORELINK_CPP11_SUPPORTED
#else
#error "Unsupported compiler version"
#endif

#if defined(CORELINK_CPP17_SUPPORTED)
#define CORELINK_CPP_ATTR_MAYBE_UNUSED [[maybe_unused]]
#else
#define CORELINK_CPP_ATTR_MAYBE_UNUSED
#endif

#define CORELINK_UNUSED(x) do {(void)x;} while(false)
