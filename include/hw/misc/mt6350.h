#ifndef MT6350_H
#define MT6350_H

#include "qom/object.h"
#include "hw/misc/mtk_pmic.h"

#define TYPE_MT6350     "mt6350"
OBJECT_DECLARE_SIMPLE_TYPE(Mt6350State, MT6350)


struct Mt6350State {
    MtkPmicState parent_obj;

    uint16_t fqmtr_enable;
    uint16_t fqmtr_measure_clock;
    uint16_t fqmtr_clock;
    uint16_t fqmtr_window_size;

    uint16_t adc_request_list;
    uint16_t rst_con;
};

#endif /* MT6350_H */
