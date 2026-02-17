#ifndef MT6580_DRAMC_H
#define MT6580_DRAMC_H

#include "hw/core/sysbus.h"
#include "qom/object.h"

#define TYPE_MT6580_DRAMC     "mt6580-dramc"
OBJECT_DECLARE_SIMPLE_TYPE(Mt6580DramcState, MT6580_DRAMC)

struct Mt6580DramcState {
    SysBusDevice parent_obj;

    MemoryRegion mmio;
};

#endif /* MT6580_DRAMC_H */
