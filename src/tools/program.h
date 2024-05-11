#ifndef IMG2SDF_PROGRAM_H
#define IMG2SDF_PROGRAM_H

#include <string>
#include "../shader_globals.h"



namespace parsing{
    constexpr const char* PROGRAM_NAME = "img2sdf";
    constexpr const char* INPUT_ARGUMENT = "Input Image";
    constexpr const char* OUTPUT_ARGUMENT = "Output";
    constexpr const char* UNSIGNED = "-u";
    constexpr const char* UNSIGNED_LONG = "--unsigned";
    constexpr const char* SIGNED = "-s";
    constexpr const char* SIGNED_LONG = "--signed";
    constexpr const char* VORONOI = "-v";
    constexpr const char* VORONOI_LONG = "--voronoi";
};


#endif //IMG2SDF_PROGRAM_H
