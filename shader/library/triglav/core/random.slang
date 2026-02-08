implementing core;

namespace triglav
{

public static constexpr uint RANDOM_MULTIPLIER = 0xCE66DU;
public static constexpr uint RANDOM_ADDEND     = 0xBU;
public static constexpr uint RANDOM_MASK       = (1U << 24U) - 1;

public struct RandomState
{
    uint state;

    public __init(uint seed)
    {
        this.state = seed;
    }

    [mutating]
    void next_state()
    {
        this.state = (this.state * RANDOM_MULTIPLIER + RANDOM_ADDEND) & RANDOM_MASK;
    }

    [mutating]
    public int next_int(int min, int max)
    {
        let value = int(this.state >> 16U);
        let result = min + value % (max - min);
        this.next_state();
        return result;
    }

    [mutating]
    float random_next_float(uint seed)
    {
        return float(this.next_int(0, 1000)) / 1000.0;
    }
};

}