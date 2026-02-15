import triglav.mesh;

// w is used as timestamp
[[vk::binding(0)]]
StructuredBuffer<float4> KeyFrames;

struct ChannelState
{
    float start_time;
    uint32_t target_mesh;
    uint32_t last_keyframe;
    uint32_t target_keyframe;
};

[[vk::binding(1)]]
RWStructuredBuffer<ChannelState> ChannelStates;

struct AnimationState {
    float current_time;
    uint32_t channel_count;
};

[[vk::binding(2)]]
ConstantBuffer<AnimationState> AnimationStateCB;

[[vk::binding(3)]]
RWStructuredBuffer<triglav::mesh::SceneMesh> SceneMeshes;


// One thread per channel
[numthreads(256, 1, 1)]
void cs_main(uint32_t threadID : SV_DispatchThreadID)
{
    if (threadID >= AnimationStateCB.channel_count)
        return;

    ChannelState s = ChannelStates[threadID];
    float animation_time = AnimationStateCB.current_time - s.start_time;

    // Find keyframe index
    while (KeyFrames[s.target_keyframe].w < animation_time) {
        if (s.target_keyframe > s.last_keyframe) {
            SceneMeshes[s.target_mesh].transform.translation = KeyFrames[s.target_keyframe].xyz;
            ChannelStates[threadID].target_keyframe = s.target_keyframe;
            return;
        }
        ++s.target_keyframe;
    }

    const uint32_t previous_keyframe = s.target_keyframe - 1;

    const float time_offset = animation_time - KeyFrames[previous_keyframe].w;
    const float time_ratio = time_offset / (KeyFrames[s.target_keyframe].w - KeyFrames[previous_keyframe].w);

    SceneMeshes[s.target_mesh].transform.translation = lerp(KeyFrames[previous_keyframe].xyz, KeyFrames[s.target_keyframe].xyz, time_ratio);

    ChannelStates[threadID].target_keyframe = s.target_keyframe;
}
