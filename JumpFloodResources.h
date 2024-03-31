//
// Created by Soren on 29/03/2024.
//

#ifndef IMG2SDF_JUMPFLOODRESOURCES_H
#define IMG2SDF_JUMPFLOODRESOURCES_H
#include "dxinit.h"
#include "dxutils.h"
#include "library.h"
#include "shader_globals.h"
#include <wrl.h>
#include <stdexcept>
#include <format>

using namespace Microsoft::WRL;

enum class RESOURCE_TYPE
{
    INPUT_SRV,
    VORONOI_UAV,
    DISTANCE_UAV,
    STAGING_TEXTURE,
};

class JumpFloodResources {
public:
    ///Initialises the input SRV and texture from the given file path.
    ///`device` is a non-owning pointer.
    JumpFloodResources(ID3D11Device* device, const std::wstring& file_path);

    ///Returns a weak pointer to the input SRV.
    ///The SRV is into an R32_Float Texture2D.
    ID3D11ShaderResourceView* get_input_srv();

    ///Creates the UAV for the output voronoi diagram. This is a Texture2D of same width and height as the input SRV,
    ///with R32G32B32A32_Float format.
    ///Returns a weak pointer to the UAV. Ownership is ultimately managed by this object.
    ID3D11UnorderedAccessView* create_voronoi_uav();

    ///Creates the UAV for the output distance transform. This is a Texture2D of same width and height as the input SRV,
    ///with R32_Float format.
    ///Returns a weak pointer to the UAV. Ownership is ultimately managed by this object.
    ID3D11UnorderedAccessView* create_distance_uav();

    ///Returns a non-owning pointer to the resource associated with the specified SRV/UAV.
    [[nodiscard]] ID3D11Texture2D* get_texture(RESOURCE_TYPE desired_texture) const;

    ///Initialises a staging texture for CPU readback, using the input `mimic_texture` as a template.
    ///Note that the staging texture subresource is *not* initialised, so it must be copied to before reading!
    ///Returns a weak pointer to the texture. Ownership is ultimately managed by this object.
    ID3D11Texture2D* create_staging_texture(ID3D11Texture2D* mimic_texture);

    ///Creates a constant buffer containing the texture Width and Height to use as a shader constant.
    ID3D11Buffer* create_const_buffer();

    ///Gets the width and height of the resources. Every resource has the same pixel width and height.
    ///returns {0,0} if the input SRV has not been loaded.
    [[nodiscard]] resolution get_resolution() const;

private:

    ///Creates a UAV of format `format` and zero initialises it with Width*Height elements of `format_type`.
    ///Requires that `input_description` has been set, i.e. that the input SRV has been loaded.
    ///may throw a std::runtime_error if UAV or texture creation fails.
    template <typename format_type>
    std::pair<ComPtr<ID3D11Texture2D>, ComPtr<ID3D11UnorderedAccessView>> create_uav(DXGI_FORMAT format)
    {

        D3D11_TEXTURE2D_DESC desc = input_description;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.Format = format;
        desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
        desc.MiscFlags = 0;

        ComPtr<ID3D11Texture2D> uav_texture = nullptr;

        D3D11_SUBRESOURCE_DATA data;
        std::vector<format_type> init_data (desc.Width * desc.Height, format_type {0});
        data.pSysMem = init_data.data();
        data.SysMemPitch = sizeof(format_type) * desc.Width;

        HRESULT out_tex = device->CreateTexture2D(&desc, &data, &uav_texture);
        if (FAILED(out_tex))
        {
            throw std::runtime_error {std::format("Failed to Instantiate Texture for UAV. HRESULT {:x}\n", out_tex)};
        }

        ComPtr<ID3D11UnorderedAccessView> buffer;

        HRESULT buffer_result = device->CreateUnorderedAccessView(uav_texture.Get(), nullptr, buffer.GetAddressOf());
        if (FAILED(buffer_result))
        {
            throw std::runtime_error {std::format("Failed to Instantiate UAV. HRESULT {:x}\n", buffer_result)};
        }

        return {uav_texture, buffer};
    }

    ///Loads an SRV and texture from a filepath. This will be an R32_Float Texture2D. Conversion is done if necessary,
    ///according to WIC rules.
    ID3D11ShaderResourceView* create_input_srv(std::wstring filepath);

    ID3D11Device* device = nullptr;

    resolution res = {0};

    ComPtr<ID3D11ShaderResourceView> preprocess_srv = nullptr;
    ComPtr<ID3D11Texture2D> preprocess_texture = nullptr;

    D3D11_TEXTURE2D_DESC input_description = {0};

    ComPtr<ID3D11UnorderedAccessView> voronoi_uav = nullptr;
    ComPtr<ID3D11Texture2D> voronoi_texture = nullptr;

    ComPtr<ID3D11UnorderedAccessView> distance_uav = nullptr;
    ComPtr<ID3D11Texture2D> distance_texture = nullptr;

    ComPtr<ID3D11Texture2D> staging_texture = nullptr;

    ComPtr<ID3D11Buffer> const_buffer = nullptr;

};


#endif //IMG2SDF_JUMPFLOODRESOURCES_H
