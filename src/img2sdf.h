//
// Created by Soren on 6/05/2024.
//

#ifndef IMG2SDF_IMG2SDF_H
#define IMG2SDF_IMG2SDF_H

#include <d3d11.h>
#include <wrl.h>
#include "JumpFloodResources.h"
#include "JumpFloodDispatch.h"

using namespace Microsoft::WRL;


class Img2SDF
{
public:
    Img2SDF(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> context, ComPtr<ID3D11Debug> debug_layer);
    Img2SDF(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> context);

    ///Computes a signed distance field from the provided input texture.
    ///@param input_texture a seed mask of size 2^n * 2^n, with D3D11_BIND_SHADER_RESOURCE, DXGI_FORMAT_R32_FLOAT, and D3D11_USAGE_DEFAULT.
    ///@param normalise whether to normalise the result to -1, 1.
    ComPtr<ID3D11Texture2D> compute_signed_distance_field(ComPtr<ID3D11Texture2D> input_texture, bool normalise = true);

    ///Computes a signed distance field from the provided input texture.
    ///@param input_texture a seed mask of size 2^n * 2^n, with D3D11_BIND_SHADER_RESOURCE, DXGI_FORMAT_R32_FLOAT, and D3D11_USAGE_DEFAULT.
    ///@param normalise whether to normalise the result to 0, 1.
    ComPtr<ID3D11Texture2D> compute_unsigned_distance_field(ComPtr<ID3D11Texture2D> input_texture, bool normalise = true);

    ///Computes a voronoi transform from the provided seed texture.
    ///@param input_texture a seed mask of size 2^n * 2^n, with D3D11_BIND_SHADER_RESOURCE, DXGI_FORMAT_R32_FLOAT, and D3D11_USAGE_DEFAUT.
    ///@param normalise whether to normalise the result to normalised texel coordinates (0-1 along width and height).
    ComPtr<ID3D11Texture2D> compute_voronoi_transform(ComPtr<ID3D11Texture2D> input_texture, bool normalise = false);

private:
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;

    ComPtr<ID3D11Debug> debug_layer;


};
#endif //IMG2SDF_IMG2SDF_H
