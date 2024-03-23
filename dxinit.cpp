//
// Created by Soren on 2/03/2024.
//

#include "dxinit.h"
#include <filesystem>
#include <string>
#include <tchar.h>

Microsoft::WRL::ComPtr<ID3D11DeviceContext> dxinit::context = nullptr;
Microsoft::WRL::ComPtr<ID3D11Device> dxinit::device = nullptr;
Microsoft::WRL::ComPtr<ID3D11ComputeShader> dxinit::shader = nullptr;
#if DEBUG
Microsoft::WRL::ComPtr<ID3D11Debug> dxinit::debug_layer = nullptr;

#endif

#define _L(x)  __L(x)
#define __L(x)  L##x

HRESULT dxinit::create_compute_device(ID3D11Device **device_out, ID3D11DeviceContext **context_out, bool force_ref) {
    *device_out = nullptr;
    *context_out = nullptr;

    HRESULT hr = S_OK;

    UINT creation_flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
#ifdef DEBUG
    creation_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_FEATURE_LEVEL feature_level_out;
    static const D3D_FEATURE_LEVEL feature_level[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0};

    bool need_ref_device = false;
    if (!force_ref)
    {
        hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, creation_flags, feature_level, sizeof(feature_level)/ sizeof(D3D_FEATURE_LEVEL),
                               D3D11_SDK_VERSION, device_out, &feature_level_out, context_out);

        if (SUCCEEDED(hr))
        {
            if (feature_level_out < D3D_FEATURE_LEVEL_11_0)
            {
#ifdef TEST_DOUBLE
                need_ref_device = true;
                printf( "No hardware Compute Shader 5.0 capable device found (required for doubles), trying to create ref device.\n" );
#else
                D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS hwopts;
                (*device_out)->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &hwopts, sizeof(hwopts));
                if (!hwopts.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x)
                {
                    need_ref_device = true;
                    printf( "No hardware Compute Shader capable device found, trying to create ref device.\n" );
                }
#endif
            }
#ifdef TEST_DOUBLE
            else {
                D3D11_FEATURE_DATA_DOUBLES hwopts;
                (*device_out)->CheckFeatureSupport(D3D11_FEATURE_DOUBLES, &hwopts, sizeof(hwopts));
                if (!hwopts.DoublePrecisionShaderOps)
                    {
                        need_ref_device = true;
                        printf( "No hardware double-precision capable device found, trying to create ref device.\n" );
                    }
            }

#endif
        }
    }
    if (force_ref || FAILED(hr) || need_ref_device)
    {
        (*device_out)->Release();
        *device_out = nullptr;
        (*context_out)->Release();
        *context_out = nullptr;

        hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_REFERENCE, nullptr, creation_flags, feature_level, sizeof(feature_level) / sizeof(D3D_FEATURE_LEVEL ),
                               D3D11_SDK_VERSION, device_out, &feature_level_out, context_out);

        if (FAILED(hr))
        {
            printf("Reference rasterizer device create failure\n");
            return hr;
        }
    }

#ifdef DEBUG
    if (!SUCCEEDED((*device_out)->QueryInterface(__uuidof(ID3D11Debug), (void**)&dxinit::debug_layer)))
        {
            printf("Failed to create debug layer");
        }
#endif

    return hr;
}

HRESULT dxinit::create_compute_shader(LPCWSTR source_file, LPCSTR function_name, ID3D11Device *d11Device,
                                      ID3D11ComputeShader **shader_out) {
    if (!d11Device || !shader_out) {
        return E_INVALIDARG;
    }

    DWORD shader_flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    shader_flags |= D3DCOMPILE_DEBUG;
    shader_flags |= D3DCOMPILE_SKIP_OPTIMIZATION;

#endif
    const D3D_SHADER_MACRO defines[] = {
#ifdef TEST_DOUBLE
            "TEST_DOUBLE", "1",
#endif
            nullptr, nullptr
    };

    LPCSTR profile = (d11Device->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0) ? "cs_5_0" : "cs_4_0";

    ID3DBlob *error_blob = nullptr;
    ID3DBlob *blob = nullptr;

    std::wstring file_path = _L(__FILE__);
    std::wstring dir_path = file_path.substr(0, file_path.rfind(L"\\"));
    std::wstring shader_path = (dir_path + L"\\preprocess.hlsl");
    HRESULT hr = D3DCompileFromFile(shader_path.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                                    function_name,
                                    profile, shader_flags, 0, &blob, &error_blob);
    if (FAILED(hr))
    {
        if (error_blob)
        {
            OutputDebugStringA((char*)error_blob->GetBufferPointer());
            error_blob->Release();
            error_blob = nullptr;
        }

        blob->Release();
        blob = nullptr;
        return hr;
    }

    hr = d11Device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, shader_out);
    if (error_blob)
    {
        error_blob->Release();
        error_blob = nullptr;
    }

    blob->Release();
    blob = nullptr;

#if defined(_DEBUG) || defined(PROFILE)
    if (SUCCEEDED(hr))
    {
        (*shader_out)->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(function_name), function_name);
    }
#endif
    return hr;
}

void dxinit::run_compute_shader(ID3D11DeviceContext *immediate_context, ID3D11ComputeShader *shader, UINT num_views,
                                   ID3D11ShaderResourceView **views, ID3D11Buffer *CBCS, void *shader_data,
                                   DWORD data_bytes, ID3D11UnorderedAccessView *uav, UINT x, UINT y, UINT z) {

    immediate_context->CSSetShader(shader, nullptr, 0);
    immediate_context->CSSetShaderResources(0, num_views, views);
    immediate_context->CSSetUnorderedAccessViews(0,1, &uav, nullptr);

    if (CBCS && shader_data)
    {
        D3D11_MAPPED_SUBRESOURCE resource;
        immediate_context->Map(CBCS, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
        memcpy(resource.pData, shader_data, data_bytes);
        immediate_context->Unmap(CBCS, 0);
        ID3D11Buffer* CB[1] = {CBCS};
        immediate_context->CSSetConstantBuffers(0,1,CB);
    }

    immediate_context->Dispatch(x,y,z);
    immediate_context->CSSetShader(nullptr,nullptr,0);
    ID3D11UnorderedAccessView* null_view[1] = {nullptr};
    immediate_context->CSSetUnorderedAccessViews(0,1,null_view, nullptr);

    ID3D11ShaderResourceView* null_SRV[2] = {nullptr,nullptr};
    immediate_context->CSSetShaderResources(0,2,null_SRV);

    ID3D11Buffer* null_CB[1] = {nullptr};
    immediate_context->CSSetConstantBuffers(0,1, null_CB);


}

ID3D11Buffer *
dxinit::create_and_copy_debug_buffer(ID3D11Device *device, ID3D11DeviceContext *context, ID3D11Buffer *buffer) {
    ID3D11Buffer* debug_buffer = nullptr;

    D3D11_BUFFER_DESC desc = {};
    buffer->GetDesc(&desc);
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.MiscFlags = 0;

    if (SUCCEEDED(device->CreateBuffer(&desc, nullptr, &debug_buffer)))
    {
#if defined(_DEBUG) || defined(PROFILE)
        debug_buffer->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof("Debug") - 1, "Debug");
#endif
        context->CopyResource(debug_buffer, buffer);
    }
    return debug_buffer;
}