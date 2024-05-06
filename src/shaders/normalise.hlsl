#include "utils.hlsi"
RWTexture2D<float> Distance : register(u0)

cbuffer minmax : register(b0) {
    float minimum;
    float maximum;
    float signed;
}

///Remaps input linearly. Used to normalise distance based on computed minmax.
[numthreads(8,8,1)]
void main(uint3 DispatchThreadId : SV_DispatchThreadID)
{
    Distance[DispatchThreadId.xy] = remap(DispatchThreadId.xy, minimum, maximum, signed, 1);
}