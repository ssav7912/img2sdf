

#include "library.h"

#include <iostream>
#include <d3d11.h>
#include "shader_globals.h"
#include <wrl.h>
#include <vector>
#include <filesystem>
#include <argparse/argparse.hpp>
#include "dxinit.h"
#include "dxutils.h"
#include "WICTextureWriter.h"
#include "JumpFloodResources.h"

//HLSL bytecode includes
#include "jumpflood.hcs"
#include "preprocess.hcs"

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
                                                              "a PNG, EXR, BMP, TIFF or DDS. Must have width & height a power of 2.");

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
        printf("Failed to create jump flood shader from compiled byte code.");
        return -1;

    }

    ComPtr<ID3D11ComputeShader> preprocess_shader;
    hr = dxinit::device->CreateComputeShader(g_preprocess, sizeof(g_preprocess), nullptr, &preprocess_shader);
    if (FAILED(hr))
    {
        printf("Failed to create preprocess shader from compiled bytecode.\n");
        return -1;
    }

    //create input texture
    auto texture_name = program_parser.get(parsing::INPUT_ARGUMENT);
    auto absolute_texture_path = std::filesystem::absolute({texture_name});

    auto jfa_resources = JumpFloodResources(dxinit::device.Get(), absolute_texture_path.wstring());

    ID3D11ShaderResourceView* in_srv = jfa_resources.get_input_srv();


    //create output UAV.
    ID3D11UnorderedAccessView* voronoi_uav = jfa_resources.create_voronoi_uav();

    //create distance UAV.
    ID3D11UnorderedAccessView* distance_uav = jfa_resources.create_distance_uav();

    //get resource for staging texture.
    ComPtr<ID3D11Texture2D> distance_texture = nullptr;

    //inner scope to release temporary distance_resource after ownership is transferred.
    {
        ComPtr<ID3D11Resource> distance_resource;
        distance_uav->GetResource(distance_resource.GetAddressOf());

        if (FAILED(distance_resource.As<ID3D11Texture2D>(&distance_texture))) {
            printf("Returned Resource from Distance UAV was not a Texture2D\n");
            return -1;
        }
    }

    //create const buffer with resource Width and Height.
    ID3D11Buffer* const_buffer = jfa_resources.create_const_buffer();

    //create staging texture for CPU read.
    ID3D11Texture2D* CPU_read_texture = jfa_resources.create_staging_texture(distance_texture.Get());

    const size_t Width = jfa_resources.get_resolution().width;
    const size_t Height = jfa_resources.get_resolution().height;
    const uint32_t num_groups_x = Width/8;
    const uint32_t num_groups_y = Height/8;

    printf("Running Preprocess Compute Shader\n");
    {
        ID3D11UnorderedAccessView* UAV = voronoi_uav;
        dxinit::run_compute_shader(dxinit::context.Get(), preprocess_shader.Get(), 1, &in_srv,
                                   const_buffer, nullptr, 0, UAV, 1, num_groups_x, num_groups_y, 1);

    }

    printf("Running Voronoi Compute Shader\n");

    ID3D11UnorderedAccessView* UAVs[] = {voronoi_uav, distance_uav};
    constexpr size_t num_uavs = sizeof(UAVs)/sizeof(UAVs[0]);
    dxinit::run_compute_shader(dxinit::context.Get(), jumpflood_shader.Get(), 1, &in_srv,
                               const_buffer, nullptr, 0, UAVs, num_uavs, num_groups_x, num_groups_y, 1);


    //copy the finished texture into our staging texture.
    dxinit::context->CopyResource(CPU_read_texture, distance_texture.Get());

    D3D11_MAPPED_SUBRESOURCE mapped_resource;
//
    std::vector<float> out_data (Width * Height, 0.0f);
    HRESULT map_hr = dxinit::context->Map(CPU_read_texture, 0, D3D11_MAP_READ, 0, &mapped_resource);

    if (SUCCEEDED(map_hr)){
        WICTextureWriter writer {};
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
