#include "USBMidi.hpp"

#include "usbd_conf.h"
#include "usbd_core.h"
#include "usbd_def.h"
#include "usbd_desc.h"
#include "usbd_midi.h"

#include "SystemHook.hpp"

#include "FreeRTOS.h"
#include "semphr.h"

namespace bsp {

static USBD_HandleTypeDef USBD_Device;

static constexpr uint32_t kEventBufferSize = MIDI_EPOUT_SIZE;
static MidiEvent midiRxBuffer_[kEventBufferSize];
static uint32_t rpos_;
static uint32_t wpos_;

static StaticSemaphore_t usbMidiSem_;
static SemaphoreHandle_t usbMidiSemHandle_ = NULL;

void USBMidi::Init() {
    HAL_PWREx_EnableUSBVoltageDetector();

    /* Init Device Library */
    auto res = USBD_Init(&USBD_Device, &HID_Desc, 0);
    if (res != USBD_OK) {
        DEVICE_ERROR_CODE("USB", "USBD_Init failed", res);
    }

    /* Add Supported Class */
    res = USBD_RegisterClass(&USBD_Device, &USBD_MIDI);
    if (res != USBD_OK) {
        DEVICE_ERROR_CODE("USB", "USBD_RegisterClass failed", res);
    }

    /* Start Device Process */
    res = USBD_Start(&USBD_Device);
    if (res != USBD_OK) {
        DEVICE_ERROR_CODE("USB", "USBD_Start failed", res);
    }

    // OS init
    usbMidiSemHandle_ = xSemaphoreCreateBinaryStatic(&usbMidiSem_);
}

void USBMidi::WaitForNextBlock() {
    xSemaphoreTake(usbMidiSemHandle_, portMAX_DELAY);
}

MidiEvent* USBMidi::GetNextEvent() {
    if (rpos_ == wpos_) {
        return nullptr;
    }

    auto* e = &midiRxBuffer_[rpos_++];
    rpos_ %= kEventBufferSize;
    return e;
}

// --------------------------------------------------------------------------------
// IRQ
// --------------------------------------------------------------------------------
extern "C" void USBD_MIDI_DataInHandler(uint8_t* usb_rx_buffer, uint8_t usb_rx_buffer_length) {
    while (usb_rx_buffer_length && *usb_rx_buffer != 0x00)
    {
        uint32_t v = (usb_rx_buffer[3] << 24) | (usb_rx_buffer[2] << 16) | (usb_rx_buffer[1] << 8) | usb_rx_buffer[0];
        memcpy(&midiRxBuffer_[wpos_++], &v, 4);

        usb_rx_buffer += 4;
        usb_rx_buffer_length -= 4;
        wpos_ %= kEventBufferSize;
    }

    xSemaphoreGiveFromISR(usbMidiSemHandle_, NULL);
}

}