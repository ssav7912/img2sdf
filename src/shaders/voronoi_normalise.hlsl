#include "utils.hlsi"

RWTexture2D<float4> Seeds : register(u0);

///debug utility to remap voronoi cell coords to a normalised form.
[numthreads(8,8,1)]
void voronoi_normalise(uint3 dispatchThreadId: SV_DispatchThreadID)
{
    float4 seed = Seeds[dispatchThreadId.xy].rgba;

    seed.r = remap(seed.r, 0, Width, 0, 1);
    seed.g = remap(seed.g, 0, Height, 0, 1);
    seed.b = 0.0;
    seed.a = 1.0;

    Seeds[dispatchThreadId.xy] = seed;

}