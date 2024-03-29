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

#pragma pack(16)
struct JFA_cbuffer
{
    float Width;
    float Height;
    const uint8_t Padding[16-2*sizeof(float)];
};


#endif //IMG2SDF_SHADER_GLOBALS_H
