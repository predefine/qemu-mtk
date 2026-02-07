#ifndef MT6580_H
#define MT6580_H

#include "hw/core/sysbus.h"
#include "hw/arm/boot.h"
#include "hw/timer/mtk_gpt.h"
#include "hw/misc/mt6580_efusec.h"
#include "hw/misc/mt6580_sej.h"

struct MT6580State {
    SysBusDevice parent_obj;

    ARMCPU *cpu[4];

    MtkGptState gpt;
    Mt6580EfusecState efusec;
    Mt6580SejState sej;
};

#define TYPE_MT6580_SOC "mt6580"
OBJECT_DECLARE_SIMPLE_TYPE(MT6580State, MT6580_SOC)

#endif /* MT6580_H */
