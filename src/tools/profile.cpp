//
// Created by Soren on 11/05/2024.
//
#include "../Profiler.h"
#include "../img2sdf.h"
#include <wrl.h>
#include <format>
#include <vector>
#include <random>
#include <iostream>
#include <fstream>
#include "../dxinit.h"

using namespace Microsoft::WRL;

static constexpr int64_t dims[] = {8,16,32,64,128,256,512,1024,2048,4096,8192};
static constexpr size_t num_dims = sizeof(dims) / sizeof(dims[0]);

int main(void)
{
    Windows::Foundation::Initialize(RO_INIT_MULTITHREADED);

    ComPtr<ID3D11Device> device {};
    ComPtr<ID3D11DeviceContext> context {};

    HRESULT hr = dxinit::create_compute_device(device.GetAddressOf(), context.GetAddressOf(), false);

    if (FAILED(hr))
    {
        std::cout << std::format("could not create compute device. HRESULT: {:x}\n", hr);
        return -1;
    }

    Img2SDF img2sdf (device, context);

    std::cout << std::format("Generating test seeds...") << std::endl;

    std::default_random_engine random_gen = std::default_random_engine {};
    auto distribution = std::uniform_int_distribution(0,1);

    std::pair<int64_t, std::vector<float>> arrays [num_dims] {};

    for (int32_t i = 0; i < num_dims; i++)
    {
        const auto width = dims[i];
        const auto height = dims[i];

        std::cout << std::format("Generating seed texture of size {}x{}...", width, height);

        arrays[i].first = width;
        auto& array = arrays[i].second;
        array.resize(width * height);

        for (int64_t j = 0; j < width * height; j++)
        {
            array[j] = distribution(random_gen);
        }

        std::cout << "Done." << std::endl;
    }

    std::ofstream out_csv {};
    out_csv.open("perf.csv");
    out_csv << "Test, ";
    for (const auto dim : dims)
    {
        out_csv << std::format("{},", dim);
    }
    out_csv << std::endl;

    std::cout << std::format("Running Coarse Profiles") << std::endl;

    out_csv << "voronoi diagram,";
    for (const auto& array : arrays)
    {
        auto tex_and_desc = JumpFloodResources::load_seeds_to_texture(device.Get(), array.second, array.first, array.first);

        std::cout << std::format("Running Profile for generating voronoi diagram of size {}x{}...", array.first, array.first);

        double out_time = 0;
        {
            ScopedProfile p(device, context, std::format("Voronoi diagram of size {}x{}", array.first, array.first), out_time);
            volatile auto output = img2sdf.compute_voronoi_transform(tex_and_desc.first);
        }
        std::cout << std::format("{} milliseconds", out_time) << std::endl;
        out_csv << std::format("{},", out_time);
    }
    out_csv << std::endl;

    std::cout << std::endl;

    out_csv << "unsigned distance field,";

    for (const auto& array : arrays)
    {
        auto tex_and_desc = JumpFloodResources::load_seeds_to_texture(device.Get(), array.second, array.first, array.first);

        std::cout << std::format("Running Profile for generating unsigned distance field of size {}x{}...", array.first, array.first);

        double out_time = 0.0;
        {
            ScopedProfile p(device, context, std::format("Unsigned Distance field of width {}", array.first), out_time);
            volatile auto output = img2sdf.compute_unsigned_distance_field(tex_and_desc.first);
        }
        std::cout << std::format("{} milliseconds", out_time) << std::endl;
        out_csv << std::format("{},", out_time);

    }
    out_csv << std::endl;

    std::cout << std::endl;

    out_csv << "signed distance field,";
    for (const auto& array :arrays)
    {
        auto tex_and_desc = JumpFloodResources::load_seeds_to_texture(device.Get(), array.second, array.first, array.first);
        std::cout << std::format("Running Profile for generating signed distance field of size {}x{}...", array.first, array.first);
        double out_time = 0.0;
        {
            ScopedProfile p(device, context, std::format("Signed Distance Field of width {}", array.first), out_time);
            volatile auto output = img2sdf.compute_signed_distance_field(tex_and_desc.first);
        }

        std::cout << std::format("{} milliseconds", out_time) << std::endl;
        out_csv << std::format("{},", out_time);
    }
    out_csv << std::endl;
    std::cout <<std::endl;

    //without normalisation

    out_csv << "voronoi diagram (unnormalised),";
    for (const auto& array : arrays)
    {
        auto tex_and_desc = JumpFloodResources::load_seeds_to_texture(device.Get(), array.second, array.first, array.first);

        std::cout << std::format("Running Profile for generating unnormalised voronoi diagram of size {}x{}...", array.first, array.first);

        double out_time = 0;
        {
            ScopedProfile p(device, context, std::format("Voronoi diagram of size {}x{}", array.first, array.first), out_time);
            volatile auto output = img2sdf.compute_voronoi_transform(tex_and_desc.first, false);
        }
        std::cout << std::format("{} milliseconds", out_time) << std::endl;
        out_csv << std::format("{},", out_time);
    }
    out_csv << std::endl;

    std::cout << std::endl;

    out_csv << "unsigned distance field (unnormalised),";

    for (const auto& array : arrays)
    {
        auto tex_and_desc = JumpFloodResources::load_seeds_to_texture(device.Get(), array.second, array.first, array.first);

        std::cout << std::format("Running Profile for generating unnormalised unsigned distance field of size {}x{}...", array.first, array.first);

        double out_time = 0.0;
        {
            ScopedProfile p(device, context, std::format("Unsigned Distance field of width {}", array.first), out_time);

            volatile auto output = img2sdf.compute_unsigned_distance_field(tex_and_desc.first, false);
        }
        std::cout << std::format("{} milliseconds", out_time) << std::endl;
        out_csv << std::format("{},", out_time);

    }
    out_csv << std::endl;

    std::cout << std::endl;

    out_csv << "signed distance field (unnormalised),";
    for (const auto& array :arrays)
    {
        auto tex_and_desc = JumpFloodResources::load_seeds_to_texture(device.Get(), array.second, array.first, array.first);
        std::cout << std::format("Running Profile for generating unnormalised signed distance field of size {}x{}...", array.first, array.first);
        double out_time = 0.0;
        {
            ScopedProfile p(device, context, std::format("Signed Distance Field of width {}", array.first), out_time);
            img2sdf.compute_signed_distance_field(tex_and_desc.first, false);
        }

        std::cout << std::format("{} milliseconds", out_time) << std::endl;
        out_csv << std::format("{},", out_time);
    }

    out_csv.close();

}