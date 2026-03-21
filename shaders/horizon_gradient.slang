struct UniformBuffer {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
    float4x4 invView;
    float4x4 invProj;
    float3 lightDir;
};
ConstantBuffer<UniformBuffer> ubo;

struct VSOutput {
    float4 position : SV_Position;
    float2 ndcPos;
}

[shader("vertex")]
VSOutput vertMain(uint vertexIndex: SV_VertexID)
{
    float2 uv = float2((vertexIndex << 1) & 2, vertexIndex & 2);
    float4 pos = float4(uv * 2.0 - 1.0, 1.0, 1.0);

    VSOutput output;
    output.position = pos;
    output.ndcPos = uv * 2.0 - 1.0;
    return output;
}

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target
{
    float4 rayView = mul(ubo.invProj, float4(input.ndcPos, -1.0, 1.0));
    rayView = float4(rayView.xy, -1.0, 0.0);
    float3 rayWorld = normalize(mul(ubo.invView, rayView).xyz);

    float t = rayWorld.y;

    float3 horizonColor = float3(0.6, 0.7, 0.8);
    float3 skyColor = float3(0.1, 0.2, 0.5);
    float3 groundColor = float3(0.08, 0.07, 0.06);

    float3 color;
    if (t >= 0.0)
        color = lerp(horizonColor, skyColor, smoothstep(0.0, 0.4, t));
    else
        color = lerp(horizonColor, groundColor, smoothstep(0.0, -0.03, t));

    return float4(color, 1.0);
}