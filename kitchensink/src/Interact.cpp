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
    connected(false), history_entry(0), buffer_end(0), buffer_cur(0),
    last_char_carriage_return(false),
    in_escape(false), in_extended_escape(false)
{
    memset(history, 0, (INTERACT_HISTORY - 1) * INTERACT_BUFFER_LEN);
    memset(history_buf, 0, INTERACT_HISTORY * INTERACT_BUFFER_LEN);
    buffer = &history_buf[history_entry][0];
}

void Interact::setup() {
    connected = false;
    history_entry = 0;
    buffer = &history_buf[history_entry][0];
    buffer_end = 0;
    buffer_cur = 0;

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
        memcpy(&history_buf[1][0], &history[0][0], (INTERACT_HISTORY - 1) * INTERACT_BUFFER_LEN);
        history_entry = 0;
        buffer = &history_buf[history_entry][0];
        buffer_end = 0;
        buffer_cur = 0;
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
            memcpy(&history_buf[1][0], &history[0][0], (INTERACT_HISTORY - 1) * INTERACT_BUFFER_LEN);
            history_entry = 0;
            buffer = &history_buf[history_entry][0];
            buffer_end = 0;
            buffer_cur = 0;
            continue;
        }

        // read new data into buffer
        char c = Serial.read();

        if (in_escape) {
            if (in_extended_escape) {
                if (c < 0x40 || c >= 0x7f) {
                    // parameter bytes, ignore
                    continue;
                }

                // extended escape sequence end
                in_escape = false;
                in_extended_escape = false;
                switch (c) {
                case 'A':
                    // cursor up
                    if (history_entry + 1 < INTERACT_HISTORY && history_buf[history_entry + 1][0] != 0) {
                        // cap current line
                        buffer[buffer_end] = 0;

                        // erase line
                        while (buffer_cur > 0) {
                            Serial.write('\b');
                            buffer_cur--;
                        }
                        while (buffer_cur < buffer_end) {
                            Serial.write(' ');
                            buffer_cur++;
                        }
                        while (buffer_cur > 0) {
                            Serial.write('\b');
                            buffer_cur--;
                        }

                        // switch to new entry
                        history_entry++;
                        buffer = &history_buf[history_entry][0];
                        buffer_end = strlen(buffer);
                        buffer_cur = buffer_end;
                        Serial.write(buffer, buffer_end);
                    }
                    break;
                case 'B':
                    // cursor down
                    if (history_entry > 0) {
                        // cap current line
                        buffer[buffer_end] = 0;

                        // erase line
                        while (buffer_cur > 0) {
                            Serial.write('\b');
                            buffer_cur--;
                        }
                        while (buffer_cur < buffer_end) {
                            Serial.write(' ');
                            buffer_cur++;
                        }
                        while (buffer_cur > 0) {
                            Serial.write('\b');
                            buffer_cur--;
                        }

                        // switch to new entry
                        history_entry--;
                        buffer = &history_buf[history_entry][0];
                        buffer_end = strlen(buffer);
                        buffer_cur = buffer_end;
                        Serial.write(buffer, buffer_end);
                    }
                    break;
                case 'C':
                    // cursor forward
                    if (buffer_cur < buffer_end) {
                        Serial.write(buffer[buffer_cur]);
                        buffer_cur++;
                    }
                    break;
                case 'D':
                    // cursor back
                    if (buffer_cur > 0) {
                        Serial.write('\b');
                        buffer_cur--;
                    }
                    break;
                default:
                    // just ignore anything unknown
                    break;
                }
                continue;
            } else {
                // normal escape sequence
                switch (c) {
                case '[':
                    in_extended_escape = true;
                    break;
                default:
                    // some others can be multibyte but lets ignore them
                    in_escape = false;
                    break;
                }
                continue;
            }
        }

        // if the last char was '\r', ignore any '\n'
        if (last_char_carriage_return) {
            last_char_carriage_return = false;
            if (c == '\n') {
                continue;
            }
        }

        switch (c) {
        case 0x01:
            // Ctrl-A, move to start of line
            while (buffer_cur > 0) {
                Serial.write('\b');
                buffer_cur--;
            }
            break;
        case 0x04:
            // Ctrl-D, delete current position
            if (buffer_cur < buffer_end) {
                memmove(&buffer[buffer_cur], &buffer[buffer_cur + 1], buffer_end - buffer_cur - 1);
                buffer_end--;
                // output edited line then backspace back
                Serial.write(&buffer[buffer_cur], buffer_end - buffer_cur);
                Serial.write(' ');
                for (int i = 0; i < buffer_end - buffer_cur + 1; i++) {
                    Serial.write('\b');
                }
            }
            break;
        case 0x05:
            // Ctrl-E, move to end of line
            Serial.write(&buffer[buffer_cur], buffer_end - buffer_cur);
            buffer_cur = buffer_end;
            break;
        case 0x0b:
            // Ctrl-K, delete to end of line
            for (int i = 0; i < buffer_end - buffer_cur; i++) {
                Serial.write(' ');
            }
            for (int i = 0; i < buffer_end - buffer_cur; i++) {
                Serial.write('\b');
            }
            buffer_end = buffer_cur;
            break;
        case '\b': // backspace
            if (buffer_cur > 0) {
                memmove(&buffer[buffer_cur - 1], &buffer[buffer_cur], buffer_end - buffer_cur);
                buffer_end--;
                buffer_cur--;
                // backspace, output edited line, then backspace back
                Serial.write('\b');
                Serial.write(&buffer[buffer_cur], buffer_end - buffer_cur);
                Serial.write(' ');
                for (int i = 0; i < buffer_end - buffer_cur + 1; i++) {
                    Serial.write('\b');
                }
            }
            break;
        case '\e': // escape
            in_escape = true;
            break;
        case '\r': // carriage return
            last_char_carriage_return = true;
            // intentional fall-through!
        case '\n': // newline
            Serial.write('\n');
            buffer[buffer_end] = 0;
            handle();

            if (buffer[0] != 0) {
                memmove(&history[1][0], &history[0][0], (INTERACT_HISTORY - 2) * INTERACT_BUFFER_LEN);
                memcpy(&history[0][0], buffer, INTERACT_BUFFER_LEN);
            }
            memcpy(&history_buf[1][0], &history[0][0], (INTERACT_HISTORY - 1) * INTERACT_BUFFER_LEN);
            history_entry = 0;
            buffer = &history_buf[history_entry][0];
            buffer_end = 0;
            buffer_cur = 0;
            prompt();
            break;
        default: // everything else
            // only use it if it is printable
            if (0x20 <= c && c < 0x7f) {
                memmove(&buffer[buffer_cur + 1], &buffer[buffer_cur], buffer_end - buffer_cur);
                buffer[buffer_cur] = c;
                buffer_end++;
                buffer_cur++;
                // write out new line and then backspace back
                Serial.write(&buffer[buffer_cur - 1], buffer_end - buffer_cur + 1);
                for (int i = 0; i < buffer_end - buffer_cur; i++) {
                    Serial.write('\b');
                }
            }
            break;
        }
    }
}

void Interact::banner() {
    Serial.println("Bluegrass Kitchen Sink");
    Serial.println("[ " BLUEGRASS_URL " ]");
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
