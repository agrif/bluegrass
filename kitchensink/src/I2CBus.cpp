#include "kitchensink.h"

I2CBus::I2CBus(TwoWire& i2c2) : i2c2(i2c2) {}

const char* I2CBus::name() {
    return "I2C Bus (STEMMA)";
}

void I2CBus::match(Matcher& m) {
    int address = 0, bytes = 0;

    if (m.match("read bytes from device", "i2c read <int:address> <int:bytes>", &address, &bytes)) {
        i2c2.requestFrom(address, bytes);
        while (i2c2.available()) {
            char c = i2c2.read();
            Serial.printf("0x%02x ", c);
        }
        Serial.println();
    }

    // FIXME variadic bytes would be nice. yeah...
    if (m.match("write a byte to device", "i2c write <int:address> <int:byte>", &address, &bytes)) {
        i2c2.beginTransmission(address);
        i2c2.write((uint8_t)bytes);
        i2c2.endTransmission();
    }
}
