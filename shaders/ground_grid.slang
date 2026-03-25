struct VSInput {
    float3 inPosition;
    float3 inColor;
    float2 inTexCoord;
    float3 inNormal;
};

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

struct VSOutput
{
    float4 pos : SV_Position;
    float3 fragColor;
    float2 fragTexCoord;
    float3 fragNormal;
    float3 posWorld;
};

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    float4 world = mul(ubo.model, float4(input.inPosition, 1.0));
    output.posWorld = world.xyz;
    output.pos = mul(ubo.proj, mul(ubo.view, world));
    output.fragColor = input.inColor;
    output.fragTexCoord = input.inTexCoord;
    output.fragNormal = input.inNormal;
    return output;
}

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET {
    float dist = length(vertIn.posWorld.xz - ubo.cameraPos.xz);
    float alpha = 1.0 - smoothstep(10.0, 200.0, dist);
    return float4(vertIn.fragColor, alpha);
}