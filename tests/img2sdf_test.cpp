//
// Created by Soren on 7/05/2024.
//
#include "../src/img2sdf.h"
#include "../src/WICTextureLoader.h"
#include <gtest/gtest.h>
#include <filesystem>

namespace {
    using namespace Microsoft::WRL;

    TEST(integration_tests, compute_unsigned_distance_field)
    {
        Windows::Foundation::Initialize(RO_INIT_MULTITHREADED);
        ComPtr<ID3D11Device> device;
        ComPtr<ID3D11DeviceContext> context;
        ASSERT_HRESULT_SUCCEEDED(dxinit::create_compute_device(device.GetAddressOf(), context.GetAddressOf(), false));

        Img2SDF sdf(device, context, dxinit::debug_layer);

        ComPtr<ID3D11Resource> in_resource = nullptr;
        ComPtr<ID3D11ShaderResourceView> _ = nullptr;

        auto absolute_texture_path = std::filesystem::absolute({L"resources/test.png"});

        ASSERT_HRESULT_SUCCEEDED(CreateWICR32FTextureFromFile(device.Get(), absolute_texture_path.wstring().c_str(),
                                                              in_resource.GetAddressOf(), _.GetAddressOf()));

        ComPtr<ID3D11Texture2D> in_texture;
        ASSERT_HRESULT_SUCCEEDED(in_resource.As(&in_texture));

        EXPECT_NO_THROW(auto texture = sdf.compute_unsigned_distance_field(in_texture, true));
    }
}