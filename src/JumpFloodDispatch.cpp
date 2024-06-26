//
// Created by Soren on 1/04/2024.
//

#include "JumpFloodDispatch.h"
#include "JumpFloodResources.h"
#include "dxinit.h"
#include <stdexcept>
#include <cassert>


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

const jump_flood_shaders JumpFloodDispatch::JUMPFLOOD_SHADERS = {
.preprocess = g_preprocess,
.preprocess_size = sizeof(g_preprocess),

.preprocess_invert = g_invert,
.preprocess_invert_size = sizeof(g_invert),

.voronoi = g_main,
.voronoi_size = sizeof(g_main),

.voronoi_normalise = g_voronoi_normalise,
.voronoi_normalise_size = sizeof(g_voronoi_normalise),

.distance_transform = g_distance,
.distance_transform_size = sizeof(g_distance),

.min_max_reduce_firstpass = g_reduce_firstpass,
.min_max_reduce_firstpass_size = sizeof(g_reduce_firstpass),

.min_max_reduce = g_reduce,
.min_max_reduce_size = sizeof(g_reduce),

.distance_normalise = g_normalise,
.distance_normalise_size = sizeof(g_normalise),

.composite = g_composite,
.composite_size = sizeof(g_composite),

};

JumpFloodDispatch::JumpFloodDispatch(ID3D11Device *device, ID3D11DeviceContext *context,
                                     JumpFloodResources* resources, jump_flood_shaders byte_code) :
device(device), context(context), resources(resources) {

    if (device == nullptr || context == nullptr)
    {
        throw std::runtime_error("Device or Context cannot be null.");
    }

    if (resources == nullptr)
    {
        throw std::runtime_error("Resources cannot be null.");
    }
    HRESULT hr = ERROR_SUCCESS;
    if (byte_code.preprocess != nullptr) {
        hr = device->CreateComputeShader(byte_code.preprocess, byte_code.preprocess_size, nullptr,
                                         preprocess_shader.GetAddressOf());

        if (FAILED(hr)) {
            throw jumpflood_error(hr, "Could not create preprocess shader. ");
        }
    }

    if (byte_code.preprocess_invert != nullptr) {
        hr = device->CreateComputeShader(byte_code.preprocess_invert, byte_code.preprocess_invert_size, nullptr,
                                         preprocess_invert_shader.GetAddressOf());

        if (FAILED(hr))
        {
            throw jumpflood_error(hr, "Could not create Preprocess inversion shader.");
        }
    }

    if (byte_code.voronoi != nullptr) {


        hr = device->CreateComputeShader(byte_code.voronoi, byte_code.voronoi_size, nullptr,
                                         voronoi_shader.GetAddressOf());
        if (FAILED(hr)) {
            throw jumpflood_error(hr, "Could not create voronoi shader. ");
        }
    }

    if (byte_code.voronoi_normalise != nullptr) {
        hr = device->CreateComputeShader(byte_code.voronoi_normalise, byte_code.voronoi_normalise_size, nullptr,
                                         voronoi_normalise_shader.GetAddressOf());

        if (FAILED(hr)) {
            throw jumpflood_error(hr, "Could not create voronoi normalisation shader. ");
        }
    }

    if (byte_code.distance_transform != nullptr) {
        hr = device->CreateComputeShader(byte_code.distance_transform, byte_code.distance_transform_size, nullptr,
                                         distance_transform_shader.GetAddressOf());

        if (FAILED(hr)) {
            throw jumpflood_error(hr, "Could not create distance transform shader. ");
        }
    }
    if (byte_code.min_max_reduce_firstpass != nullptr)
    {
        hr = device->CreateComputeShader(byte_code.min_max_reduce_firstpass, byte_code.min_max_reduce_firstpass_size, nullptr,
                                         min_max_reduce_firstpass_shader.GetAddressOf());
        if (FAILED(hr))
        {
            throw jumpflood_error(hr, "Could not create min max reduction first pass shader. ");
        }
    }

    if (byte_code.min_max_reduce != nullptr) {


        hr = device->CreateComputeShader(byte_code.min_max_reduce, byte_code.min_max_reduce_size, nullptr,
                                         min_max_reduce_shader.GetAddressOf());
        if (FAILED(hr)) {
            throw jumpflood_error(hr, "Could not create min max reduction shader. ");
        }
    }

    if (byte_code.distance_normalise != nullptr) {
        hr = device->CreateComputeShader(byte_code.distance_normalise, byte_code.distance_normalise_size, nullptr,
                                         distance_normalise_shader.GetAddressOf());

        if (FAILED(hr)) {
            throw jumpflood_error(hr, "Could not create distance transform shader.");
        }
    }

    if (byte_code.composite != nullptr)
    {
        hr = device->CreateComputeShader(byte_code.composite, byte_code.composite_size, nullptr,
                                         composite_shader.GetAddressOf());
        if (FAILED(hr))
        {
            throw jumpflood_error(hr, "Coud not create composite shader.");
        }
    }

}



ID3D11ComputeShader *JumpFloodDispatch::get_shader(SHADERS shader) const {
    switch (shader)
    {
        case SHADERS::PREPROCESS: {return this->preprocess_shader.Get();}
        case SHADERS::PREPROCESS_INVERT: {return this->preprocess_invert_shader.Get();}
        case SHADERS::VORONOI: {return this->voronoi_shader.Get();}
        case SHADERS::VORONOI_NORMALISE: {return this->voronoi_normalise_shader.Get();}
        case SHADERS::DISTANCE: {return this->distance_transform_shader.Get();}
        case SHADERS::DISTANCE_NORMALISE: {return this->distance_normalise_shader.Get();}
        case SHADERS::MINMAXREDUCE: {return this->min_max_reduce_shader.Get();}
        case SHADERS::MINMAXREDUCE_FIRST: { return this->min_max_reduce_firstpass_shader.Get(); }
        default: return nullptr;
    }
}

void
JumpFloodDispatch::dispatch_preprocess_shader(bool invert) {
    const uint32_t num_groups_x = resources->get_resolution().width / threads_per_group_width;
    const uint32_t num_groups_y = resources->get_resolution().height / threads_per_group_width;

    auto srv = resources->get_input_srv();
    auto cbuffer = resources->create_const_buffer(false);
    auto uav = resources->create_voronoi_uav(false);

    auto& shader = invert ? preprocess_invert_shader : preprocess_shader;


    assert(shader);
    dxinit::run_compute_shader(context, shader.Get(), 1, &srv,
                       cbuffer, nullptr, 0, &uav, 1, num_groups_x, num_groups_y, 1);
}

void JumpFloodDispatch::dispatch_voronoi_shader() {

    const int32_t num_steps = resources->num_steps();
    const uint32_t num_groups_x = resources->get_resolution().width / threads_per_group_width;
    const uint32_t num_groups_y = resources->get_resolution().height / threads_per_group_width;


    auto cbuffer = resources->create_const_buffer(false);
    auto voronoi_uav = resources->create_voronoi_uav(false);

    for (int32_t i = num_steps; i > 0; i--)
    {
        auto local_buf = resources->get_local_cbuffer();
        local_buf.Iteration = i;
        resources->update_const_buffer(context, local_buf);

        assert(this->voronoi_shader);
        dxinit::run_compute_shader(context, voronoi_shader.Get(), 0,nullptr,
                                   cbuffer, nullptr, 0, &voronoi_uav, 1, num_groups_x, num_groups_y,1);

    }


}

void JumpFloodDispatch::dispatch_voronoi_normalise_shader() {
    const size_t num_steps = resources->num_steps();
    const uint32_t num_groups_x = resources->get_resolution().width / threads_per_group_width;
    const uint32_t num_groups_y = resources->get_resolution().height / threads_per_group_width;

    auto cbuffer = resources->create_const_buffer(false);
    auto voronoi_uav = resources->create_voronoi_uav(false);

    assert(this->voronoi_normalise_shader);
    dxinit::run_compute_shader(context, voronoi_normalise_shader.Get(), 0, nullptr,
                               cbuffer, nullptr, 0, &voronoi_uav, 1, num_groups_x, num_groups_y, 1);

}

void JumpFloodDispatch::dispatch_distance_transform_shader() {

    const uint32_t num_groups_x = resources->get_resolution().width / threads_per_group_width;
    const uint32_t num_groups_y = resources->get_resolution().height / threads_per_group_width;

    auto cbuffer = resources->create_const_buffer(false);
    auto voronoi_uav = resources->create_voronoi_uav(false);
    auto distance_uav = resources->create_distance_uav(false);

    ID3D11UnorderedAccessView* UAVs[] = {voronoi_uav, distance_uav};

    assert(this->distance_transform_shader);
    dxinit::run_compute_shader(context, distance_transform_shader.Get(), 0, nullptr,
                               cbuffer, nullptr, 0, UAVs, 2, num_groups_x, num_groups_y, 1);

}

bool JumpFloodDispatch::dispatch_minmax_reduce_shader(ID3D11ShaderResourceView* explicit_srv) {
    uint32_t num_groups_x = 0;
    uint32_t num_groups_y = 0;
    if (explicit_srv)
    {
        ComPtr<ID3D11Resource> srv_resource;
        explicit_srv->GetResource(srv_resource.GetAddressOf());

        ComPtr<ID3D11Texture2D> srv_texture;

        HRESULT resource_result = srv_resource.As<ID3D11Texture2D>(&srv_texture);
        if (FAILED(resource_result))
        {
            throw jumpflood_error(resource_result, "srv_resource was not an ID3D11Texture2D. This should never happen!");
        }

        D3D11_TEXTURE2D_DESC desc;
        srv_texture->GetDesc(&desc);

        num_groups_x = desc.Width / threads_per_group_width;
        num_groups_y = desc.Height / threads_per_group_width;
    }
    else {
        num_groups_x = resources->get_resolution().width / threads_per_group_width;
        num_groups_y = resources->get_resolution().height / threads_per_group_width;
    }

    auto srv = explicit_srv == nullptr ? resources->create_reduction_view() : explicit_srv;
    auto uav = resources->create_reduction_uav(num_groups_x, num_groups_y);


    //dispatch silently fails if shader is nullptr.
    assert(this->min_max_reduce_shader);
    assert(this->min_max_reduce_firstpass_shader);
    //run the first pass, 'seeding' the reduction with the minmax from the input SRV.
    dxinit::run_compute_shader(context, this->min_max_reduce_firstpass_shader.Get(), 1, &srv, nullptr, nullptr,
                               0, &uav, 1, num_groups_x, num_groups_y, 1);


    //only bother with the recursion if there's at least an 8x8 UAV to work with.
    if (num_groups_x < threads_per_group_width && num_groups_y < threads_per_group_width)
    {
        return false;
    }
    //recursively reduce, dropping the number of groups by half each time. Eventually UAV[0,0] will
    //store the final min-max.

    while (num_groups_x > 1u && num_groups_y > 1u)
    {

        num_groups_x = std::max(1ull, num_groups_x / threads_per_group_width);
        num_groups_y = std::max(1ull, num_groups_y / threads_per_group_width);


        dxinit::run_compute_shader(context, this->min_max_reduce_shader.Get(), 0, nullptr, nullptr ,
                               nullptr, 0, &uav, 1, num_groups_x, num_groups_y, 1);
    }

    return true;
}

void JumpFloodDispatch::dispatch_distance_normalise_shader(float minimum, float maximum, bool is_signed_field) {

    const uint32_t num_groups_x = resources->get_resolution().width / threads_per_group_width;
    const uint32_t num_groups_y = resources->get_resolution().height / threads_per_group_width;


    auto uav = resources->create_distance_uav(false);

    auto cbuffer = resources->get_local_cbuffer();
    cbuffer.Minimum = minimum;
    cbuffer.Maximum = maximum;
    cbuffer.Signed = is_signed_field ? -1 : 0;

    auto const_buffer_resource = resources->update_const_buffer(context, cbuffer);


   assert(this->distance_normalise_shader);
   dxinit::run_compute_shader(context, this->distance_normalise_shader.Get(), 0, nullptr, const_buffer_resource, nullptr, 0,
                               &uav, 1, num_groups_x, num_groups_y, 1);

}

void JumpFloodDispatch::dispatch_composite_shader(ID3D11UnorderedAccessView *outer_uav) {


    const uint32_t num_groups_x = resources->get_resolution().width / threads_per_group_width;
    const uint32_t num_groups_y = resources->get_resolution().height / threads_per_group_width;


    auto inner_uav = resources->create_distance_uav(false);

    ID3D11UnorderedAccessView* uavs[2] = {outer_uav, inner_uav};

    assert(this->composite_shader);
    dxinit::run_compute_shader(context, this->composite_shader.Get(), 0, nullptr, nullptr, nullptr,
                               0, uavs, 2, num_groups_x, num_groups_y, 1);

}


