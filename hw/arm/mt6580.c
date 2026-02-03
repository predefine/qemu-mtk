#include "qemu/osdep.h"
#include "cpu-qom.h"
#include "qapi/error.h"
#include "cpu.h"
#include "hw/core/sysbus.h"
#include "hw/arm/mt6580.h"
#include "system/address-spaces.h"
#include "system/system.h"
#include "hw/char/serial-mtk.h"
#include "hw/misc/unimp.h"

#define MT6580_GPT_BASE   0x10008000
#define MT6580_UART0_BASE 0x11005000

static void mt6580_realize(DeviceState *socdev, Error **errp)
{
    MT6580State *s = MT6580_SOC(socdev);
    // TODO: implement smp
    for (int n = 0; n < 1; n++) {
        Object *cpuobj = object_new(ARM_CPU_TYPE_NAME("cortex-a7"));

        object_property_add_child(OBJECT(s), "cpu[*]", cpuobj);

        object_property_set_bool(cpuobj, "has_el3", false, &error_fatal);

        s->cpu[n] = ARM_CPU(cpuobj);
        qdev_realize(DEVICE(cpuobj), NULL, &error_fatal);
    }

    serial_mtk_init(get_system_memory(), MT6580_UART0_BASE, 2, NULL, 921600, serial_hd(0), DEVICE_NATIVE_ENDIAN);

    sysbus_realize(SYS_BUS_DEVICE(&s->gpt), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->gpt), 0, MT6580_GPT_BASE);
    // TODO: add interrupt controller
    // sysbus_connect_irq

    create_unimplemented_device("topckgen",     0x10000000, 0x1000);
    create_unimplemented_device("gpio",         0x10005000, 0x1000);
    create_unimplemented_device("wdt",          0x10007000, 0x1000);
    create_unimplemented_device("pmic_wrap",    0x1000f000, 0x1000);
    create_unimplemented_device("iocfg_b",      0x10015000, 0x1000);
    create_unimplemented_device("iocfg_r",      0x10017000, 0x1000);
    create_unimplemented_device("apmixed",      0x10018000, 0x1000);
    create_unimplemented_device("display_pwm",  0x1100f000, 0x1000);
    create_unimplemented_device("mmc0",         0x11120000, 0x1000);
    create_unimplemented_device("mmc1",         0x11130000, 0x1000);
    create_unimplemented_device("mmsys",        0x14000000, 0x1000);
}

static void mt6580_init(Object *obj)
{
    MT6580State *s = MT6580_SOC(obj);

    object_initialize_child(obj, "gpt", &s->gpt, TYPE_MTK_GPT);
}

static void mt6580_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = mt6580_realize;
}

static const TypeInfo mt6580_info = {
    .name = TYPE_MT6580_SOC,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(MT6580State),
    .instance_init = mt6580_init,
    .class_init = mt6580_class_init,
};

static void mt6580_register_types(void)
{
    type_register_static(&mt6580_info);
}

type_init(mt6580_register_types)
