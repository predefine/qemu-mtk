#include "qemu/osdep.h"
#include "qemu/bswap.h"
#include "hw/sd/sd.h"
#include "qemu/log.h"
#include "hw/core/qdev-properties.h"
#include "hw/sd/mtk-msdc.h"

#define MSDC_FIFO_THRESHOLD 0x80

static void mtk_msdc_clear_fifo(MtkMsdcState *state)
{
    state->blocks = 0;
    state->block_len = 0;
    state->current_block = 0;
    state->current_offset = 0;
}

static uint64_t mtk_msdc_get_fifo_rx_bytes(MtkMsdcState *state)
{
    uint64_t remain_of_this_block = state->block_len - state->current_offset;
    if (state->blocks > state->current_block)
        return MIN(remain_of_this_block + state->block_len * state->blocks, MSDC_FIFO_THRESHOLD);
    else
        return MIN(remain_of_this_block, MSDC_FIFO_THRESHOLD);
}

static uint64_t mtk_msdc_read(void *o, hwaddr offset, unsigned int size)
{
    uint64_t ret = 0;
    MtkMsdcState *state = MTK_MSDC(o);

    switch (offset)
    {
        case 0x0:
            return (1 << 7) | (1 << 6) | (1 << 0); // CLOCK_STABLE | BV18PASS | MSDC
        case 0x4:
            return state->msdc_iocon;
        case 0x8:
            return 0xf << 16;
        case 0xc:
            // TODO: msdc interrupts
            ret = state->msdc_int;
            break;
        case 0x10:
            return state->msdc_int_en;
        case 0x14:

            ret |= mtk_msdc_get_fifo_rx_bytes(state) & 0xff;

            qemu_log_mask(LOG_UNIMP, "mtk_msdc: fifo value: %.8lx, %.8x, %.8x\n", mtk_msdc_get_fifo_rx_bytes(state), state->blocks, state->block_len);

            break;
        case 0x1c:
            qemu_log_mask(LOG_UNIMP, "mtk_msdc: fifo rxdbg: %.8lx, %.8x, %.8x\n", mtk_msdc_get_fifo_rx_bytes(state), state->blocks, state->block_len);
            if (mtk_msdc_get_fifo_rx_bytes(state) < size)
                size = mtk_msdc_get_fifo_rx_bytes(state);
            assert(state->block_len >= state->current_offset + size); // read at edge of blocks
            for (uint32_t i = 0; i < size; i++)
            {
                ret |= sdbus_read_byte(&state->sdbus) << i * 8;
                state->current_offset++;
            }
            if (state->current_offset == state->block_len)
            {
                state->current_block++;
                state->current_offset = 0;
                if (state->current_block > state->blocks)
                {
                    mtk_msdc_clear_fifo(state);
                    state->msdc_int |= 1 << 12; // MSDC_INT |= DATA_XFER_COMPLETE
                }
            }
            break;
        case 0x30:
            return state->sdc_cfg;
        case 0x34:
            return state->sdc_cmd;
        case 0x38:
            return state->sdc_arg;
        case 0x3c: // SDC_STS
            return 0;
        case 0x40:
        case 0x44:
        case 0x48:
        case 0x4c:
            return state->sdc_resp[(offset - 0x40) >> 2];
        case 0x50:
            return state->sdc_blknum;
        case 0x78:
            // TODO: emmc status
            return state->emmc_sts;
        case 0x7c:
            return state->emmc_iocon;
        case 0xb8:
            return state->patch_bit2;
        case 0xf0:
            return state->pad_tune0;
        default:
            qemu_log_mask(LOG_UNIMP, "mtk_msdc: unimplemented device read  "
            "(size %d, offset 0x%0*" HWADDR_PRIx ")\n",
                        size, 2, offset);
    }
    return ret;
}

static void mtk_msdc_write(void *o, hwaddr offset,
                           uint64_t value, unsigned int size)
{
    MtkMsdcState *state = MTK_MSDC(o);
    switch (offset)
    {
        case 0x0: {
            if (value & (1 << 2))
            {
                // TODO: reset
            }
            break;
        }
        case 0x4:
            state->msdc_iocon = value;
            break;
        case 0xc:
            state->msdc_int &= ~value;
            break;
        case 0x10:
            state->msdc_int_en = value;
            break;
        case 0x14: {
            if (value & (1 << 31))
            {
                // TODO: clear fifo
                mtk_msdc_clear_fifo(state);

                // TODO: sdbus reset
            }
            break;
        }
        case 0x30:
            state->sdc_cfg = value;
            break;
        case 0x34:
        {
            state->sdc_cmd = value;
            if (state->fuck_the_mmc)
            {
                state->msdc_int |= (1 << 9); // msdc_int |= SD_CMD_TIMEOUT
                return;
            }

            uint8_t resp[16];

            for (uint32_t i = 0; i < 4; i++)
                state->sdc_resp[i] = 0;

            SDRequest request = {};
            request.cmd = state->sdc_cmd & 0x3f;
            request.arg = state->sdc_arg;

            uint32_t cmd_dtype = (state->sdc_cmd >> 11) & 3;
            uint32_t block_len = (state->sdc_cmd >> 16) & 0xfff;
            uint32_t blocks = state->sdc_blknum;

            size_t rlen = sdbus_do_command(&state->sdbus, &request, resp, sizeof(resp));

            if (rlen == 4) {
                state->sdc_resp[0] = ldl_be_p(resp);
            } else if (rlen == 16) {
                state->sdc_resp[0] = ldl_be_p(&resp[11]);
                state->sdc_resp[1] = ldl_be_p(&resp[7]);
                state->sdc_resp[2] = ldl_be_p(&resp[3]);
                state->sdc_resp[3] = (resp[0] << 16) | (resp[1] << 8) | (resp[2]);
            } else if (cmd_dtype != 0) {
                state->msdc_int |= (1 << 9); // msdc_int |= SD_CMD_TIMEOUT
            }


            if (!(state->msdc_int & (1 << 9)))
                state->msdc_int |= (1 << 8); // msdc_int |= SD_CMD_RDY

            if (cmd_dtype == 1)
                blocks = 1;

            if (cmd_dtype != 0)
            {
                state->blocks = blocks;
                state->block_len = block_len;
                state->current_block = 1;
                state->current_offset = 0;
            }

            qemu_log_mask(LOG_UNIMP, "mtk_msdc: cmd call(%d), arg=%x, rlen=%ld\n",
                          state->sdc_cmd & 0b111111, state->sdc_arg, rlen);

            for (uint32_t i = 0x40; i < 0x50; i+=4)
                qemu_log_mask(LOG_UNIMP, "mtk_msdc: sdc_resp[%.2x]: %.8x\n", i, state->sdc_resp[(i - 0x40) >> 2]);
            break;
        }
        case 0x38:
            state->sdc_arg = value;
            break;
        case 0x50:
            state->sdc_blknum = value;
            break;
        case 0x78:
            state->emmc_sts &= ~value;
            break;
        case 0x7c:
            state->emmc_iocon = value;
            if (value & 1) {
                // TODO: enter to boot-up/pre-idle emmc state

                // set boot-up mode bit for emmc_cfg0
                //state->emmc_cfg0 |= 1 << 2
            }
            break;
        case 0xb8:
            state->patch_bit2 = value;
            break;
        case 0xf0:
            state->pad_tune0 = value;
            break;
        default:
            qemu_log_mask(LOG_UNIMP, "mtk_msdc: unimplemented device write "
            "(size %d, offset 0x%0*" HWADDR_PRIx
            ", value 0x%0*" PRIx64 ")\n",
            size, 2, offset, size << 1, value);
    }
    return ;
}

static const MemoryRegionOps mtk_msdc_ops = {
    .read = mtk_msdc_read,
    .write = mtk_msdc_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 1,
        .max_access_size = 4
    }
};

static const Property mtk_msdc_properties[] = {
    DEFINE_PROP_BOOL("fuck-the-mmc", MtkMsdcState, fuck_the_mmc, false),
};


static void mtk_msdc_instance_init(Object *obj)
{
    MtkMsdcState *state = MTK_MSDC(obj);

    qbus_init(&state->sdbus, sizeof(state->sdbus), TYPE_SD_BUS, DEVICE(state), "sd-bus");

    memory_region_init_io(&state->mmio, OBJECT(state), &mtk_msdc_ops,
                          state, TYPE_MTK_MSDC, 0x120);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &state->mmio);
}

static void mtk_msdc_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    device_class_set_props(dc, mtk_msdc_properties);
}

static const TypeInfo mtk_msdc_info = {
    .name = TYPE_MTK_MSDC,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_init = mtk_msdc_instance_init,
    .instance_size = sizeof(MtkMsdcState),
    .class_init = mtk_msdc_class_init,
};

static void mtk_msdc_register_types(void)
{
    type_register_static(&mtk_msdc_info);
}

type_init(mtk_msdc_register_types)
