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
    if (m.match("read VBUS pin", "vbus read")) {
        pinMode(PVBUS, INPUT_PULLDOWN);
        Serial.println(digitalRead(PVBUS));
    }
}

void Vbus::test(Tester& t) {
    DO_TEST(t, "checking") {
        pinMode(PVBUS, INPUT_PULLDOWN);
        if (digitalRead(PVBUS) == LOW) {
            // this cannot happen, if connected over usb
            break;
        }
    }
}
