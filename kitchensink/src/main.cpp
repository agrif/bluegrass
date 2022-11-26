#include "kitchensink.h"

TwoWire i2c2(PB11, PB10);

CoreMark coremark;
Led led(LED_BUILTIN);
Vbus vbus;
Rtc rtc;
FuelGauge fuel(i2c2);
AnalogInput analoginput;
I2CBus i2cbus(i2c2);
Sd sd;

Interactable* interactables[] = {
    &coremark,
    &led,
    &vbus,
    &rtc,
    &fuel,
    &analoginput,
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
