#ifndef _ESP_AUDIO_ALLOC_H_
#define _ESP_AUDIO_ALLOC_H_

//#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

void *EspAudioAlloc(int n, int size);
void *EspAudioAllocInner(int n, int size);
void EspAudioPrintMemory(const char *Tag);

#ifdef __cplusplus
}
#endif

#endif  //_ESP_AUDIO_ALLOC_H_
