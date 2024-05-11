//
// Created by Soren on 11/05/2024.
//


#ifndef IMG2SDF_PROFILER_H
#define IMG2SDF_PROFILER_H

#include <d3d11.h>
#include <wrl.h>
#include <string>
#include <unordered_map>

using namespace Microsoft::WRL;
class Profiler
{
public:
    Profiler(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> context);

    void start(const std::string &profile_name);
    double end(const std::string& profile_name);

private:

    struct profile_data
    {
        ComPtr<ID3D11Query> disjoint_query;
        ComPtr<ID3D11Query> timestamp_start;
        ComPtr<ID3D11Query> timestamp_end;

        bool query_start = false;
        bool query_finished = false;
    };

    std::unordered_map<std::string, profile_data> profile_map;

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;


};

class ScopedProfile
{
public:
    ScopedProfile(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> context, std::string  name, double& out_time);
    ~ScopedProfile();

private:
    Profiler p;
    std::string name;
    double& out_time;
};


#endif //IMG2SDF_PROFILER_H


