#include "kitchensink.h"
#include "tom-thumb.h"

#define BMP_PRIMARY_LEN 14
#define BMP_SECONDARY_LEN 12
#define BMP_HEADER_LEN (BMP_PRIMARY_LEN + BMP_SECONDARY_LEN)
#define BMP_WIDTH 640
#define BMP_HEIGHT 480

#define FONT_SCALE 2

#define MANDELBROT_ITER 100

#define SPINNER_LEN 4
static const char* spinner = "-\\|/";

typedef struct {
    uint8_t r, g, b;
} Pixel;

#define COLORMAP_LEN 16
Pixel colormap[COLORMAP_LEN] = {
    {66, 30, 15},
    {25, 7, 26},
    {9, 1, 47},
    {4, 4, 73},

    {0, 7, 100},
    {12, 44, 138},
    {24, 82, 177},
    {57, 125, 209},

    {134, 181, 229},
    {211, 236, 248},
    {241, 233, 191},
    {248, 201, 95},

    {255, 170, 0},
    {204, 128, 0},
    {153, 87, 0},
    {106, 52, 3},
};

Pixel lerp(const Pixel& a, const Pixel& b, float p) {
    Pixel c;
    uint16_t pb = 255 * p;
    uint16_t pa = 255 - pb;
    c.r = (a.r * pa + b.r * pb) / 255;
    c.g = (a.g * pa + b.g * pb) / 255;
    c.b = (a.b * pa + b.b * pb) / 255;
    return c;
}

Pixel mandelbrot(uint16_t x, uint16_t y) {
    float fx = (float)x / (BMP_WIDTH - 1);
    float fy = (float)y / (BMP_HEIGHT - 1);

    // centering
    fx -= 0.5;
    fy -= 0.5;

    // aspect ratio correction
    fx *= BMP_WIDTH;
    fx /= BMP_HEIGHT;

    // scale
    fx *= 2.5;
    fy *= 2.5;
    fx -= 0.7;

    uint16_t it;
    float zr = fx;
    float zi = fy;
    for (it = 0; it < MANDELBROT_ITER; it++) {
        if (zr * zr + zi * zi >= (1 << 4)) {
            break;
        }
        float tmp = zr * zr - zi * zi + fx;
        zi = 2 * zr * zi + fy;
        zr = tmp;
    }

    if (it == MANDELBROT_ITER) {
        return {0, 0, 0};
    }

    float nu = log2f(log2f(zr * zr + zi * zi) / 2.0);
    float iter = it + 1 - nu;

    float ipart;
    float fpart = modf(iter, &ipart);
    uint8_t idx = (uint8_t)ipart;
    return lerp(colormap[idx % COLORMAP_LEN],
                colormap[(idx + 1) % COLORMAP_LEN], fpart);
}

unsigned short font_index(const struct bitmap_font& font, unsigned short c) {
    unsigned short end = font.chars;
    unsigned short start = 0;
    unsigned short i = end / 2;
    while (start != end) {
        if (font.index[i] == c) {
            return i;
        } else if (font.index[i] < c) {
            start = i;
            i = (start + end) / 2;
        } else {
            end = i;
            i = (start + end) / 2;
        }
    }
    // this is the space character usually, good enough
    return 0;
}

void draw_text(FsFile& file, uint16_t x, uint16_t y, const Pixel& p, const char* msg) {
    const struct bitmap_font& font = tom_thumb;

    for (uint8_t sy = 0; sy < font.height; sy++) {
        for (uint8_t scale_j = 0; scale_j < FONT_SCALE; scale_j++) {
            file.seek(BMP_HEADER_LEN + 3 * (y * BMP_WIDTH + x));
            y--;

            for (unsigned int i = 0; msg[i] != 0; i++) {
                unsigned char data = font.bitmap[font.height * font_index(font, msg[i]) + sy];
                for (uint8_t sx = 0; sx < font.width; sx++) {
                    for (uint8_t scale_i = 0; scale_i < FONT_SCALE; scale_i++) {
                        if (data & 0x80) {
                            file.write(p.b);
                            file.write(p.g);
                            file.write(p.r);
                        } else {
                            file.seekCur(3);
                        }
                    }

                    data <<= 1;
                }
            }
        }
    }
}

void draw_text_outline(FsFile& file, uint16_t x, uint16_t y, const Pixel& f, const Pixel& b, const char* msg) {
    draw_text(file, x - 1, y, b, msg);
    draw_text(file, x + 1, y, b, msg);
    draw_text(file, x, y - 1, b, msg);
    draw_text(file, x, y + 1, b, msg);
    draw_text(file, x, y, f, msg);
}

void write_u16(FsFile& file, uint16_t val) {
    file.write((char)(val & 0xff));
    file.write((char)(val >> 8));
}

void write_u32(FsFile& file, uint32_t val) {
    file.write((char)(val & 0xff));
    file.write((char)((val >> 8) & 0xff));
    file.write((char)((val >> 16) & 0xff));
    file.write((char)((val >> 24) & 0xff));
}

void draw_bmp(FsFile& file) {
    // magic number
    file.print("BM");
    // size of the file, in bytes
    write_u32(file, BMP_HEADER_LEN + 3 * BMP_WIDTH * BMP_HEIGHT);
    // reserved
    write_u32(file, 0);
    // offset to pixel data
    write_u32(file, BMP_HEADER_LEN);
    // size of secondary header
    write_u32(file, BMP_SECONDARY_LEN);
    // width
    write_u16(file, BMP_WIDTH);
    // height
    write_u16(file, BMP_HEIGHT);
    // color panes
    write_u16(file, 1);
    // bits per pixel
    write_u16(file, 24);

    // now, for pixel data
    Serial.print(" [ ]     ");
    for (int y = 0; y < BMP_HEIGHT; y++) {
        unsigned int progress = (y + 1) * 100 / BMP_HEIGHT;
        Serial.printf("\b\b\b\b\b\b\b\b\b [%c] %3i%%",
                      spinner[y % SPINNER_LEN], progress);

        for (int x = 0; x < BMP_WIDTH; x++) {
            Pixel p = mandelbrot(x, y);
            file.write(p.b);
            file.write(p.g);
            file.write(p.r);
        }
    }

    char buf[32];
    size_t size = 32;
    size_t written = 0;
    written += snprintf(buf, size - written, "ID # ");
    id_number(&buf[written], size - written);

    draw_text_outline(file, 20, 20 + 3 * tom_thumb.height * FONT_SCALE,
                      {255, 255, 255}, {0, 0, 0},
                      BLUEGRASS_NAME " (" BLUEGRASS_REV ", " BLUEGRASS_GIT ")");
    draw_text_outline(file, 20, 20 + 2 * tom_thumb.height * FONT_SCALE,
                      {255, 255, 255}, {0, 0, 0},
                      buf);
    draw_text_outline(file, 20, 20 + tom_thumb.height * FONT_SCALE,
                      {255, 255, 255}, {0, 0, 0},
                      BLUEGRASS_URL);

    Serial.println("\b\b\b\b\b\b\b\b\b [X] done.");
}
