#include "utils.hlsi"

RWTexture2D<float> Outer : register(u0);
RWTexture2D<float> Inner : register(u1);


[numthreads(GROUP_THREAD_DIM, GROUP_THREAD_DIM, 1)]
void composite(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    if (Outer[dispatchThreadId.xy] == 0 && Inner[dispatchThreadId.xy] > 0)
    {
        Inner[dispatchThreadId.xy] = -Inner[dispatchThreadId.xy];
    }
    else if (Outer[dispatchThreadId.xy] > 0 && Inner[dispatchThreadId.xy] == 0)
    {
        Inner[dispatchThreadId.xy] = Outer[dispatchThreadId.xy];
    }
    else if (Outer[dispatchThreadId.xy] == 0 && Inner[dispatchThreadId.xy] == 0)
    {
        Inner[dispatchThreadId.xy] = 0;
    }

    //Inner[dispatchThreadId.xy] = max(Outer[dispatchThreadId.xy], Inner[dispatchThreadId.xy]);


}