#include "kitchensink.h"

#define EXPECTED_SCORE 87.77

static bool coremark_quiet = false;

extern "C" {
    extern float coremark_main(void);

    float bluegrass_coremark_result = 0.0;

    // https://github.com/PaulStoffregen/CoreMark
    int ee_printf(const char *format, ...)
    {
        if (coremark_quiet) {
            return 1;
        }

        va_list args;
        va_start(args, format);
        for (; *format; format++) {
            if (*format == '%') {
                bool islong = false;
                format++;
                if (*format == '%') { Serial.print(*format); continue; }
                if (*format == '-') format++; // ignore size
                while (*format >= '0' && *format <= '9') format++; // ignore size
                if (*format == 'l') { islong = true; format++; }
                if (*format == '\0') break;
                if (*format == 's') {
                    Serial.print((char *)va_arg(args, int));
                } else if (*format == 'f') {
                    Serial.print(va_arg(args, double));
                } else if (*format == 'd') {
                    if (islong) Serial.print(va_arg(args, long));
                    else Serial.print(va_arg(args, int));
                } else if (*format == 'u') {
                    if (islong) Serial.print(va_arg(args, unsigned long));
                    else Serial.print(va_arg(args, unsigned int));
                } else if (*format == 'x') {
                    if (islong) Serial.print(va_arg(args, unsigned long), HEX);
                    else Serial.print(va_arg(args, unsigned int), HEX);
                } else if (*format == 'c' ) {
                    Serial.print(va_arg(args, int));
                }
            } else {
                if (*format == '\n') Serial.print('\r');
                Serial.print(*format);
            }
        }
        va_end(args);
        return 1;
    }

    // https://github.com/PaulStoffregen/CoreMark
    uint32_t Arduino_millis(void)
    {
        return millis();
    }
}

const char* CoreMark::name() {
    return "EEMBC CoreMark";
}

void CoreMark::match(Matcher& m) {
    if (m.match("run the EEMBC CoreMark benchmark", "coremark")) {
        Serial.println("EEMBC CoreMark Performance Benchmark");
        Serial.println();
        Serial.println("CoreMark measures how quickly your processor can manage linked");
        Serial.println("lists, do matrix multiplication, and execute state machine code.");
        Serial.println();
        Serial.println("Iterations/Sec is the main benchmark result; higher numbers are better.");
        Serial.println("This usually requires 12 to 20 seconds...");
        Serial.println();

        coremark_main();
    }
}

void CoreMark::test(Tester& t) {
    DO_TEST(t, "benchmark") {
        coremark_quiet = true;
        coremark_main();
        coremark_quiet = false;

        float score = bluegrass_coremark_result;
        if (abs(score - EXPECTED_SCORE) >= 0.1 * EXPECTED_SCORE) {
            break;
        }
    }
}
