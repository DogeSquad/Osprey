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

float gridAlpha(float2 worldXZ, float scale, float baseThickness, float dist)
{
    float2 coord = worldXZ * scale;
    float2 fw = fwidth(coord);
    float2 grid = abs(fract(coord - 0.5) - 0.5) / fw;
    float line = min(grid.x, grid.y);

    // thickness shrinks with distance
    float thickness = baseThickness * (1.0 - smoothstep(0.0, 150.0, dist));
    thickness = max(thickness, 0.1); // minimum thickness so they don't disappear entirely

    return 1.0 - smoothstep(0.0, thickness, line);
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

    // Render grid

    // intersect with y=0 plane
    //float t = -ro.y / rd.y;
    //if (t > 0.0) {
    //    float3 worldPos = ro + rd * t;
    //    float dist = length(worldPos.xz - ro.xz);
    //
    //    float lineThickness = 0.01; // lower = thinner
    //
    //    // grid from world XZ
    //    float2 grid = abs(fract(worldPos.xz - 0.5) - 0.5) / (fwidth(worldPos.xz) * lineThickness);
    //    float line = min(grid.x, grid.y);
    //    float alpha = gridAlpha(worldPos.xz, 1.0, 2.0, dist);
    //
    //    // thicker lines at every 10 units
    //    float2 grid10 = abs(fract(worldPos.xz * 0.1 - 0.5) - 0.5) / fwidth(worldPos.xz * 0.1);
    //    float line10 = min(grid10.x, grid10.y);
    //    float alpha10 = gridAlpha(worldPos.xz, 0.1, 2.0, dist);
    //
    //    alpha = max(alpha, alpha10 * 0.5);
    //
    //    // fade with distance
    //    float fade = 1.0 - smoothstep(50.0, 200.0, dist);
    //    alpha *= fade;
    //
    //    if (alpha > 0.01) {
    //        backgroundColor = lerp(backgroundColor, 0.2 * float3(1.0, 1.0, 1.0), alpha);
    //    }
    //}
    return float4(backgroundColor, 1.0);
}