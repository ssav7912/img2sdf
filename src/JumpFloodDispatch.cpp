//
// Created by Soren on 1/04/2024.
//

#include "JumpFloodDispatch.h"
#include "JumpFloodResources.h"
#include "dxinit.h"
#include <stdexcept>

JumpFloodDispatch::JumpFloodDispatch(ID3D11Device *device, ID3D11DeviceContext *context, jump_flood_shaders byte_code,
                                     JumpFloodResources* resources) :
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

    if (byte_code.min_max_reduce != nullptr) {


        hr = device->CreateComputeShader(byte_code.min_max_reduce, byte_code.min_max_reduce_size, nullptr,
                                         min_max_reduce_shader.GetAddressOf());
        if (FAILED(hr)) {
            throw jumpflood_error(hr, "Could not create min max reduction shader. ");
        }
    }

    if (byte_code.distance_normalise != nullptr) {
        hr = device->CreateComputeShader(byte_code.distance_normalise, byte_code.distance_normalise_size, nullptr,
                                         distance_transform_shader.GetAddressOf());

        if (FAILED(hr)) {
            throw jumpflood_error(hr, "Cound not create distance transform shader.");
        }
    }
}

ID3D11ComputeShader *JumpFloodDispatch::get_shader(SHADERS shader) const {
    switch (shader)
    {
        case SHADERS::PREPROCESS: {return this->preprocess_shader.Get();}
        case SHADERS::VORONOI: {return this->voronoi_shader.Get();}
        case SHADERS::VORONOI_NORMALISE: {return this->voronoi_normalise_shader.Get();}
        case SHADERS::DISTANCE: {return this->distance_transform_shader.Get();}
        case SHADERS::DISTANCE_NORMALISE: {return this->distance_normalise_shader.Get();}
        case SHADERS::MINMAXREDUCE: {return this->min_max_reduce_shader.Get();}
        default: return nullptr;
    }
}

void
JumpFloodDispatch::dispatch_preprocess_shader() {
    const uint32_t num_groups_x = resources->get_resolution().width / 8;
    const uint32_t num_groups_y = resources->get_resolution().height / 8;

    auto srv = resources->get_input_srv();
    auto cbuffer = resources->create_const_buffer(false);
    auto uav = resources->create_voronoi_uav(false);

    dxinit::run_compute_shader(context, preprocess_shader.Get(), 1, &srv,
                               cbuffer, nullptr, 0, &uav, 1, num_groups_x, num_groups_y, 1);
}

void JumpFloodDispatch::dispatch_voronoi_shader() {

    const size_t num_steps = resources->num_steps();
    const uint32_t num_groups_x = resources->get_resolution().width / 8;
    const uint32_t num_groups_y = resources->get_resolution().height / 8;


    auto cbuffer = resources->create_const_buffer(false);
    auto voronoi_uav = resources->create_voronoi_uav(false);

    for (size_t i = num_steps; i > 0; i--)
    {
        auto local_buf = resources->get_local_cbuffer();
        local_buf.Iteration = i;
        resources->update_const_buffer(context, local_buf);

        dxinit::run_compute_shader(context, voronoi_shader.Get(), 0,nullptr,
                                   cbuffer, nullptr, 0, &voronoi_uav, 1, num_groups_x, num_groups_y,1);

    }


}

void JumpFloodDispatch::dispatch_voronoi_normalise_shader() {
    const size_t num_steps = resources->num_steps();
    const uint32_t num_groups_x = resources->get_resolution().width / 8;
    const uint32_t num_groups_y = resources->get_resolution().height / 8;

    auto cbuffer = resources->create_const_buffer(false);
    auto voronoi_uav = resources->create_voronoi_uav(false);

    dxinit::run_compute_shader(context, voronoi_normalise_shader.Get(), 0, nullptr,
                               cbuffer, nullptr, 0, &voronoi_uav, 1, num_groups_x, num_groups_y, 1);

}

void JumpFloodDispatch::dispatch_distance_transform_shader() {

    const uint32_t num_groups_x = resources->get_resolution().width / 8;
    const uint32_t num_groups_y = resources->get_resolution().height / 8;

    auto cbuffer = resources->create_const_buffer(false);
    auto voronoi_uav = resources->create_voronoi_uav(false);
    auto distance_uav = resources->create_distance_uav(false);

    ID3D11UnorderedAccessView* UAVs[] = {voronoi_uav, distance_uav};

    dxinit::run_compute_shader(context, distance_transform_shader.Get(), 0, nullptr,
                               cbuffer, nullptr, 0, UAVs, 2, num_groups_x, num_groups_y, 1);

}


