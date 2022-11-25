#include <Arduino.h>
#include <STM32RTC.h>
#include <STM32LowPower.h>
#include <Adafruit_MAX1704X.h>
#include <SdFat.h>

#define LOG_INTERVAL 60 // in seconds
#define LOG_FILE "bluegrass-battery-logger.txt"

TwoWire i2c2(PB11, PB10);
STM32RTC& rtc = STM32RTC::getInstance();
Adafruit_MAX17048 fuel;
SdFs fs;

void fail(const char* message) {
    while (true) {
        Serial.print("ERROR: ");
        Serial.println(message);
        delay(1000);
        digitalWrite(LED_BUILTIN, LOW);
        delay(1000);
        digitalWrite(LED_BUILTIN, HIGH);
    }
}

#define log_to(stream, ...) do {                      \
        stream.printf("[%04d/%02d/%02d ",             \
                      rtc.getYear() + 2000,           \
                      rtc.getMonth(), rtc.getDay());  \
        stream.printf("%02d:%02d:%02d] ",             \
                      rtc.getHours(),                 \
                      rtc.getMinutes(),               \
                      rtc.getSeconds());              \
        stream.printf(__VA_ARGS__);                   \
        stream.println();                             \
    } while (false)

#define log(...) do {                               \
        if (Serial) {                               \
            log_to(Serial, __VA_ARGS__);            \
        }                                           \
        FsFile f = fs.open(LOG_FILE, FILE_WRITE);   \
        if (f) {                                    \
            log_to(f, __VA_ARGS__);                 \
        }                                           \
        f.close();                                  \
    } while(false)

void do_nothing() {}
void do_nothing_arg(void* data) {}

int old_vbus = 0;

void setup() {
    pinMode(PVBUS, INPUT_PULLDOWN);
    old_vbus = digitalRead(PVBUS);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    Serial.begin();

    rtc.setClockSource(STM32RTC::LSE_CLOCK);
    rtc.begin();
    LowPower.begin();
    LowPower.attachInterruptWakeup(PVBUS, do_nothing, RISING, DEEP_SLEEP_MODE);
    LowPower.enableWakeupFrom(&rtc, do_nothing_arg, NULL);

    i2c2.begin();
    if (!fuel.begin(&i2c2)) {
        fail("could not connect to fuel gauge");
    }

    if (!fs.begin(SdSpiConfig(PA4, SHARED_SPI, SD_SCK_MHZ(16)))) {
        fail("could not open SD card");
    }

    log("Booting...");
}

void heartbeat() {
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
}

void loop() {
    int new_vbus = digitalRead(PVBUS);
    if (old_vbus != new_vbus) {
        if (new_vbus) {
            log("VBUS high, charging...");
        } else {
            log("VBUS low, discharging...");
        }
        old_vbus = new_vbus;
    }

    log("Voltage: %.02f V, Cell: %.02f%%, Rate: %.02f %%/hr", fuel.cellVoltage(), fuel.cellPercent(), fuel.chargeRate());

    if (!new_vbus) {
        // discharging, conserve power
        rtc.setAlarmEpoch(rtc.getEpoch() + LOG_INTERVAL);
        LowPower.deepSleep();
        heartbeat();
    } else {
        // stay awake when connected to power so USB works
        uint32_t end = rtc.getEpoch() + LOG_INTERVAL;
        while (rtc.getEpoch() < end && digitalRead(PVBUS)) {
            delay(1000);
            heartbeat();
        }
    }
}
