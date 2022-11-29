#include "kitchensink.h"

struct pin_name_entry {
    const char* name;
    uint32_t pin;
    bool analog_in;
};

static const struct pin_name_entry PIN_NAMES[] = {
    {"D0", D0, false},
    {"D1", D1, false},
    {"D2", D2, false},
    {"D3", D3, false},
    {"D4", D4, false},
    {"D5", D5, false},
    {"D6", D6, false},
    {"D7", D7, false},
    {"D8", D8, false},
    {"D9", D9, false},
    {"D10", D10, false},
    {"D11", D11, false},
    {"D12", D12, false},
    {"D13", D13, false},
    {"D14", D14, false},
    {"D15", D15, false},
    {"D16", D16, false},
    {"D17", D17, false},
    {"D18", D18, false},
    {"D19", D19, false},
    {"D20", D20, false},
    {"D21", D21, false},
    {"D22", D22, false},
    {"D23", D23, false},
    {"D24", D24, false},
    {"D25", D25, false},
    {"D26", D26, false},
    {"D27", D27, false},
    {"D28", D28, false},
    {"D29", D29, false},
    {"D30", D30, false},
    {"D31", D31, false},
    {"D32", D32, false},
    {"D33", D33, false},
    {"D34", D34, false},

    {"A0", A0, true},
    {"A1", A1, true},
    {"A2", A2, true},
    {"A3", A3, true},
    {"A4", A4, true},
    {"A5", A5, true},
    {"A6", A6, true},
    {"A7", A7, true},
    {"A8", A8, true},
    {"A9", A9, true},

    {"PA0", PA0, true},
    {"PA1", PA1, true},
    {"PA2", PA2, true},
    {"PA3", PA3, true},
    {"PA4", PA4, true},
    {"PA5", PA5, true},
    {"PA6", PA6, true},
    {"PA7", PA7, true},
    {"PA8", PA8, false},
    {"PA9", PA9, false},
    {"PA10", PA10, false},
    {"PA11", PA11, false},
    {"PA12", PA12, false},
    {"PA13", PA13, false},
    {"PA14", PA14, false},
    {"PA15", PA15, false},

    {"PB0", PB0, true},
    {"PB1", PB1, true},
    {"PB2", PB2, false},
    {"PB3", PB3, false},
    {"PB4", PB4, false},
    {"PB5", PB5, false},
    {"PB6", PB6, false},
    {"PB7", PB7, false},
    {"PB8", PB8, false},
    {"PB9", PB9, false},
    {"PB10", PB10, false},
    {"PB11", PB11, false},
    {"PB12", PB12, false},
    {"PB13", PB13, false},
    {"PB14", PB14, false},
    {"PB15", PB15, false},

    {"PC13", PC13, false},
    {"PC14", PC14, false},
    {"PC15", PC15, false},

    {"LED_YELLOW", LED_YELLOW, false},
    {"LED_BUILTIN", LED_BUILTIN, false},

    {"PVBUS", PVBUS, false},

    {NULL, 0},
};

uint32_t pin_lookup(const char* name, uint32_t* pin, bool analog_in) {
    if (isdigit(name[0]) && strlen(name) <= 2) {
        // try to interpret this as either D## or A##
        char new_name[4];
        if (analog_in) {
            new_name[0] = 'A';
        } else {
            new_name[0] = 'D';
        }

        new_name[1] = name[0];
        new_name[2] = name[1];
        new_name[3] = 0;

        if (pin_lookup(new_name, pin, analog_in)) {
            return true;
        }
    }

    for (int i = 0; PIN_NAMES[i].name != NULL; i++) {
        if (analog_in && !PIN_NAMES[i].analog_in) {
            continue;
        }

        if (strcasecmp(PIN_NAMES[i].name, name) == 0) {
            *pin = PIN_NAMES[i].pin;
            return true;
        }
    }

    return false;
}
