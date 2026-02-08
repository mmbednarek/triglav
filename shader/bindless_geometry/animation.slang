import triglav.mesh;

struct AnimationKeyFrame
{
    float time_stamp;
    float3 translation;
};

[[vk::binding(0)]]
StructuredBuffer<AnimationKeyFrame> KeyFrames;

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

    // Find keyframe index
    while (KeyFrames[s.target_keyframe].time_stamp < AnimationStateCB.current_time) {
        if (s.target_keyframe == s.last_keyframe) {
            SceneMeshes[s.target_mesh].transform.translation = KeyFrames[s.target_keyframe].translation;
            ChannelStates[threadID].target_keyframe = s.target_keyframe;
            return;
        }
        ++s.target_keyframe;
    }

    const uint32_t previous_keyframe = s.target_keyframe - 1;

    const float time_offset = AnimationStateCB.current_time - KeyFrames[previous_keyframe].time_stamp;
    const float time_ratio = time_offset / (KeyFrames[s.target_keyframe].time_stamp - KeyFrames[previous_keyframe].time_stamp);

    SceneMeshes[s.target_mesh].transform.translation = lerp(KeyFrames[previous_keyframe].translation, KeyFrames[s.target_keyframe].translation, time_ratio);

    ChannelStates[threadID].target_keyframe = s.target_keyframe;
}
