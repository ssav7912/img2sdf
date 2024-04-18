//
// Created by Soren on 10/03/2024.
//

#include "dxutils.h"
#include <algorithm>
#include "WICTextureLoader.h"
#include <limits>

void dxutils::copy_to_buffer(const void *in_buffer, size_t buffer_height, size_t stride, size_t bytes_per_row, void *out_buffer) {

    for (size_t height = 0; height < buffer_height; height++)
    {
            const size_t offset = height*stride;
            memcpy(static_cast<uint8_t*>(out_buffer) + height*bytes_per_row, static_cast<const uint8_t*>(in_buffer) + offset, bytes_per_row);
    }

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

std::pair<float, float> dxutils::serial_min_max(std::vector<float> array) {
    float minimum = std::numeric_limits<float>::infinity();
    float maximum = -std::numeric_limits<float>::infinity();

    for (const auto value : array)
    {
        minimum = std::min(minimum, value);
        maximum = std::max(maximum, value);
    }

    return {minimum, maximum};
}
