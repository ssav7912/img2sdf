

#include "library.h"

#include <iostream>
#include <d3d11.h>
#include "preprocess.hcs"
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
    ComPtr<ID3D11ComputeShader> preprocess_shader;
    hr = dxinit::device->CreateComputeShader(g_preprocess, sizeof(g_preprocess), nullptr, &preprocess_shader);
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

    //create staging texture for CPU read.
    D3D11_TEXTURE2D_DESC CPU_read = structured;
    CPU_read.Usage = D3D11_USAGE_STAGING;
    CPU_read.BindFlags = 0;

    ID3D11Texture2D* CPU_read_texture = nullptr;
    D3D11_SUBRESOURCE_DATA CPU_read_data = structured_data;

    HRESULT out_text_result = dxinit::device->CreateTexture2D(&CPU_read, &CPU_read_data, &CPU_read_texture);
    if (FAILED(out_text_result))
    {
        printf("Could not create CPU readback texture\n");
        return -1;
    }


    printf("Running Compute Shader\n");
    auto num_groups_x = structured.Width/8;
    auto num_groups_y = structured.Height/8;
    dxinit::run_compute_shader(dxinit::context.Get(), preprocess_shader.Get(), 1, in_texture.second.GetAddressOf(),  nullptr, nullptr, 0, structured_buffer.Get(), num_groups_x,num_groups_y,1);

    //copy the finished texture into our staging texture.
    dxinit::context->CopyResource(CPU_read_texture, structured_texture.Get());

    D3D11_MAPPED_SUBRESOURCE mapped_resource;
//
    std::vector<float4> out_data {structured.Width * structured.Height, float4{0,0,0,0}};
    HRESULT map_hr = dxinit::context->Map(CPU_read_texture, 0, D3D11_MAP_READ, 0, &mapped_resource);



    if (SUCCEEDED(map_hr)){
        WICTextureWriter writer;
        dxutils::copy_to_buffer(mapped_resource.pData, structured.Height, mapped_resource.RowPitch, sizeof(float4)*structured.Width, out_data.data());


        printf("Finished shader. Writing Output File.\n");

        WICPixelFormatGUID format = GUID_WICPixelFormat128bppRGBAFloat;
        auto output_file = program_parser.get(parsing::OUTPUT_ARGUMENT);
        writer.write_texture(output_file, structured.Width, structured.Height, mapped_resource.RowPitch,
                             mapped_resource.RowPitch *structured.Width * structured.Height, format,  mapped_resource.pData);

        //write texture

    }
    else {
        printf("Could not map structured_texture.\n");
        return -1;
    }

}
