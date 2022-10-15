#include "kitchensink.h"

const char* Interactable::name() {
    return "!! UNNAMED !!";
}

void Interactable::setup() {}

void Interactable::match(Matcher& m) {}

void Interactable::test(Tester& t) {}

class HelpMatcher : public Matcher {
    int width;

public:
    HelpMatcher() : width(20) {}

    void section(const char* title) {
        Serial.println();
        Serial.println(title);
        for (int i = strlen(title); i > 0; i--) {
            Serial.write('=');
        }
        Serial.println();
    }

    bool match(const char* description, const char* fmt, ...) {
        Serial.print(fmt);
        int spaces = width - strlen(fmt);
        if (spaces < 0) {
            Serial.println();
            spaces = width;
        }
        spaces += 2;
        for (; spaces > 0; spaces--) {
            Serial.write(' ');
        }
        Serial.println(description);

        return false;
    }
};

Interact::Interact(Interactable** interactables, size_t n) :
    interactables(interactables), n_interactables(n),
    connected(false), buffer_end(0), last_char_carriage_return(false)
{}

void Interact::setup() {
    connected = false;
    buffer_end = 0;

    Serial.begin();

    for (int j = 0; j < n_interactables; j++) {
        interactables[j]->setup();
    }
}

void Interact::loop() {
    if (!Serial) {
        connected = false;
        return;
    }

    // we are definitely connected. is it newly connected?
    if (!connected) {
        connected = true;
        buffer_end = 0;
        delay(500);
        banner();
        prompt();
    }

    while (Serial.available() > 0) {
        // don't overflow, and leave room for a null
        if (buffer_end + 1 >= INTERACT_BUFFER_LEN) {
            // uh-oh
            Serial.println();
            Serial.println("ERROR: input buffer overflow!");
            prompt();
            buffer_end = 0;
            continue;
        }

        // read new data into buffer
        char c = Serial.read();

        // if the last char was '\r', ignore any '\n'
        if (last_char_carriage_return) {
            last_char_carriage_return = false;
            if (c == '\n') {
                continue;
            }
        }

        // echo
        Serial.write(c);

        switch (c) {
        case '\b': // backspace
            if (buffer_end > 0) {
                buffer_end--;
                // erase and backspace again
                Serial.write(' ');
                Serial.write('\b');
            }
            break;
        case '\r': // carriage return
            last_char_carriage_return = true;
            Serial.write('\n');
        case '\n': // newline
            buffer[buffer_end] = 0;
            handle();
            buffer_end = 0;
            prompt();
            break;
        default: // everything else
            buffer[buffer_end] = c;
            buffer_end++;
            break;
        }
    }
}

void Interact::banner() {
    Serial.println("Bluegrass Kitchen Sink");
    Serial.println("Run `help` to list commands.");
}

void Interact::prompt() {
    Serial.print("> ");
}

void Interact::handle() {
    if (buffer_end == 0) {
        banner();
        return;
    }

    CommandMatcher cmd(buffer);
    match(cmd);
    if (!cmd.was_matched()) {
        Serial.println("ERROR: unknown command!");
    }
}

void Interact::help() {
    HelpMatcher help;
    match(help);
    Serial.println();
}

void Interact::test() {
    unsigned int tested = 0;
    unsigned int failed = 0;

    for (int j = 0; j < n_interactables; j++) {
        Tester t(interactables[j]->name(), tested, failed);
        interactables[j]->test(t);
    }

    Serial.println();
    if (failed == 0) {
        Serial.printf("All %d tests succeeded.\r\n", tested);
    } else {
        Serial.printf("%d/%d tests failed, see log above.\r\n", failed, tested);
    }
}

void Interact::match(Matcher& m) {
    m.section("Main");

    if (m.match("show this help text", "help")) {
        help();
    }

    if (m.match("run the built-in test", "test")) {
        test();
    }

    for (int j = 0; j < n_interactables; j++) {
        m.section(interactables[j]->name());
        interactables[j]->match(m);
    }
}
