import triglav.mesh;

// w is used as timestamp
[[vk::binding(0)]]
StructuredBuffer<float4> KeyFrames;

[[vk::binding(1)]]
StructuredBuffer<float> TimeStamps;

struct ChannelState
{
    float start_time;
    uint32_t target_mesh;
    uint32_t last_keyframe;
    uint32_t target_keyframe;
    uint32_t channel_type;
};

[[vk::binding(2)]]
RWStructuredBuffer<ChannelState> ChannelStates;

struct AnimationState {
    float current_time;
    uint32_t channel_count;
};

[[vk::binding(3)]]
ConstantBuffer<AnimationState> AnimationStateCB;

[[vk::binding(4)]]
RWStructuredBuffer<triglav::mesh::SceneMesh> SceneMeshes;

void set_target_value(in ChannelState state, float4 value)
{
    if (state.channel_type == 0) {
        SceneMeshes[state.target_mesh].transform.translation = value.xyz;
    } else if (state.channel_type == 1) {
        SceneMeshes[state.target_mesh].transform.rotation = value;
    }
}

// One thread per channel
[numthreads(256, 1, 1)]
void cs_main(uint32_t threadID : SV_DispatchThreadID)
{
    if (threadID >= AnimationStateCB.channel_count)
        return;

    ChannelState s = ChannelStates[threadID];
    float animation_time = AnimationStateCB.current_time - s.start_time;

    if (s.target_keyframe > s.last_keyframe) {
        set_target_value(s, KeyFrames[s.last_keyframe]);
        ChannelStates[threadID].target_keyframe = s.target_keyframe;
        return;
    }

    // Find keyframe index
    while (TimeStamps[s.target_keyframe] < animation_time) {
        if (s.target_keyframe > s.last_keyframe) {
            set_target_value(s, KeyFrames[s.last_keyframe]);
            ChannelStates[threadID].target_keyframe = s.target_keyframe;
            return;
        }
        ++s.target_keyframe;
    }

    const uint32_t previous_keyframe = s.target_keyframe - 1;

    const float time_offset = animation_time - TimeStamps[previous_keyframe];
    const float time_ratio = time_offset / (TimeStamps[s.target_keyframe] - TimeStamps[previous_keyframe]);

    set_target_value(s, lerp(KeyFrames[previous_keyframe], KeyFrames[s.target_keyframe], time_ratio));

    ChannelStates[threadID].target_keyframe = s.target_keyframe;
}
