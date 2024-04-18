#include "utils.hlsi"
#include "threadgroup_reduce.hlsi"

RWTexture2D<float2> Reduce : register(u0);


///Multipass form of the reduction. Every value in Reduce holds the minmax
///of the thread groups for the previous pass. We overwrite the values at groupID with
///the new minmax and repeat.
[numthreads(8,8,1)]
void reduce(uint3 dispatchThreadId: SV_DispatchThreadID, uint3 groupID : SV_GroupID, uint3 threadID : SV_GroupThreadID)
{

    uint mem_index = index_2D_to_1D(threadID.xy, 8);

    minmax[mem_index] = Reduce[dispatchThreadId.xy];

    //bc compiler is buggy and passing groupshared mem makes it trip a race condition false positive,
    //we have to declare the groupshared mem in the header file and use it implicitly in the function.
    thread_group_minmax(mem_index);

    GroupMemoryBarrierWithGroupSync();

    if (mem_index == 0)
    {
        Reduce[groupID.xy] = minmax[mem_index];
    }
}