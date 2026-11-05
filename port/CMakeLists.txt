# SPDX-License-Identifier: GPL-2.0-or-later
# The native game. A 32-bit build: guest memory is at its real addresses (docs/port.md).
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "The port is a 32-bit build: see docs/port.md")
endif()

enable_language(C ASM)

include(FetchContent)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.8
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(SDL3)
# SDL takes Clang for a Unix compiler and links libm, which Windows doesn't have.
foreach(sdl_target SDL3-static SDL_uclibc)
    if(WIN32 AND TARGET ${sdl_target})
        foreach(property LINK_LIBRARIES INTERFACE_LINK_LIBRARIES)
            get_target_property(sdl_libraries ${sdl_target} ${property})
            if(sdl_libraries)
                list(REMOVE_ITEM sdl_libraries m "$<LINK_ONLY:m>")
                set_target_properties(${sdl_target} PROPERTIES ${property} "${sdl_libraries}")
            endif()
        endforeach()
    endif()
endforeach()

file(GLOB GAME_SOURCES CONFIGURE_DEPENDS
    game/exe/*.cpp game/exe/*.c
    game/ovl0/*.cpp game/ovl1/*.cpp game/ovl2/*.cpp game/ovl3/*.cpp game/ovl4/*.cpp game/ovl5/*.cpp)

# GCC 2.8.1 behaviour the game relies on: unsigned char, wrapping signed arithmetic,
# aliasing between types, and no library calls made up by the compiler.
add_library(game OBJECT ${GAME_SOURCES})
# The data names whose address depends on the overlay (tools/gen_syms.py).
foreach(unit ovl0 ovl1 ovl4)
    file(GLOB unit_sources CONFIGURE_DEPENDS game/${unit}/*.cpp)
    set_property(SOURCE ${unit_sources} APPEND PROPERTY COMPILE_DEFINITIONS
        g_Page_SaveGame=g_Page_SaveGame_${unit} g_LicensePrefixes=g_LicensePrefixes_${unit})
endforeach()
target_include_directories(game PRIVATE game/include rt)
target_compile_definitions(game PRIVATE GTE_C)
target_compile_options(game PRIVATE
    -w -fwrapv -fno-strict-aliasing -fno-delete-null-pointer-checks -fno-exceptions -fno-rtti -fno-builtin
    -fno-stack-protector -fno-vectorize -fno-slp-vectorize -fno-omit-frame-pointer -fno-strict-return
    $<$<COMPILE_LANGUAGE:CXX>:-funsigned-char -std=gnu++20>
    $<$<COMPILE_LANGUAGE:C>:-fsigned-char -std=gnu89>
    -nostdlibinc)

add_executable(gt2ap
    $<TARGET_OBJECTS:game>
    hw/card.cpp
    hw/cd.cpp
    hw/gpu.cpp
    hw/host.cpp
    hw/pad.cpp
    rt/irq.cpp
    rt/main.cpp
    rt/memmap.cpp
    rt/stubs.cpp
    gen/guest_syms.S
    gen/fixups_exe.cpp
    gen/fixups_ovl0.cpp
    gen/fixups_ovl1.cpp
    gen/fixups_ovl2.cpp
    gen/fixups_ovl3.cpp
    gen/fixups_ovl4.cpp
    gen/fixups_ovl5.cpp)
target_include_directories(gt2ap PRIVATE . hw rt)
target_compile_options(gt2ap PRIVATE -fno-omit-frame-pointer)
target_link_libraries(gt2ap PRIVATE gt2ap::discio SDL3::SDL3-static)
if(WIN32)
    target_link_libraries(gt2ap PRIVATE user32)
endif()
if(WIN32)
    target_link_options(gt2ap PRIVATE
        -Wl,/BASE:0x400000,/FIXED,/LARGEADDRESSAWARE,/DYNAMICBASE:NO,/NXCOMPAT,/SAFESEH:NO)
else()
    target_compile_options(gt2ap PRIVATE -fno-pic)
    target_link_options(gt2ap PRIVATE -no-pie)
endif()
