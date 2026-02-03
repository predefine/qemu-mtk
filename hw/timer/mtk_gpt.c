#include "qemu/osdep.h"
#include "qemu/log.h"
#include "hw/core/ptimer.h"
#include "hw/timer/mtk_gpt.h"

#define MTK_GPT_SYSTEM_CLOCK 26000000 /* 13 MHZ */
#define MTK_GPT_RTC_CLOCK 32768

uint32_t mtk_gpt_divs[16] = {
    1,
    2,
    3,
    4,
    5,
    6,
    8,
    9,
    10,
    11,
    12,
    13,
    16,
    32,
    64
};

static uint64_t mtk_gpt_read(void *o, hwaddr offset, unsigned int size)
{
    MtkGptState *state = MTK_GPT(o);

    hwaddr timer_number = (offset >> 4) - 1;

    // TODO: implement 0x00..0x10 registers and gpt6 64bit
    if (timer_number > 5)
        return 0;

    struct MtkGptTimer* timer = &state->timers[timer_number];

    switch (offset & 0xf) {
        case 0x0: // GPTx_CON
            return timer->regs[(offset & 0xf) >> 2];
        case 0x4: // GPTx_CLK
            return timer->regs[(offset & 0xf) >> 2];
        case 0x8:
            return 0xffffffff - ptimer_get_count(timer->ptimer);
        case 0xc:
            return 0;
    }

    return 0;
}


static void mtk_gpt_write(void *o, hwaddr offset,
            uint64_t value, unsigned int size)
{
    MtkGptState *state = MTK_GPT(o);

    hwaddr timer_number = (offset >> 4) - 1;

    // TODO: implement 0x00..0x10 registers and gpt6 64bit
    if (timer_number > 5)
        return;

    struct MtkGptTimer* timer = &state->timers[timer_number];

    timer->regs[(offset & 0xf) >> 2] = 0;

    switch (offset & 0xf) {
        case 0x0: // GPTx_CON
            ptimer_transaction_begin(timer->ptimer);

            if (value & (1 << 1)) // GPT_CLR
                ptimer_set_count(timer->ptimer, 0);

            if (value & (1 << 0)) // GPT_EN
            {
                timer->regs[(offset & 0xf) >> 2] |= 1 << 0;
                ptimer_run(timer->ptimer, 0);
            } else
                ptimer_stop(timer->ptimer);

            ptimer_transaction_commit(timer->ptimer);
            break;

        case 0x4: // GPTx_CLK
            ptimer_transaction_begin(timer->ptimer);

            timer->clock_divider = value & 0xf;
            timer->regs[(offset & 0xf) >> 2] |= value & 0xf;

            if (value & (1 << 4)) // GPT_CLK_SOURCE
            {
                timer->regs[(offset & 0xf) >> 2] |= 1 << 4;
                ptimer_set_freq(timer->ptimer,
                                MTK_GPT_RTC_CLOCK / mtk_gpt_divs[timer->clock_divider]);
            } else {
                ptimer_set_freq(timer->ptimer,
                                MTK_GPT_SYSTEM_CLOCK / mtk_gpt_divs[timer->clock_divider]);
            }

            ptimer_transaction_commit(timer->ptimer);
            break;
        case 0x8:
            break;
        case 0xc:
            // TODO: implement "COMPARE"
            break;
    }

}

static const MemoryRegionOps mtk_gpt_ops = {
    .read = mtk_gpt_read,
    .write = mtk_gpt_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4
    }
};

static void mtk_gpt_hit(void *o)
{

}

static void mtk_gpt_instance_init(Object *obj)
{
    MtkGptState *state = MTK_GPT(obj);

    for (int i = 0; i < NUM_TIMERS; i++)
    {
        struct MtkGptTimer *timer = &state->timers[i];
        timer->clock_divider = 0;

        timer->ptimer = ptimer_init(mtk_gpt_hit, timer, PTIMER_POLICY_LEGACY);
        ptimer_transaction_begin(timer->ptimer);
        ptimer_set_freq(timer->ptimer, MTK_GPT_RTC_CLOCK);
        ptimer_set_limit(timer->ptimer, 0XFFFFFFFFUL, 1);
        ptimer_transaction_commit(timer->ptimer);

        // TODO: add irq support
        // sysbus_init_irq(SYS_BUS_DEVICE(obj), &st->irq);
    }

    memory_region_init_io(&state->mmio, OBJECT(state), &mtk_gpt_ops,
                          state, TYPE_MTK_GPT, 32 << 2);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &state->mmio);
}

static const TypeInfo mtk_gpt_info = {
    .name = TYPE_MTK_GPT,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_init = mtk_gpt_instance_init,
    .instance_size = sizeof(MtkGptState),
};

static void mtk_gpt_register_types(void)
{
    type_register_static(&mtk_gpt_info);
}

type_init(mtk_gpt_register_types)
