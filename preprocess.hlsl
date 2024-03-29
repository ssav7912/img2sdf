
Texture2D<float> MaskIn : register(t0);
RWTexture2D<float4> Seeds : register(u0);

///Preprocess a bw mask to format required for seed points
///X coord, Y coord, SeedID, Distance (init to 9999)
[numthreads(8,8,1)]
void preprocess(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    if (MaskIn[dispatchThreadId.xy] > 0.0)
    {
        Seeds[dispatchThreadId.xy] = float4(dispatchThreadId.x, dispatchThreadId.y, 0, 1.#INF);
    }


}
