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


namespace dxutils
{
    using namespace Microsoft::WRL;
    ///Copies strided data into contiguous array.
    ///@param buffer_height the height in 'stride units' of the source array.
    ///@param stride the stride in bytes of the source array. This includes any packing data!
    ///@param bytes_per_row the data bytes in a single row of the source array, excluding packing data.
    void copy_to_buffer(const void* in_buffer, size_t buffer_height, size_t stride, size_t bytes_per_row, void* out_buffer);

    std::pair<ComPtr<ID3D11Resource>, ComPtr<ID3D11ShaderResourceView>> load_texture_to_srv(const std::wstring &texture_path, ID3D11Device* device, ID3D11DeviceContext* context);

}

#endif //IMG2SDF_DXUTILS_H
