

#include "library.h"

#include <iostream>
#include <d3d11.h>
#include "jumpflood.hcs"
#include "shader_globals.h"
#include <wrl.h>
#include <vector>
#include <filesystem>
#include <argparse/argparse.hpp>
#include "dxinit.h"
#include "dxutils.h"
#include "WICTextureWriter.h"

using namespace Microsoft::WRL;



int main(int32_t argc, const char** argv)
{
    Windows::Foundation::Initialize(RO_INIT_MULTITHREADED);

    HRESULT hr = dxinit::create_compute_device(&dxinit::device, &dxinit::context, false);

    if (FAILED(hr))
    {
        printf("Failed to create compute device.");
        return -1;
    }
#ifdef DEBUG
    dxinit::debug_layer->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
#endif

    //parse arguments
    argparse::ArgumentParser program_parser {parsing::PROGRAM_NAME};
    program_parser.add_argument(parsing::INPUT_ARGUMENT).help("Input image to jumpflood. Must be either"
                                                              "a PNG, EXR, BMP, TIFF or DDS.");

    program_parser.add_argument(parsing::OUTPUT_ARGUMENT).help("Output image.");

    try {
        program_parser.parse_args(argc, argv);
    }
    catch (const std::exception& err)
    {
        std::cerr << err.what() << std::endl;
        std::cerr << program_parser;
        return 1;
    }


    //create shaders.
    ComPtr<ID3D11ComputeShader> jumpflood_shader;
    hr = dxinit::device->CreateComputeShader(g_main, sizeof(g_main), nullptr, &jumpflood_shader);
    if (FAILED(hr))
    {
        printf("Failed to create preprocess shader from compiled byte code.");
        return -1;

    }

    //create input texture
    auto texture_name = program_parser.get(parsing::INPUT_ARGUMENT);
    auto absolute_texture_path = std::filesystem::absolute({texture_name});

    auto in_texture = dxutils::load_texture_to_srv(absolute_texture_path.wstring(), dxinit::device.Get(), dxinit::context.Get());

    ComPtr<ID3D11Texture2D> preprocess_texture;
    if (FAILED(in_texture.first.As<ID3D11Texture2D>(&preprocess_texture)))
    {
        printf("Returned resource was not a texture.");
        return -1;
    }

    D3D11_TEXTURE2D_DESC input_texture_description;
    preprocess_texture->GetDesc(&input_texture_description);

    if (input_texture_description.Format != DXGI_FORMAT_R32_FLOAT)
    {
        printf("WARNING: Input format is not a grayscale mask.\n");
    }


    //create output UAV.

    D3D11_TEXTURE2D_DESC structured = input_texture_description;
    structured.Usage = D3D11_USAGE_DEFAULT;
    structured.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    structured.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    structured.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    structured.MiscFlags = NULL;

    ComPtr<ID3D11Texture2D> structured_texture = nullptr;

    D3D11_SUBRESOURCE_DATA structured_data;
    std::vector<float4> initstructureddata {structured.Width * structured.Height, float4 {0,0,0,0}};
    structured_data.pSysMem = initstructureddata.data();
    structured_data.SysMemPitch = sizeof(float4) * structured.Width;


    HRESULT out_tex_result = dxinit::device->CreateTexture2D(&structured, &structured_data, &structured_texture);
    if (FAILED(out_tex_result))
    {
        printf("Could not create output texture");
        return -1;
    }

    ComPtr<ID3D11UnorderedAccessView> structured_buffer;

    HRESULT buffer_result = dxinit::device->CreateUnorderedAccessView(static_cast<ID3D11Resource*>(structured_texture.Get()),
                                                                      nullptr, &structured_buffer);

    if (FAILED(buffer_result))
    {
        printf("Could not create output UAV\n");
        return -1;
    }

    D3D11_TEXTURE2D_DESC distance_output = structured;
    distance_output.Format = DXGI_FORMAT_R32_FLOAT;

    ComPtr<ID3D11Texture2D> distance_texture = nullptr;

    D3D11_SUBRESOURCE_DATA distance_data;
    std::vector<float> init_distance_data (distance_output.Width * distance_output.Height, 0.0f);
    distance_data.pSysMem = init_distance_data.data();
    distance_data.SysMemPitch = sizeof(float) * distance_output.Width;

    HRESULT out_distance_tex_result = dxinit::device->CreateTexture2D(&distance_output, &distance_data, distance_texture.GetAddressOf());

    if (FAILED(out_distance_tex_result))
    {
        printf("Could not create output distance texture.\n");
        return -1;
    }

    ComPtr<ID3D11UnorderedAccessView> distance_buffer;
    HRESULT dist_buffer_result = dxinit::device->CreateUnorderedAccessView(static_cast<ID3D11Resource*>(distance_texture.Get()), nullptr, distance_buffer.GetAddressOf());

    if (FAILED(dist_buffer_result))
    {
        printf("Could not create output distance UAV\n");
        return -1;
    }

    //create input constants
    const float width_float = static_cast<float>(structured.Width);
    const JFA_cbuffer cbuffer = {width_float, static_cast<float>(structured.Height), 0};


    D3D11_BUFFER_DESC const_buffer_desc = {0};
    const_buffer_desc.ByteWidth = sizeof(cbuffer);
    const_buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
    const_buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    D3D11_SUBRESOURCE_DATA const_buffer_resource = {};
    const_buffer_resource.pSysMem = &cbuffer;
    const_buffer_resource.SysMemPitch = 0;
    const_buffer_resource.SysMemSlicePitch = 0;

    ComPtr<ID3D11Buffer> const_buffer;
    HRESULT const_buffer_result = dxinit::device->CreateBuffer(&const_buffer_desc, &const_buffer_resource, const_buffer.GetAddressOf());
    if (FAILED(const_buffer_result))
    {
        printf("Could not create buffer for shader constants.\n");
        return -1;
    }

    //create staging texture for CPU read.
    D3D11_TEXTURE2D_DESC CPU_read = distance_output;
    CPU_read.Usage = D3D11_USAGE_STAGING;
    CPU_read.BindFlags = 0;

    ID3D11Texture2D* CPU_read_texture = nullptr;
    D3D11_SUBRESOURCE_DATA CPU_read_data = distance_data;

    HRESULT out_text_result = dxinit::device->CreateTexture2D(&CPU_read, &CPU_read_data, &CPU_read_texture);
    if (FAILED(out_text_result))
    {
        printf("Could not create CPU readback texture\n");
        return -1;
    }

    const size_t Width = distance_output.Width;
    const size_t Height = distance_output.Height;

    printf("Running Compute Shader\n");
    uint32_t num_groups_x = Width/8;
    uint32_t num_groups_y = Height/8;
    ID3D11UnorderedAccessView* UAVs[] = {structured_buffer.Get(), distance_buffer.Get()};
    dxinit::run_compute_shader(dxinit::context.Get(), jumpflood_shader.Get(), 1, in_texture.second.GetAddressOf(),
                               const_buffer.Get(), nullptr, 0, UAVs, 2, num_groups_x, num_groups_y, 1);


    //copy the finished texture into our staging texture.
    dxinit::context->CopyResource(CPU_read_texture, distance_texture.Get());

    D3D11_MAPPED_SUBRESOURCE mapped_resource;
//
    std::vector<float> out_data (Width * Height, 0.0f);
    HRESULT map_hr = dxinit::context->Map(CPU_read_texture, 0, D3D11_MAP_READ, 0, &mapped_resource);




    if (SUCCEEDED(map_hr)){
        WICTextureWriter writer;
        dxutils::copy_to_buffer(mapped_resource.pData, Height, mapped_resource.RowPitch, sizeof(float)*Width, out_data.data());


        printf("Finished shader. Writing Output File.\n");

        WICPixelFormatGUID format = GUID_WICPixelFormat32bppGrayFloat;
        auto output_file = program_parser.get(parsing::OUTPUT_ARGUMENT);
        HRESULT out_result = writer.write_texture(output_file, Width, Height, mapped_resource.RowPitch,
                             mapped_resource.RowPitch * Height, format, GUID_WICPixelFormat16bppGray,  mapped_resource.pData);

        //write texture
        if (FAILED(out_result))
        {
            printf("Could not write output texture!\n");
        }

    }
    else {
        printf("Could not map structured_texture.\n");
        return -1;
    }

}
