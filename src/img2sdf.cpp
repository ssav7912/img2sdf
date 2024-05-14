//
// Created by Soren on 6/05/2024.
//

#include "img2sdf.h"

#include "JumpFloodResources.h"
#include "jumpflooderror.h"
#include "JumpFloodDispatch.h"
#include "dxutils.h"
#include "dxinit.h"

//shaders
#include "shaders/jumpflood.hcs"
#include "shaders/preprocess.hcs"
#include "shaders/voronoi_normalise.hcs"
#include "shaders/distance.hcs"
#include "shaders/minmax_reduce.hcs"
#include "shaders/minmaxreduce_firstpass.hcs"
#include "shaders/normalise.hcs"
#include "shaders/invert.hcs"
#include "shaders/composite.hcs"


Img2SDF::Img2SDF(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> context, ComPtr<ID3D11Debug> debug_layer)
: device(std::move(device)), context(std::move(context)), debug_layer(std::move(debug_layer)) { }


Img2SDF::Img2SDF(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> context) : device(std::move(device)),
                                                                                     context(std::move(context)), debug_layer(nullptr) {

}

Microsoft::WRL::ComPtr<ID3D11Texture2D>
Img2SDF::compute_signed_distance_field(Microsoft::WRL::ComPtr<ID3D11Texture2D> input_texture, bool normalise)
{
    //TODO: make signed.
    auto outer_jfa_resources = JumpFloodResources(device.Get(), input_texture);
    auto inner_jfa_resources = JumpFloodResources(device.Get(), input_texture);

    const size_t Width = outer_jfa_resources.get_resolution().width;
    const size_t Height = outer_jfa_resources.get_resolution().height;

    ID3D11UnorderedAccessView* outer_voronoi_uav = outer_jfa_resources.create_voronoi_uav(false);
    ID3D11UnorderedAccessView* inner_voronoi_uav = inner_jfa_resources.create_voronoi_uav(false);

    ID3D11UnorderedAccessView* outer_distance_uav = outer_jfa_resources.create_distance_uav(false);
    ID3D11UnorderedAccessView* inner_distance_uav = inner_jfa_resources.create_distance_uav(false);

    ComPtr<ID3D11Texture2D> inner_distance_texture = nullptr;
    //inner scope to release temporary distance_resource after ownership is transferred.
    {
        ComPtr<ID3D11Resource> inner_distance_resource;
        inner_distance_uav->GetResource(inner_distance_resource.GetAddressOf());
        HRESULT out = inner_distance_resource.As<ID3D11Texture2D>(&inner_distance_texture);
        if (FAILED(out)) {
            throw jumpflood_error(out, "Returned Resource from Distance UAV was not a Texture2D\n");
            return nullptr;
        }
    }

    ID3D11Buffer* outer_const_buffer = outer_jfa_resources.create_const_buffer();
    ID3D11Buffer* inner_const_buffer = inner_jfa_resources.create_const_buffer();

    //dispatcher probably shouldn't also be compiling.
    JumpFloodDispatch outer_dispatch {this->device.Get(), this->context.Get(), &outer_jfa_resources};
    JumpFloodDispatch inner_dispatch {this->device.Get(), this->context.Get(), &inner_jfa_resources};

    outer_dispatch.dispatch_preprocess_shader();
    outer_dispatch.dispatch_voronoi_shader();

    outer_dispatch.dispatch_distance_transform_shader();

    inner_dispatch.dispatch_preprocess_shader(true);
    inner_dispatch.dispatch_voronoi_shader();
    inner_dispatch.dispatch_distance_transform_shader();


    inner_dispatch.dispatch_composite_shader(outer_distance_uav);

    if (normalise)
    {
        bool minmax_reduce_completed = inner_dispatch.dispatch_minmax_reduce_shader();

        ID3D11Texture2D* reduce_texture = inner_jfa_resources.get_texture(RESOURCE_TYPE::REDUCE_UAV);
        ID3D11Texture2D* reduce_staging = inner_jfa_resources.create_owned_staging_texture(reduce_texture);


        float minimum = 0;
        float maximum = 0;
        auto out_minmax = dxutils::copy_to_vector<float2>(this->context.Get(), reduce_staging, reduce_texture);
        if (!minmax_reduce_completed)
        {
            auto computed = dxutils::serial_min_max(out_minmax);
            minimum = computed.first;
            maximum = computed.second;
        }
        else {
            minimum = out_minmax[0].x;
            maximum = out_minmax[0].y;
        }

        inner_dispatch.dispatch_distance_normalise_shader(minimum, maximum, true);

    }



    return inner_jfa_resources.get_texture(RESOURCE_TYPE::DISTANCE_UAV);
}

ComPtr<ID3D11Texture2D>
Img2SDF::compute_unsigned_distance_field(ComPtr<ID3D11Texture2D> input_texture, bool normalise) {
    auto jfa_resources = JumpFloodResources(device.Get(), std::move(input_texture));

    const size_t Width = jfa_resources.get_resolution().width;
    const size_t Height = jfa_resources.get_resolution().height;

    ID3D11UnorderedAccessView* voronoi_uav = jfa_resources.create_voronoi_uav(true);

    ID3D11UnorderedAccessView* distance_uav = jfa_resources.create_distance_uav(true);

    ComPtr<ID3D11Texture2D> distance_texture = nullptr;
    //inner scope to release temporary distance_resource after ownership is transferred.
    {
        ComPtr<ID3D11Resource> distance_resource;
        distance_uav->GetResource(distance_resource.GetAddressOf());
        HRESULT out = distance_resource.As<ID3D11Texture2D>(&distance_texture);
        if (FAILED(out)) {
            throw jumpflood_error(out, "Returned Resource from Distance UAV was not a Texture2D\n");
            return nullptr;
        }
    }

    ID3D11Buffer* const_buffer = jfa_resources.create_const_buffer();

    //dispatcher probably shouldn't also be compiling.
    JumpFloodDispatch dispatch {this->device.Get(), this->context.Get(), &jfa_resources};

    dispatch.dispatch_preprocess_shader();
    dispatch.dispatch_voronoi_shader();

    dispatch.dispatch_distance_transform_shader();

#if DEBUG
    auto distance_transform_staging = dxutils::create_staging_texture(this->device.Get(), distance_texture.Get());

    auto distance_transform_data = dxutils::copy_to_vector<float>(this->context.Get(), distance_transform_staging.Get(),
                                                                  distance_texture.Get());

#endif

    if (normalise)
    {
        bool minmax_reduce_completed = dispatch.dispatch_minmax_reduce_shader();

        ID3D11Texture2D* reduce_texture = jfa_resources.get_texture(RESOURCE_TYPE::REDUCE_UAV);
        ID3D11Texture2D* reduce_staging = jfa_resources.create_owned_staging_texture(reduce_texture);


        float minimum = 0;
        float maximum = 0;
        auto out_minmax = dxutils::copy_to_vector<float2>(this->context.Get(), reduce_staging, reduce_texture);
        if (!minmax_reduce_completed)
        {
            auto computed = dxutils::serial_min_max(out_minmax);
            minimum = computed.first;
            maximum = computed.second;
        }
        else {
            minimum = out_minmax[0].x;
            maximum = out_minmax[0].y;
        }

        dispatch.dispatch_distance_normalise_shader(minimum, maximum, false);

    }



    return jfa_resources.get_texture(RESOURCE_TYPE::DISTANCE_UAV);
}


ComPtr<ID3D11Texture2D> Img2SDF::compute_voronoi_transform(ComPtr<ID3D11Texture2D> input_texture, bool normalise) {
   auto jfa_resources = JumpFloodResources(device.Get(), input_texture);

    ID3D11UnorderedAccessView* voronoi_uav = jfa_resources.create_voronoi_uav(true);


    ID3D11Buffer* const_buffer = jfa_resources.create_const_buffer();


    //dispatcher probably shouldn't also be compiling.
    JumpFloodDispatch dispatch {this->device.Get(), this->context.Get(), &jfa_resources};

    dispatch.dispatch_preprocess_shader();
    dispatch.dispatch_voronoi_shader();

    if (normalise)
    {
        dispatch.dispatch_voronoi_normalise_shader();
    }

    return jfa_resources.get_texture(RESOURCE_TYPE::VORONOI_UAV);

}

