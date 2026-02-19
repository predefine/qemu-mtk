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
#include "system/blockdev.h"

#define MT6580_SPM_BASE    0x10006000
#define MT6580_GPT_BASE    0x10008000
#define MT6580_EFUSEC_BASE 0x10009000
#define MT6580_SEJ_BASE    0x1000a000
#define MT6580_PWRAP_BASE  0x1000f000
#define MT6580_DRAMC0_BASE 0x10207000
#define MT6580_UART0_BASE  0x11005000
#define MT6580_UART1_BASE  0x11006000
#define MT6580_MSDC0_BASE  0x11120000
#define MT6580_MSDC1_BASE  0x11130000

hwaddr uart_bases[] = {
    MT6580_UART0_BASE,
    MT6580_UART1_BASE,
};

hwaddr msdc_addrs[] = {
    MT6580_MSDC0_BASE,
    MT6580_MSDC1_BASE,
};


static void mt6580_realize(DeviceState *socdev, Error **errp)
{
    MT6580State *s = MT6580_SOC(socdev);
    // TODO: implement smp
    for (int n = 0; n < 1; n++) {
        Object *cpuobj = object_new(ARM_CPU_TYPE_NAME("cortex-a7"));

        object_property_add_child(OBJECT(s), "cpu[*]", cpuobj);

        s->cpu[n] = ARM_CPU(cpuobj);
        qdev_realize(DEVICE(cpuobj), NULL, &error_fatal);
    }

    for (uint32_t i = 0; i < NUM_UARTS; i++)
        serial_mtk_init(get_system_memory(), uart_bases[i], 2, NULL, 921600, serial_hd(i), DEVICE_NATIVE_ENDIAN);

    sysbus_realize(SYS_BUS_DEVICE(&s->gpt), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->gpt), 0, MT6580_GPT_BASE);
    // TODO: add interrupt controller
    // sysbus_connect_irq

    sysbus_realize(SYS_BUS_DEVICE(&s->efusec), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->efusec), 0, MT6580_EFUSEC_BASE);

    sysbus_realize(SYS_BUS_DEVICE(&s->sej), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->sej), 0, MT6580_SEJ_BASE);


    // fuck the emmc for now, i don't know if qemu supports pre-idle emmc state
    // qdev_prop_set_bit(DEVICE(&s->msdc[0]), "fuck-the-mmc", true);
    // qdev_prop_set_bit(DEVICE(&s->msdc[1]), "fuck-the-mmc", true);

    for (int i = 0; i < NUM_MSDCS; i++) {
        sysbus_realize(SYS_BUS_DEVICE(&s->msdc[i]), &error_abort);
        sysbus_mmio_map(SYS_BUS_DEVICE(&s->msdc[i]), 0, msdc_addrs[i]);

        DriveInfo* sdcard_di = drive_get(IF_SD, 0, i);
        assert(sdcard_di != NULL);
        DeviceState* msdc_sdcard = qdev_new(TYPE_SD_CARD);
        qdev_prop_set_drive(msdc_sdcard, "drive", blk_by_legacy_dinfo(sdcard_di));
        qdev_realize_and_unref(msdc_sdcard, qdev_get_child_bus(DEVICE(&s->msdc[i]), "sd-bus"),
                            &error_fatal);
    }

    sysbus_realize(SYS_BUS_DEVICE(&s->spm), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->spm), 0, MT6580_SPM_BASE);


    qdev_realize(DEVICE(&s->pmic), NULL, &error_fatal);
    object_property_set_link(OBJECT(&s->pwrap), "pmic", OBJECT(DEVICE(&s->pmic)), &error_fatal);

    sysbus_realize(SYS_BUS_DEVICE(&s->pwrap), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->pwrap), 0, MT6580_PWRAP_BASE);

    sysbus_realize(SYS_BUS_DEVICE(&s->dramc), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->dramc), 0, MT6580_DRAMC0_BASE);

    // preloader uses this in dram init
    create_unimplemented_device("some_dram_address", 0x800000C, 0x4);

    create_unimplemented_device("topckgen",     0x10000000, 0x1000);
    create_unimplemented_device("infracfg",     0x10001000, 0x1000);
    create_unimplemented_device("keypad",       0x10002000, 0x1000);
    create_unimplemented_device("gpio",         0x10005000, 0x1000);
    create_unimplemented_device("wdt",          0x10007000, 0x1000);
    create_unimplemented_device("iocfg_t",      0x10014000, 0x1000);
    create_unimplemented_device("iocfg_b",      0x10015000, 0x1000);
    create_unimplemented_device("iocfg_r",      0x10017000, 0x1000);
    create_unimplemented_device("apmixed",      0x10018000, 0x1000);
    create_unimplemented_device("dbgsys",       0x1011a000, 0x1000);
    create_unimplemented_device("mcucfg",       0x10200000, 0x1000);
    create_unimplemented_device("ddrphy",       0x10208000, 0x1000);
    create_unimplemented_device("sramrom",      0x10209000, 0x1000);
    create_unimplemented_device("display_pwm",  0x1100f000, 0x1000);
    create_unimplemented_device("usb0",         0x11100000, 0x1000);
    create_unimplemented_device("usb0_phy",     0x11110000, 0x1000);
    create_unimplemented_device("audiosys",     0x11140000, 0x1000);
    create_unimplemented_device("mfgcfg",       0x13000000, 0x1000);
    create_unimplemented_device("mmsys",        0x14000000, 0x1000);
}

static void mt6580_init(Object *obj)
{
    MT6580State *s = MT6580_SOC(obj);

    object_initialize_child(obj, "gpt", &s->gpt, TYPE_MTK_GPT);

    object_initialize_child(obj, "efusec", &s->efusec, TYPE_MT6580_EFUSEC);

    object_initialize_child(obj, "sej", &s->sej, TYPE_MT6580_SEJ);

    for (int i = 0; i < NUM_MSDCS; i++) {
        object_initialize_child(obj, "msdc[*]", &s->msdc[i], TYPE_MTK_MSDC);
    }

    object_initialize_child(obj, "spm", &s->spm, TYPE_MT6580_SPM);

    object_initialize_child(obj, "pwrap", &s->pwrap, TYPE_MT6580_PWRAP);

    object_initialize_child(obj, "pmic", &s->pmic, TYPE_MT6350);

    object_initialize_child(obj, "dramc", &s->dramc, TYPE_MT6580_DRAMC);
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
