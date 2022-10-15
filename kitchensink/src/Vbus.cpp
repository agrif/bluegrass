#include "kitchensink.h"

Vbus::Vbus() {}

const char* Vbus::name() {
    return "VBUS sensing";
}

void Vbus::setup() {
    // must be pull down, if pull up, it never goes down
    pinMode(PVBUS, INPUT_PULLDOWN);
}

void Vbus::match(Matcher& m) {
    if (m.match("read VBUS pin", "vbus get")) {
        if (digitalRead(PVBUS)) {
            Serial.println("VBUS is high.");
        } else {
            Serial.println("VBUS is low.");
        }
    }
}

void Vbus::test(Tester& t) {
    DO_TEST(t, "checking") {
        if (digitalRead(PVBUS) == LOW) {
            // this cannot happen, if connected over usb
            break;
        }
    }
}
