#ifndef MT6580_EFUSEC_H
#define MT6580_EFUSEC_H

#include "hw/core/sysbus.h"
#include "qom/object.h"

#define TYPE_MT6580_EFUSEC     "mt6580-efusec"
OBJECT_DECLARE_SIMPLE_TYPE(Mt6580EfusecState, MT6580_EFUSEC)


struct Mt6580EfusecState {
    SysBusDevice parent_obj;

    MemoryRegion mmio;
    uint32_t efusec_con;
};

#endif /* MT6580_EFUSEC_H */
