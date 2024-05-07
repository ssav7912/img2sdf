//
// Created by Soren on 29/03/2024.
//

#include "JumpFloodResources.h"
#include <stdexcept>
#include <format>
#include <limits>
#include <vector>
#include <array>
#include "jumpflooderror.h"

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


JumpFloodResources::JumpFloodResources(ID3D11Device *device, const std::vector<float> data, int32_t width,
                                       int32_t height) : device(device) {
    if (device == nullptr)
    {
        throw std::runtime_error("ID3D11Device is NULL");
    }

    D3D11_TEXTURE2D_DESC srv_description = {0};
    srv_description.Width = width;
    srv_description.Height = height;
    srv_description.Usage = D3D11_USAGE_DEFAULT;
    srv_description.Format = DXGI_FORMAT_R32_FLOAT;
    srv_description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    srv_description.MiscFlags = 0;
    srv_description.ArraySize = 1;
    srv_description.MipLevels = 1;
    srv_description.SampleDesc = {1,0};

    ComPtr<ID3D11Texture2D> srv_texture {};

    D3D11_SUBRESOURCE_DATA subresource = {0};
    subresource.pSysMem = data.data();
    subresource.SysMemPitch = sizeof(float) * width;
    subresource.SysMemSlicePitch = 0;

    HRESULT out_tex = device->CreateTexture2D(&srv_description, &subresource, srv_texture.GetAddressOf());
    if (FAILED(out_tex))
    {
        throw jumpflood_error(out_tex, "Could not create SRV texture from input buffer.");
    }

    ComPtr<ID3D11ShaderResourceView> srv = nullptr;

    HRESULT out_srv = device->CreateShaderResourceView(srv_texture.Get(), nullptr, srv.GetAddressOf());
    if (FAILED(out_srv))
    {
        throw jumpflood_error(out_tex, "Could not create SRV from input buffer.");
    }

    this->preprocess_srv = srv;
    this->preprocess_texture = srv_texture;
    this->input_description = srv_description;
    this->res.height = srv_description.Height;
    this->res.width = srv_description.Width;

}


JumpFloodResources::JumpFloodResources(ID3D11Device *device, ComPtr<ID3D11Texture2D> input_texture)
    : device(device) {

    if (device == nullptr)
    {
        throw std::runtime_error("ID3D11Device is NULL");
    }

    D3D11_TEXTURE2D_DESC in_desc = {0};
    input_texture->GetDesc(&in_desc);

    if (in_desc.Format != DXGI_FORMAT_R32_FLOAT)
    {
        throw std::runtime_error("Format must be DXGI_FORMAT_R32_FLOAT");
    }
    else if (in_desc.Usage != D3D11_USAGE_DEFAULT)
    {
        throw std::runtime_error("Usage must be D3D11_USAGE_DEFAULT");
    }
    else if ((in_desc.BindFlags & D3D11_BIND_SHADER_RESOURCE) == 0)
    {
        throw std::runtime_error("Must be bindable as an SRV!");
    }

    else if (!dxutils::is_power_of_two(in_desc.Width) || !dxutils::is_power_of_two(in_desc.Height))
    {
        throw std::runtime_error("Texture must be a power of two size!");
    }

    HRESULT out_srv = device->CreateShaderResourceView(input_texture.Get(), nullptr, preprocess_srv.GetAddressOf());
    if (FAILED(out_srv))
    {
        throw jumpflood_error(out_srv, "Could not create SRV from input_texture");
    }

    this->preprocess_texture = input_texture;
    this->input_description = in_desc;
    this->res.height = in_desc.Height;
    this->res.width = in_desc.Width;
}

ID3D11ShaderResourceView *JumpFloodResources::create_input_srv(std::wstring filepath) {
    auto input = dxutils::load_texture_to_srv(filepath, device);

    if (FAILED(input.first.As<ID3D11Texture2D>(&preprocess_texture)))
    {
        this->preprocess_texture = nullptr;
        throw std::runtime_error("Returned resource was not a texture.");
        return nullptr;
    }

    this->preprocess_srv = input.second;
#ifdef DEBUG
    D3D_SET_OBJECT_NAME_A(this->preprocess_texture, "JumpFloodResources::preprocess_texture");
    D3D_SET_OBJECT_NAME_A(this->preprocess_srv, "JumpFloodResources::preprocess_srv");
#endif

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

#ifdef DEBUG
        D3D_SET_OBJECT_NAME_A(this->voronoi_uav, "JumpFloodResources:voronoi_uav");
        D3D_SET_OBJECT_NAME_A(this->voronoi_texture, "JumpFloodResources::voronoi_texture");
#endif

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
        auto distance = create_uav<float>(DXGI_FORMAT_R32_FLOAT, D3D11_BIND_SHADER_RESOURCE);
        this->distance_texture = distance.first;
        this->distance_uav = distance.second;

#ifdef DEBUG
        D3D_SET_OBJECT_NAME_A(this->distance_uav, "JumpFloodResources::distance_uav");
        D3D_SET_OBJECT_NAME_A(this->distance_texture, "JumpFloodResources::distance_texture");
#endif

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
#ifdef DEBUG
    std::array<char, 256> buffer {0};
    UINT private_data_size = buffer.size();

    mimic_texture->GetPrivateData(WKPDID_D3DDebugObjectName, &private_data_size, buffer.data());

    std::string name (buffer.data());
    name += "_staging";

    D3D_SET_OBJECT_NAME_A(this->staging_texture, name.c_str());
#endif
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
        case RESOURCE_TYPE::REDUCE_UAV: {return this->reduce_texture.Get();};
        default: return nullptr;
    }
}

int32_t JumpFloodResources::num_steps() const {
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

ID3D11UnorderedAccessView *JumpFloodResources::create_reduction_uav(size_t num_groups_x, size_t num_groups_y, bool regenerate) {
    if (this->reduce_uav != nullptr && !regenerate)
    {
        return this->reduce_uav.Get();
    }
    D3D11_TEXTURE2D_DESC desc = input_description;
    desc.Width = num_groups_x;
    desc.Height = num_groups_y;
    desc.Format = DXGI_FORMAT_R32G32_FLOAT;
    desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.ArraySize = 1;
    desc.MipLevels = 1;
    desc.CPUAccessFlags = 0;


    constexpr float inf = std::numeric_limits<float>::infinity();
    const std::vector<float2> init_minmax (num_groups_x * num_groups_y, float2{inf, -inf});
    D3D11_SUBRESOURCE_DATA data = {nullptr};
    data.pSysMem = init_minmax.data();
    data.SysMemPitch = sizeof(float2) * desc.Width;
    data.SysMemSlicePitch = 0;


    HRESULT out_reduction = device->CreateTexture2D(&desc, &data, this->reduce_texture.GetAddressOf());
    if (FAILED(out_reduction))
    {
        throw jumpflood_error(out_reduction, "Could not create output buffer for minmax reduction.");
    }

    HRESULT out_uav = device->CreateUnorderedAccessView(this->reduce_texture.Get(), nullptr, this->reduce_uav.GetAddressOf());
    if (FAILED(out_uav))
    {
        throw jumpflood_error(out_uav, "Could not create output UAV for minmax reduction.");
    }

#ifdef DEBUG
    D3D_SET_OBJECT_NAME_A(this->reduce_texture, "JumpFloodResources::reduce_texture");
    D3D_SET_OBJECT_NAME_A(this->reduce_uav, "JumpFloodResources::reduce_uav");
#endif

    return this->reduce_uav.Get();

}

ID3D11ShaderResourceView *JumpFloodResources::create_reduction_view(bool regenerate) {
    if (this->reduce_input_srv != nullptr && !regenerate)
    {
        return this->reduce_input_srv.Get();
    }


    HRESULT srv = device->CreateShaderResourceView(this->distance_texture.Get(), nullptr, this->reduce_input_srv.GetAddressOf());
    if (FAILED(srv))
    {
        throw jumpflood_error(srv, "Could not create input SRV for minmax reduction.");
    }
#ifdef DEBUG
    D3D_SET_OBJECT_NAME_A(this->reduce_input_srv, "JumpFloodResources::reduce_input_srv");
#endif

    return this->reduce_input_srv.Get();
}


