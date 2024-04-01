#include "utils.hlsi"
RWTexture2D<float4> Seeds : register(u0);
RWTexture2D<float> Distance : register(u1);


[numthreads(8,8,1)]
void distance(uint3 dispatchThreadId: SV_DispatchThreadID)
{
        float d = distance(Seeds[dispatchThreadId.xy].xy, dispatchThreadId.xy);
        Distance[dispatchThreadId.xy] = d;

}