/*
 * QEMU 16550A UART emulation
 *
 * Copyright (c) 2003-2004 Fabrice Bellard
 * Copyright (c) 2008 Citrix Systems, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "qemu/osdep.h"
#include "hw/char/serial-mtk.h"
#include "exec/cpu-common.h"
#include "migration/vmstate.h"
#include "qapi/error.h"
#include "hw/core/qdev-properties.h"

static uint64_t serial_mtk_read(void *opaque, hwaddr addr, unsigned size)
{
    SerialMTK *s = SERIAL_MTK(opaque);

    hwaddr reg = addr >> s->regshift;
    if (reg >= 8) return 0;

    return serial_io_ops.read(&s->serial, reg, 1);
}

static void serial_mtk_write(void *opaque, hwaddr addr,
                            uint64_t value, unsigned size)
{
    SerialMTK *s = SERIAL_MTK(opaque);

    hwaddr reg = addr >> s->regshift;
    if (reg >= 8) return;

    value &= 255;
    serial_io_ops.write(&s->serial, reg, value, 1);
}

static const MemoryRegionOps serial_mtk_ops[] = {
    [DEVICE_NATIVE_ENDIAN] = {
        .read = serial_mtk_read,
        .write = serial_mtk_write,
        .endianness = DEVICE_NATIVE_ENDIAN,
        .valid.max_access_size = 8,
        .impl.max_access_size = 8,
    },
    [DEVICE_LITTLE_ENDIAN] = {
        .read = serial_mtk_read,
        .write = serial_mtk_write,
        .endianness = DEVICE_LITTLE_ENDIAN,
        .valid.max_access_size = 8,
        .impl.max_access_size = 8,
    },
    [DEVICE_BIG_ENDIAN] = {
        .read = serial_mtk_read,
        .write = serial_mtk_write,
        .endianness = DEVICE_BIG_ENDIAN,
        .valid.max_access_size = 8,
        .impl.max_access_size = 8,
    },
};

static void serial_mtk_realize(DeviceState *dev, Error **errp)
{
    SerialMTK *smm = SERIAL_MTK(dev);
    SerialState *s = &smm->serial;

    if (!qdev_realize(DEVICE(s), NULL, errp)) {
        return;
    }

    memory_region_init_io(&s->io, OBJECT(dev),
                          &serial_mtk_ops[smm->endianness], smm, "serial",
                          16 << smm->regshift);
    sysbus_init_mmio(SYS_BUS_DEVICE(smm), &s->io);
    sysbus_init_irq(SYS_BUS_DEVICE(smm), &smm->serial.irq);
}

static const VMStateDescription vmstate_serial_mtk = {
    .name = "serial",
    .version_id = 3,
    .minimum_version_id = 2,
    .fields = (const VMStateField[]) {
        VMSTATE_STRUCT(serial, SerialMTK, 0, vmstate_serial, SerialState),
        VMSTATE_END_OF_LIST()
    }
};

SerialMTK *serial_mtk_init(MemoryRegion *address_space,
                         hwaddr base, int regshift,
                         qemu_irq irq, int baudbase,
                         Chardev *chr, enum device_endian end)
{
    SerialMTK *smm = SERIAL_MTK(qdev_new(TYPE_SERIAL_MTK));
    MemoryRegion *mr;

    qdev_prop_set_uint8(DEVICE(smm), "regshift", regshift);
    qdev_prop_set_uint32(DEVICE(smm), "baudbase", baudbase);
    qdev_prop_set_chr(DEVICE(smm), "chardev", chr);
    qdev_set_legacy_instance_id(DEVICE(smm), base, 2);
    qdev_prop_set_uint8(DEVICE(smm), "endianness", end);
    sysbus_realize_and_unref(SYS_BUS_DEVICE(smm), &error_fatal);

    sysbus_connect_irq(SYS_BUS_DEVICE(smm), 0, irq);
    mr = sysbus_mmio_get_region(SYS_BUS_DEVICE(smm), 0);
    memory_region_add_subregion(address_space, base, mr);

    return smm;
}

static void serial_mtk_instance_init(Object *o)
{
    SerialMTK *smm = SERIAL_MTK(o);

    object_initialize_child(o, "serial", &smm->serial, TYPE_SERIAL);

    qdev_alias_all_properties(DEVICE(&smm->serial), o);
}

static const Property serial_mtk_properties[] = {
    /*
     * Set the spacing between adjacent memory-mapped UART registers.
     * Each register will be at (1 << regshift) bytes after the previous one.
     */
    DEFINE_PROP_UINT8("regshift", SerialMTK, regshift, 0),
    DEFINE_PROP_UINT8("endianness", SerialMTK, endianness, DEVICE_NATIVE_ENDIAN),
};

static void serial_mtk_class_init(ObjectClass *oc, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    device_class_set_props(dc, serial_mtk_properties);
    dc->realize = serial_mtk_realize;
    dc->vmsd = &vmstate_serial_mtk;
}

static const TypeInfo types[] = {
    {
        .name = TYPE_SERIAL_MTK,
        .parent = TYPE_SYS_BUS_DEVICE,
        .class_init = serial_mtk_class_init,
        .instance_init = serial_mtk_instance_init,
        .instance_size = sizeof(SerialMTK),
    },
};

DEFINE_TYPES(types)
