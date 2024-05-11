

#include "program.h"

#include <iostream>
#include <d3d11.h>
#include <wrl.h>
#include <vector>
#include <filesystem>
#include <argparse/argparse.hpp>
#include "../dxinit.h"
#include "../dxutils.h"
#include "../WICTextureWriter.h"
#include "../JumpFloodResources.h"
#include "../JumpFloodDispatch.h"

//HLSL bytecode includes
#include "../shaders/jumpflood.hcs"
#include "../shaders/preprocess.hcs"
#include "../shaders/voronoi_normalise.hcs"
#include "../shaders/distance.hcs"
#include "../shaders/minmax_reduce.hcs"
#include "../shaders/minmaxreduce_firstpass.hcs"
#include "../shaders/normalise.hcs"
#include "../img2sdf.h"
#include "../WICTextureLoader.h"

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

    //parse arguments
    argparse::ArgumentParser program_parser {parsing::PROGRAM_NAME};
    program_parser.add_argument(parsing::INPUT_ARGUMENT).help("Input image to jumpflood. Must be either"
                                                              "a PNG, EXR, BMP, TIFF or DDS. Must have width & height a power of 2.");

    program_parser.add_argument(parsing::OUTPUT_ARGUMENT).help("Output image.");

    auto& group = program_parser.add_mutually_exclusive_group(true);
    group.add_argument(parsing::UNSIGNED, parsing::UNSIGNED_LONG).help("Generate an unsigned distance field.").flag();
    group.add_argument(parsing::VORONOI, parsing::VORONOI_LONG).help("Generate a voronoi diagram.").flag();

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


    ComPtr<ID3D11Resource> in_resource = nullptr;
    ComPtr<ID3D11ShaderResourceView> _ = nullptr;

#ifdef DEBUG
    Img2SDF img2sdf{dxinit::device, dxinit::context, dxinit::debug_layer};
#else
    Img2SDF img2sdf{dxinit::device, dxinit::context};
#endif

    HRESULT wic_hr = CreateWICR32FTextureFromFile(dxinit::device.Get(), absolute_texture_path.wstring().c_str(),
                                                  in_resource.GetAddressOf(), _.GetAddressOf());
    if (FAILED(wic_hr)) {
        printf("Failed to load input texture.\n");
        return -1;
    }

    ComPtr<ID3D11Texture2D> in_texture;
    if (FAILED(in_resource.As(&in_texture))) {
        printf("input texture is not an ID3D11Texture2D!\n");
        return -1;
    }

    WICPixelFormatGUID resource_format;
    ComPtr<ID3D11Texture2D> out_texture = nullptr;
    if (program_parser.is_used(parsing::UNSIGNED))
    {
        resource_format = GUID_WICPixelFormat32bppGrayFloat;
        out_texture = img2sdf.compute_unsigned_distance_field(in_texture, true);

    }
    else if (program_parser.is_used(parsing::VORONOI))
    {
        resource_format = GUID_WICPixelFormat128bppRGBAFloat;
        out_texture = img2sdf.compute_voronoi_transform(in_texture, true);
    }

    auto staging = dxutils::create_staging_texture(dxinit::device.Get(), out_texture.Get());
    D3D11_TEXTURE2D_DESC out_desc{0};
    auto mapped_resource = dxutils::copy_to_staging(dxinit::context.Get(), staging.Get(), out_texture.Get(),
                                                    &out_desc);


    const auto Width = out_desc.Width;
    const auto Height = out_desc.Height;

    WICTextureWriter writer{};


    printf("Finished shader. Writing Output File.\n");

    const WICPixelFormatGUID output_format = GUID_WICPixelFormat32bppRGBA;
    auto output_file = program_parser.get(parsing::OUTPUT_ARGUMENT);
    HRESULT out_result = writer.write_texture(output_file, Width, Height, mapped_resource.RowPitch,
                                              mapped_resource.RowPitch * Height, resource_format, output_format,
                                              mapped_resource.pData);

    //write texture
    if (FAILED(out_result)) {
        printf("Could not write output texture!\n");
        return -1;

    }
}
