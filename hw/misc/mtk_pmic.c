#include "qemu/osdep.h"
#include "qemu/log.h"
#include "hw/misc/mtk_pmic.h"
#include "hw/core/sysbus.h"

static void mtk_pmic_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *k = DEVICE_CLASS(klass);
    set_bit(DEVICE_CATEGORY_MISC, k->categories);
}

static const TypeInfo mtk_pmic_info = {
    .name = TYPE_MTK_PMIC,
    .parent = TYPE_DEVICE,
    .abstract = true,
    .instance_size = sizeof(MtkPmicState),
    .class_size = sizeof(MtkPmicClass),
    .class_init = mtk_pmic_class_init,
};

static void mtk_pmic_register_types(void)
{
    type_register_static(&mtk_pmic_info);
}

type_init(mtk_pmic_register_types)
