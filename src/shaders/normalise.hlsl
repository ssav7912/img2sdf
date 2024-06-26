#include "utils.hlsi"
RWTexture2D<float> Distance : register(u0);

///Remaps input linearly. Used to normalise distance based on computed minmax.
[numthreads(GROUP_THREAD_DIM,GROUP_THREAD_DIM,1)]
void normalise(uint3 DispatchThreadId : SV_DispatchThreadID)
{
    Distance[DispatchThreadId.xy] = remap(Distance[DispatchThreadId.xy], minimum, maximum, is_signed, 1);
}