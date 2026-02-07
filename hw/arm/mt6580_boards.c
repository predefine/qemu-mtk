#include "qemu/osdep.h"
#include "qapi/error.h"
#include "hw/arm/machines-qom.h"
#include "qemu/typedefs.h"
#include "system/address-spaces.h"
#include "cpu.h"
#include "hw/arm/boot.h"
#include "hw/arm/mt6580.h"
#include "qemu/units.h"
#include "qemu/error-report.h"
#include "qemu/datadir.h"
#include "hw/core/loader.h"
#include "system/memory.h"

typedef struct MT6580BoardState {
    MT6580State soc;
    MemoryRegion ram;
} MT6580BoardState;

static struct arm_boot_info mt6580_board_boot_info = {
    .loader_start     = 0x80008000,
    // .smp_loader_start = EXYNOS4210_SMP_BOOT_ADDR,
    // .write_secondary_boot = exynos4210_write_secondary,
};

static void mt6580_test_init(MachineState *machine)
{
    MT6580BoardState *s = g_new(MT6580BoardState, 1);

    object_initialize_child(OBJECT(machine), "soc", &s->soc,
                            TYPE_MT6580_SOC);
    sysbus_realize(SYS_BUS_DEVICE(&s->soc), &error_fatal);
    mt6580_board_boot_info.ram_size = 1 * GiB;

    memory_region_add_subregion(get_system_memory(), 0x80000000, machine->ram);

    MemoryRegion* brom_region = g_new(MemoryRegion, 1);
    memory_region_init_ram(brom_region, NULL, "brom", 128 * KiB, &error_fatal);
    memory_region_add_subregion(get_system_memory(), 0, brom_region);

    MemoryRegion* brom_sram_region = g_new(MemoryRegion, 1);
    memory_region_init_ram(brom_sram_region, NULL, "brom.sram", 64 * KiB, &error_fatal);
    memory_region_add_subregion(get_system_memory(), 0x00100000, brom_sram_region);


    if (machine->firmware) {
        char *fn;
        int image_size;

        if (drive_get(IF_PFLASH, 0, 0)) {
            error_report("The contents of the first flash device may be "
                         "specified with -bios or with -drive if=pflash... "
                         "but you cannot use both options at once");
            exit(1);
        }
        fn = qemu_find_file(QEMU_FILE_TYPE_BIOS, machine->firmware);
        if (!fn) {
            error_report("Could not find ROM image '%s'", machine->firmware);
            exit(1);
        }
        image_size = load_image_targphys(fn, 0, 128 * KiB, &error_fatal);
        g_free(fn);
        if (image_size < 0) {
            error_report("Could not load ROM image '%s'", machine->firmware);
            exit(1);
        }
    }
    // arm_load_kernel(s->soc.cpu[0], machine, &mt6580_board_boot_info);
}

static const char * const valid_cpu_types[] = {
    ARM_CPU_TYPE_NAME("cortex-a7"),
    NULL
};

static void mt6580_test_class_init(ObjectClass *oc, const void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);

    mc->desc = "MT6580 test board (MT6580)";
    mc->init = mt6580_test_init;
    mc->valid_cpu_types = valid_cpu_types;
    mc->max_cpus = 1;
    mc->min_cpus = 1;
    mc->default_cpus = 1;
    mc->ignore_memory_transaction_failures = false;
    mc->default_ram_size = 1 * GiB;
    mc->default_ram_id = "ram";
}

static const TypeInfo mt6580_test_type = {
    .name = MACHINE_TYPE_NAME("mt6580"),
    .parent = TYPE_MACHINE,
    .class_init = mt6580_test_class_init,
    .interfaces = arm_machine_interfaces,
};

static void mt6580_machines_init(void)
{
    type_register_static(&mt6580_test_type);
}

type_init(mt6580_machines_init)
