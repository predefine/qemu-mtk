#include "qemu/osdep.h"
#include "qemu/log.h"
#include "hw/misc/mt6580_efusec.h"

static uint64_t mt6580_efusec_read(void *o, hwaddr offset, unsigned int size)
{
    uint64_t ret = 0;
    Mt6580EfusecState *state = MT6580_EFUSEC(o);

    switch (offset)
    {
        case 0x0:
            ret = state->efusec_con;
            state->efusec_con &= ~1;
            break;
        default:
            qemu_log_mask(LOG_UNIMP, "mt6580_efusec: unimplemented device read  "
            "(size %d, offset 0x%0*" HWADDR_PRIx ")\n",
                          size, 2, offset);
    }
    return ret;
}


static void mt6580_efusec_write(void *o, hwaddr offset,
            uint64_t value, unsigned int size)
{
    switch (offset)
    {
        default:
            qemu_log_mask(LOG_UNIMP, "mt6580_efusec: unimplemented device write "
            "(size %d, offset 0x%0*" HWADDR_PRIx
            ", value 0x%0*" PRIx64 ")\n",
            size, 2, offset, size << 1, value);
    }
    return ;
}

static const MemoryRegionOps mt6580_efusec_ops = {
    .read = mt6580_efusec_read,
    .write = mt6580_efusec_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4
    }
};

static void mt6580_efusec_instance_init(Object *obj)
{
    Mt6580EfusecState *state = MT6580_EFUSEC(obj);

    memory_region_init_io(&state->mmio, OBJECT(state), &mt6580_efusec_ops,
                          state, TYPE_MT6580_EFUSEC, 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &state->mmio);

    state->efusec_con = 1; // bootrom expect funny flow of EFUSEC_CON(first read - lsb bit is one, second read - lsb bit is zero)
}

static const TypeInfo mt6580_efusec_info = {
    .name = TYPE_MT6580_EFUSEC,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_init = mt6580_efusec_instance_init,
    .instance_size = sizeof(Mt6580EfusecState),
};

static void mt6580_efusec_register_types(void)
{
    type_register_static(&mt6580_efusec_info);
}

type_init(mt6580_efusec_register_types)
