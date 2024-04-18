

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
#include "JumpFloodDispatch.h"

//HLSL bytecode includes
#include "shaders/jumpflood.hcs"
#include "shaders/preprocess.hcs"
#include "shaders/voronoi_normalise.hcs"
#include "shaders/distance.hcs"
#include "shaders/minmax_reduce.hcs"
#include "shaders/minmaxreduce_firstpass.hcs"

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

    //create input texture
    auto texture_name = program_parser.get(parsing::INPUT_ARGUMENT);
    auto absolute_texture_path = std::filesystem::absolute({texture_name});

    auto jfa_resources = JumpFloodResources(dxinit::device.Get(), absolute_texture_path.wstring());

    ID3D11ShaderResourceView* in_srv = jfa_resources.get_input_srv();

    //create output UAV.
    ID3D11UnorderedAccessView* voronoi_uav = jfa_resources.create_voronoi_uav(false);

    //create distance UAV.
    ID3D11UnorderedAccessView* distance_uav = jfa_resources.create_distance_uav(false);

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
    ID3D11Texture2D* voronoi_texture = jfa_resources.get_texture(RESOURCE_TYPE::VORONOI_UAV);
    ID3D11Texture2D* CPU_read_texture = jfa_resources.create_staging_texture(voronoi_texture);

    ID3D11Texture2D* reduce_texture = jfa_resources.get_texture(RESOURCE_TYPE::REDUCE_UAV);
    ID3D11Texture2D* reduce_staging = jfa_resources.create_staging_texture(reduce_texture);

    const size_t Width = jfa_resources.get_resolution().width;
    const size_t Height = jfa_resources.get_resolution().height;

    jump_flood_shaders shaders {
        .preprocess = g_preprocess,
        .preprocess_size = sizeof(g_preprocess),

        .voronoi = g_main,
        .voronoi_size = sizeof(g_main),

        .voronoi_normalise = g_voronoi_normalise,
        .voronoi_normalise_size = sizeof(g_voronoi_normalise),

        .distance_transform = g_distance,
        .distance_transform_size = sizeof(g_distance),

        .min_max_reduce = g_reduce,
        .min_max_reduce_size = sizeof(g_reduce),

        .min_max_reduce_firstpass = g_reduce_firstpass,
        .min_max_reduce_firstpass_size = sizeof(g_reduce_firstpass)


    };

    //dispatcher probably shouldn't also be compiling.
    JumpFloodDispatch dispatcher {dxinit::device.Get(), dxinit::context.Get(), shaders, &jfa_resources};

    printf("Running Preprocess Compute Shader\n");
    dispatcher.dispatch_preprocess_shader();


    printf("Running Voronoi Compute Shader\n");
    dispatcher.dispatch_voronoi_shader();


    printf("Running Distance Transform Compute Shader\n");

    dispatcher.dispatch_distance_transform_shader();

    printf("DEBUG: Running voronoi normalise\n");
    dispatcher.dispatch_voronoi_normalise_shader();

    printf("Running Min Max Reduction\n");
    dispatcher.dispatch_minmax_reduce_shader();

    //copy the finished texture into our staging texture.
    dxinit::context->CopyResource(CPU_read_texture, voronoi_texture);

    D3D11_MAPPED_SUBRESOURCE mapped_resource;
//
    std::vector<float4> out_data (Width * Height, {0});
    HRESULT map_hr = dxinit::context->Map(CPU_read_texture, 0, D3D11_MAP_READ, 0, &mapped_resource);

    auto out_minmax = dxutils::copy_to_staging<float2>(dxinit::context.Get(), reduce_staging, reduce_texture);

    if (SUCCEEDED(map_hr)){
        WICTextureWriter writer {};
        dxutils::copy_to_buffer(mapped_resource.pData, Height, mapped_resource.RowPitch, sizeof(float4)*Width, out_data.data());


        printf("Finished shader. Writing Output File.\n");

        const WICPixelFormatGUID resource_format = GUID_WICPixelFormat128bppRGBAFloat;
        const WICPixelFormatGUID output_format = GUID_WICPixelFormat32bppRGBA;
        auto output_file = program_parser.get(parsing::OUTPUT_ARGUMENT);
        HRESULT out_result = writer.write_texture(output_file, Width, Height, mapped_resource.RowPitch,
                             mapped_resource.RowPitch * Height, resource_format, output_format,  mapped_resource.pData);

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
