cmake_minimum_required(VERSION 3.16)
project(amxx_memstore LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# AMX Mod X (GoldSrc engine) is strictly 32-bit (x86 / i386)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    if(MSVC)
        message(FATAL_ERROR "GoldSrc/AMX Mod X requires 32-bit build. Please run CMake with: -A Win32")
    else()
        message(STATUS "Configuring for 32-bit target (-m32)...")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -m32")
    endif()
endif()

# Source files
set(SOURCES
    src/MemStore.cpp
    src/MemStore.h
    src/Natives.cpp
    src/amxxapi.cpp
    src/moduleconfig.h
    sdk/amxxmodule.cpp
    sdk/amxxmodule.h
)

if(WIN32)
    list(APPEND SOURCES src/version.rc)
endif()

add_library(memstore SHARED ${SOURCES})

target_include_directories(memstore PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk
)

target_compile_definitions(memstore PRIVATE
    HAVE_STDINT_H
    $<$<BOOL:${WIN32}>:_CRT_SECURE_NO_WARNINGS>
)

if(MSVC)
    target_compile_options(memstore PRIVATE
        /W3
        /O2
        /fp:precise
    )
    set_target_properties(memstore PROPERTIES
        PREFIX ""
        OUTPUT_NAME "memstore_amxx"
    )
else()
    target_compile_options(memstore PRIVATE
        -m32
        -O3
        -Wall
        -Wextra
        -Wno-unused-parameter
        -fvisibility=hidden
        -fvisibility-inlines-hidden
        -fPIC
    )
    target_link_options(memstore PRIVATE
        -m32
        -static-libgcc
        -static-libstdc++
    )
    set_target_properties(memstore PROPERTIES
        PREFIX ""
        OUTPUT_NAME "memstore_amxx_i386"
        SUFFIX ".so"
    )
endif()

# Install / Package target
install(TARGETS memstore
    RUNTIME DESTINATION addons/amxmodx/modules
    LIBRARY DESTINATION addons/amxmodx/modules
)

install(FILES include/memstore.inc
    DESTINATION addons/amxmodx/scripting/include
)

install(FILES 
    plugins/test_memstore.sma
    plugins/example_top15.sma
    DESTINATION addons/amxmodx/scripting
)
