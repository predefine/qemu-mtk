#ifndef MT6580_H
#define MT6580_H

#include "hw/core/sysbus.h"
#include "hw/arm/boot.h"
#include "hw/timer/mtk_gpt.h"

struct MT6580State {
    SysBusDevice parent_obj;
    ARMCPU *cpu[4];
    MtkGptState gpt;
//     qemu_irq irq_table[EXYNOS4210_MAX_INT_COMBINER_IN_IRQ];
//
//     MemoryRegion chipid_mem;
//     MemoryRegion iram_mem;
//     MemoryRegion irom_mem;
//     MemoryRegion irom_alias_mem;
//     MemoryRegion boot_secondary;
//     MemoryRegion bootreg_mem;
//     I2CBus *i2c_if[EXYNOS4210_I2C_NUMBER];
//     OrIRQState pl330_irq_orgate[EXYNOS4210_NUM_DMA];
//     OrIRQState cpu_irq_orgate[EXYNOS4210_NCPUS];
//     A9MPPrivState a9mpcore;
//     Exynos4210GicState ext_gic;
//     Exynos4210CombinerState int_combiner;
//     Exynos4210CombinerState ext_combiner;
//     SplitIRQ splitter[EXYNOS4210_NUM_SPLITTERS];
};

#define TYPE_MT6580_SOC "mt6580"
OBJECT_DECLARE_SIMPLE_TYPE(MT6580State, MT6580_SOC)

#endif /* MT6580_H */
