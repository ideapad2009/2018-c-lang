#ifndef _FDK_MACRO_H_
#define _FDK_MACRO_H_

#include "EspAudioAlloc.h"
#include <stdlib.h>
//#define FDKAAC_LIMITER_ENABLE

#define FKDAAC_MAX_CHANNEL (2)

//#define FDKAAC_SBR_PS_DISABLE

//#define MEM_FDKAAC_ALL_IN_PSRAM /////put all memory into PSRAM, slowest

#define FDK_AAC_SBR_MEM_SHARE

///FDKAAC_AAC_STATICCHANNELINFO_MEMORY_IN_PSRAM and FDKAAC_AACOVERLAP_MEMORY_IN_PSRAM can only define one
///otherwise the speed is slow
///define FDKAAC_AAC_STATICCHANNELINFO_MEMORY_IN_PSRAM speed is faster than define FDKAAC_AACOVERLAP_MEMORY_IN_PSRAM
///however, if define FDKAAC_AACOVERLAP_MEMORY_IN_PSRAM, the memory move to psram is about double compare with define FDKAAC_AAC_STATICCHANNELINFO_MEMORY_IN_PSRAM

//#define FDKAAC_AAC_STATICCHANNELINFO_MEMORY_IN_PSRAM
#define FDKAAC_AACOVERLAP_MEMORY_IN_PSRAM

#define FDKAAC_PS_DEC_MEMORY_IN_PSRAM

#endif