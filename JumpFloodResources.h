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

using namespace Microsoft::WRL;

class JumpFloodResources {


    JumpFloodResources(ID3D11Device* device, ID3D11DeviceContext* context);

    ID3D11ShaderResourceView* create_input_srv(std::wstring filepath);

    ID3D11UnorderedAccessView* create_voronoi_uav();

    ID3D11UnorderedAccessView* create_distance_uav();

    ID3D11Texture2D* create_staging_texture();

private:
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;

    ComPtr<ID3D11ShaderResourceView> preprocess_srv = nullptr;
    ComPtr<ID3D11Texture2D> preprocess_texture = nullptr;

    ComPtr<ID3D11UnorderedAccessView> voronoi_uav = nullptr;
    ComPtr<ID3D11Texture2D> voronoi_texture = nullptr;

    ComPtr<ID3D11UnorderedAccessView> distance_uav = nullptr;
    ComPtr<ID3D11Texture2D> distance_texture = nullptr;

    ComPtr<ID3D11Texture2D> staging_texture = nullptr;

};


#endif //IMG2SDF_JUMPFLOODRESOURCES_H
