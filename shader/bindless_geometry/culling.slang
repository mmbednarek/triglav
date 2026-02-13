import triglav.core;
import triglav.mesh;

[[vk::binding(0)]]
StructuredBuffer<triglav::mesh::SceneMesh> SceneMeshes;

[[vk::binding(1)]]
ConstantBuffer<uint32_t> SceneMeshCount;

[[vk::binding(2)]]
RWStructuredBuffer<uint32_t> Count;

[[vk::binding(3)]]
ConstantBuffer<triglav::ViewProperties> ViewProps;

[[vk::binding(4)]]
[[vk::image_format("r16")]]
Texture2D<float> HiZBuffer;

[[vk::binding(5)]]
RWStructuredBuffer<triglav::mesh::DrawCall> DrawCalls_MT0;

[[vk::binding(6)]]
RWStructuredBuffer<triglav::mesh::DrawCall> DrawCalls_MT1;

[[vk::binding(7)]]
RWStructuredBuffer<triglav::mesh::DrawCall> DrawCalls_MT2;

uint2 to_texture_space(float2 screenSpace, float2 texRes)
{
    return uint2(clamp(0.5*(screenSpace + float2(1, 1)) * texRes, float2(0,0), texRes - float2(1, 1)));
}

uint2 translate_coord_to_mip(uint2 texCoord, uint mipLevel)
{
    for (uint i = 0; i < mipLevel; ++i)
    {
        texCoord /= 2;
    }
    return texCoord;
}

uint32_t size_to_mip_level(uint32_t size)
{
    uint32_t mipLevel = 0;
    while (size > 1) {
        mipLevel++;
        size /= 2;
    }
    return min(mipLevel, 8);
}

bool is_object_visible(in triglav::mesh::SceneMesh mesh)
{
   // Do frustum culling
    float4x4 transformMat = mesh.transform.matrix();

    float4x4 mat = mul(ViewProps.proj, mul(ViewProps.view, transformMat));

    const float3 points[] = {
        {mesh.boundingBox.min.x, mesh.boundingBox.min.y, mesh.boundingBox.min.z},
        {mesh.boundingBox.min.x, mesh.boundingBox.min.y, mesh.boundingBox.max.z},
        {mesh.boundingBox.min.x, mesh.boundingBox.max.y, mesh.boundingBox.min.z},
        {mesh.boundingBox.min.x, mesh.boundingBox.max.y, mesh.boundingBox.max.z},
        {mesh.boundingBox.max.x, mesh.boundingBox.min.y, mesh.boundingBox.min.z},
        {mesh.boundingBox.max.x, mesh.boundingBox.min.y, mesh.boundingBox.max.z},
        {mesh.boundingBox.max.x, mesh.boundingBox.max.y, mesh.boundingBox.min.z},
        {mesh.boundingBox.max.x, mesh.boundingBox.max.y, mesh.boundingBox.max.z},
    };

    float3 minPoint = float3(1.#INF, 1.#INF, 1.#INF);
    float3 maxPoint = float3(-1.#INF, -1.#INF, -1.#INF);

    [unroll]
    for (int i = 0; i < 8; ++i)
    {
        let projectedPoint = mul(mat, float4(points[i], 1));
        let linearZ = ViewProps.to_linear_depth(projectedPoint.z / projectedPoint.w);

        let hmPoint = projectedPoint / abs(projectedPoint.w);

        if (hmPoint.x < minPoint.x) {
            minPoint.x = hmPoint.x;
        }
        if (hmPoint.y < minPoint.y) {
            minPoint.y = hmPoint.y;
        }
        if (linearZ < minPoint.z) {
            minPoint.z = linearZ;
        }
        if (hmPoint.x > maxPoint.x) {
            maxPoint.x = hmPoint.x;
        }
        if (hmPoint.y > maxPoint.y) {
            maxPoint.y = hmPoint.y;
        }
        if (linearZ > maxPoint.z) {
            maxPoint.z = linearZ;
        }
    }


   if (minPoint.x > 1.0f || maxPoint.x < -1.0f || minPoint.y > 1.0f || maxPoint.y < -1.0f || minPoint.z > 1.0f || maxPoint.z < 0.0f)
        return false;

   // printf("MIN(%f, %f, %f), MAX(%f, %f, %f)", minPoint.x, minPoint.y, minPoint.z, maxPoint.x, maxPoint.y, maxPoint.z);

   // Do occlusion culling

   uint32_t width, height, levelCount;
   HiZBuffer.GetDimensions(0, width, height, levelCount);

   const float2 hiZBuffRes = float2(width, height);
   uint2 minTexSpace = to_texture_space(minPoint.xy, hiZBuffRes);
   uint2 maxTexSpace = to_texture_space(maxPoint.xy, hiZBuffRes);

   const uint2 diff = maxTexSpace - minTexSpace;
   uint32_t mipLevel = size_to_mip_level(max(diff.x, diff.y));

   uint32_t mipWidth = width >> mipLevel;
   uint32_t mipHeight = height >> mipLevel;

   uint2 minTexSpaceMip = clamp(translate_coord_to_mip(minTexSpace, mipLevel), uint2(0, 0), uint2(mipWidth-1, mipHeight-1));
   uint2 maxTexSpaceMip = clamp(translate_coord_to_mip(maxTexSpace, mipLevel), uint2(0, 0), uint2(mipWidth-1, mipHeight-1));

   float depthSamples[4];
   depthSamples[0] = ViewProps.to_linear_depth(HiZBuffer.Load(int3(minTexSpaceMip, mipLevel)));
   depthSamples[1] = ViewProps.to_linear_depth(HiZBuffer.Load(int3(minTexSpaceMip.x, maxTexSpaceMip.y, mipLevel)));
   depthSamples[2] = ViewProps.to_linear_depth(HiZBuffer.Load(int3(maxTexSpaceMip.x, minTexSpaceMip.y, mipLevel)));
   depthSamples[3] = ViewProps.to_linear_depth(HiZBuffer.Load(int3(maxTexSpaceMip, mipLevel)));

   [unroll]
   for (int i = 0; i < 4; ++i)
   {
        if (minPoint.z < depthSamples[i])
            return true;
   }


   return false;
}

[numthreads(1024, 1, 1)]
void cs_main(uint3 threadID : SV_DispatchThreadID)
{
    if (threadID.x >= SceneMeshCount)
        return;

    triglav::mesh::SceneMesh sceneMesh = SceneMeshes[threadID.x];
    // triglav::mesh::SceneMesh sceneMesh; // = SceneMeshes[threadID.x];

    if (!is_object_visible(sceneMesh))
        return;

    const uint templateID = sceneMesh.materialID & 0b111;
    sceneMesh.materialID >>= 3;

    uint32_t dstIndex;
    InterlockedAdd(Count[templateID], 1, dstIndex);

    triglav::mesh::DrawCall drawCall = sceneMesh.to_draw_call();

    switch (templateID)
    {
    case 0:
        DrawCalls_MT0[dstIndex] = drawCall;
        break;
    case 1:
        DrawCalls_MT1[dstIndex] = drawCall;
        break;
    case 2:
        DrawCalls_MT2[dstIndex] = drawCall;
        break;
    default: break;
    }
}
