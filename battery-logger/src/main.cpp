#include <Arduino.h>
#include <STM32RTC.h>
#include <STM32LowPower.h>
#include <Adafruit_MAX1704X.h>
#include <SdFat.h>
#include <Morse.h>

#define LOG_INTERVAL 60 // in seconds
#define LOG_FILE "bluegrass-battery-logger.txt"

// 5.3.19 temperature sensor characteristics
#define TEMP_V25       1430 // voltage at 25C (mV)
#define TEMP_T25       25   // temperature for the measurement above (C)
#define TEMP_AVG_SLOPE 4300 // in uV / C

// 5.3.4 embedded reference voltage
#define VREFINT        1200 // reference voltage (mV)

// range of our ADC
#define ADC_RANGE ((1 << ADC_RESOLUTION) - 1)

TwoWire i2c2(PB11, PB10);
STM32RTC& rtc = STM32RTC::getInstance();
Adafruit_MAX17048 fuel;
SdFs fs;

static void send_morse_v(const char* fmt, va_list args) {
    Morse morse(0, 15);
    char buffer[256];

    vsnprintf(buffer, 256, fmt, args);
    buffer[255] = 0;

    morse.send(buffer);

    while (morse.busy) {
        morse.update();
        digitalWrite(PC13, morse.tx ? LOW : HIGH);
        delay(1);
    }

    digitalWrite(PC13, HIGH);
}

static void send_morse(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    send_morse_v(fmt, args);
    va_end(args);
}

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
    analogReadResolution(ADC_RESOLUTION);
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

void heartbeat(float cell, float rate, int temp) {
    char up_down = (rate > 0.0) ? 'U' : 'D';

    send_morse("B %c   %i   T   %i   ", up_down, (int)cell, temp);
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

    float voltage = fuel.cellVoltage();
    float cell = fuel.cellPercent();
    float rate = fuel.chargeRate();
    int temp = read_temp_sensor(read_vdda());

    log("Voltage: %.02f V, Cell: %.02f%%, Rate: %.02f %%/hr, Temperature: %i C", voltage, cell, rate, temp);
    heartbeat(cell, rate, temp);

    if (!new_vbus) {
        // discharging, conserve power
        rtc.setAlarmEpoch(rtc.getEpoch() + LOG_INTERVAL);
        LowPower.deepSleep();
    } else {
        // stay awake when connected to power so USB works
        uint32_t end = rtc.getEpoch() + LOG_INTERVAL;
        while (rtc.getEpoch() < end && digitalRead(PVBUS)) {
            delay(1000);
            heartbeat(cell, rate, temp);
        }
    }
}
