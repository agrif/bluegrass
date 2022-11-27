#pragma once

#include <Arduino.h>
#include <STM32RTC.h>
#include <Adafruit_MAX1704X.h>
#include <SdFat.h>

#include "Interact.h"

class CoreMark : public Interactable {
public:
    const char* name();
    void match(Matcher& m);
    void test(Tester& t);
};

class Led : public Interactable {
    uint32_t pin;
public:
    Led(uint32_t pin);
    const char* name();
    void setup();
    void match(Matcher& m);
    void test(Tester& t);
};

class Vbus : public Interactable {
public:
    Vbus();
    const char* name();
    void setup();
    void match(Matcher& m);
    void test(Tester& t);
};

class Rtc : public Interactable {
    STM32RTC& rtc;
public:
    Rtc();
    const char* name();
    void setup();
    void match(Matcher& m);
    void test(Tester& m);
};

class FuelGauge : public Interactable {
private:
    TwoWire& i2c2;
    Adafruit_MAX17048 fuel;
    bool connected;
    bool ensure();
public:
    FuelGauge(TwoWire& i2c2);
    const char* name();
    void setup();
    void match(Matcher& m);
    void test(Tester& t);
};

class AnalogInput : public Interactable {
public:
    const char* name();
    void setup();
    void match(Matcher& m);
    void test(Tester& t);
};

class I2CBus : public Interactable {
private:
    TwoWire& i2c2;
public:
    I2CBus(TwoWire& i2c2);
    const char* name();
    void match(Matcher& m);
};

class Sd : public Interactable {
    SdFs fs;
    bool card_present;
    bool volume_present;

    bool ensure(bool volume = true);
public:
    Sd();
    const char* name();
    void setup();
    void match(Matcher& m);
    void test(Tester& t);
};
