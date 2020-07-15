#ifndef __ASR_ENGINE_H__
#define __ASR_ENGINE_H__



#define ONE_BLOCK_SIZE 3200     // 100ms[16k,16bit,1channel]

#define DEFAULT_ASR_ENGINE_CONFIG() {\
    .i2s_num                = 0,\
    .sensitivity            = 0,\
    .vad_off_time           = 500,\
    .wakeup_keep_time       = 5000,\
}

typedef enum asr_event_type_t {
    ASR_EVENT_WAKEUP_START,
    ASR_EVENT_WAKEUP_END,
    ASR_EVENT_VAD_START,
    ASR_EVENT_VAD_STOP,
} asr_event_type_t;


typedef enum asr_voice_hold_t {
    ASR_VOICE_HOLD_OFF,
    ASR_VOICE_HOLD_ON,
    ASR_VOICE_END,
} asr_voice_hold_t;

typedef void (*asr_callback)(asr_event_type_t type);

typedef struct asr_engine_t {
    int i2s_num;    // I2s number
    int sensitivity; // For vad sensitivity.Uint is ms. Not support now.
    int vad_off_time;  // Will be stop by vad silence. unit is ms.
    int wakeup_keep_time;  // keep time after vad off. unit is ms.
} asr_engine_t;


/**
 * @brief Create ASR engine according to parameters.
 *
 * @param eng   See PlayerConfig structure for more help
 * @param func  ASR event callback function
 *
 * @return
 *     - 0: Success
 *     - -1: Error
 */
esp_err_t asr_engine_create(asr_engine_t *eng, asr_callback func);

/**
 * @brief Read voice data after ASR_EVENT_VAD_START.
 *
 * @param buffer     data pointer
 * @param buffer_size  Size of buffer, must be equality ONE_BLOCK_SIZE.
 * @param waitting_time  Timeout for read data, time of ONE_BLOCK_SIZE is 100ms, recommend larger than 100ms.
 *
 * @return
 *      - 0: no voice block,or last voice block..
 *      - others: voice block index.
 */
int asr_engine_data_read(uint8_t *buffer, int buffer_size, int waitting_time);

/**
 * @brief Detect voice data or not.
 *
 * @param flag  ASR_VOICE_HOLD_ON: do not detect voice.
 *              ASR_VOICE_HOLD_OFF:will be detect voice.
 *              ASR_VOICE_END: out of wakeup time.
 *
 * @return
 *      - 0: Success
 *      - -1: Error
 */
esp_err_t asr_engine_wakeup_hold(asr_voice_hold_t flag);

/**
 * @brief Start voice by force.
 *
 * @param none.
 *
 * @return
 *      - 0: Success
 *      - -1: Error
 */
esp_err_t asr_engine_trigger_start(void);

/**
 * @brief Stop voice by force.
 *
 * @param none.
 *
 * @return
 *      - 0: Success
 *      - -1: Error
 */
esp_err_t asr_engine_trigger_stop(void);

/**
 * @brief Destroy the asr engine.
 *
 * @param None.
 *
 * @return
 *      - 0: Success
 *      - -1: Error
 */
esp_err_t asr_engine_destroy(void);

#endif