#include "qemu/osdep.h"
#include "qemu/log.h"
#include <stdint.h>
#include "hw/misc/mt6580_sej.h"

static uint64_t mt6580_sej_read(void *o, hwaddr offset, unsigned int size)
{
    Mt6580SejState *state = MT6580_SEJ(o);

    switch (offset) {
        case 0x0: // HACC_CON
            return 0 | 0x3; // 0x3 - sej revision? idk, but bootrom requires this to be any number from 1 to 3
        case 0x4: // HACC_ACON
            return state->aes_mode;
        case 0x8: // HACC_ACON2
            return state->aes_state;
        case 0xc: // HACC_ACONK
            return state->aes_key_mode;
        case 0x10: // HACC_ASRCn
        case 0x14:
        case 0x18:
        case 0x1c:
            return state->aes_in[(offset - 0x10) >> 2];
        case 0x20: // HACC_AKEYn
        case 0x24:
        case 0x28:
        case 0x2c:
        case 0x30:
        case 0x34:
        case 0x38:
        case 0x3c:
            return state->aes_key[(offset - 0x20) >> 2];
        case 0x40: // HACC_ACFGn(SEJ_AIVn in preloader source code)
        case 0x44:
        case 0x48:
        case 0x4c:
            return state->aes_iv[(offset - 0x40) >> 2];
        case 0x50: // HACC_AOUTn
        case 0x54:
        case 0x58:
        case 0x5c:
            if (state->aes_state & 0x8000) // if (AES_READY)
                return state->aes_out[(offset - 0x50) >> 2];
            return 0;
        case 0x60: // HACC_SW_OTPn
        case 0x64:
        case 0x68:
        case 0x6c:
        case 0x70:
        case 0x74:
        case 0x78:
        case 0x7c:
            return state->sw_otp[(offset - 0x60) >> 2];
        default:
            qemu_log_mask(LOG_UNIMP, "mt6850_sej: unimplemented device read  "
                "(size %d, offset 0x%0*" HWADDR_PRIx ")\n",
                          size, 2, offset);
            return 0;
    }
}


static void mt6580_sej_write(void *o, hwaddr offset,
            uint64_t value, unsigned int size)
{
    Mt6580SejState *state = MT6580_SEJ(o);

    switch (offset) {
        case 0x0: // HACC_CON, can't find any information about this reg, probably "enable clock" bit
            break;
        case 0x4: // HACC_ACON
            state->aes_mode = 0;

            if (value & 0x1) {
                state->aes_mode |= 0x1;
                // AES ENCODE
            } else {
                // AES DECODE
            }

            if (value & 0x2) {
                state->aes_mode |= 0x2;
                // AES CBC
            } else {
                // AES EBC
            }

            if (value & 0x10) {
                state->aes_mode |= 0x10;
                // AES 192
            } else if (value & 0x20) {
                state->aes_mode |= 0x20;
                // AES 256
            } else {
                // AES 128
            }

            break;
        case 0x8: // HACC_ACON2
            if (value & 0x2) { // AES_CLR
                // TODO: reset
                return;
            }
            if (value & 0x1) { // AES_START
                // TODO: real aes decode or encode


                for (int i = 0; i < 4; i++)
                    state->aes_out[i] = state->aes_in[i];

                state->aes_state |= 0x8000; // AES_READY
            }

            break;
        case 0xc: // HACC_ACONK
            if (value & 0x10) { // HACC_AES_BK2C
                state->aes_key_mode |= 0x10;
                // bind hardware key to hacc?
            }

            if (value & 0x100) { // HACC_AES_R2K
                state->aes_key_mode |= 0x100;
                // r2k, seems like "KEY = OUTPUT" mode
            }
            break;
        case 0x10: // HACC_ASRCn
        case 0x14:
        case 0x18:
        case 0x1c:
            state->aes_in[(offset - 0x10) >> 2] = value;
            break;
        case 0x20: // HACC_AKEYn
        case 0x24:
        case 0x28:
        case 0x2c:
        case 0x30:
        case 0x34:
        case 0x38:
        case 0x3c:
            state->aes_key[(offset - 0x20) >> 2] = value;
            break;
        case 0x40: // HACC_ACFGn(SEJ_AIVn in preloader source code)
        case 0x44:
        case 0x48:
        case 0x4c:
            state->aes_iv[(offset - 0x40) >> 2] = value;
            break;
        case 0x50: // HACC_AOUTn (lets hope thats won't happened ever)
        case 0x54:
        case 0x58:
        case 0x5c:
            break;
        case 0x60: // HACC_SW_OTPn
        case 0x64:
        case 0x68:
        case 0x6c:
        case 0x70:
        case 0x74:
        case 0x78:
        case 0x7c:
            state->sw_otp[(offset - 0x60) >> 2] = value;
            break;
        default:
            qemu_log_mask(LOG_UNIMP, "mt6580_sej: unimplemented device write "
                "(size %d, offset 0x%0*" HWADDR_PRIx
                ", value 0x%0*" PRIx64 ")\n",
                size, 2, offset, size << 1, value);
    }
}

static const MemoryRegionOps mt6580_sej_ops = {
    .read = mt6580_sej_read,
    .write = mt6580_sej_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4
    }
};

static void mt6580_sej_instance_init(Object *obj)
{
    Mt6580SejState *state = MT6580_SEJ(obj);

    memory_region_init_io(&state->mmio, OBJECT(state), &mt6580_sej_ops,
                          state, TYPE_MT6580_SEJ, 0xff);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &state->mmio);
}

static const TypeInfo mt6580_sej_info = {
    .name = TYPE_MT6580_SEJ,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_init = mt6580_sej_instance_init,
    .instance_size = sizeof(Mt6580SejState),
};

static void mt6580_sej_register_types(void)
{
    type_register_static(&mt6580_sej_info);
}

type_init(mt6580_sej_register_types)
