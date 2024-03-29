//
// Created by Soren on 29/03/2024.
//

#include "JumpFloodResources.h"
#include <stdexcept>

JumpFloodResources::JumpFloodResources(ID3D11Device *device, ID3D11DeviceContext *context) :
        device(device), context(context)
        {
            if (device == nullptr || context == nullptr)
            {
                throw std::runtime_error("ID3D11Device or ID3D11DeviceContext are NULL");
            }
        };

ID3D11ShaderResourceView *JumpFloodResources::create_input_srv(std::wstring filepath) {
    auto input = dxutils::load_texture_to_srv(filepath, device, context);

    if (FAILED(input.first.As<ID3D11Texture2D>(&preprocess_texture)))
    {
        throw std::runtime_error("Returned resource was not a texture.");
        this->preprocess_texture = nullptr;
        return nullptr;
    }

    this->preprocess_srv = input.second;
    return this->preprocess_srv.Get();
}

ID3D11UnorderedAccessView *JumpFloodResources::create_voronoi_uav() {}