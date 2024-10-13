import ray_tracing;

[shader("closesthit")]
void chit_main(inout ray_payload.Shadow payload)
{
    payload.isShadowed = true;
}

[shader("miss")]
void miss_main(inout ray_payload.Shadow payload)
{
    payload.isShadowed = false;
}