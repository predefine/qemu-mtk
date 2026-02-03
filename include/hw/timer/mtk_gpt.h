#ifndef MTK_GPT_H
#define MTK_GPT_H

#include "hw/core/sysbus.h"
#include "hw/core/ptimer.h"
#include "qom/object.h"

#define TYPE_MTK_GPT     "mtk-gpt"
OBJECT_DECLARE_SIMPLE_TYPE(MtkGptState, MTK_GPT)

#define NUM_TIMERS        6

struct MtkGptTimer {
    ptimer_state *ptimer;
    uint32_t clock_divider;

    uint64_t regs[4];
};

struct MtkGptState {
    SysBusDevice parent_obj;

    MemoryRegion mmio;
    struct MtkGptTimer timers[NUM_TIMERS];
};

#endif /* MTK_GPT_H */
