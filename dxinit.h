//
// Created by Soren on 2/03/2024.
//

#ifndef IMG2SDF_DXINIT_H
#define IMG2SDF_DXINIT_H

#include <cstdio>
#include <d3d11.h>
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <winnt.h>
#include <wrl.h>

namespace dxinit {
    using namespace Microsoft::WRL;

    extern ComPtr<ID3D11Device> device;
    extern ComPtr<ID3D11DeviceContext> context;
    extern ComPtr<ID3D11ComputeShader> shader;
#ifdef DEBUG
    extern ComPtr<ID3D11Debug> debug_layer;

#endif

    _Use_decl_annotations_
    HRESULT create_compute_device(ID3D11Device** device_out, ID3D11DeviceContext** context_out, bool force_ref);

    _Use_decl_annotations_
    HRESULT create_compute_shader(LPCWSTR source_file, LPCSTR function_name, ID3D11Device* d11Device, ID3D11ComputeShader** shader_out);

    _Use_decl_annotations_
    void run_compute_shader(ID3D11DeviceContext* immediate_context, ID3D11ComputeShader* shader, UINT num_views,
                               ID3D11ShaderResourceView** views, ID3D11Buffer* CBCS, void* shader_data, DWORD data_bytes,
                               ID3D11UnorderedAccessView* uav, UINT x, UINT y, UINT z);

    _Use_decl_annotations_
    ID3D11Buffer* create_and_copy_debug_buffer(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Buffer* buffer);

}





#endif //IMG2SDF_DXINIT_H
