#ifndef IMG2SDF_LIBRARY_H
#define IMG2SDF_LIBRARY_H

#include <string>

#pragma pack()
struct float4
{
    float x = 0;
    float y = 0;
    float z = 0;
    float w = 0;
};
static_assert(sizeof(float4) == sizeof(float)*4, "Size of float4 struct does not conform to HLSL float4 size!");

namespace parsing{
    constexpr const char* PROGRAM_NAME = "img2sdf";
    constexpr const char* INPUT_ARGUMENT = "Input Image";
    constexpr const char* OUTPUT_ARGUMENT = "Output";
}

void jumpflood(const std::string& texture_path);

#endif //IMG2SDF_LIBRARY_H
