#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

#define BOARD_TUD_RHPORT 0
#define BOARD_TUD_MAX_SPEED OPT_MODE_DEFAULT_SPEED

#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS OPT_OS_NONE
#endif

#define CFG_TUSB_RHPORT0_MODE OPT_MODE_DEVICE
#define CFG_TUD_ENABLED 1
#define CFG_TUD_MAX_SPEED BOARD_TUD_MAX_SPEED
#define CFG_TUSB_MEM_ALIGN __attribute__((aligned(4)))
#define CFG_TUD_ENDPOINT0_SIZE 64

// MIDI only: no CDC serial, mass storage, HID, or vendor interface.
#define CFG_TUD_CDC 0
#define CFG_TUD_MSC 0
#define CFG_TUD_HID 0
#define CFG_TUD_MIDI 1
#define CFG_TUD_VENDOR 0

#define CFG_TUD_MIDI_RX_BUFSIZE 64
#define CFG_TUD_MIDI_TX_BUFSIZE 256

#endif
