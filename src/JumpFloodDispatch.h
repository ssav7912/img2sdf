//
// Created by Soren on 1/04/2024.
//

#ifndef IMG2SDF_JUMPFLOODDISPATCH_H
#define IMG2SDF_JUMPFLOODDISPATCH_H

#include <cstdint>
#include <wrl.h>
#include <d3d11.h>
#include "jumpflooderror.h"

using namespace Microsoft::WRL;

struct jump_flood_shaders
{
    ///pointer to the preprocess shader bytecode
    const uint8_t* preprocess;
    size_t preprocess_size;

    ///pointer to the voronoi diagram shader bytecode (jumpflood.hlsl)
    const uint8_t* voronoi;
    size_t voronoi_size;

    ///pointer to the voronoi_normalisation shader bytecode, for debugging generated voronoi diagrams.
    const uint8_t* voronoi_normalise;
    size_t voronoi_normalise_size;

    ///pointer to the distance transform shader bytecode
    const uint8_t* distance_transform;
    size_t distance_transform_size;

    ///pointer to the min_max reduction shader bytecode
    const uint8_t* min_max_reduce;
    size_t min_max_reduce_size;

    ///pointer to the distance normalisation shader bytecode.
    const uint8_t* distance_normalise;
    size_t distance_normalise_size;

};

enum class SHADERS
{
    PREPROCESS,
    VORONOI,
    VORONOI_NORMALISE,
    DISTANCE,
    MINMAXREDUCE,
    DISTANCE_NORMALISE
};

class JumpFloodDispatch {
public:
    ///Creates shaders from compiled bytecode. Note that any of byte_code's fields may be nullptr,
    ///and the constructor will simply skip this shader.
    JumpFloodDispatch(ID3D11Device* device, ID3D11DeviceContext* context,
                      jump_flood_shaders byte_code, class JumpFloodResources* resources);

    ///Gets the D3D shader interface for the specified shader.
    [[nodiscard]] ID3D11ComputeShader* get_shader(SHADERS shader) const;

    ///Dispatches the preprocess shader.
    void dispatch_preprocess_shader();

    ///dispatches the voronoi jumpflood shader.
    /// Note that this will generate a new UAV if the one in `resources` is not yet created.
    void dispatch_voronoi_shader();

    void dispatch_distance_transform_shader();

    ///dispatches the voronoi normalisation shader.
    void dispatch_voronoi_normalise_shader();


private:
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    class JumpFloodResources* resources = nullptr;

    ComPtr<ID3D11ComputeShader> preprocess_shader = nullptr;
    ComPtr<ID3D11ComputeShader> voronoi_shader = nullptr;
    ComPtr<ID3D11ComputeShader> voronoi_normalise_shader = nullptr;
    ComPtr<ID3D11ComputeShader> distance_transform_shader = nullptr;
    ComPtr<ID3D11ComputeShader> min_max_reduce_shader = nullptr;
    ComPtr<ID3D11ComputeShader> distance_normalise_shader = nullptr;


};


#endif //IMG2SDF_JUMPFLOODDISPATCH_H
