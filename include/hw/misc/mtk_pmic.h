#ifndef MTK_PMIC_H
#define MTK_PMIC_H

#include "hw/core/qdev.h"
#include "qom/object.h"

#define TYPE_MTK_PMIC     "mtk-pmic"
OBJECT_DECLARE_TYPE(MtkPmicState, MtkPmicClass, MTK_PMIC)

struct MtkPmicClass {
    DeviceClass parent_class;

    uint16_t (*read_reg)(MtkPmicState *pmic, uint16_t reg);
    void (*write_reg)(MtkPmicState *pmic, uint16_t reg, uint16_t value);
};

struct MtkPmicState {
    DeviceState parent_obj;
};

#endif /* MTK_PMIC_H */
