#include "kitchensink.h"

void Matcher::section(const char* title) {}

bool Matcher::match(const char* description, const char* fmt, ...) {
    return false;
}

Tokenizer::Tokenizer(const char* line) : line(line) {
    reset();
}

void Tokenizer::reset() {
    strncpy(buffer, line, INTERACT_BUFFER_LEN);
    i = 0;
}

const char* Tokenizer::next() {
    while (isspace(buffer[i])) {
        i++;
    }

    size_t start = i;
    bool is_string = false;
    if (buffer[i] == '"') {
        // string!
        is_string = true;
        start++;
        i++;
        while (buffer[i] != 0 && buffer[i] != '"') {
            if (buffer[i] == '\\' && buffer[i + 1] != 0) {
                // FIXME escapes, just skip them for now
                i++;
            }
            i++;
        }
    } else {
        while (buffer[i] != 0 && !isspace(buffer[i])) {
            i++;
        }
    }

    size_t len = i - start;
    if (!is_string && len == 0) {
        return NULL;
    }

    if (buffer[i] != 0) {
        buffer[i] = 0;
        i++;
    }

    return &buffer[start];
}

enum ArgumentType {
    ARG_LITERAL,
    ARG_STRING,
    ARG_INTEGER,
};

class Argument {
    const char* fmt;
    ArgumentType typ;
public:
    Argument(const char* fmt) : fmt(fmt), typ(ARG_LITERAL) {
        if (fmt[0] == '<') {
            typ = ARG_STRING;
            if (strncmp(&fmt[1], "int:", 4) == 0) {
                typ = ARG_INTEGER;
            }
        }
    }

    bool match(const char* tok, va_list* args) {
        switch (typ) {
        case ARG_LITERAL:
            return strcmp(fmt, tok) == 0;
        case ARG_STRING:
            *va_arg(*args, const char**) = tok;
            return true;
        case ARG_INTEGER:
            int base = 10;
            if (tok[0] == '0') {
                switch (tok[1]) {
                case 'b':
                case 'B':
                    base = 2;
                    tok = &tok[2];
                    break;
                case 'o':
                case 'O':
                    base = 8;
                    tok = &tok[2];
                    break;
                case 'x':
                case 'X':
                    base = 16;
                    tok = &tok[2];
                    break;
                }
            }
            char* endptr = NULL;
            long int val = strtol(tok, &endptr, base);
            if (*endptr != 0) {
                return false;
            }
            *va_arg(*args, int*) = val;
            return true;
        };

        return false;
    }
};

CommandMatcher::CommandMatcher(const char* line) : line(line), tokline(line), matched(false) {}

bool CommandMatcher::match(const char* description, const char* fmt, ...) {
    if (matched) {
        // only match once
        return false;
    }

    tokline.reset();
    Tokenizer tokfmt(fmt);

    const char* f;
    const char* l;

    va_list args;
    va_start(args, fmt);

    while (true) {
        f = tokfmt.next();
        l = tokline.next();

        if (f == NULL || l == NULL) {
            break;
        }

        Argument farg(f);
        if (!farg.match(l, &args)) {
            break;
        }
    }

    va_end(args);
    if (f == NULL && l == NULL) {
        matched = true;
        return true;
    } else {
        return false;
    }
}

bool CommandMatcher::was_matched() {
    return matched;
}
