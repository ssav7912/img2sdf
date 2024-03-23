

RWTexture2D<float4> Seeds : register(u0);
RWTexture2D<float4> Result : register(u1);
//Texture2D<float4> Seeds : register(u1);
const int iterations : register(u2);
const int width : register(u3)

float offset(float width, int pass_index)
{
    return floor(pow(2, (log2(width) - pass_index - 1)));
}

int num_steps(float width)
{
    return floor(log2(width));
}

void minimum_distance(float2 position, float4 target_point, inout float4 mindistancepoint)
{
    if (target_point.z > 0)
    {
        float distance = dot(position - target_point.xy, position - target_point.xy);
        if (distance < mindistancepoint.w)
        {
            mindistancepoint = float4(target_point, distance);
        }
    }
}

///Preprocess a bw mask to format required for seed points
///X coord, Y coord, SeedID, Distance (init to 9999)
[numthreads(8,8,1)]
void mask_preprocess(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    if (Seeds[dispatchThreadId.xy] > 0)
    {
        Seeds[dispatchThreadId.xy] = float4(dispatchThreadId.x, dispatchThreadId.y, dispatchThreadId.x * dispatchThreadId.y, #INF);
    }
}

[numthreads(8,8,1)]
void main(uint3 groupId : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID, uint3 dispatchThreadId : SV_DispatchThreadID,
            uint groupIndex : SV_GroupIndex)
{

    int iterations;
    Seeds.GetDimensions(iterations);
    //iterations = num_steps(iterations);

    for (int i = iterations; i > 0; i--)
    {
        float4 mindistance = {0};
        float delta = offset(width, i);

        float2 north_offset = {0, delta};
        float2 northeast_offset = {delta, delta};
        float2 east_offset = {delta, 0};
        float2 southeast_offset = {delta, -delta};
        float2 south_offset = {0, -delta};
        float2 southwest_offset = {-delta, -delta};
        float2 west_offset = {-delta, 0};
        float2 northwest_offset = {-delta, delta};


        float4 center = Seeds[dispatchThreadId.xy];
        float4 north = Seeds[dispatchThreadId.xy + north_offset];
        float4 northeast = Seeds[dispatchThreadId.xy + northeast_offset];
        float4 east = Seeds[dispatchThreadId.xy + east_offset].xyz;
        float4 southeast = Seeds[dispatchThreadId.xy + southeast_offset];
        float4 south = Seeds[dispatchThreadId.xy + south_offset];
        float4 southwest = Seeds[dispatchThreadId.xy + southwest_offset];
        float4 west = Seeds[dispatchThreadId.xy + west];
        float4 northwest = Seeds[dispatchThreadId.xy + northwest_offset];


        minimum_distance(dispatchThreadId.xy, center, mindistance);
        minimum_distance(dispatchThreadId.xy, north, mindistance);
        minimum_distance(dispatchThreadId.xy, northeast, mindistance);
        minimum_distance(dispatchThreadId.xy, east, mindistance);
        minimum_distance(dispatchThreadId.xy, southeast, mindistance);
        minimum_distance(dispatchThreadId.xy, south, mindistance);
        minimum_distance(dispatchThreadId.xy, southwest, mindistance);
        minimum_distance(dispatchThreadId.xy, west, mindistance);
        minimum_distance(dispatchThreadId.xy, northwest, mindistance);
        Seeds[dispatchThreadId.xy] = mindistance;

        AllMemoryBarrierWithGroupSync();
    }

}

