#include <Arduino.h>
#include "Interact.h"

Tester::Tester(const char* category, unsigned int& tested, unsigned int& failed) : category(category), tested(tested), failed(failed) {}

Test Tester::test(const char* name) {
    return Test(category, name, tested, failed);
}

Test::Test(const char* category, const char* name, unsigned int& tested, unsigned int& failed) : category(category), name(name), tested(tested), failed(failed), closed(false), position(0) {
    Serial.printf("[%s] %s", category, name);
    position = strlen(category) + strlen(name) + 3;
    tested += 1;
}

void Test::print_end(const char* label) {
    size_t spaces = TERMINAL_WIDTH - position - strlen(label) - 2;
    for (; spaces > 0; spaces--) {
        Serial.write(' ');
    }
    Serial.printf("[%s]\r\n", label);
}

Test::~Test() {
    if (!closed) {
        closed = true;
        print_end("FAIL");
        failed += 1;
    }
}

bool Test::active() {
    return !closed;
}

void Test::ok() {
    if (!closed) {
        closed = true;
        print_end("ok");
    }
}
