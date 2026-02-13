module mesh;

import triglav.core;

namespace triglav::mesh
{
    public struct DrawCall
    {
        public uint32_t indexCount;
        public uint32_t instanceCount;
        public uint32_t indexOffset;
        public uint32_t vertexOffset;
        public uint32_t instanceOffset;
        public uint32_t materialID;
        public float4x4 transform;
        public float4x4 normalTransform;
    };

    public struct SceneMesh
    {
        public uint32_t indexCount;
        public uint32_t instanceCount;
        public uint32_t indexOffset;
        public uint32_t vertexOffset;
        public uint32_t instanceOffset;
        public uint32_t materialID;
        public Transform transform;
        public BoundingBox boundingBox;

        public DrawCall to_draw_call()
        {
            DrawCall dc;
            dc.indexCount = indexCount;
            dc.instanceCount = instanceCount;
            dc.indexOffset = indexOffset;
            dc.vertexOffset = vertexOffset;
            dc.instanceOffset = instanceOffset;
            dc.materialID = materialID;

            const float4x4 transform_matrix = transform.matrix();
            dc.transform = transform_matrix;
            dc.normalTransform = extent_mat3x3(normal_transform_matrix(transform_matrix));

            return dc;
        }
    };
}