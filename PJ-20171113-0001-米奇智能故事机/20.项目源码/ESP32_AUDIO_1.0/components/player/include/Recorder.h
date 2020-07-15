// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __RECORDER_H__
#define __RECORDER_H__
#include "Recorder.h"

struct RingBuf;
enum AudioStatus;
enum TerminationType;

typedef struct RecorderPara {
    int i2sNum;
    const char *uri;
    struct RingBuf *rb;
    void *sem;
    void *rbSem;
} RecorderPara;

int EspAudioRecorderStateGet(enum AudioStatus *state);
int EspAudioRecorderStart(const char *uri, int bufSize, int i2sNum);
int EspAudioRecorderRead(uint8_t *data, int bufSize, int *outSize);
int EspAudioRecorderStop(enum TerminationType type);

#endif //__RECORDER_H__
