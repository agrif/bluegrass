#pragma once

#include <Arduino.h>
#include <STM32RTC.h>
#include <Adafruit_MAX1704X.h>
#include <SdFat.h>

#define BLUEGRASS_URL "https://github.com/agrif/bluegrass"
#define INTERACT_BUFFER_LEN 256
#define TERMINAL_WIDTH 80

class Matcher {
public:
    virtual void section(const char* title);
    virtual bool match(const char* description, const char* fmt, ...);
};

class Tokenizer {
    const char* line;
    char buffer[INTERACT_BUFFER_LEN];
    size_t i;
    
public:
    Tokenizer(const char* line);
    void reset();
    const char* next();
};

class CommandMatcher : public Matcher {
    const char* line;
    Tokenizer tokline;
    bool matched;
public:
    CommandMatcher(const char* line);
    bool match(const char* description, const char* fmt, ...);
    bool was_matched();
};

class Test {
    const char* category;
    const char* name;
    unsigned int& tested;
    unsigned int& failed;
    bool closed;
    size_t position;

    void print_end(const char* label);
public:
    Test(const char* category, const char* name, unsigned int& tested, unsigned int& failed);
    ~Test();
    bool active();
    void ok();
};

class Tester {
    const char* category;
    unsigned int& tested;
    unsigned int& failed;
public:
    Tester(const char* category, unsigned int& tested, unsigned int& failed);
    Test test(const char* name);
};

#define DO_TEST(t, name) for (Test __current_test = (t).test(name); __current_test.active(); __current_test.ok())

class Interactable {
public:
    virtual const char* name();
    virtual void setup();
    virtual void match(Matcher& m);
    virtual void test(Tester& t);
};

class Interact {
    Interactable** interactables;
    size_t n_interactables;

    bool connected;
    char buffer[INTERACT_BUFFER_LEN];
    int buffer_end;
    bool last_char_carriage_return;

public:
    Interact(Interactable** interactables, size_t n);

    void setup();
    void loop();

private:
    void banner();
    void prompt();
    void handle();
    void help();
    void test();

    void match(Matcher& m);
};

class CoreMark : public Interactable {
public:
    const char* name();
    void match(Matcher& m);
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
