#ifndef MT6580_H
#define MT6580_H

#include "hw/core/sysbus.h"
#include "hw/arm/boot.h"
#include "hw/timer/mtk_gpt.h"
#include "hw/misc/mt6580_efusec.h"
#include "hw/misc/mt6580_sej.h"
#include "hw/sd/mtk-msdc.h"
#include "hw/misc/mt6580_spm.h"
#include "hw/misc/mt6580_pwrap.h"
#include "hw/misc/mt6350.h"
#include "hw/misc/mt6580_dramc.h"

#define NUM_UARTS 2
#define NUM_MSDCS 2

struct MT6580State {
    SysBusDevice parent_obj;

    ARMCPU *cpu[4];

    MtkGptState gpt;
    Mt6580EfusecState efusec;
    Mt6580SejState sej;
    MtkMsdcState msdc[NUM_MSDCS];
    Mt6580SpmState spm;
    Mt6580PwrapState pwrap;
    Mt6350State pmic;
    Mt6580DramcState dramc;
};

#define TYPE_MT6580_SOC "mt6580"
OBJECT_DECLARE_SIMPLE_TYPE(MT6580State, MT6580_SOC)

#endif /* MT6580_H */
