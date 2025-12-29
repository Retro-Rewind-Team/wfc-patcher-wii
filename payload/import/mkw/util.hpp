#pragma once

#if RMC


namespace wwfc::mkw::Util
{

class Random
{
public:
    Random* dt(Random* random, s32 type)
    {
        [[gnu::longcall]] Random* dt( //
            Random * random, s32 type
        ) AT(RMCXD_PORT(0x80555538, 0x8054F518, 0x80554EB8, 0x80543590, DEMOTODO));

        return dt(this, type);
    }

    u32 nextInt()
    {
        [[gnu::longcall]] u32 nextInt( //
            Random * random
        ) AT(RMCXD_PORT(0x80555578, 0x8054F558, 0x80554EF8, 0x805435D0, DEMOTODO));

        return nextInt(this);
    }

    u32 nextInt(u32 limit)
    {
        [[gnu::longcall]] u32 nextInt( //
            Random * random, u32 limit
        ) AT(RMCXD_PORT(0x805555CC, 0x8054F5AC, 0x80554F4C, 0x80543624, DEMOTODO));

        return nextInt(this, limit);
    }

private:
    /* 0x00 */ u8 _00[0x18 - 0x00];
};

static_assert(sizeof(Random) == 0x18);

} // namespace wwfc::mkw::Util

#endif // RMC