#include "kitchensink.h"

const char* Gpio::name() {
    return "GPIO";
}

void Gpio::match(Matcher& m) {
    const char* name = "X";
    uint32_t pin = 0;
    int val = 0;

    if (m.match("read from pin", "gpio read <pin>", &name)) {
        if (!pin_lookup(name, &pin)) {
            Serial.println("bad pin name");
            return;
        }
        pinMode(pin, INPUT);
        Serial.println(digitalRead(pin));
    }

    if (m.match("read from pin with a pullup", "gpio read pullup <pin>", &name)) {
        if (!pin_lookup(name, &pin)) {
            Serial.println("bad pin name");
            return;
        }
        pinMode(pin, INPUT_PULLUP);
        Serial.println(digitalRead(pin));
    }

    if (m.match("read from pin with a pulldown", "gpio read pulldown <pin>", &name)) {
        if (!pin_lookup(name, &pin)) {
            Serial.println("bad pin name");
            return;
        }
        pinMode(pin, INPUT_PULLDOWN);
        Serial.println(digitalRead(pin));
    }

    if (m.match("write to pin", "gpio write <pin> <int:value>", &name, &val)) {
        if (!pin_lookup(name, &pin)) {
            Serial.println("bad pin name");
            return;
        }
        pinMode(pin, OUTPUT);
        digitalWrite(pin, val);
    }

    if (m.match("write PWM to pin", "gpio write analog <pin> <int:value>", &name, &val)) {
        if (!pin_lookup(name, &pin)) {
            Serial.println("bad pin name");
            return;
        }
        pinMode(pin, OUTPUT);
        if (val > 255) {
            val = 255;
        } else if (val < 0) {
            val = 0;
        }
        analogWrite(pin, val);
    }
}
