//
// Created by Soren on 29/03/2024.
//

#include "JumpFloodResources.h"
#include <stdexcept>
#include <format>

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

ID3D11UnorderedAccessView *JumpFloodResources::create_voronoi_uav(bool regenerate) {
    if (this->voronoi_uav != nullptr && !regenerate)
    {
        return this->voronoi_uav.Get();
    }

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
        return nullptr;
    }

}

ID3D11UnorderedAccessView *JumpFloodResources::create_distance_uav(bool regenerate) {
    if (this->distance_uav != nullptr && !regenerate)
    {
        return this->distance_uav.Get();
    }

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
        return nullptr;
    }
}

ID3D11Texture2D* JumpFloodResources::create_staging_texture(ID3D11Texture2D* mimic_texture)
{
    D3D11_TEXTURE2D_DESC staging_desc;
    mimic_texture->GetDesc(&staging_desc);

    staging_desc.Usage = D3D11_USAGE_STAGING;
    staging_desc.BindFlags = 0;
    staging_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ComPtr<ID3D11Texture2D> CPU_read_texture = nullptr;
    HRESULT out_staging = device->CreateTexture2D(&staging_desc, nullptr, CPU_read_texture.GetAddressOf());
    if (FAILED(out_staging))
    {
        throw std::runtime_error(std::format("Could not create staging texture. HRESULT {:x}\n", out_staging));
        return nullptr;
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

ID3D11Buffer *JumpFloodResources::create_const_buffer(bool regenerate) {
    if (this->const_buffer != nullptr && !regenerate)
    {
        return this->const_buffer.Get();
    }

    const auto width_float = static_cast<float>(res.width);
    const auto height_float = static_cast<float>(res.height);
    const JFA_cbuffer cbuffer = {width_float, height_float, 0, 0, 0};

    D3D11_BUFFER_DESC desc {0};
    desc.ByteWidth = sizeof(cbuffer);
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA resource {};
    resource.pSysMem = &cbuffer;
    resource.SysMemPitch = 0;
    resource.SysMemSlicePitch = 0;

    HRESULT r = device->CreateBuffer(&desc, &resource, this->const_buffer.GetAddressOf());
    if (FAILED(r))
    {
        throw std::runtime_error(std::format("Could not create const buffer for shader constants. HRESULT: {:x} \n",r));
        return nullptr;
    }

    this->local_buffer = cbuffer;
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

size_t JumpFloodResources::num_steps() const {
    return floor(log2(static_cast<double>(res.width)));
}

ID3D11Buffer *JumpFloodResources::update_const_buffer(ID3D11DeviceContext* context, JFA_cbuffer buffer) {
    D3D11_MAPPED_SUBRESOURCE mapped_resource;

    HRESULT hr = context->Map(this->const_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
    if (FAILED(hr))
    {
        throw std::runtime_error(std::format("Could not map constant buffer for updating. HRESULT: {:x}", hr));
        return nullptr;
    }

    std::memcpy(mapped_resource.pData, &buffer, sizeof(buffer));
    this->local_buffer = buffer;

    context->Unmap(this->const_buffer.Get(), 0);

    return this->const_buffer.Get();
}

JFA_cbuffer JumpFloodResources::get_local_cbuffer() const {
    return this->local_buffer;
}
