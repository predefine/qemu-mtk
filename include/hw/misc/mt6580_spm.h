#ifndef MT6580_SPM_H
#define MT6580_SPM_H

#include "hw/core/sysbus.h"
#include "qom/object.h"

#define TYPE_MT6580_SPM     "mt6580-spm"
OBJECT_DECLARE_SIMPLE_TYPE(Mt6580SpmState, MT6580_SPM)

struct Mt6580SpmState {
    SysBusDevice parent_obj;

    MemoryRegion mmio;
};

#endif /* MT6580_SPM_H */
