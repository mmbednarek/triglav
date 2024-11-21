module mesh;

namespace mesh
{
    public struct SceneMesh
    {
        public uint32_t indexCount;
        public uint32_t instanceCount;
        public uint32_t indexOffset;
        public uint32_t vertexOffset;
        public uint32_t instanceOffset;
        public uint32_t materialID;
        public float4x4 transform;
        public float3   boundingBoxMin;
        public float3   boundingBoxMax;
    };
}