// This file is included by sw2.c

#include <string.h>

#include "tusb.h"

// made up vid/pid
#define USB_VID 0xcaff
#define USB_PID 0x2026

enum {
    USB_STRING_LANGUAGE = 0,
    USB_STRING_MANUFACTURER,
    USB_STRING_PRODUCT,
    USB_STRING_MIDI,
    USB_STRING_COUNT,
};

static const char *const usb_strings[USB_STRING_COUNT] = {
    (const char[]){0x09, 0x04}, // English (United States), 0x0409
    "Plinky",
    "miniboard",
    "miniboard MIDI",
};

static const tusb_desc_device_t usb_device_descriptor = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = USB_VID,
    .idProduct = USB_PID,
    .bcdDevice = 0x0100,
    .iManufacturer = USB_STRING_MANUFACTURER,
    .iProduct = USB_STRING_PRODUCT,
    .iSerialNumber = 0,
    .bNumConfigurations = 1,
};

enum {
    USB_INTERFACE_MIDI = 0,
    USB_INTERFACE_MIDI_STREAMING,
    USB_INTERFACE_COUNT,
};

#define USB_MIDI_OUT_ENDPOINT 0x01
#define USB_MIDI_IN_ENDPOINT 0x81
#define USB_CONFIGURATION_LENGTH (TUD_CONFIG_DESC_LEN + TUD_MIDI_DESC_LEN)

static const uint8_t usb_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, USB_INTERFACE_COUNT, 0, USB_CONFIGURATION_LENGTH,
                          0, 100),
    TUD_MIDI_DESCRIPTOR(USB_INTERFACE_MIDI, USB_STRING_MIDI,
                        USB_MIDI_OUT_ENDPOINT, USB_MIDI_IN_ENDPOINT, 64),
};

uint8_t const *tud_descriptor_device_cb(void) {
    return (const uint8_t *)&usb_device_descriptor;
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return usb_configuration_descriptor;
}

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t language_id) {
    (void)language_id;
    static uint16_t descriptor[32];
    size_t character_count;

    if (index == USB_STRING_LANGUAGE) {
        memcpy(&descriptor[1], usb_strings[0], 2);
        character_count = 1;
    } else {
        if (index >= USB_STRING_COUNT) return NULL;
        const char *string = usb_strings[index];
        character_count = strlen(string);
        if (character_count > 31) character_count = 31;
        for (size_t i = 0; i < character_count; ++i) descriptor[1 + i] = (uint8_t)string[i];
    }

    descriptor[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * character_count + 2));
    return descriptor;
}

static void usb_midi_init(void) {
    const tusb_rhport_init_t device_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_AUTO,
    };
    if (!tusb_init(BOARD_TUD_RHPORT, &device_init)) printf("USB MIDI init failed\n");
}

static void usb_midi_task(void) {
    tud_task();

    // TinyUSB's MIDI descriptor has an OUT jack too; discard anything received
    // so a host cannot fill the receive FIFO and stall.
    while (tud_midi_available()) {
        uint8_t packet[4];
        tud_midi_packet_read(packet);
    }
}

static void usb_midi_write(uint8_t status, uint8_t data1, uint8_t data2, int length) {
    if (!tud_mounted() || tud_suspended()) return;

    const uint8_t message[3] = {status, data1, data2};
    uint32_t written = tud_midi_stream_write(0, message, (uint32_t)length);
    if (written != (uint32_t)length) printf("USB MIDI TX buffer full\n");
}
