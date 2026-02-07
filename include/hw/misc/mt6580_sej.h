#ifndef MT6580_SEJ_H
#define MT6580_SEJ_H

#include "hw/core/sysbus.h"
#include "qom/object.h"

#define TYPE_MT6580_SEJ     "mt6580-sej"
OBJECT_DECLARE_SIMPLE_TYPE(Mt6580SejState, MT6580_SEJ)

struct Mt6580SejState {
    SysBusDevice parent_obj;

    MemoryRegion mmio;

    uint32_t sw_otp[8]; // idk why this is even exists

    uint32_t aes_mode;
    uint32_t aes_state;
    uint32_t aes_key_mode;
    uint32_t aes_key[8];
    uint32_t aes_iv[4];
    uint32_t aes_in[4];
    uint32_t aes_out[4];
};

#endif /* MT6580_SEJ_H */
