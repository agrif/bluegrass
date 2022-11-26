#include "kitchensink.h"
#include <stm32yyxx_ll_adc.h>

// some devices have baked-in calibration data (stm32f4xx)
// those devices should use that data! but we (stm32f103cbt) do not
// so, read the datasheet

// 5.3.19 temperature sensor characteristics
#define TEMP_V25       1430 // voltage at 25C (mV)
#define TEMP_T25       25   // temperature for the measurement above (C)
#define TEMP_AVG_SLOPE 4300 // in uV / C

// 5.3.4 embedded reference voltage
#define VREFINT        1200 // reference voltage (mV)

// range of our ADC
#define ADC_RANGE ((1 << ADC_RESOLUTION) - 1)

static int32_t read_vdda() {
    // read vref to calculate vdda
    // note, this returns the voltage of V_DDA (mV)

    // AVREF / ADC_RANGE * V_DDA = VREFINT
    // therefore,
    return (VREFINT * ADC_RANGE / analogRead(AVREF));
}

// in mV
static int32_t read_voltage(int32_t vdda, uint32_t pin) {
    return vdda * analogRead(pin) / ADC_RANGE;
}

// in C
static int32_t read_temp_sensor(int32_t vdda) {
    int32_t dv = TEMP_V25 - read_voltage(vdda, ATEMP);
    return (dv * 1000 / TEMP_AVG_SLOPE) + TEMP_T25;
}

const char* AnalogInput::name() {
    return "Analog Inputs";
}

void AnalogInput::setup() {
    analogReadResolution(ADC_RESOLUTION);

    for (int i = 0; i < NUM_ANALOG_INPUTS; i++) {
        pinMode(PNUM_ANALOG_BASE + i, INPUT_ANALOG);
    }
}

void AnalogInput::match(Matcher& m) {
    if (m.match("read voltage reference", "read vdda")) {
        Serial.print(read_vdda());
        Serial.println(" mV");
    }

    if (m.match("read internal temperature sensor", "read temp")) {
        Serial.print(read_temp_sensor(read_vdda()));
        Serial.println(" °C");
    }

    int pin = 0;
    if (m.match("read pin value", "read analog <int:pin>", &pin)) {
        if (0 <= pin && pin < NUM_ANALOG_INPUTS) {
            Serial.println(analogRead(PNUM_ANALOG_BASE + pin) / (float)ADC_RANGE);
        } else {
            Serial.println("bad analog input");
        }
    }

    if (m.match("read pin voltage", "read voltage <int:pin>", &pin)) {
        if (0 <= pin && pin < NUM_ANALOG_INPUTS) {
            Serial.print(read_voltage(read_vdda(), PNUM_ANALOG_BASE + pin));
            Serial.println(" mV");

        } else {
            Serial.println("bad analog input");
        }
    }
}

void AnalogInput::test(Tester& t) {
    DO_TEST(t, "VDDA sanity check") {
        int32_t vdda = read_vdda();
        if (vdda < 3100 || vdda > 3500) {
            break;
        }
    }

    DO_TEST(t, "temperature sanity check") {
        int32_t temp = read_temp_sensor(read_vdda());
        if (temp < 0 || temp > 50) {
            break;
        }
    }
}
