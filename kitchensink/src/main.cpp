#include "kitchensink.h"

TwoWire i2c2(PB11, PB10);

Led led(LED_BUILTIN);
Vbus vbus;
Rtc rtc;
FuelGauge fuel(i2c2);
I2CBus i2cbus(i2c2);
Sd sd;

Interactable* interactables[] = {
    &led,
    &vbus,
    &rtc,
    &fuel,
    &i2cbus,
    &sd,
};

Interact interact(
    interactables,
    sizeof(interactables) / sizeof(interactables[0])
    );

void setup() {
    i2c2.begin();
    interact.setup();
}

void loop() {
    interact.loop();
}
