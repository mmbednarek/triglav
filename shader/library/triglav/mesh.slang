module mesh;

import triglav.core;

namespace triglav::mesh
{
    static const uint32_t MATRIX_OFFSET_DISABLED = ~0u;

    public struct DrawCall
    {
        public uint32_t indexCount;
        public uint32_t instanceCount;
        public uint32_t indexOffset;
        public uint32_t vertexOffset;
        public uint32_t instanceOffset;
        public uint32_t materialID;
        public uint32_t matrixOffset;
        public float4x4 transform;
        public float4x4 normalTransform;
    };

    public struct SceneMesh
    {
        public uint32_t indexCount;
        public uint32_t indexOffset;
        public uint32_t vertexOffset;
        public uint32_t materialID;
        public uint32_t transformID;
        public uint32_t matrixOffset;
        public BoundingBox boundingBox;

        public DrawCall to_draw_call(StructuredBuffer<float4x4> matrix_buffer)
        {
            DrawCall dc;
            dc.indexCount = indexCount;
            dc.instanceCount = 1;
            dc.indexOffset = indexOffset;
            dc.vertexOffset = vertexOffset;
            dc.instanceOffset = 0;
            dc.materialID = materialID;
            dc.matrixOffset = matrixOffset;

            const float4x4 transform_matrix = matrix_buffer[transformID];
            dc.transform = transform_matrix;
            dc.normalTransform = extent_mat3x3(normal_transform_matrix(transform_matrix));

            return dc;
        }

        public bool is_skeletal()
        {
            return matrixOffset != MATRIX_OFFSET_DISABLED;
        }
    };
}