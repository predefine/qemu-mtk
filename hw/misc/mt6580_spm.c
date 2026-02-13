#include "qemu/osdep.h"
#include "qemu/log.h"
#include <stdint.h>
#include "hw/misc/mt6580_spm.h"

static uint64_t mt6580_spm_read(void *o, hwaddr offset, unsigned int size)
{
    uint64_t ret = 0;

    switch (offset) {
        case 0x60C: // SPM_PWR_STATUS
        case 0x610: // SPM_PWR_STATUS_2ND
            ret = 1 << 3; // DIS_PWR_STA_MASK
            break;
        default:
            qemu_log_mask(LOG_UNIMP, "mt6850_spm: unimplemented device read  "
                "(size %d, offset 0x%0*" HWADDR_PRIx ")\n",
                          size, 2, offset);
    }
    return ret;
}


static void mt6580_spm_write(void *o, hwaddr offset,
            uint64_t value, unsigned int size)
{
    switch (offset) {
        default:
            qemu_log_mask(LOG_UNIMP, "mt6580_spm: unimplemented device write "
                "(size %d, offset 0x%0*" HWADDR_PRIx
                ", value 0x%0*" PRIx64 ")\n",
                size, 2, offset, size << 1, value);
    }
}

static const MemoryRegionOps mt6580_spm_ops = {
    .read = mt6580_spm_read,
    .write = mt6580_spm_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4
    }
};

static void mt6580_spm_instance_init(Object *obj)
{
    Mt6580SpmState *state = MT6580_SPM(obj);

    memory_region_init_io(&state->mmio, OBJECT(state), &mt6580_spm_ops,
                          state, TYPE_MT6580_SPM, 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &state->mmio);
}

static const TypeInfo mt6580_spm_info = {
    .name = TYPE_MT6580_SPM,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_init = mt6580_spm_instance_init,
    .instance_size = sizeof(Mt6580SpmState),
};

static void mt6580_spm_register_types(void)
{
    type_register_static(&mt6580_spm_info);
}

type_init(mt6580_spm_register_types)
