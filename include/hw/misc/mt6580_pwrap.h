#ifndef MT6580_PWRAP_H
#define MT6580_PWRAP_H

#include "hw/core/sysbus.h"
#include "qom/object.h"
#include "hw/misc/mtk_pmic.h"

#define TYPE_MT6580_PWRAP     "mt6580-pwrap"
OBJECT_DECLARE_SIMPLE_TYPE(Mt6580PwrapState, MT6580_PWRAP)


struct Mt6580PwrapState {
    SysBusDevice parent_obj;
    MemoryRegion mmio;

    MtkPmicState *pmic;

    uint32_t wacs2_fsm;
    uint32_t wacs2_value;

    bool mux_manual;
    bool wrap_enable;
    bool dio_enable;
    bool man_enable;
    bool wacs2_enable;
    bool swrst;
};

#endif /* MT6580_PWRAP_H */
