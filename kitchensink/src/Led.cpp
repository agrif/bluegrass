#include "kitchensink.h"
#include <Morse.h>

static void send_morse(uint32_t pin, const char* message) {
    Morse morse(0, 15);
    morse.send(message);

    while (morse.busy) {
        morse.update();
        digitalWrite(pin, morse.tx ? LOW : HIGH);
        delay(1);
    }

    digitalWrite(pin, HIGH);
}

Led::Led(uint32_t pin) : pin(pin) {}

const char* Led::name() {
    return "On-board LED";
}

void Led::setup() {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
}

void Led::match(Matcher& m) {
    if (m.match("turn LED on", "led on")) {
        digitalWrite(pin, LOW);
    }

    if (m.match("turn LED off", "led off")) {
        digitalWrite(pin, HIGH);
    }

    if (m.match("blink LED", "led blink")) {
        for (int i = 0; i < 10; i++) {
            digitalWrite(pin, LOW);
            delay(100);
            digitalWrite(pin, HIGH);
            delay(100);
        }
    }

    const char* message = "bluegrass";
    if (m.match("send a Morse code message", "led morse <message>", &message)) {
        send_morse(pin, message);
    }
}

void Led::test(Tester& t) {
    DO_TEST(t, "turn on") {
        digitalWrite(pin, LOW);
        delay(1000);
        if (digitalRead(pin) != LOW) {
            break;
        }
    }

    DO_TEST(t, "turn off") {
        digitalWrite(pin, HIGH);
        delay(1000);
        if (digitalRead(pin) != HIGH) {
            break;
        }
    }

    DO_TEST(t, "blink") {
        for (int i = 0; i < 10; i++) {
            digitalWrite(pin, LOW);
            delay(100);
            if (digitalRead(pin) != LOW) {
                break;
            }

            digitalWrite(pin, HIGH);
            delay(100);
            if (digitalRead(pin) != HIGH) {
                break;
            }
        }
    }

    DO_TEST(t, "morse") {
        send_morse(pin, "bluegrass");
        if (digitalRead(pin) != HIGH) {
            break;
        }
    }
}
