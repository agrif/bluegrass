#include "kitchensink.h"

#define YEAR_OFFSET 2000

Rtc::Rtc() : rtc(STM32RTC::getInstance()) {}

const char* Rtc::name() {
    return "Real-time Clock";
}

void Rtc::setup() {
    rtc.setClockSource(STM32RTC::LSE_CLOCK);
    rtc.begin();
}

void Rtc::match(Matcher& m) {
    if (m.match("read the RTC", "rtc get")) {
        Serial.printf("%02d/%02d/%02d %02d:%02d:%02d\r\n",
                      rtc.getYear() + YEAR_OFFSET, rtc.getMonth(), rtc.getDay(),
                      rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());
    }

    if (m.match("read the RTC as unix time", "rtc get epoch")) {
        Serial.println(rtc.getEpoch());
    }

    if (m.match("get the weekday", "rtc get weekday")) {
        switch (rtc.getWeekDay() % 7) {
        case RTC_WEEKDAY_MONDAY:
            Serial.println("Monday");
            break;
        case RTC_WEEKDAY_TUESDAY:
            Serial.println("Tuesday");
            break;
        case RTC_WEEKDAY_WEDNESDAY:
            Serial.println("Wednesday");
            break;
        case RTC_WEEKDAY_THURSDAY:
            Serial.println("Thursday");
            break;
        case RTC_WEEKDAY_FRIDAY:
            Serial.println("Friday");
            break;
        case RTC_WEEKDAY_SATURDAY:
            Serial.println("Saturday");
            break;
        case RTC_WEEKDAY_SUNDAY:
            Serial.println("Sunday");
            break;
        default:
            Serial.println("Unknown");
        }
    }

    int day = 1, month = 1, year = 1;
    if (m.match("set the RTC date", "rtc set date <int:year> <int:month> <int:day>", &year, &month, &day)) {
        if (year >= 100) {
            if (year >= YEAR_OFFSET) {
                year -= YEAR_OFFSET;
            } else {
                Serial.println("Invalid year.");
                return;
            }
        }
        rtc.setDate(day, month, year % 100);
    }

    int hour = 0, minute = 0, second = 0;
    if (m.match("set the RTC time", "rtc set time <int:hour> <int:minute> <int:second>", &hour, &minute, &second)) {
        rtc.setTime(hour, minute, second);
    }

    int epoch = 0;
    if (m.match("set the RTC time as unix time", "rtc set epoch <int:epoch>", &epoch)) {
        rtc.setEpoch(epoch, 0);
    }
}

void Rtc::test(Tester& t) {
    DO_TEST(t, "sanity check") {
        uint8_t hours, minutes, seconds;
        uint8_t day, month, year;

        rtc.getTime(&hours, &minutes, &seconds, NULL);
        rtc.getDate(NULL, &day, &month, &year);
        if (hours >= 24 || minutes >= 60 || seconds >= 61) {
            break;
        }
        if (day >= 32 || month >= 13) {
            break;
        }
    }

    DO_TEST(t, "timing check") {
        uint16_t wait = 10;
        uint16_t margin = 1;
        uint32_t a = rtc.getEpoch();
        delay(wait * 1000);
        uint32_t b = rtc.getEpoch();
        if (b > a + wait + margin || b < a + wait - margin) {
            break;
        }
    }

    DO_TEST(t, "set and get") {
        uint8_t hours, minutes, seconds;
        uint8_t day, month, year;
        uint32_t epoch = rtc.getEpoch();

        rtc.setTime(1, 2, 3);
        rtc.setDate(4, 5, 6);
        rtc.getTime(&hours, &minutes, &seconds, NULL);
        rtc.getDate(NULL, &day, &month, &year);
        rtc.setEpoch(epoch);

        if (hours != 1 || minutes != 2 || seconds != 3) {
            break;
        }
        if (day != 4 || month != 5 || year != 6) {
            break;
        }
    }
}
