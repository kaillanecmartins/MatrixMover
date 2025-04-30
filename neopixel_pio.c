#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2818b.pio.h"

#define LED_COUNT 25
#define LED_PIN 7
#define CENTER 2048
#define DEADZONE 400

PIO np_pio;
uint sm;
typedef struct {
    uint8_t G, R, B;
} npLED_t;
npLED_t leds[LED_COUNT];

void npInit(uint pin) {
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;

    sm = pio_claim_unused_sm(np_pio, false);
    if (sm < 0) {
        np_pio = pio1;
        sm = pio_claim_unused_sm(np_pio, true);
    }

    ws2818b_program_init(np_pio, sm, offset, pin, 800000.0f);
    for (uint i = 0; i < LED_COUNT; ++i) {
        leds[i].R = 0;
        leds[i].G = 0;
        leds[i].B = 0;
    }
}

void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
    if (index < LED_COUNT) {
        leds[index].R = r;
        leds[index].G = g;
        leds[index].B = b;
    }
}

void npClear() {
    for (uint i = 0; i < LED_COUNT; ++i) {
        npSetLED(i, 0, 0, 0);
    }
}

void npWrite() {
    for (uint i = 0; i < LED_COUNT; ++i) {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
}

void draw_shape(const uint *indices, size_t len, uint8_t r, uint8_t g, uint8_t b) {
    npClear();
    for (size_t i = 0; i < len; i++) {
        npSetLED(indices[i], r, g, b);
    }
    npWrite();
}

int main() {
    stdio_init_all();
    adc_init();
    adc_gpio_init(26); //VRY
    adc_gpio_init(27); //VRX

    npInit(LED_PIN);

    uint8_t intensity = 244;

    const uint down[] = {2,6,7,8,10,12,14,17,22}; 
    const uint up[] = {2,7,10,12,14,16,17,18,22};
    const uint right[] = {2,8,10,11,12,13,14,18,22};
    const uint left[] = {2,6,10,11,12,13,14,16,22};
    const uint center[] = {2,6,8,10,14,15,17,19,21,23};

    while (true) {
        adc_select_input(0);
        uint16_t vry = adc_read();

        adc_select_input(1);
        uint16_t vrx = adc_read();

        if (vrx > CENTER + DEADZONE) {
            draw_shape(right, sizeof(right)/sizeof(right[0]), 0, intensity, intensity);
        } else if (vrx < CENTER - DEADZONE) {
            draw_shape(left, sizeof(left)/sizeof(left[0]), 0, 0, intensity);
        } else if (vry > CENTER + DEADZONE) {
            draw_shape(up, sizeof(down)/sizeof(down[0]), intensity, 0, intensity);
        } else if (vry < CENTER - DEADZONE) {
            draw_shape(down, sizeof(up)/sizeof(up[0]), intensity, intensity, 0);
        } else {
            draw_shape(center, sizeof(center)/sizeof(center[0]), intensity, intensity, intensity);
        }

        sleep_ms(200);
    }

    return 0;
}
