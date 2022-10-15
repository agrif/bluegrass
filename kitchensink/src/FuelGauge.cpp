#include "kitchensink.h"

FuelGauge::FuelGauge(TwoWire& i2c2) : i2c2(i2c2), connected(false) {}

const char* FuelGauge::name() {
    return "Fuel Gauge (MAX17048)";
}

bool FuelGauge::ensure() {
    if (!connected) {
        if (fuel.begin(&i2c2)) {
            connected = true;
            return true;
        }
        return false;
    }

    if (fuel.getICversion() & 0xFFF0 != 0x0010) {
        connected = false;
        return false;
    }

    return true;
}

void FuelGauge::setup() {
    ensure();
}

void FuelGauge::match(Matcher& m) {
    if (m.match("get the fuel IC info", "fuel info")) {
        if (!ensure()) {
            Serial.println("Battery not detected.");
            return;
        }

        Serial.printf("IC Version: %d\n", fuel.getICversion());
        Serial.printf("ID: %d\n", fuel.getChipID());
    }

    if (m.match("get the fuel state", "fuel get")) {
        if (!ensure()) {
            Serial.println("Battery not detected.");
            return;
        }

        Serial.print("Voltage (V): ");
        Serial.println(fuel.cellVoltage());
        Serial.print("Percentage (%): ");
        Serial.println(fuel.cellPercent());
        Serial.print("Charge Rate (% / hr): ");
        Serial.println(fuel.chargeRate());
    }
}

void FuelGauge::test(Tester& t) {
    DO_TEST(t, "connecting") {
        if (!ensure()) {
            break;
        }
    }

    DO_TEST(t, "checking version") {
        if (fuel.getICversion() & 0xFFF0 != 0x0010) {
            break;
        }
    }

    DO_TEST(t, "voltage sanity check") {
        float v = fuel.cellVoltage();
        if (v > 5.0 || v < 3.0) {
            break;
        }
    }
}
