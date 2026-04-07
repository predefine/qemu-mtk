#include "qemu/osdep.h"
#include "qemu/log.h"
#include "hw/misc/mt6580_pwrap.h"
#include "hw/core/qdev-properties.h"

static uint64_t mt6580_pwrap_read(void *o, hwaddr offset, unsigned int size)
{
    uint64_t ret = 0;
    Mt6580PwrapState *state = MT6580_PWRAP(o);

    switch (offset)
    {
        case 0x94: // WACS2_EN
            ret = state->wacs2_enable;
            break;
        case 0x98: // WACS2_INITDONE
            ret = 1;
            break;
        case 0xa0: // WACS2_RDATA
            ret |= 1 << 21; // WACS2_RDATA |= INIT_DONE
            ret |= 1 << 20; // WACS2_RDATA |= SYNC_IDLE
            ret |= ((state->wacs2_fsm) & 7) << 16;
            ret |= state->wacs2_value & 0xffff;
            break;
        case 0x130: // CIPHER_RDY
            ret = 1;
            break;
        case 0x180:
            ret = state->swrst;
            break;
        default:
            qemu_log_mask(LOG_UNIMP, "mt6580_pwrap: unimplemented device read  "
            "(size %d, offset 0x%0*" HWADDR_PRIx ")\n",
                          size, 2, offset);
    }
    return ret;
}


static void mt6580_pwrap_write(void *o, hwaddr offset,
            uint64_t value, unsigned int size)
{
    Mt6580PwrapState *state = MT6580_PWRAP(o);

    switch (offset)
    {
        case 0x00: // MUX_SEL
            state->mux_manual = value & 1;
            break;
        case 0x04: // WRAP_EN
            state->wrap_enable = value & 1;
            break;
        case 0x08: // DIO_EN
            state->dio_enable = value & 1;
            break;
        case 0x5c: // MAN_EN
            state->man_enable = value & 1;
            break;
        case 0x60: // MAN_CMD
            if (state->man_enable == 0)
                return;
            break;
        case 0x94: // WACS2_EN
            state->wacs2_enable = value & 1;
            break;
        case 0x9c: // WACS2_CMD
            if (state->wacs2_enable == 0 || state->wrap_enable == 0)
                return;
            uint16_t addr = ((value >> 16) & 0x7fff) << 1;
            uint16_t write_value = value & 0xffff;
            MtkPmicState *pmic = MTK_PMIC(state->pmic);
            MtkPmicClass *pmic_class = MTK_PMIC_GET_CLASS(pmic);

            if (value & (1 << 31))
                pmic_class->write_reg(pmic, addr, write_value);
            else
            {
                state->wacs2_value = pmic_class->read_reg(pmic, addr);
                state->wacs2_fsm = 0x6; // VLDCLR
            }
            break;
        case 0xa4: // WACS2_VLDCLR
            state->wacs2_fsm = 0; // IDLE
            state->wacs2_value = 0;

            break;
        case 0x180: // SWRST
            state->swrst = value & 1;

            if (state->swrst) {
                // TODO: reset wacs
            }
            break;
        default:
            qemu_log_mask(LOG_UNIMP, "mt6580_pwrap: unimplemented device write "
            "(size %d, offset 0x%0*" HWADDR_PRIx
            ", value 0x%0*" PRIx64 ")\n",
            size, 2, offset, size << 1, value);
    }
    return ;
}

static const MemoryRegionOps mt6580_pwrap_ops = {
    .read = mt6580_pwrap_read,
    .write = mt6580_pwrap_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4
    }
};

static void mt6580_pwrap_instance_init(Object *obj)
{
    Mt6580PwrapState *state = MT6580_PWRAP(obj);

    memory_region_init_io(&state->mmio, OBJECT(state), &mt6580_pwrap_ops,
                          state, TYPE_MT6580_PWRAP, 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &state->mmio);

    object_property_add_link(OBJECT(state), "pmic", TYPE_MTK_PMIC, (Object**) &state->pmic, qdev_prop_allow_set_link_before_realize, 0);
    state->wrap_enable = 1;
    state->wacs2_enable = 1;
}

static const TypeInfo mt6580_pwrap_info = {
    .name = TYPE_MT6580_PWRAP,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_init = mt6580_pwrap_instance_init,
    .instance_size = sizeof(Mt6580PwrapState),
};

static void mt6580_pwrap_register_types(void)
{
    type_register_static(&mt6580_pwrap_info);
}

type_init(mt6580_pwrap_register_types)
