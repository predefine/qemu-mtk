#include "qemu/osdep.h"
#include "qemu/log.h"
#include <stdint.h>
#include "hw/misc/mt6580_dramc.h"

static uint64_t mt6580_dramc_read(void *o, hwaddr offset, unsigned int size)
{
    uint64_t ret = 0;

    switch (offset) {
        case 0x3c0:
            return 0x40404040;
        case 0x3c4:
            return 0x40404040;
        case 0x3fc:
            return 1 << 10;
        default:
            qemu_log_mask(LOG_UNIMP, "mt6850_dramc: unimplemented device read  "
                "(size %d, offset 0x%0*" HWADDR_PRIx ")\n",
                          size, 2, offset);
    }
    return ret;
}

static void mt6580_dramc_write(void *o, hwaddr offset,
            uint64_t value, unsigned int size)
{
    switch (offset) {
        default:
            qemu_log_mask(LOG_UNIMP, "mt6580_dramc: unimplemented device write "
                "(size %d, offset 0x%0*" HWADDR_PRIx
                ", value 0x%0*" PRIx64 ")\n",
                size, 2, offset, size << 1, value);
    }
}

static const MemoryRegionOps mt6580_dramc_ops = {
    .read = mt6580_dramc_read,
    .write = mt6580_dramc_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4
    }
};

static void mt6580_dramc_instance_init(Object *obj)
{
    Mt6580DramcState *state = MT6580_DRAMC(obj);

    memory_region_init_io(&state->mmio, OBJECT(state), &mt6580_dramc_ops,
                          state, TYPE_MT6580_DRAMC, 0x400);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &state->mmio);
}

static const TypeInfo mt6580_dramc_info = {
    .name = TYPE_MT6580_DRAMC,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_init = mt6580_dramc_instance_init,
    .instance_size = sizeof(Mt6580DramcState),
};

static void mt6580_dramc_register_types(void)
{
    type_register_static(&mt6580_dramc_info);
}

type_init(mt6580_dramc_register_types)
