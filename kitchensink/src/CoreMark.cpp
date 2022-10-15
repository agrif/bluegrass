#include "kitchensink.h"

extern "C" int coremark_main(void);

// https://github.com/PaulStoffregen/CoreMark
extern "C" int ee_printf(const char *format, ...)
{
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
extern "C" uint32_t Arduino_millis(void)
{
    return millis();
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
