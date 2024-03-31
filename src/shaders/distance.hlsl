#include "utils.hlsi"
RWTexture2D<float4> Seeds : register(u0);
RWTexture2D<float4> Distance : register(u1);


void main(uint3 dispatchThreadId: SV_DispatchThreadID)
{
        float d = distance(Seeds[dispatchThreadId.xy].xy, dispatchThreadId.xy);
        Distance[dispatchThreadId.xy] = d;

}