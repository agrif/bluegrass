#pragma once

#include <stdlib.h>

#define BLUEGRASS_NAME "Bluegrass"
#define BLUEGRASS_URL "https://github.com/agrif/bluegrass"
// BLUEGRASS_REV defined in platformio.ini
// BLUEGRASS_GIT defined in platformio.ini by git-revision.py
#define INTERACT_BUFFER_LEN 256
#define INTERACT_HISTORY 8
#define TERMINAL_WIDTH 80

#define STRINGIFY(s) __STRINGIFY_X(s)
#define __STRINGIFY_X(s) #s

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

void id_number(char* dest, size_t size);

class Interact {
    Interactable** interactables;
    size_t n_interactables;

    bool connected;
    char history[INTERACT_HISTORY - 1][INTERACT_BUFFER_LEN];
    char history_buf[INTERACT_HISTORY][INTERACT_BUFFER_LEN];
    int history_entry;
    char* buffer;
    int buffer_end;
    int buffer_cur;
    bool last_char_carriage_return;
    bool in_escape;
    bool in_extended_escape;

public:
    Interact(Interactable** interactables, size_t n);

    void setup();
    void loop();

private:
    void version();
    void help_hint();
    void banner();
    void prompt();
    void handle();
    void help();
    void test();

    void match(Matcher& m);
};
