//
// Created by Soren on 26/04/2024.
//
#include <random>
#include <filesystem>

#include "../src/dxutils.h"
#include "../src/dxinit.h"
#include "../src/JumpFloodDispatch.h"
#include "../src/JumpFloodResources.h"

#include "../src/shaders/minmax_reduce.hcs"
#include "../src/shaders/minmaxreduce_firstpass.hcs"
#include "../src/img2sdf.h"
#include "../src/WICTextureLoader.h"

#include "gtest/gtest.h"

namespace {
    using namespace Microsoft::WRL;



class DXSetup : public testing::TestWithParam<int32_t> {
    protected:
    void SetUp() override
    {
        Windows::Foundation::Initialize(RO_INIT_MULTITHREADED);
        ASSERT_HRESULT_SUCCEEDED(dxinit::create_compute_device(device.GetAddressOf(),
                                      context.GetAddressOf(), false));
        debug_layer = dxinit::debug_layer;

        width = GetParam();
        height = GetParam();


        //want same distribution every time.
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
    int32_t width = 8;
    int32_t height = 8;

    std::optional<JumpFloodResources> jfa_resources;
    std::optional<JumpFloodDispatch> dispatcher;

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<ID3D11Debug> debug_layer;
};

    TEST_P(DXSetup, MinMaxGroundTruth)
{

    auto srv = jfa_resources.value().get_input_srv();

    ComPtr<ID3D11Resource> srv_resource;
    srv->GetResource(&srv_resource);

    ComPtr<ID3D11Texture2D> srv_texture;
    ASSERT_HRESULT_SUCCEEDED(srv_resource.As<ID3D11Texture2D>(&srv_texture));

    D3D11_TEXTURE2D_DESC srv_desc;
    srv_texture->GetDesc(&srv_desc);
    ASSERT_EQ(srv_desc.Width, width);
    ASSERT_EQ(srv_desc.Height, height);
    ASSERT_EQ(srv_desc.Format, DXGI_FORMAT_R32_FLOAT);




    //compute minmax via parallel reduction.
    bool complete = dispatcher->dispatch_minmax_reduce_shader(srv);

    auto minmax_texture = jfa_resources->get_texture(RESOURCE_TYPE::REDUCE_UAV);

    auto staging = jfa_resources.value().create_owned_staging_texture(minmax_texture);

    D3D11_TEXTURE2D_DESC minmax_desc;
    minmax_texture->GetDesc(&minmax_desc);

    ASSERT_EQ(minmax_desc.Width, width/JumpFloodDispatch::threads_per_group_width); //num thread groups
    ASSERT_EQ(minmax_desc.Height, height/JumpFloodDispatch::threads_per_group_width); //num thread groups
    ASSERT_EQ(minmax_desc.Format, DXGI_FORMAT_R32G32_FLOAT);

    D3D11_TEXTURE2D_DESC staging_desc;
    staging->GetDesc(&staging_desc);

    ASSERT_EQ(staging_desc.Width, width/JumpFloodDispatch::threads_per_group_width);
    ASSERT_EQ(staging_desc.Height, height/JumpFloodDispatch::threads_per_group_width);
    ASSERT_EQ(staging_desc.Format, DXGI_FORMAT_R32G32_FLOAT);

    auto minmax_data = dxutils::copy_to_vector<float2>(context.Get(), staging, minmax_texture);

    std::pair<float, float> computed_minmax = {minmax_data[0].x, minmax_data[0].y };
    if (!complete)
    {
        //this path should only run when the UAV is not large enough to loop on in a single warp.
        //(that is, we would get OOB access with a width smaller than threads_per_group_width).
        ASSERT_LT(staging_desc.Width, JumpFloodDispatch::threads_per_group_width);
        ASSERT_LT(staging_desc.Height, JumpFloodDispatch::threads_per_group_width);
        auto CPU_iter = dxutils::serial_min_max(minmax_data);
        computed_minmax = CPU_iter;
    }


    //compute ground truth reference.
    auto ground_truth = dxutils::serial_min_max(debug_image);

    ASSERT_FLOAT_EQ(computed_minmax.first, ground_truth.first);
    ASSERT_FLOAT_EQ(computed_minmax.second, ground_truth.second);

}

///Kind of a dummy test!
///Doesn't assert anything serious, Just used for debugging behaviour with the shader by exposing a
///larger UAV to write to rather than the num_groups_x * num_groups_y one.
TEST_P(DXSetup, ReduceCustomUAV)
{
    auto srv = jfa_resources.value().get_input_srv();

    //lying to the generate interface for our debug UAV.
    auto uav = jfa_resources->create_reduction_uav(GetParam(),GetParam());

    auto shader = dispatcher->get_shader(SHADERS::MINMAXREDUCE_FIRST);

    ASSERT_TRUE(shader);
    constexpr uint32_t num_groups_dim = 1;
    dxinit::run_compute_shader(context.Get(), shader, 1, &srv, nullptr, nullptr,
                               0, &uav, 1, num_groups_dim,num_groups_dim,num_groups_dim);

    auto minmax_texture = jfa_resources->get_texture(RESOURCE_TYPE::REDUCE_UAV);

    auto staging = jfa_resources.value().create_owned_staging_texture(minmax_texture);

    auto minmax_data = dxutils::copy_to_vector<float2>(context.Get(), staging, minmax_texture);


}

    TEST(integration_tests, compute_unsigned_distance_field)
    {
        Windows::Foundation::Initialize(RO_INIT_MULTITHREADED);
        ComPtr<ID3D11Device> device;
        ComPtr<ID3D11DeviceContext> context;
        ASSERT_HRESULT_SUCCEEDED(dxinit::create_compute_device(device.GetAddressOf(), context.GetAddressOf(), false));

        Img2SDF sdf(device, context, dxinit::debug_layer);

        ComPtr<ID3D11Resource> in_resource = nullptr;
        ComPtr<ID3D11ShaderResourceView> _ = nullptr;

        auto absolute_texture_path = std::filesystem::absolute({"../tests/resources/test.png"});

        ASSERT_HRESULT_SUCCEEDED(CreateWICR32FTextureFromFile(device.Get(), absolute_texture_path.wstring().c_str(),
                                                              in_resource.GetAddressOf(), _.GetAddressOf()));

        ComPtr<ID3D11Texture2D> in_texture;
        ASSERT_HRESULT_SUCCEEDED(in_resource.As(&in_texture));

        ComPtr<ID3D11Texture2D> texture;
        EXPECT_NO_THROW(texture = sdf.compute_unsigned_distance_field(in_texture, true));

        auto staging = dxutils::create_staging_texture(device.Get(), texture.Get());

        auto input_staging = dxutils::create_staging_texture(device.Get(), in_texture.Get());

        auto data = dxutils::copy_to_vector<float>(context.Get(), staging.Get(), texture.Get());

        auto input_data = dxutils::copy_to_vector<float>(context.Get(), input_staging.Get(), in_texture.Get());

        ASSERT_EQ(data.size(), input_data.size());

        for (int i = 0; i < data.size(); ++i) {
            EXPECT_NE(data[i], input_data[i]) << "Vectors x and y differ at index " << i;
        }
    }

INSTANTIATE_TEST_SUITE_P(MinMaxTests, DXSetup, ::testing::Values(8, 16, 32, 64,128,256,512,1024,2048,4096,8192) );
}