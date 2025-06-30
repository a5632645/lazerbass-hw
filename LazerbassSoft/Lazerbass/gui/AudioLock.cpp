#include "AudioLock.hpp"

#include "FreeRTOS.h"
#include "semphr.h"

namespace gui {

void AudioLock::Lock() {
    SemaphoreHandle_t handle = static_cast<SemaphoreHandle_t>(rtosSemHandle_);
    xSemaphoreTake(handle, portMAX_DELAY);
}

void AudioLock::Unlock() {
    SemaphoreHandle_t handle = static_cast<SemaphoreHandle_t>(rtosSemHandle_);
    xSemaphoreGive(handle);
}

}