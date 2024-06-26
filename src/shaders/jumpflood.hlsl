#include "utils.hlsi"

RWTexture2D<float4> Seeds : register(u0);


float offset(float width, int pass_index)
{
    return floor(pow(2, (log2(width) - pass_index - 1)));
}

///Returns the number of passes the JFA kernel needs to make for a square texture.
int num_steps(float width)
{
    return floor(log2(width));
}

void minimum_distance(float2 position, float4 target_point, inout float4 mindistancepoint)
{
    if (target_point.z > 0)
    {
        float distance = dot(position - target_point.xy, position - target_point.xy);
        if (distance < mindistancepoint.w)
        {
            mindistancepoint = float4(target_point.xyz, distance);
        }
    }
}



[numthreads(GROUP_THREAD_DIM,GROUP_THREAD_DIM,1)]
void main(uint3 groupId : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID, uint3 dispatchThreadId : SV_DispatchThreadID,
            uint groupIndex : SV_GroupIndex)
{
    //compute voronoi diagram.

    float4 mindistance = float4(0,0,0,1.#INF);
    float delta = offset(Width, iteration);

    float2 north_offset = {0, delta};
    float2 northeast_offset = {delta, delta};
    float2 east_offset = {delta, 0};
    float2 southeast_offset = {delta, -delta};
    float2 south_offset = {0, -delta};
    float2 southwest_offset = {-delta, -delta};
    float2 west_offset = {-delta, 0};
    float2 northwest_offset = {-delta, delta};


    float4 center = Seeds[dispatchThreadId.xy];
    float4 north = Seeds[dispatchThreadId.xy + north_offset];
    float4 northeast = Seeds[dispatchThreadId.xy + northeast_offset];
    float4 east = Seeds[dispatchThreadId.xy + east_offset];
    float4 southeast = Seeds[dispatchThreadId.xy + southeast_offset];
    float4 south = Seeds[dispatchThreadId.xy + south_offset];
    float4 southwest = Seeds[dispatchThreadId.xy + southwest_offset];
    float4 west = Seeds[dispatchThreadId.xy + west_offset];
    float4 northwest = Seeds[dispatchThreadId.xy + northwest_offset];


    minimum_distance(dispatchThreadId.xy, center, mindistance);
    minimum_distance(dispatchThreadId.xy, north, mindistance);
    minimum_distance(dispatchThreadId.xy, northeast, mindistance);
    minimum_distance(dispatchThreadId.xy, east, mindistance);
    minimum_distance(dispatchThreadId.xy, southeast, mindistance);
    minimum_distance(dispatchThreadId.xy, south, mindistance);
    minimum_distance(dispatchThreadId.xy, southwest, mindistance);
    minimum_distance(dispatchThreadId.xy, west, mindistance);
    minimum_distance(dispatchThreadId.xy, northwest, mindistance);

    //try and avoid?
    if (mindistance.w != 1.#INF)
    {
        Seeds[dispatchThreadId.xy] = mindistance;

    }

//
//     AllMemoryBarrierWithGroupSync();
//
//     //do distance transform
//     float maximum = -1.#INF;
//     float minimum = 1.#INF;
//
//     float d = distance(Seeds[dispatchThreadId.xy].xy, dispatchThreadId.xy);
//     Distance[dispatchThreadId.xy] = d;
//     maximum = max(d, maximum);
//     minimum = min(d, minimum);
//
//     AllMemoryBarrierWithGroupSync();
//
//     float dist = Distance[dispatchThreadId.xy];
//
//     Distance[dispatchThreadId.xy] = minimum;

}

