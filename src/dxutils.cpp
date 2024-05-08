//
// Created by Soren on 10/03/2024.
//

#include "dxutils.h"
#include <algorithm>
#include "WICTextureLoader.h"
#include <limits>
#include <format>
#include <wrl.h>
#include <array>

void dxutils::copy_to_buffer(const void *in_buffer, size_t buffer_height, size_t stride, size_t bytes_per_row, void *out_buffer) {

    for (size_t height = 0; height < buffer_height; height++)
    {
            const size_t offset = height*stride;
            memcpy(static_cast<uint8_t*>(out_buffer) + height*bytes_per_row, static_cast<const uint8_t*>(in_buffer) + offset, bytes_per_row);
    }

}


Microsoft::WRL::ComPtr<ID3D11Texture2D> dxutils::create_staging_texture(ID3D11Device* device, ID3D11Texture2D *mimic_texture) {
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
#ifdef DEBUG
    std::array<char, 256> buffer {0};
    UINT private_data_size = buffer.size();

    mimic_texture->GetPrivateData(WKPDID_D3DDebugObjectName, &private_data_size, buffer.data());

    std::string name (buffer.data());
    name += "_staging";

    D3D_SET_OBJECT_NAME_A(CPU_read_texture, name.c_str());
#endif
    return CPU_read_texture;
}

std::pair<dxutils::ComPtr<ID3D11Resource>, dxutils::ComPtr<ID3D11ShaderResourceView>> dxutils::load_texture_to_srv(const std::wstring &texture_path, ID3D11Device* device) {


    ID3D11Resource* resource;
    ID3D11ShaderResourceView* view;

    //should translate input file into R32F.
    HRESULT hr = CreateWICR32FTextureFromFile(device, texture_path.c_str(), &resource, &view);

    if (FAILED(hr))
    {
        printf("Failed to create texture from input path. Is the path correct?");
        return {nullptr, nullptr};
    }

    return {resource, view};


}

std::pair<float, float> dxutils::serial_min_max(const std::vector<float>& array) {
    float minimum = std::numeric_limits<float>::infinity();
    float maximum = -std::numeric_limits<float>::infinity();

    for (const auto value : array)
    {
        minimum = std::min(minimum, value);
        maximum = std::max(maximum, value);
    }

    return {minimum, maximum};
}

std::pair<float, float> dxutils::serial_min_max(const std::vector<float2>& array) {
    float minimum = std::numeric_limits<float>::infinity();
    float maximum = -std::numeric_limits<float>::infinity();

    for (const auto value : array)
    {
        minimum = std::min(minimum, value.x);
        maximum = std::max(maximum, value.y);
    }

    return {minimum, maximum};

}

bool dxutils::is_power_of_two(uint32_t n) {
    return !(n & (n - 1));
}
