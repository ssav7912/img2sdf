//
// Created by Soren on 11/05/2024.
//

#include "Profiler.h"
#include "jumpflooderror.h"
#include <cassert>
#include <format>
#include <utility>

Profiler::Profiler(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> context) : device(std::move(device)), context(std::move(context)) {

}

void Profiler::start(const std::string &profile_name) {

    auto& data = profile_map[profile_name];
    assert(data.query_start == false);
    assert(data.query_finished == false);


    D3D11_QUERY_DESC disjoint_desc {};
    disjoint_desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
    disjoint_desc.MiscFlags = 0;


    HRESULT query_disjoint_hr = device->CreateQuery(&disjoint_desc, &data.disjoint_query);
    if (FAILED(query_disjoint_hr))
    {
        throw jumpflood_error(query_disjoint_hr, std::format("Could not create disjoint profiling query for {}", profile_name));
    }


    D3D11_QUERY_DESC desc {};
    desc.Query = D3D11_QUERY_TIMESTAMP;
    desc.MiscFlags = 0;


    HRESULT query_hr = device->CreateQuery(&desc, &data.timestamp_start);
    if (FAILED(query_hr))
    {
        throw jumpflood_error(query_hr, std::format("Could not create profiling query for {}", profile_name));
    }

    query_hr = device->CreateQuery(&desc, &data.timestamp_end);

    if (FAILED(query_hr))
    {
        throw jumpflood_error(query_hr, std::format("Could not create profiling query for {}", profile_name));
    }

    context->Begin(data.disjoint_query.Get());
    context->End(data.timestamp_start.Get());

    data.query_start = true;

}

double Profiler::end(const std::string &profile_name) {


    auto& data = profile_map[profile_name];
    assert(data.query_start == true);
    assert(data.query_finished == false);

    context->End(data.timestamp_end.Get());

    context->End(data.disjoint_query.Get());


    data.query_start = false;
    data.query_finished = true;

    uint64_t start_time = 0;
    while (context->GetData(data.timestamp_start.Get(), &start_time, sizeof(start_time), 0) != S_OK);

    uint64_t end_time = 0;
    while (context->GetData(data.timestamp_end.Get(), &end_time, sizeof(end_time), 0) != S_OK);

    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint {};
    while (context->GetData(data.disjoint_query.Get(), &disjoint, sizeof(disjoint), 0) != S_OK);

    double time = 0.0;
    if (disjoint.Disjoint == false)
    {
        uint64_t delta = end_time - start_time;
        double frequency = static_cast<double>(disjoint.Frequency);
        time = (delta / frequency) * 1000.0;
    }

    return time;


}

ScopedProfile::ScopedProfile(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> context, std::string name, double &out_time)
    : name(std::move(name)), out_time(out_time), p(std::move(device), std::move(context)){

    p.start(this->name);

}

ScopedProfile::~ScopedProfile()
{
    out_time = p.end(this->name);

}
