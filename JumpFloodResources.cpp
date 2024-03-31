//
// Created by Soren on 29/03/2024.
//

#include "JumpFloodResources.h"
#include <stdexcept>

JumpFloodResources::JumpFloodResources(ID3D11Device *device, const std::wstring& file_path) :
        device(device)
        {
            if (device == nullptr)
            {
                throw std::runtime_error("ID3D11Device is NULL");
            }
            try {
                create_input_srv(file_path);
            }
            catch (...)
            {
                auto eptr = std::current_exception();
                std::rethrow_exception(eptr);
            }

        };

ID3D11ShaderResourceView *JumpFloodResources::create_input_srv(std::wstring filepath) {
    auto input = dxutils::load_texture_to_srv(filepath, device);

    if (FAILED(input.first.As<ID3D11Texture2D>(&preprocess_texture)))
    {
        this->preprocess_texture = nullptr;
        throw std::runtime_error("Returned resource was not a texture.");
        return nullptr;
    }

    this->preprocess_srv = input.second;
    D3D11_TEXTURE2D_DESC texture_description = {0};
    this->preprocess_texture->GetDesc(&texture_description);
    //TODO: link up to a logger class.
    if (texture_description.Format != DXGI_FORMAT_R32_FLOAT)
    {
        printf("WARNING: Input format is not a grayscale mask.\n");
    }

    this->input_description = texture_description;
    this->res.height = texture_description.Height;
    this->res.width = texture_description.Width;

    return this->preprocess_srv.Get();
}

ID3D11UnorderedAccessView *JumpFloodResources::create_voronoi_uav() {

    try {
        auto voronoi = create_uav<float4>(DXGI_FORMAT_R32G32B32A32_FLOAT);
        this->voronoi_texture = voronoi.first;
        this->voronoi_uav = voronoi.second;

        return this->voronoi_uav.Get();
    }
    catch (...)
    {
        auto eptr = std::current_exception();
        std::rethrow_exception(eptr);
    }

}

ID3D11UnorderedAccessView *JumpFloodResources::create_distance_uav() {
    try {
        auto distance = create_uav<float>(DXGI_FORMAT_R32_FLOAT);
        this->distance_texture = distance.first;
        this->distance_uav = distance.second;

        return this->distance_uav.Get();
    }
    catch (...)
    {
        auto eptr = std::current_exception();
        std::rethrow_exception(eptr);
    }
}

ID3D11Texture2D* JumpFloodResources::create_staging_texture(ID3D11Texture2D* mimic_texture)
{
    D3D11_TEXTURE2D_DESC staging_desc;
    mimic_texture->GetDesc(&staging_desc);

    staging_desc.Usage = D3D11_USAGE_STAGING;
    staging_desc.BindFlags = 0;

    ComPtr<ID3D11Texture2D> CPU_read_texture = nullptr;
    HRESULT out_staging = device->CreateTexture2D(&staging_desc, nullptr, CPU_read_texture.GetAddressOf());
    if (FAILED(out_staging))
    {
        throw std::runtime_error(std::format("Could not create staging texture. HRESULT {:x}\n", out_staging));
    }
    this->staging_texture = CPU_read_texture;
    return this->staging_texture.Get();
}

resolution JumpFloodResources::get_resolution() const {
    return this->res;
}

ID3D11ShaderResourceView *JumpFloodResources::get_input_srv() {
    return this->preprocess_srv.Get();
}

ID3D11Buffer *JumpFloodResources::create_const_buffer() {
    const auto width_float = static_cast<float>(res.width);
    const auto height_float = static_cast<float>(res.height);
    const JFA_cbuffer cbuffer = {width_float, height_float, 0};

    D3D11_BUFFER_DESC desc {0};
    desc.ByteWidth = sizeof(cbuffer);
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    D3D11_SUBRESOURCE_DATA resource {};
    resource.pSysMem = &cbuffer;
    resource.SysMemPitch = 0;
    resource.SysMemSlicePitch = 0;

    HRESULT r = device->CreateBuffer(&desc, &resource, this->const_buffer.GetAddressOf());
    if (FAILED(r))
    {
        throw std::runtime_error(std::format("Could not create const buffer for shader constants. HRESULT: {:x} \n",r));
    }

    return this->const_buffer.Get();

}

ID3D11Texture2D *JumpFloodResources::get_texture(RESOURCE_TYPE desired_texture) const {
    switch(desired_texture)
    {
        case RESOURCE_TYPE::DISTANCE_UAV: {return this->distance_texture.Get();};
        case RESOURCE_TYPE::INPUT_SRV: {return this->preprocess_texture.Get();};
        case RESOURCE_TYPE::STAGING_TEXTURE: {return this->staging_texture.Get();};
        case RESOURCE_TYPE::VORONOI_UAV: {return this->voronoi_texture.Get();};
        default: return nullptr;
    }
}
