# Author : Sarthak Tickoo (NYU IT HSRN)

cmake_minimum_required(VERSION 3.20)

project(corelink-client VERSION 1.0.0
        LANGUAGES C CXX)

if (EXISTS "${PROJECT_SOURCE_DIR}/CMakeListsUser.txt")
    include("${PROJECT_SOURCE_DIR}/CMakeListsUser.txt")
endif ()

if (WIN32)
    add_definitions("/wd4251")
endif ()

set(CMAKE_CXX_STANDARD_REQUIRED True)


set(CORELINK_TARGET corelinkclient)

file(GLOB_RECURSE CORELINK_SOURCES "src/**/*.cc")

add_library(${CORELINK_TARGET} SHARED ${CORELINK_SOURCES})


target_compile_options(${CORELINK_TARGET} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror -Wno-deprecated-declarations>
)

target_compile_definitions(${CORELINK_TARGET} PUBLIC
        CORELINK_ENABLE_ALL_FEATURES
        CORELINK_USE_CONCURRENT_QUEUE
        CORELINK_USE_UDP
        CORELINK_USE_TCP
        CORELINK_USE_WEBSOCKET
)

target_include_directories(${CORELINK_TARGET}
        PUBLIC "include"
        PUBLIC "${CORELINK_ASIO_CPP_PATH}"
        PUBLIC "${CORELINK_RAPID_JSON_CPP_PATH}"
        PUBLIC "${CORELINK_LWS_INCLUDE_PATH}"
        PUBLIC "${CORELINK_OPENSSL_INCLUDE_PATH}"
)
target_link_directories(${CORELINK_TARGET}
        PUBLIC "${CORELINK_LWS_LIB_PATH}"
        PUBLIC "${CORELINK_OPENSSL_LIB_PATH}"
        PUBLIC "${CORELINK_OPENSSL_BIN_PATH}"
)
set(DYNA_LIB_EXT so)
set(LWS_PREFIX lib)
if (APPLE)
    set(DYNA_LIB_EXT dylib)
endif (APPLE)
if (WIN32)
    set(DYNA_LIB_EXT dll)
    set(LWS_PREFIX "")
endif (WIN32)
target_link_libraries(${CORELINK_TARGET} ${LWS_PREFIX}websockets.${DYNA_LIB_EXT} libssl.${DYNA_LIB_EXT} libcrypto.${DYNA_LIB_EXT})