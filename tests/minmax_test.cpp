//
// Created by Soren on 26/04/2024.
//
#include <random>

#include "../src/dxutils.h"
#include "../src/dxinit.h"
#include "../src/JumpFloodDispatch.h"
#include "../src/JumpFloodResources.h"

#include "../src/shaders/minmax_reduce.hcs"
#include "../src/shaders/minmaxreduce_firstpass.hcs"

#include "gtest/gtest.h"

namespace {
    using namespace Microsoft::WRL;

    class DXSetup : public testing::Test {
    protected:
    void SetUp() override
    {
        Windows::Foundation::Initialize(RO_INIT_MULTITHREADED);
        ASSERT_HRESULT_SUCCEEDED(dxinit::create_compute_device(device.GetAddressOf(),
                                      context.GetAddressOf(), false));
        debug_layer = dxinit::debug_layer;


        std::default_random_engine random_gen = std::default_random_engine {0};
        auto distribution = std::uniform_real_distribution<float> (std::numeric_limits<float>::min(), std::numeric_limits<float>::max());
        std::vector<float> debug (width * height);
        for (int i = 0; i < width * height; i++)
        {
            debug[i] = distribution(random_gen);
        }
        debug_image = debug;
         jfa_resources = std::make_optional(JumpFloodResources(device.Get(), debug, width, height));

        jump_flood_shaders shaders {
            .min_max_reduce_firstpass = g_reduce_firstpass,
            .min_max_reduce_firstpass_size = sizeof(g_reduce_firstpass),

            .min_max_reduce = g_reduce,
            .min_max_reduce_size = sizeof(g_reduce),
        };

        dispatcher = std::make_optional<JumpFloodDispatch>(device.Get(), context.Get(), shaders, &jfa_resources.value());


    }


    std::vector<float> debug_image;
    static constexpr int32_t width = 8;
    static constexpr int32_t height = 8;

    std::optional<JumpFloodResources> jfa_resources;
    std::optional<JumpFloodDispatch> dispatcher;

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<ID3D11Debug> debug_layer;
};

    TEST_F(DXSetup, MinMaxGroundTruth)
{

    auto srv = jfa_resources.value().get_input_srv();

    ComPtr<ID3D11Resource> srv_resource;
    srv->GetResource(&srv_resource);

    ComPtr<ID3D11Texture2D> srv_texture;
    ASSERT_HRESULT_SUCCEEDED(srv_resource.As<ID3D11Texture2D>(&srv_texture));

    D3D11_TEXTURE2D_DESC srv_desc;
    srv_texture->GetDesc(&srv_desc);
    ASSERT_EQ(srv_desc.Width, 8);
    ASSERT_EQ(srv_desc.Height, 8);
    ASSERT_EQ(srv_desc.Format, DXGI_FORMAT_R32_FLOAT);



    debug_layer->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);

    //compute minmax via parallel reduction.
    dispatcher->dispatch_minmax_reduce_shader(srv);

    auto minmax_texture = jfa_resources->get_texture(RESOURCE_TYPE::REDUCE_UAV);

    auto staging = jfa_resources.value().create_staging_texture(minmax_texture);

    D3D11_TEXTURE2D_DESC minmax_desc;
    minmax_texture->GetDesc(&minmax_desc);

    ASSERT_EQ(minmax_desc.Width, 1);
    ASSERT_EQ(minmax_desc.Height, 1);
    ASSERT_EQ(minmax_desc.Format, DXGI_FORMAT_R32G32_FLOAT);

    D3D11_TEXTURE2D_DESC staging_desc;
    staging->GetDesc(&staging_desc);

    ASSERT_EQ(staging_desc.Width, 1);
    ASSERT_EQ(staging_desc.Height, 1);
    ASSERT_EQ(staging_desc.Format, DXGI_FORMAT_R32G32_FLOAT);

    auto minmax_data = dxutils::copy_to_staging<float2>(context.Get(), staging, minmax_texture);

    std::pair<float, float> computed_minmax = {minmax_data[0].x, minmax_data[0].y };

    //compute ground truth reference.
    auto ground_truth = dxutils::serial_min_max(debug_image);

    ASSERT_FLOAT_EQ(minmax_data[0].x, ground_truth.first);
    ASSERT_FLOAT_EQ(minmax_data[0].y, ground_truth.second);

}
}