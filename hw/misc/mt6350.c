#include "qemu/osdep.h"
#include "qemu/log.h"
#include "hw/misc/mt6350.h"

#define VCHARGER_CHANNEL_NUMBER 4
#define BATSNS_CHANNEL_NUMBER 7

#define FQMTR_FQM26M_CK 4

uint32_t fqmtr_clock_freq_list[4] = {
    26000000,
    0,
    32000,
    0
};

uint32_t fqmtr_measure_freq_list[8] = {
    [FQMTR_FQM26M_CK] = 26000000,
};

static uint16_t mt6350_read_reg(MtkPmicState *pmic, uint16_t reg)
{
    Mt6350State *state = MT6350(pmic);

    switch (reg)
    {
        case 0x114: // MT_TOP_RST_CON
            return state->rst_con;
        case 0x126: // MT_TOP_CKCON1
            return state->fqmtr_clock << 6;
        case 0x142: // MT_CHRSTATUS
            return 1 << 2; // "release" homekey(pwrkey is still "pressed")
        case 0x182: // FQMTR_CON0
            return (state->fqmtr_enable << 15) | state->fqmtr_measure_clock;
        case 0x184: // FQMTR_CON1
            return state->fqmtr_window_size;
        case 0x186: // FQMTR_CON2
        {
            uint32_t fqmtr_clock_freq = fqmtr_clock_freq_list[state->fqmtr_clock];
            if (fqmtr_clock_freq == 0)
                return 0;
            return fqmtr_measure_freq_list[state->fqmtr_measure_clock] / fqmtr_clock_freq * state->fqmtr_window_size;
        }
        case 0x18c:
            return 0x5aa5;
        case 0x714: // battery
            if ((state->adc_request_list & (1 << BATSNS_CHANNEL_NUMBER)) == 0)
                return 0;
            // 3.7V
            return (1 << 15) | (3700 & 0x7fff);
        case 0x718: // vcharger
            if ((state->adc_request_list & (1 << VCHARGER_CHANNEL_NUMBER)) == 0)
                return 0;
            // 5.5V
            return (1 << 15) | ((5500 / 123 * 13) & 0x7fff);
        case 0x76e: // AUXADC_CON22
            return state->adc_request_list & 0x1ff;
        default:
            qemu_log_mask(LOG_UNIMP, "mt6350: unimplemented device read  "
                "(offset 0x%0*x)\n",
                        2, reg);
            break;
    }
    return 0;
}

static void mt6350_write_reg(MtkPmicState *pmic, uint16_t reg, uint16_t value)
{
    Mt6350State *state = MT6350(pmic);

    switch (reg)
    {
        case 0x108:
            break;
        case 0x114: // MT_TOP_RST_CON
            state->rst_con = value;
            if (state->rst_con & (1 << 8)) // freq meter
            {
                state->fqmtr_enable = 0;
                state->fqmtr_measure_clock = 0;
                state->fqmtr_window_size = 0;
            }
            break;
        case 0x126: // MT_TOP_CKCON1
            state->fqmtr_clock = (value >> 6) & 3;
            break;
        case 0x182: // FQMTR_CON0
            state->fqmtr_enable = !!(value & (1 << 15));
            state->fqmtr_measure_clock = value & 7;
            break;
        case 0x184: // FQMTR_CON1
            state->fqmtr_window_size = value;
            break;
        case 0x76e: // AUXADC_CON22
            state->adc_request_list = value & 0x1ff;
            break;
        default:
            qemu_log_mask(LOG_UNIMP, "mt6350: unimplemented device write "
                "(offset 0x%0*x, value 0x%0*x)\n",
                2, reg, 4, value);
            break;
    }
    return ;
}

static void mt6350_class_init(ObjectClass *klass, const void *data)
{
    MtkPmicClass *pmic_class = MTK_PMIC_CLASS(klass);

    pmic_class->read_reg = mt6350_read_reg;
    pmic_class->write_reg = mt6350_write_reg;
}

static const TypeInfo mt6350_info = {
    .name = TYPE_MT6350,
    .parent = TYPE_MTK_PMIC,
    .instance_size = sizeof(Mt6350State),
    .class_init = mt6350_class_init
};

static void mt6350_register_types(void)
{
    type_register_static(&mt6350_info);
}

type_init(mt6350_register_types)
