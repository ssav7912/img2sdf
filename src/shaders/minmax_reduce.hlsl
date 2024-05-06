#include "utils.hlsi"
#include "threadgroup_reduce.hlsi"

RWTexture2D<float2> Reduce : register(u0);


///Multipass form of the reduction. Every value in Reduce holds the minmax
///of the thread groups for the previous pass. We overwrite the values at groupID with
///the new minmax and repeat.
[numthreads(GROUP_THREAD_DIM,GROUP_THREAD_DIM,1)]
void reduce(uint3 dispatchThreadId: SV_DispatchThreadID, uint3 groupID : SV_GroupID, uint3 threadID : SV_GroupThreadID)
{

    uint mem_index = index_2D_to_1D(threadID.xy, GROUP_THREAD_DIM);

    minmax[mem_index] = Reduce[dispatchThreadId.xy];
    GroupMemoryBarrierWithGroupSync();

    thread_group_minmax(mem_index);

    GroupMemoryBarrierWithGroupSync();

    if (mem_index == 0)
    {
        Reduce[groupID.xy] = minmax[mem_index];
    }
}