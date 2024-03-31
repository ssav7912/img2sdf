//
// Created by Soren on 15/03/2024.
//

#ifndef IMG2SDF_SHADER_GLOBALS_H
#define IMG2SDF_SHADER_GLOBALS_H


#include <d3d11.h>
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <memory>
#include <vector>

struct resolution
{
    size_t width;
    size_t height;
};

#pragma pack()
struct float4
{
    float x = 0;
    float y = 0;
    float z = 0;
    float w = 0;
};
static_assert(sizeof(float4) == sizeof(float)*4, "Size of float4 struct does not conform to HLSL float4 size!");


#pragma pack(16)
struct JFA_cbuffer
{
    float Width;
    float Height;
    const uint8_t Padding[16-2*sizeof(float)];
};


#endif //IMG2SDF_SHADER_GLOBALS_H
