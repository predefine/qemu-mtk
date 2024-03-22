#include "qemu/osdep.h"
#include "hw/qdev-properties.h"
#include "cpu.h"
#include "exec/address-spaces.h"
#include "qapi/error.h"
#include "qemu/module.h"
#include "qom/object.h"
#include "hw/core/cpu.h"
#include "hw/boards.h"
#include "hw/sysbus.h"
#include "hw/arm/mt6580.h"
#include "sysemu/reset.h"
#include "sysemu/sysemu.h"
#include "hw/char/pl011.h"
#include "hw/char/serial.h"

static void allocate_ram(MemoryRegion *top, const char *name, uint32_t addr, uint32_t size)
{
    MemoryRegion *sec = g_new(MemoryRegion, 1);
    memory_region_init_ram(sec, NULL, name, size, &error_fatal);
    memory_region_add_subregion(top, addr, sec);
}

static void mt6580_memory_setup(MachineState *machine, MemoryRegion *sysmem, AddressSpace *nsas)
{
    allocate_ram(sysmem, "testz", 0x0, 0x10000);

    char *file_data = NULL;
    unsigned long file_size;
    allocate_ram(sysmem, "ram", 0x80000000, 0x40000000);
    if(g_file_get_contents("/home/user/u-boot/out/u-boot-mtk.bin", &file_data, &file_size, NULL)){
        allocate_ram(sysmem, "LK", 0x81e00000, file_size);
        address_space_rw(nsas, 0x81e00000, MEMTXATTRS_UNSPECIFIED, ((uint8_t *)file_data)+0x200, file_size, 1);
    }
    DeviceState *dev = qdev_new(TYPE_SERIAL_MM);
    SysBusDevice *s = SYS_BUS_DEVICE(dev);
    qdev_prop_set_chr(dev, "chardev", serial_hd(0));
    sysbus_realize_and_unref(SYS_BUS_DEVICE(dev), &error_fatal);
    memory_region_add_subregion(sysmem, 0x11005000,
                                sysbus_mmio_get_region(s, 0));
    // sysbus_connect_irq(s, 0, qdev_get_gpio_in(vms->gic, irq));
}

static void mt6580_cpu_reset(void *opaque){
    MT6580MachineState* state = MT6580_MACHINE((MachineState*)opaque);
    ARMCPU* cpu = state->cpu;
    CPUState* cs = CPU(cpu);
    // CPUARMState* acs = &cpu->env;
    cpu_reset(cs);
    cpu_set_pc(cs, 0x81e00000);
}

static void mt6580_cpu_setup(MachineState *machine, MemoryRegion **sysmem, ARMCPU **cpu, AddressSpace **nsas)
{
    Object *cpuobj = object_new(machine->cpu_type);
    *cpu = ARM_CPU(cpuobj);
    CPUState *cs = CPU(*cpu);

    *sysmem = get_system_memory();

    object_property_set_link(cpuobj, "memory", OBJECT(*sysmem), &error_abort);
    object_property_set_bool(cpuobj, "has_el3", false, NULL);
    object_property_set_bool(cpuobj, "has_el2", false, NULL);
    object_property_set_bool(cpuobj, "realized", true, &error_fatal);

    *nsas = cpu_get_address_space(cs, ARMASIdx_NS);

    object_unref(cpuobj);
}

static void mt6580_machine_init(MachineState *machine){
    MT6580MachineState* state = MT6580_MACHINE(machine);

    MemoryRegion *sysmem;
    AddressSpace *nsas;
    ARMCPU* cpu;

    mt6580_cpu_setup(machine, &sysmem, &cpu, &nsas);
    state->cpu = cpu;

    mt6580_memory_setup(machine, sysmem, nsas);

    qemu_register_reset(mt6580_cpu_reset, state);
}

static void mt6580_class_init(ObjectClass *klass, void *data){
    MachineClass *mc = MACHINE_CLASS(klass);
    mc->desc = "mt6580";
    mc->init = mt6580_machine_init;
    mc->max_cpus = 4;
    mc->default_cpus = 4;
    mc->default_cpu_type = ARM_CPU_TYPE_NAME("cortex-a7");
}


static void mt6580_instance_init(Object *obj){

}

static const TypeInfo mt6580_machine_info = {
    .name = TYPE_MT6580_MACHINE,
    .parent = TYPE_MACHINE,
    .instance_size = sizeof(MT6580MachineState),
    .class_size = sizeof(MT6580MachineClass),
    .class_init = mt6580_class_init,
    .instance_init = mt6580_instance_init
};

static void mt6580_machine_types(void){
    type_register_static(&mt6580_machine_info);
}

type_init(mt6580_machine_types);
