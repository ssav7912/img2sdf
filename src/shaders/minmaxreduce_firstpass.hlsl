#include "utils.hlsi"
#include "threadgroup_reduce.hlsi"

Texture2D<float> Input : register(t0);
RWTexture2D<float2> Reduce : register(u0);

///First pass form of the reduction, reads the distance transform from input SRV and
///writes out the min-max per-group into Reduce.
///each groupID cell in Reduce stores the minmax for the threadgroup. 
[numthreads(GROUP_THREAD_DIM,GROUP_THREAD_DIM,1)]
void reduce_firstpass(uint3 dispatchThreadId: SV_DispatchThreadID, uint3 groupID : SV_GroupID, uint3 threadID: SV_GroupThreadID)
{

    uint mem_index = index_2D_to_1D(threadID.xy, GROUP_THREAD_DIM);
    minmax[mem_index] = float2(Input[dispatchThreadId.xy], Input[dispatchThreadId.xy]);
    //can't start until the data is full!
    GroupMemoryBarrierWithGroupSync();

    thread_group_minmax(mem_index);
    GroupMemoryBarrierWithGroupSync();

    if (mem_index == 0)
    {
        Reduce[groupID.xy] = minmax[mem_index];
    }

}