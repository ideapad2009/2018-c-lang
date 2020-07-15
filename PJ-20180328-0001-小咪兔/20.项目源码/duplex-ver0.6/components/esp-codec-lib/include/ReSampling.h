#ifndef _RE_SAMPLING_H_
#define _RE_SAMPLING_H_

#define INPCM_DELAY_SIZE (6)

typedef struct {
    short inpcm[INPCM_DELAY_SIZE * 2];  ///the pcm value of last time calling.  maximum should be 6: 48000/8000;
    int innum;  /// the total input pcm number
    int outnum; /// the total outnum pcm number
    float hp_mem[4]; ///for filter, the first two is for first channel, the last two is for second channel
} RESAMPLE;

/**
 * @brief re-sampling
 *
 * @param inpcm     : input buffer that musn't be NULL
 * @param outpcm    : output buffer that musn't be NULL
 * @param srcrate   : source sample rate
 * @param tarrate   : target sample rate
 * @param inpcm     : input buffer size in bytes
 * @param channel   : source channel quantity
 * @param resample  : control structure
 *
 * @return
 *     - (0) error
 *     - (Other, > 0) output buffer bytes
 */
int unitSampleProc(short *inPcm, short *outPcm, int srcRate, int tarRate, int inNum, int channel, RESAMPLE *reSample);



int unitSampleProcUpCh(short *inPcm, short *outPcm, int srcRate, int tarRate, int inNum, RESAMPLE *reSample);



int unitSampleProcDownCh(short *inPcm, short *outPcm, int srcRate, int tarRate, int inNum, int channelIdx, RESAMPLE *reSample);


#endif/*_UP_SAMPLING_H_*/
