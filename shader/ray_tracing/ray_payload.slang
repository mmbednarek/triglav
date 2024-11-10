module ray_tracing;

namespace ray_payload
{
    public struct Shadow
    {
        public bool isShadowed;
    };

    public struct AmbientOcclusion
    {
        public float distance;
        public bool isOccluded;
    };

    public struct Color
    {
        public float shadowTerm;
        public float ambientOcclusionTerm;
    };
}