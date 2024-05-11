#include "utils.hlsi"


Texture2D<float> MaskIn : register(t0);
RWTexture2D<float4> Seeds : register(u0);

[numthreads(GROUP_THREAD_DIM,GROUP_THREAD_DIM,1)]
void invert(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    if (saturate(1 - MaskIn[dispatchThreadId.xy]) > 0.0)
    {
        Seeds[dispatchThreadId.xy] = float4(dispatchThreadId.x, dispatchThreadId.y, index_2D_to_1D(dispatchThreadId.xy, Width) + 1, 1.#INF);
    }


}