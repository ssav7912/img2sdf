#ifndef IMG2SDF_LIBRARY_H
#define IMG2SDF_LIBRARY_H

#include <string>
#include "shader_globals.h"



namespace parsing{
    constexpr const char* PROGRAM_NAME = "img2sdf";
    constexpr const char* INPUT_ARGUMENT = "Input Image";
    constexpr const char* OUTPUT_ARGUMENT = "Output";
}

void jumpflood(const std::string& texture_path);

#endif //IMG2SDF_LIBRARY_H
