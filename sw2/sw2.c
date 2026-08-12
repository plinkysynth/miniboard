#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "hardware/adc.h"
#include "hardware/pio.h"
#include "pico/stdio_rtt.h"
#include "pico/stdlib.h"
#include "uart_tx.pio.h"
#include "usb.c"

// Set to 0 to build without channel aftertouch from ADC0 / GPIO26.
#ifndef ENABLE_AFTERTOUCH
#define ENABLE_AFTERTOUCH 1
#endif

// Set to 1 to swap which TRS contact carries MIDI. Type A is the default.
#ifndef MIDI_TYPE_B
#define MIDI_TYPE_B 0
#endif

#define MIDI_CHANNEL 0
#define BASE_MIDI_NOTE 48
#define VELOCITY_SENSITIVITY 64

#define MIDI_TX_TIP 23
#define MIDI_TX_RING 24
#define AFTERTOUCH_PIN 26
#define AFTERTOUCH_ADC_CHANNEL 0

#define MIDI_TX_PIO pio0
#define MIDI_TX_SM 0
#define MIDI_BAUD 31250

// Fatar keybed pins, in GPIO order:
// B4, R0, M4, B3, R1, M3, R2, B2,
// M2, R3, B1, R4, M1, R5, B0, R6, R7, M0.
#define B4 0
#define R0 1
#define M4 2
#define B3 3
#define R1 4
#define M3 5
#define R2 6
#define B2 7
#define M2 8
#define R3 9
#define B1 10
#define R4 11
#define M1 12
#define R5 13
#define B0 14
#define R6 15
#define R7 16
#define M0 17

#define KEY_ROWS 8
#define KEY_COLUMNS 5
#define KEY_COUNT (KEY_ROWS * KEY_COLUMNS)

// The schematic's row labels run in the opposite direction to the key order.
static const uint8_t row_pins[KEY_ROWS] = {R7, R6, R5, R4, R3, R2, R1, R0};
static const uint8_t break_pins[KEY_COLUMNS] = {B0, B1, B2, B3, B4};
static const uint8_t make_pins[KEY_COLUMNS] = {M0, M1, M2, M3, M4};

#define ROW_PIN_MASK                                                                                                  \
    ((1u << R0) | (1u << R1) | (1u << R2) | (1u << R3) | (1u << R4) | (1u << R5) | (1u << R6) | (1u << R7))

static uint32_t break_time[KEY_COUNT];
static uint8_t note_velocity[KEY_COUNT];

static int clamp_int(int value, int minimum, int maximum) {
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

static void gpio_init_output(uint pin, bool value) {
    gpio_init(pin);
    gpio_put(pin, value);
    gpio_set_dir(pin, GPIO_OUT);
}

static void gpio_init_input_pulldown(uint pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_set_pulls(pin, false, true);
}

static void keyboard_init(void) {
    for (int row = 0; row < KEY_ROWS; ++row) gpio_init_output(row_pins[row], false);
    for (int column = 0; column < KEY_COLUMNS; ++column) {
        gpio_init_input_pulldown(break_pins[column]);
        gpio_init_input_pulldown(make_pins[column]);
    }
}

static void midi_out_init(bool type_b) {
    // A TRS MIDI output drives one contact with serial data and holds the other high.
    gpio_init_output(MIDI_TX_TIP, true);
    gpio_init_output(MIDI_TX_RING, true);

    uint offset = pio_add_program(MIDI_TX_PIO, &uart_tx_program);
    uint tx_pin = type_b ? MIDI_TX_TIP : MIDI_TX_RING;
    uart_tx_program_init(MIDI_TX_PIO, MIDI_TX_SM, offset, tx_pin, MIDI_BAUD);
}

static int midi_message_length(uint8_t status) {
    uint8_t message_type = status & 0xf0u;
    return (message_type == 0xc0u || message_type == 0xd0u) ? 2 : 3;
}

static void midi_send(uint8_t status, uint8_t data1, uint8_t data2) {
    int length = midi_message_length(status);
    pio_sm_put_blocking(MIDI_TX_PIO, MIDI_TX_SM, status);
    pio_sm_put_blocking(MIDI_TX_PIO, MIDI_TX_SM, data1);
    if (length == 3) pio_sm_put_blocking(MIDI_TX_PIO, MIDI_TX_SM, data2);
    usb_midi_write(status, data1, data2, length);

    if (length == 2) printf("MIDI: %02x %02x\n", status, data1);
    else printf("MIDI: %02x %02x %02x\n", status, data1, data2);
}

#if ENABLE_AFTERTOUCH
static void aftertouch_init(void) {
    adc_init();
    gpio_set_pulls(AFTERTOUCH_PIN, false, false);
    adc_gpio_init(AFTERTOUCH_PIN);
    adc_select_input(AFTERTOUCH_ADC_CHANNEL);
    hw_set_bits(&adc_hw->cs, ADC_CS_START_ONCE_BITS);
}

// Input is a 12-bit ADC reading scaled by 16, which preserves precision when
// it comes from the 256-sample accumulator below.
static uint8_t aftertouch_from_adc_16x(int adc_16x) {
    if (adc_16x <= 0) return 0;

    // The Fatar strip is approximately 2.2k at the start of its useful range
    // to 200 ohms fully pressed, through the board's 1k divider to ground.
    const int start_resistance = 2200;
    const int end_resistance = 200;
    const int divider_resistance = 1000;
    const int adc_full_scale_16x = 4095 * 16;
    int resistance = divider_resistance * adc_full_scale_16x / adc_16x - divider_resistance;
    int aftertouch_q10 = (start_resistance - resistance) * 1024 / (start_resistance - end_resistance);
    aftertouch_q10 = clamp_int(aftertouch_q10, 0, 1023);
    return (uint8_t)((aftertouch_q10 * aftertouch_q10) >> 13);
}

static void aftertouch_update(void) {
    if (!(adc_hw->cs & ADC_CS_READY_BITS)) return;

    static int accumulator;
    static int sample_count;
    static uint8_t last_value;
    static uint32_t last_send_time;
    static uint32_t last_raw_print_time;
    enum { OVERSAMPLE_COUNT = 256 };

    uint16_t raw_adc = (uint16_t)adc_hw->result;
    accumulator += raw_adc;
    ++sample_count;
    hw_set_bits(&adc_hw->cs, ADC_CS_START_ONCE_BITS);

    uint32_t now = time_us_32();
    if ((uint32_t)(now - last_raw_print_time) >= 100000u) {
        uint8_t raw_mapped = aftertouch_from_adc_16x((int)raw_adc * 16);
        printf("aftertouch: raw ADC=%u mapped MIDI=%u\n", raw_adc, raw_mapped);
        last_raw_print_time = now;
    }

    if (sample_count != OVERSAMPLE_COUNT) return;

    int adc_16x = accumulator / (OVERSAMPLE_COUNT / 16);
    uint8_t aftertouch = aftertouch_from_adc_16x(adc_16x);

    accumulator = 0;
    sample_count = 0;

    if ((uint32_t)(now - last_send_time) > 10000u && aftertouch != last_value) {
        midi_send(0xd0u | MIDI_CHANNEL, aftertouch, 0);
        last_value = aftertouch;
        last_send_time = now;
    }
}
#endif

static void keyboard_scan(void) {
    static uint8_t scan_row = KEY_ROWS - 1;
    static uint32_t scan_start_time;
    static uint32_t full_scan_time;

    scan_row = (scan_row + 1) & (KEY_ROWS - 1);

    // Only touch the matrix row latches: gpio_put_all() would pull the idle
    // MIDI contact low because it is also an SIO output.
    gpio_put_masked(ROW_PIN_MASK, 1u << row_pins[scan_row]);
    sleep_us(20);

    uint32_t gpios = gpio_get_all();
    uint32_t now = time_us_32();
    if (now == 0) now = 1; // Zero is the "not yet pressed" sentinel.

    if (scan_row == 0) {
        if (scan_start_time != 0) full_scan_time = now - scan_start_time;
        scan_start_time = now;
    }

    for (int column = 0; column < KEY_COLUMNS; ++column) {
        bool break_contact = (gpios >> break_pins[column]) & 1u;
        bool make_contact = (gpios >> make_pins[column]) & 1u;
        int key = scan_row + column * KEY_ROWS;

        if (break_contact) {
            if (break_time[key] == 0) break_time[key] = now;

            if (make_contact && note_velocity[key] == 0) {
                uint32_t travel_time = now - break_time[key];
                int hard_time_us = VELOCITY_SENSITIVITY * 100;
                const int soften_us = 1000;
                int semitone = key % 12;
                bool black_key = semitone == 1 || semitone == 3 || semitone == 6 || semitone == 8 || semitone == 10;
                if (black_key) hard_time_us = hard_time_us * 3 / 4;

                int velocity = (hard_time_us + soften_us) * 127 / ((int)travel_time + soften_us);
                velocity = clamp_int(velocity, 1, 127);
                note_velocity[key] = (uint8_t)velocity;

                uint8_t note = (uint8_t)(BASE_MIDI_NOTE + key);
                printf("note on: key=%d note=%u travel=%uus velocity=%d scan=%uus\n", key, note,
                       (unsigned)travel_time, velocity, (unsigned)full_scan_time);
                midi_send(0x90u | MIDI_CHANNEL, note, (uint8_t)velocity);
            }
        } else {
            if (note_velocity[key] != 0) {
                uint8_t note = (uint8_t)(BASE_MIDI_NOTE + key);
                printf("note off: key=%d note=%u\n", key, note);
                midi_send(0x80u | MIDI_CHANNEL, note, 0);
                note_velocity[key] = 0;
            }
            break_time[key] = 0;
        }
    }
}

int main(void) {
    stdio_rtt_init();
    keyboard_init();
    midi_out_init(MIDI_TYPE_B != 0);
    usb_midi_init();
#if ENABLE_AFTERTOUCH
    aftertouch_init();
#endif

    printf("sw2: Fatar scanner, RTT debug, TRS MIDI type %c on GPIO%d/%d\n",
           MIDI_TYPE_B ? 'B' : 'A', MIDI_TX_TIP, MIDI_TX_RING);
    printf("aftertouch: %s\n", ENABLE_AFTERTOUCH ? "enabled" : "disabled");

    while (true) {
        usb_midi_task();
#if ENABLE_AFTERTOUCH
        aftertouch_update();
#endif
        keyboard_scan();
    }
}
