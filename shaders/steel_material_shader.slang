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
    float3 normal = normalize(vertIn.fragNormal);
    float3 lightDir = normalize(ubo.lightDir);
    float3 viewDir = normalize(ubo.cameraPos - vertIn.posWorld);
    float3 halfDir = normalize(lightDir + viewDir);

    float ambientStrength = 0.15f;
    float3 ambient = ambientStrength * vertIn.fragColor;

    float diff = max(dot(normal, lightDir), 0.0);
    float3 diffuse = diff * vertIn.fragColor;

    float shininess = 64.0;
    float spec = pow(max(dot(normal, halfDir), 0.0), shininess);
    float3 specular = spec * float3(1.0, 1.0, 1.0);

    float3 result = ambient + diffuse + specular;
    return float4(result, 1.0);
}