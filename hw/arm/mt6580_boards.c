#include "qemu/osdep.h"
#include "qapi/error.h"
#include "hw/arm/machines-qom.h"
#include "system/address-spaces.h"
#include "cpu.h"
#include "hw/arm/boot.h"
#include "hw/arm/mt6580.h"
#include "qemu/units.h"

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
    arm_load_kernel(s->soc.cpu[0], machine, &mt6580_board_boot_info);
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
