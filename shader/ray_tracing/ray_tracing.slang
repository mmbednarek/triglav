module ray_tracing;

namespace ray_tracing
{

public typedef BuiltInTriangleIntersectionAttributes Attributes;

public struct VertexData
{
    public float3 position;
    public float2 texCoord;
    public float3 normal;
    public float3 tangent;
    public float3 bitangent;
};

public struct ObjectData
{
    public int*        indicies;
    public VertexData* verticies;
};

public struct ObjectHitInfo
{
    public float3 objectSpacePosition;
    public float3 worldSpacePosition;
    public float3 normal;

    public __init(ObjectData objData, uint primitiveIndex, Attributes attribs, float3x4 transform)
    {
        const int3 indicies = {
            objData.indicies[3 * primitiveIndex],
            objData.indicies[3 * primitiveIndex + 1],
            objData.indicies[3 * primitiveIndex + 2],
        };

        let vecData0 = objData.verticies[indicies.x];
        let vecData1 = objData.verticies[indicies.y];
        let vecData2 = objData.verticies[indicies.z];

        const float3 barycentrics = {
            1.0 - attribs.barycentrics.x - attribs.barycentrics.y,
            attribs.barycentrics.x,
            attribs.barycentrics.y,
        };

        let positionMatrix = transpose(float3x3(
            vecData0.position.xyz,
            vecData1.position.xyz,
            vecData2.position.xyz,
        ));
        this.objectSpacePosition = mul(positionMatrix, barycentrics);

        let normalMatrix = transpose(float3x3(
            vecData0.normal.xyz,
            vecData1.normal.xyz,
            vecData2.normal.xyz,
        ));
        this.normal = normalize(mul(normalMatrix, barycentrics));

        this.worldSpacePosition = mul(ObjectToWorld3x4(), float4(this.objectSpacePosition, 1));
    }
};

}
