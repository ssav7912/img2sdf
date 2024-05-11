//
// Created by Soren on 10/03/2024.
//

#ifndef IMG2SDF_DXUTILS_H
#define IMG2SDF_DXUTILS_H
#include <cstddef>
#include <utility>
#include <wrl.h>
#include <d3d11.h>
#include <string>
#include <vector>

#include <stdexcept>
#include "shader_globals.h"

namespace dxutils
{
    using namespace Microsoft::WRL;
    ///Copies strided data into contiguous array.
    ///@param buffer_height the height in 'stride units' of the source array.
    ///@param stride the stride in bytes of the source array. This includes any packing data!
    ///@param bytes_per_row the data bytes in a single row of the source array, excluding packing data.
    void copy_to_buffer(const void* in_buffer, size_t buffer_height, size_t stride, size_t bytes_per_row, void* out_buffer);

    std::pair<ComPtr<ID3D11Resource>, ComPtr<ID3D11ShaderResourceView>> load_texture_to_srv(const std::wstring &texture_path, ID3D11Device* device);

    ComPtr<ID3D11Texture2D> create_staging_texture(ID3D11Device* device, ID3D11Texture2D *mimic_texture);

    ///Serial (pixel-by-pixel) implementation of min/max statistic calculator for an array.
    ///Used as a ground-truth reference for the parallel reduce minmax.
    std::pair<float, float> serial_min_max(const std::vector<float>& array);
    std::pair<float, float> serial_min_max(const std::vector<float2>& array);

    D3D11_MAPPED_SUBRESOURCE
    copy_to_staging(ID3D11DeviceContext *context, ID3D11Texture2D *staging_texture, ID3D11Texture2D *texture, D3D11_TEXTURE2D_DESC* out_desc = nullptr);


///maps a D3D11Texture to a staging resource, and then copies it out to a vector with the specified template type.
    ///Accounts for stride & padding in the texture.
    ///@param staging_texture a staging texture to copy to. Must match the @param texture description, use JumpFloodResources::create_owned_staging_texture to copy.
    ///@tparam data_type *must* match the width and layout of the DXGI_FORMAT in the @param texture and @param staging_texture
    template<typename data_type>
    std::vector<data_type> copy_to_vector(ID3D11DeviceContext* context, ID3D11Texture2D* staging_texture, ID3D11Texture2D* texture)
    {
        D3D11_TEXTURE2D_DESC desc = {0};
        D3D11_MAPPED_SUBRESOURCE resource = copy_to_staging(context, staging_texture, texture, &desc);
        std::vector<data_type> out_data(desc.Width * desc.Height, {0});

        dxutils::copy_to_buffer(resource.pData, desc.Height, resource.RowPitch, sizeof(data_type)*desc.Width, out_data.data());

        context->Unmap(staging_texture, 0);

        return out_data;
    }

    bool is_power_of_two(uint32_t n);


    struct query_profile
    {
        explicit query_profile(ID3D11DeviceContext* context) : context(context) {
            context->Begin(nullptr);
        };


        ~query_profile()
        {

        }
        ID3D11DeviceContext* context;
    };

}

#endif //IMG2SDF_DXUTILS_H
