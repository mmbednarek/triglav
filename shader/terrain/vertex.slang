struct Vertex
{
    float3 position : POSITION;
    float2 tex_coord : TEXCOORD0;
};

Vertex vs_main(Vertex in_vert)
{
  return in_vert;
}
