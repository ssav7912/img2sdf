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

static constexpr int32_t NUM_RUNS = 1000;

static constexpr bool coarse_profiles = true;

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

    if (coarse_profiles) {


        std::ofstream out_csv{};
        out_csv.open("perf.csv");
        out_csv << "Test, ";
        for (const auto dim: dims) {
            out_csv << std::format("{},", dim);
        }
        out_csv << std::endl;

        std::cout << std::format("Running Coarse Profiles") << std::endl;

        out_csv << "voronoi diagram,";
        for (const auto &array: arrays) {
            double min_time = std::numeric_limits<double>::infinity();
            double max_time = -min_time;
            double out_time = 0;
            std::cout << std::format("Running Profile for generating voronoi diagram of size {}x{}...", array.first,
                                     array.first);
            for (int32_t i = 0; i < NUM_RUNS; i++) {


                auto tex_and_desc = JumpFloodResources::load_seeds_to_texture(device.Get(), array.second, array.first,
                                                                              array.first);


                double time = 0;
                {
                    ScopedProfile p(device, context,
                                    std::format("Voronoi diagram of size {}x{}", array.first, array.first),
                                    time);
                    volatile auto output = img2sdf.compute_voronoi_transform(tex_and_desc.first);
                }
                out_time += time;
                min_time = std::min(min_time, time);
                max_time = std::max(max_time, time);

            }
            out_time = out_time / static_cast<double>(NUM_RUNS);
            std::cout << std::format("min: {} max: {} average: {} milliseconds", min_time, max_time, out_time)
                      << std::endl;
            out_csv << std::format("{},", out_time);
        }
        out_csv << std::endl;

        std::cout << std::endl;

        out_csv << "unsigned distance field,";

        for (const auto &array: arrays) {
            double min_time = std::numeric_limits<double>::infinity();
            double max_time = -min_time;
            double out_time = 0.0;
            std::cout << std::format("Running Profile for generating unsigned distance field of size {}x{}...",
                                     array.first, array.first);
            for (int32_t i = 0; i < NUM_RUNS; i++) {


                auto tex_and_desc = JumpFloodResources::load_seeds_to_texture(device.Get(), array.second, array.first,
                                                                              array.first);


                double time = 0.0;
                {
                    ScopedProfile p(device, context, std::format("Unsigned Distance field of width {}", array.first),
                                    time);
                    volatile auto output = img2sdf.compute_unsigned_distance_field(tex_and_desc.first);
                }
                out_time += time;
                min_time = std::min(min_time, time);
                max_time = std::max(max_time, time);
            }
            out_time = out_time / static_cast<double>(NUM_RUNS);
            std::cout << std::format("min: {} max: {} average: {} milliseconds", min_time, max_time, out_time)
                      << std::endl;
            out_csv << std::format("{},", out_time);

        }
        out_csv << std::endl;

        std::cout << std::endl;

        out_csv << "signed distance field,";
        for (const auto &array: arrays) {
            double min_time = std::numeric_limits<double>::infinity();
            double max_time = -min_time;
            double out_time = 0.0;
            std::cout
                    << std::format("Running Profile for generating signed distance field of size {}x{}...", array.first,
                                   array.first);
            for (int32_t i = 0; i < NUM_RUNS; i++) {
                auto tex_and_desc = JumpFloodResources::load_seeds_to_texture(device.Get(), array.second, array.first,
                                                                              array.first);

                double time = 0.0;
                {
                    ScopedProfile p(device, context, std::format("Signed Distance Field of width {}", array.first),
                                    time);
                    volatile auto output = img2sdf.compute_signed_distance_field(tex_and_desc.first);
                }
                out_time += time;
                min_time = std::min(min_time, time);
                max_time = std::max(max_time, time);
            }
            out_time = out_time / static_cast<double>(NUM_RUNS);
            std::cout << std::format("min: {} max: {} average: {} milliseconds", min_time, max_time, out_time)
                      << std::endl;
            out_csv << std::format("{},", out_time);
        }
        out_csv << std::endl;
        std::cout << std::endl;

        //without normalisation

        out_csv << "voronoi diagram (unnormalised),";
        for (const auto &array: arrays) {
            double out_time = 0;
            double min_time = std::numeric_limits<double>::infinity();
            double max_time = -min_time;
            std::cout << std::format("Running Profile for generating unnormalised voronoi diagram of size {}x{}...",
                                     array.first, array.first);
            for (int32_t i = 0; i < NUM_RUNS; i++) {


                auto tex_and_desc = JumpFloodResources::load_seeds_to_texture(device.Get(), array.second, array.first,
                                                                              array.first);

                double time = 0.0;
                {
                    ScopedProfile p(device, context,
                                    std::format("Voronoi diagram of size {}x{}", array.first, array.first),
                                    time);
                    volatile auto output = img2sdf.compute_voronoi_transform(tex_and_desc.first, false);
                }
                out_time += time;
                min_time = std::min(min_time, time);
                max_time = std::max(max_time, time);
            }
            out_time = out_time / static_cast<double>(NUM_RUNS);
            std::cout << std::format("min: {} max: {} average: {} milliseconds", min_time, max_time, out_time)
                      << std::endl;
            out_csv << std::format("{},", out_time);
        }

        out_csv << std::endl;

        std::cout << std::endl;

        out_csv << "unsigned distance field (unnormalised),";

        for (const auto &array: arrays) {
            std::cout << std::format(
                    "Running Profile for generating unnormalised unsigned distance field of size {}x{}...", array.first,
                    array.first);

            double out_time = 0.0;
            double min_time = std::numeric_limits<double>::infinity();
            double max_time = -min_time;
            for (int32_t i = 0; i < NUM_RUNS; i++) {
                auto tex_and_desc = JumpFloodResources::load_seeds_to_texture(device.Get(), array.second, array.first,
                                                                              array.first);
                double time = 0.0;
                {
                    ScopedProfile p(device, context, std::format("Unsigned Distance field of width {}", array.first),
                                    time);

                    volatile auto output = img2sdf.compute_unsigned_distance_field(tex_and_desc.first, false);
                }
                out_time += time;
                min_time = std::min(min_time, time);
                max_time = std::max(max_time, time);
            }
            out_time = out_time / static_cast<double>(NUM_RUNS);
            std::cout << std::format("min: {} max: {} average: {} milliseconds", min_time, max_time, out_time)
                      << std::endl;
            out_csv << std::format("{},", out_time);

        }
        out_csv << std::endl;

        std::cout << std::endl;

        out_csv << "signed distance field (unnormalised),";
        for (const auto &array: arrays) {
            std::cout
                    << std::format("Running Profile for generating unnormalised signed distance field of size {}x{}...",
                                   array.first, array.first);
            double out_time = 0.0;
            double min_time = std::numeric_limits<double>::infinity();
            double max_time = -min_time;
            for (int32_t i = 0; i < NUM_RUNS; i++) {

                auto tex_and_desc = JumpFloodResources::load_seeds_to_texture(device.Get(), array.second, array.first,
                                                                              array.first);
                double time = 0.0;
                {
                    ScopedProfile p(device, context, std::format("Signed Distance Field of width {}", array.first),
                                    time);
                    img2sdf.compute_signed_distance_field(tex_and_desc.first, false);
                }
                out_time += time;
                min_time = std::min(min_time, time);
                max_time = std::max(max_time, time);
            }

            out_time = out_time / static_cast<double>(NUM_RUNS);
            std::cout << std::format("min: {} max: {} average: {} milliseconds", min_time, max_time, out_time)
                      << std::endl;
            out_csv << std::format("{},", out_time);
        }
        std::cout << std::endl;
        out_csv.close();
    }

    const auto input_2k = arrays[num_dims - 3];

    std::cout << std::format("Running Fine Profiling on {} image", input_2k.first) << std::endl;


    double preprocess_time = 0.0;
    double preprocess_min = std::numeric_limits<double>::infinity();
    double preprocess_max = -preprocess_min;

    double voronoi_time = 0.0;
    double voronoi_min = preprocess_min;
    double voronoi_max = preprocess_max;

    double distance_transform_time = 0.0;
    double distance_min = preprocess_min;
    double distance_max = preprocess_max;

    double minmaxreduce_time = 0.0;
    double minmax_min = preprocess_min;
    double minmax_max = preprocess_max;

    double distance_normalise_time = 0.0;
    double distnorm_min = preprocess_min;
    double distnorm_max = preprocess_max;

    double composite_time =0.0;
    double composite_min = preprocess_min;
    double composite_max = preprocess_max;

    for (int32_t i = 0; i < NUM_RUNS; i++)
    {
        std::cout << std::format("{:4}/{:4}", i, NUM_RUNS) << "\r";

        JumpFloodResources resources {device.Get(), input_2k.second, static_cast<int32_t>(input_2k.first), static_cast<int32_t>(input_2k.first)};
        JumpFloodDispatch dispatch {device.Get(), context.Get(), &resources};
        double cumulative = 0.0;
        {
            ScopedProfile p(device, context, "preprocess profile", cumulative);
            dispatch.dispatch_preprocess_shader();
        }
        volatile auto preprocessed = resources.get_texture(RESOURCE_TYPE::VORONOI_UAV);
        preprocess_time += cumulative;
        preprocess_min = std::min(preprocess_min, cumulative);
        preprocess_max = std::max(preprocess_max, cumulative);

        cumulative = 0.0;
        {
            ScopedProfile p(device, context, "voronoi profile", cumulative);
            dispatch.dispatch_voronoi_shader();
        }
        volatile auto voronoi = resources.get_texture(RESOURCE_TYPE::VORONOI_UAV);
        voronoi_time += cumulative;
        voronoi_min = std::min(voronoi_min, cumulative);
        voronoi_max = std::max(voronoi_max, cumulative);

        cumulative = 0.0;
        {
            ScopedProfile p(device, context, "distance transform", cumulative);
            dispatch.dispatch_distance_transform_shader();
        }
        volatile auto distance = resources.get_texture(RESOURCE_TYPE::DISTANCE_UAV);
        distance_transform_time += cumulative;
        distance_min = std::min(distance_min, cumulative);
        distance_max = std::max(distance_max, cumulative);

        cumulative = 0.0;
        {
            ScopedProfile p(device, context, "minmax reduce", cumulative);
            auto _ = dispatch.dispatch_minmax_reduce_shader();
        }
        volatile auto reduce = resources.get_texture(RESOURCE_TYPE::REDUCE_UAV);
        minmaxreduce_time += cumulative;
        minmax_min = std::min(minmax_min, cumulative);
        minmax_max = std::max(minmax_max, cumulative);

        cumulative = 0.0;
        {
            ScopedProfile p(device, context, "distance normalise", cumulative);
            dispatch.dispatch_distance_normalise_shader(0, 5, true); //doesn't matter!
        }
        volatile auto normalise = resources.get_texture(RESOURCE_TYPE::DISTANCE_UAV);

        distance_normalise_time += cumulative;
        distnorm_min = std::min(distnorm_min, cumulative);
        distnorm_max = std::max(distnorm_max, cumulative);


        volatile auto UAV = resources.create_distance_uav(false);
        cumulative = 0.0;
        {
            ScopedProfile p(device, context, "composite", cumulative);
            dispatch.dispatch_composite_shader(UAV);
        }
        volatile auto composite = resources.get_texture(RESOURCE_TYPE::DISTANCE_UAV);

        composite_time += cumulative;
        composite_min = std::min(composite_min, cumulative);
        composite_max = std::max(composite_max, cumulative);


    }
    std::cout << std::endl;
    preprocess_time /= static_cast<double>(NUM_RUNS);
    voronoi_time /= static_cast<double>(NUM_RUNS);
    distance_transform_time /= static_cast<double>(NUM_RUNS);
    minmaxreduce_time /= static_cast<double>(NUM_RUNS);
    distance_normalise_time /= static_cast<double>(NUM_RUNS);
    composite_time /= static_cast<double>(NUM_RUNS);

    std::cout << std::format("Preprocess Shader: min: {} max: {} average: {}", preprocess_min, preprocess_max, preprocess_time) << std::endl;
    std::cout << std::format("Voronoi Shader: min: {} max: {} average: {}", voronoi_min, voronoi_max, voronoi_time) << std::endl;
    std::cout << std::format("Distance Shader: min: {} max: {} average: {}", distance_min, distance_max, distance_transform_time) << std::endl;
    std::cout << std::format("MinMaxReduce Shader: min: {} max: {} average: {}", minmax_min, minmax_max, minmaxreduce_time) << std::endl;
    std::cout << std::format("Distance Normalise Shader: min: {} max: {} average: {}", distnorm_min, distnorm_max, distance_normalise_time) << std::endl;
    std::cout << std::format("Composite Shader: min: {} max: {} average: {}", composite_min, composite_max, composite_time) << std::endl;


}