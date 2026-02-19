#ifndef MTK_MSDC_H
#define MTK_MSDC_H

#include "hw/core/sysbus.h"
#include "qom/object.h"
#include "hw/sd/sd.h"
#include "qemu/fifo8.h"

#define TYPE_MTK_MSDC     "mtk-msdc"
OBJECT_DECLARE_SIMPLE_TYPE(MtkMsdcState, MTK_MSDC)

struct MtkMsdcState {
    SysBusDevice parent_obj;

    MemoryRegion mmio;
    bool fuck_the_mmc;
    SDBus sdbus;

    // dma
    uint32_t dma_burst_size;
    uint32_t dma_mode; // 0 = basic, 1 - enchanced
    uint32_t dma_addr;
    uint32_t dma_cfg;

    // CMD_READ
    uint32_t blocks;
    uint32_t block_len;
    uint32_t current_block;
    uint32_t current_offset;

    uint32_t msdc_int;
    uint32_t msdc_int_en;
    uint32_t msdc_iocon;
    uint32_t sdc_cfg;
    uint32_t sdc_arg;
    uint32_t sdc_cmd;
    uint32_t sdc_resp[4];
    uint32_t sdc_blknum;
    uint32_t patch_bit2;
    uint32_t emmc_sts;
    uint32_t emmc_iocon;
    uint32_t pad_tune0;
};

#endif /* MTK_MSDC_H */
