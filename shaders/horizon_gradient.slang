struct UniformBuffer {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
    float4x4 invView;
    float4x4 invProj;
    float3 lightDir;
    float3 cameraPos;
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
    // reconstruct world ray
    float4 rayView = mul(ubo.invProj, float4(input.ndcPos, -1.0, 1.0));
    rayView = float4(rayView.xy, -1.0, 0.0);
    float3 rd = normalize(mul(ubo.invView, rayView).xyz);
    float3 ro = ubo.cameraPos;

    float u = rd.y;

    float3 horizonColor = float3(0.6, 0.7, 0.8);
    float3 skyColor = float3(0.1, 0.2, 0.5);
    float3 groundColor = float3(0.08, 0.07, 0.06);

    float3 backgroundColor;
    if (u >= 0.0)
        backgroundColor = lerp(horizonColor, skyColor, smoothstep(0.0, 0.4, u));
    else
        backgroundColor = lerp(horizonColor, groundColor, smoothstep(0.0, -0.03, u));

    float3 sunIntensity = smoothstep(0.9999, 0.99999, dot(rd, normalize(ubo.lightDir)));
    backgroundColor = (1-sunIntensity) * backgroundColor + sunIntensity * float3(1.0, 0.9, 0.9);

    return float4(backgroundColor, 1.0);
}