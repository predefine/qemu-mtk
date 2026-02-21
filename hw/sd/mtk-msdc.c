#include "qemu/osdep.h"
#include "qemu/bswap.h"
#include "hw/sd/sd.h"
#include "qemu/log.h"
#include "hw/core/qdev-properties.h"
#include "system/dma.h"
#include "exec/memattrs.h"
#include "hw/core/irq.h"
#include "hw/sd/mtk-msdc.h"

#define MSDC_FIFO_THRESHOLD 0x80

// stolen from preloader source code
typedef struct {
    uint32_t hwo:1;
    uint32_t bdp:1;
    uint32_t rsv0:6;
    uint32_t chksum:8;
    uint32_t intr:1;
    uint32_t rsv1:7;
    uint32_t nexth4:4;
    uint32_t ptrh4:4;
    uint32_t next; // gpd_t*
    uint32_t ptr;  // bd_t*
    uint32_t buflen:24;
    uint32_t extlen:8;
    uint32_t arg;
    uint32_t blknum;
    uint32_t cmd;
} gpd_t;

typedef struct {
    uint32_t eol:1;
    uint32_t rsv0:7;
    uint32_t chksum:8;
    uint32_t rsv1:1;
    uint32_t blkpad:1;
    uint32_t dwpad:1;
    uint32_t rsv2:5;
    uint32_t nexth4:4;
    uint32_t ptrh4:4;
    uint32_t next; // bd_t*
    uint32_t ptr;  // addr to memory
    uint32_t buflen:24;
    uint32_t rsv3:8;
} bd_t;

static void mtk_msdc_update_irq(MtkMsdcState *state)
{
    qemu_set_irq(state->irq, state->msdc_int & state->msdc_int_en);
}

static void mtk_msdc_clear_fifo(MtkMsdcState *state)
{
    state->blocks = 0;
    state->block_len = 0;
    state->current_block = 0;
    state->current_offset = 0;
}

static uint64_t mtk_msdc_get_fifo_rx_bytes_raw(MtkMsdcState *state)
{
    uint64_t remain_of_this_block = state->block_len - state->current_offset;
    if (state->blocks > state->current_block)
        return remain_of_this_block + state->block_len * state->blocks;
    else
        return remain_of_this_block;
}


static uint64_t mtk_msdc_get_fifo_rx_bytes(MtkMsdcState *state)
{
    return MIN(mtk_msdc_get_fifo_rx_bytes_raw(state), MSDC_FIFO_THRESHOLD);
}

static uint32_t mtk_msdc_get_fifo_rx_byte(MtkMsdcState *state, uint8_t *byte)
{
    if (mtk_msdc_get_fifo_rx_bytes_raw(state) < sizeof(*byte))
        return 1;

    *byte = sdbus_read_byte(&state->sdbus);
    state->current_offset++;
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

    return 0;
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
            for (uint32_t i = 0; i < size; i++)
            {
                uint8_t byte = 0;
                assert(0 == mtk_msdc_get_fifo_rx_byte(state, &byte));
                ret |= byte << i * 8;
            }
            mtk_msdc_update_irq(state);
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
        case 0x98:
            return (state->dma_burst_size << 12) | (state->dma_mode << 8) | (1 << 3); // BURST_SIZE | DMA_MODE | AHB_READYM
        case 0x9c:
            return state->dma_cfg;
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
                break;
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
        case 0x90:
            state->dma_addr = value;
            break;
        case 0x98:
        {
            state->dma_mode = (value >> 8) & 1;
            state->dma_burst_size = (value >> 12) & 7;
            if (value & 1) // DMA_START
            {
                assert(state->dma_mode == 1); // TODO: "dma basic mode" isn't implemented yet
                qemu_log_mask(LOG_UNIMP, "mtk_msdc: dma start: %.8x, burst_size=%.8d bytes,mode=%d\n",
                              state->dma_addr, 1 << state->dma_burst_size, state->dma_mode);

                gpd_t gpd = {.next = state->dma_addr};
                do {
                    assert(MEMTX_OK == dma_memory_read(&address_space_memory, gpd.next, &gpd, sizeof(gpd), MEMTXATTRS_UNSPECIFIED));
                    if (gpd.hwo == 0) // "gpd is not null"?
                        break;

                    if (gpd.bdp == 0) // "bd pointer is specified"?
                        continue;

                    bd_t bd = {.next = gpd.ptr};

                    do {
                        assert(MEMTX_OK == dma_memory_read(&address_space_memory, bd.next, &bd, sizeof(bd), MEMTXATTRS_UNSPECIFIED));

                        if (mtk_msdc_get_fifo_rx_bytes_raw(state) < bd.buflen) // hack
                        {
                            bd.buflen = mtk_msdc_get_fifo_rx_bytes_raw(state);
                            bd.next = 0;
                            gpd.next = 0;
                        }

                        for (uint32_t offs = 0; offs < bd.buflen; offs++)
                        {
                            uint8_t byte = 0;
                            assert(0 == mtk_msdc_get_fifo_rx_byte(state, &byte));
                            dma_memory_write(&address_space_memory, bd.ptr + offs, &byte, sizeof(byte), MEMTXATTRS_UNSPECIFIED);
                        }
                    } while (bd.next);
                } while(gpd.next);

                if (mtk_msdc_get_fifo_rx_bytes_raw(state) == 0)
                {
                    mtk_msdc_clear_fifo(state);
                    state->msdc_int |= 1 << 12; // MSDC_INT |= DATA_XFER_COMPLETE
                }
                qemu_log_mask(LOG_UNIMP, "mtk_msdc: dma fifo state: %.8lx\n", mtk_msdc_get_fifo_rx_bytes_raw(state));
            }
            break;
        }
        case 0x9c:
            state->dma_cfg = value & ~1;
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

    mtk_msdc_update_irq(state);

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
                          state, TYPE_MTK_MSDC, 0x240);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &state->mmio);
    sysbus_init_irq(SYS_BUS_DEVICE(obj), &state->irq);
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
