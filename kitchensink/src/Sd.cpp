#include <SdFat.h>

#include "kitchensink.h"

#define TESTSIZE (1 * 1024 * 1024 / INTERACT_BUFFER_LEN)

// https://en.wikipedia.org/wiki/Xorshift#Example_implementation
class PRNG {
    uint32_t a;
public:
    PRNG(uint32_t seed = 0) : a(seed) {}

    inline uint32_t next() {
        uint32_t x = a;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        a = x;
        return x;
    }

    void fill(uint8_t* buffer, size_t len) {
        for (size_t i = 0; i < len; i++) {
            buffer[i] = next() & 0xff;
        }
    }

    bool verify(uint8_t* buffer, size_t len) {
        for (size_t i = 0; i < len; i++) {
            if (buffer[i] != next() & 0xff) {
                return false;
            }
        }
        return true;
    }
};

Sd::Sd() : fs(), card_present(false), volume_present(false) {}

const char* Sd::name() {
    return "SD Card";
}

void Sd::setup() {
}

bool Sd::ensure(bool volume) {
    if (!card_present) {
        volume_present = false;
        card_present = fs.cardBegin(SdSpiConfig(PA4, SHARED_SPI, SD_SCK_MHZ(16)));
    }

    if (card_present) {
        // verify that the card is present
        uint32_t ocr = 0;
        if (!fs.card()->readOCR(&ocr)) {
            card_present = false;
            volume_present = false;
            return false;
        }

        volume_present = fs.volumeBegin();
    }

    if (volume) {
        return volume_present;
    }
    return card_present;
}

void Sd::match(Matcher& m) {
    if (m.match("get card info", "sd info")) {
        if (!ensure(false)) {
            Serial.println("SD card not detected.");
            return;
        }

        cid_t cid;
        csd_t csd;
        scr_t scr;
        uint32_t ocr = 0;

        SdCard* card = fs.card();
        card->readCID(&cid);
        card->readCSD(&csd);
        card->readSCR(&scr);
        card->readOCR(&ocr);

        Serial.print("Card type: ");
        switch (fs.card()->type()) {
        case SD_CARD_TYPE_SD1:
            Serial.println("SD1");
            break;
        case SD_CARD_TYPE_SD2:
            Serial.println("SD2");
            break;
        case SD_CARD_TYPE_SDHC:
            if (csd.capacity() < 70000000) {
                Serial.println("SDHC");
            } else {
                Serial.println("SDXC");
            }
            break;
        default:
            Serial.println("Unknown");
            break;
        }
        int16_t sdver = scr.sdSpecVer();
        Serial.printf("Specification Version: %d.%02d\r\n", sdver / 100, sdver % 100);
        Serial.printf("Manufacturer ID: 0x%02x\r\n", cid.mid);
        Serial.printf("OEM ID: 0x%02x 0x%02x\r\n", cid.oid[0], cid.oid[1]);
        Serial.print("Product: ");
        for (int i = 0; i < 5; i++) {
            Serial.write(cid.pnm[i]);
        }
        Serial.println();
        Serial.printf("Revision: %d.%d\r\n", cid.prvN(), cid.prvM());
        Serial.printf("Serial Number: 0x%08x\r\n", cid.psn());
        Serial.printf("Manufacturing Date: %d/%d\r\n", cid.mdtMonth(), cid.mdtYear());
        Serial.printf("Card Size: %d kiB\r\n", csd.capacity() / 2);
        Serial.printf("Flash Erase Size: %d blocks\r\n", csd.eraseSize());
        Serial.printf("Erase Single Block: %s\r\n", csd.eraseSingleBlock() ? "true" : "false");
        Serial.printf("Data After Erase: %s\r\n", scr.dataAfterErase() ? "ones" : "zeros");
        Serial.printf("OCR: 0x%08x\r\n", ocr);

        Serial.println();

        if (!ensure()) {
            Serial.println("Could not find FAT16/FAT32/exFAT volume.");
            return;
        }

        Serial.print("Filesystem: ");
        switch (fs.fatType()) {
        case FAT_TYPE_EXFAT:
            Serial.println("exFAT");
            break;
        case FAT_TYPE_FAT32:
            Serial.println("FAT32");
            break;
        case FAT_TYPE_FAT16:
            Serial.println("FAT16");
            break;
        default:
            Serial.println("Unknown");
            break;
        }
        Serial.printf("Clusters: %d\r\n", fs.clusterCount());
        Serial.printf("Cluster Size: %d bytes\r\n", fs.bytesPerCluster());
        Serial.printf("Volume Size: %d kiB\r\n",
                      fs.clusterCount() * (fs.bytesPerCluster() / 1024));
    }

    const char* filenamedat = "bluegrass.dat";
    if (m.match("run a speed test", "sd speed <filename>", &filenamedat)) {
        if (!ensure()) {
            Serial.println("SD card or FAT volume not detected.");
            return;
        }

        uint8_t buffer[INTERACT_BUFFER_LEN];

        FsFile fw = fs.open(filenamedat, O_WRITE | O_CREAT | O_TRUNC);
        if (!fw) {
            Serial.println("Could not open file for writing.");
            return;
        }

        PRNG rng;
        rng.fill(buffer, INTERACT_BUFFER_LEN);

        unsigned long write_start = millis();
        for (size_t i = 0; i < TESTSIZE; i++) {
            if (fw.write(buffer, INTERACT_BUFFER_LEN) != INTERACT_BUFFER_LEN) {
                Serial.println("Could not write to file.");
                return;
            }
        }
        int write_time = millis() - write_start;
        Serial.printf("Wrote %i bytes in %i ms, %f KiB/s.\r\n", TESTSIZE * INTERACT_BUFFER_LEN, write_time, 1000.0 * TESTSIZE * INTERACT_BUFFER_LEN / 1024 / write_time);
        fw.close();

        FsFile fr = fs.open(filenamedat);
        if (!fr) {
            Serial.println("Could not open file for reading.");
            return;
        }

        unsigned long read_start = millis();
        for (size_t i = 0; i < TESTSIZE; i++) {
            if (fr.read(buffer, INTERACT_BUFFER_LEN) != INTERACT_BUFFER_LEN) {
                Serial.println("Could not read from file.");
                return;
            }
        }
        int read_time = millis() - read_start;
        Serial.printf("Read %i bytes in %i ms, %f KiB/s.\r\n", TESTSIZE * INTERACT_BUFFER_LEN, read_time, 1000.0 * TESTSIZE * INTERACT_BUFFER_LEN / 1024 / read_time);
        fr.close();

        if (!fs.remove(filenamedat)) {
            Serial.println("Could not remove file.");
            return;
        }
    }

    const char* path = "/";
    if (m.match("get card info", "sd ls <path>", &path)) {
        if (!ensure()) {
            Serial.println("SD card or FAT volume not detected.");
            return;
        }

        fs.ls(path, LS_DATE | LS_SIZE);
    }

    const char* filename = "bluegrass.txt";
    const char* contents = "Hello, world!";
    if (m.match("write a file", "sd write <filename> <contents>", &filename, &contents)) {
        if (!ensure()) {
            Serial.println("SD card or FAT volume not detected.");
            return;
        }

        FsFile f = fs.open(filename, FILE_WRITE);
        if (!f) {
            Serial.println("Could not open file.");
            return;
        }
        f.println(contents);
        f.close();
    }

    if (m.match("read a file", "sd read <filename>", &filename)) {
        if (!ensure()) {
            Serial.println("SD card or FAT volume not detected.");
            return;
        }

        File f = fs.open(filename);
        if (!f) {
            Serial.println("Could not open file.");
            return;
        }
        char buffer[INTERACT_BUFFER_LEN];
        while (true) {
            int amount = f.read(buffer, INTERACT_BUFFER_LEN - 1);
            if (amount <= 0) {
                break;
            }
            buffer[amount] = 0;
            Serial.print(buffer);
        }
        f.close();
    }

    if (m.match("delete a file", "sd rm <filename>", &filename)) {
        if (!ensure()) {
            Serial.println("SD card or FAT volume not detected.");
            return;
        }

        if (!fs.remove(filename)) {
            Serial.println("Could not remove file.");
        }
    }
}

void Sd::test(Tester& t) {
    DO_TEST(t, "connect") {
        if (!ensure()) {
            break;
        }
    }

    uint8_t buffer[INTERACT_BUFFER_LEN];
    const char* filename = "/bluegrass-test-random.dat";

    DO_TEST(t, "write") {
        FsFile f = fs.open(filename, O_WRITE | O_CREAT | O_TRUNC);
        if (!f) {
            break;
        }
        PRNG rng;
        size_t i;
        for (i = 0; i < TESTSIZE; i++) {
            rng.fill(buffer, INTERACT_BUFFER_LEN);
            if (f.write(buffer, INTERACT_BUFFER_LEN) != INTERACT_BUFFER_LEN) {
                break;
            }
        }
        f.close();
        if (i != TESTSIZE) {
            break;
        }
    }

    DO_TEST(t, "read") {
        FsFile f = fs.open(filename);
        if (!f) {
            break;
        }
        PRNG rng;
        size_t i;
        for (i = 0; i < TESTSIZE; i++) {
            if (f.read(buffer, INTERACT_BUFFER_LEN) != INTERACT_BUFFER_LEN) {
                break;
            }
            if (!rng.verify(buffer, INTERACT_BUFFER_LEN)) {
                break;
            }
        }
        f.close();
        if (i != TESTSIZE) {
            break;
        }
    }

    DO_TEST(t, "delete") {
        if (!fs.remove(filename)) {
            break;
        }
    }
}
