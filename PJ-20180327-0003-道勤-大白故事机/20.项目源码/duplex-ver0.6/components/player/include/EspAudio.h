#ifndef _ESP_AUDIO_H_
#define _ESP_AUDIO_H_
#include "AudioDef.h"

struct PlayerBufCfg;

typedef enum EspOutStreamType {
    OUT_STREAM_I2S = 0x01,
    OUT_STREAM_SDCARD = 0x02,
} EspOutStreamType;

typedef enum EspInStreamType {
    IN_STREAM_I2S = 0x01,
    IN_STREAM_SDCARD = 0x02,
    IN_STREAM_HTTP = 0x04,
    IN_STREAM_FLASH = 0x08,
    IN_STREAM_LIVE = 0x10,
    IN_STREAM_ARRAY = 0x20,
} EspInStreamType;

typedef enum EspAudioEn {
    ESP_AUDIO_ENABLE = 0X01,
    ESP_AUDIO_DISABLE = 0X02,
} EspAudioEn;

typedef struct PlayerConfig {
    struct PlayerBufCfg bufCfg;
    EspInStreamType inputStreamType;
    EspOutStreamType outputStreamType;
    EspAudioEn playListEn;
    EspAudioEn toneEn;
    enum PlayMode playMode;
} PlayerConfig;


/**
 * @brief Init player according to 'cfg' param
 *
 * @param cfg     See PlayerConfig structure for more help
 * @param handle  Pass the player pointer to 'handle' which used in 'player->addService' or other activities
 *
 * @return See AudioErr structure
 */
int EspAudioInit(const PlayerConfig *cfg, void **handle);

/**
 * @brief Uninit player and free all the memory
 *
 * @return See AudioErr structure
 */
int EspAudioUninit(void);

/**
 * @brief Add a codec lib that can decode or encode this kind of music
 *
 * @param type          Decode or encode
 * @param stackSize     Decoder or encoder task stack size
 * @param extension     Filename extension, such as "mp3"
 * @param open          Function to open this type of music file
 * @param process       Function to process the music file
 * @param close         Function to shut down the 'process' and to invoke 'triggerStop'
 * @param triggerStop   Function to kill the process that decodes or encodes the music data
 *
 * @return See AudioErr structure
 */
int EspAudioCodecLibAdd(AudioCodecType type, int stackSize, const char *extension, void *open, void *process, void *close, void *triggerStop);

/*
 * @brief Check if this kind of music file is supported or not
 *
 * @param type       Decoder or encoder
 * @param extension  Such as "mp3", "wav", "aac"...
 *
 * @return See AudioErr structure
 */
int EspAudioCodecLibQuery (AudioCodecType type, const char *extension);

/*
 * @brief Add a queue reciever that recieves a PlayerStatus structure that indicate the current player status,
 *          see PlayerStatus structure for more help
 *
 * @param handle Freertos queue descriptor
 *
 * @return See AudioErr structure
 */
int EspAudioStatusListenerAdd(void *handle);

/**
 * @brief Play the given uri
 *
 * @param uri  Such as "file:///sdcard/short.wav" or "http://iot.espressif.com/file/example.mp3" or "flsh:///example.aac"
 *               if NUll then play current uri that already set before
 *
 * @return See AudioErr structure
 */
int EspAudioPlay(const char *uri);

/**
 * @brief Stop the music immediately
 *
 * @return See AudioErr structure
 */
int EspAudioStop(void);

/**
 * @brief Pause the music, can not pause Raw and Tone
 *
 * @return See AudioErr structure
 */
int EspAudioPause(void);

/**
 * @brief Resume the music paused
 *
 * @return See AudioErr structure
 */
int EspAudioResume(void);

/*
 * @brief Skip to a desired position in seconds
 *
 * @param time  Position in seconds
 *
 * @return See AudioErr structure
 */
int EspAudioSeek(int time);

/**
 * @brief Play the given uri as tone which can interrupt the current music playing
 *
 * @param uri  Such as "file:///sdcard/short.wav" or "http://iot.espressif.com:8008/file/Windows.wav",
 *
 * @param type See TerminationType enum for more help
 *
 * @return See AudioErr structure
 */
int EspAudioTonePlay(const char *uri, enum TerminationType type);

/**
 * @brief Stop the tone immediately
 *
 * @return See AudioErr structure
 */
int EspAudioToneStop(void);

/**
 * @brief Play the music according to 'uri' param and 'EspAudioRawWrite()', where 'uri' set up the music type and 'EspAudioRawWrite' throw the data.
 *          This function can also record data from MIC and store the data into a buffer that 'EspAudioRawRead()' can read.
 *          Refer to TerminalControlService.c
 *
 * @param uri  Must conform to uri's rule like ( "raw://%d:%d@from.pcm/to.%s#i2s", 16000[sampleRate], 2[channel], "mp3"[music type] ) for play,
 *               while "i2s://16000:1@record.pcm#raw" for record
 *
 * @return See AudioErr structure
 */
int EspAudioRawStart(const char *uri);

/**
 * @brief Stop playting raw data
 *
 *  @param type See TerminationType enum for more help
 *
 * @return See AudioErr structure
 */
int EspAudioRawStop(enum TerminationType type);

/**
 * @brief See 'EspAudioRawStart' comments
 *
 * @param data      Pointer to a buffer to store the data from the input driver
 * @param bufSize   Buffer size
 * @param readBytes Bytes read
 *
 * @return See AudioErr structure
 */
int EspAudioRawRead(void *data, int bufSize, int *readBytes);

/**
 * @brief See 'EspAudioRawStart' comments
 *
 * @param data          Pointer to a buffer to feed the data into output driver
 * @param bufSize       Buffer size
 * @param writtenBytes  Bytes written
 *
 * @return See AudioErr structure
 */
int EspAudioRawWrite(void *data, int bufSize, int *writtenBytes);

/*
 * @brief Volume setting
 *
 * @param vol Volume ranged 0 ~ 100, precision is 3
 *
 * @return See AudioErr structure
 */
int EspAudioVolSet(int vol);

/*
 * @brief Get volume
 *
 * @param vol Pointer to an int that gets volume ranged 0 ~ 100, precision is 3
 *
 * @return See AudioErr structure
 */
int EspAudioVolGet(int *vol);

/*
 * @brief Get player status
 *
 * @param mode See PlayerStatus structure for more help
 *
 * @return See AudioErr structure
 */
int EspAudioStatusGet(PlayerStatus *player);

/*
 * @brief Get the playing time of current music in seconds
 *
 * @param pos  Pointer to an int that stores the result
 *
 * @return See AudioErr structure
 */
int EspAudioTimeGet(int *pos);

/*
 * @brief Get information of current music that is playing
 *
 * @param info  See MusicInfo structure
 *
 * @return See AudioErr structure
 */
int EspAudioInfoGet(struct MusicInfo *info);

/*
 * @brief Get the source of the music
 *
 * @param src  Refer to 'PlaySrc'
 *
 * @return See AudioErr
 */
int EspAudioPlayerSrcGet(PlaySrc *src);

/*
 * @brief Set the source of the music
 *
 * @param src  Refer to 'PlaySrc', and users can add other value
 *
 * @return See AudioErr structure
 */
int EspAudioPlayerSrcSet(PlaySrc src);

/*
 * @brief Set the default decoder of the player,
 *          especially when the parameter is "NULL" then the decoder will be determined by every file's extension
 *
 * @param str  Can be "OPUS", "AMR", "WAV", "AAC", "M4A", "MP3", "OGG"
 *
 * @return See AudioErr structure
 */
int EspAudioSetDecoder(const char *str);

/*
 * @brief Set up-sampling or down-sampling aimed sample rate
 *
 * @param sampleRate  Aimed sample rate
 *
 * @return See AudioErr structure
 */
int EspAudioSamplingSetup(int sampleRate);

/*
 * @brief Set current uri as next uri in current play list,
 *          play list is cluster of uri stored in flash or sd_card, player can select next or previous music in play list
 *
 * @return See AudioErr structure
 */
int EspAudioNext(void);

/*
 * @brief Set current uri as previous uri in current play list,
 *          play list is cluster of uri stored in flash or sd_card, player can select next or previous music in play list
 *
 * @return See AudioErr structure
 */
int EspAudioPrev(void);

/*
 * @brief Set player play mode
 *
 * @param mode See PlayMode enum for more help
 *
 * @return See AudioErr structure
 */
int EspAudioPlaylistModeSet(int mode);

/*
 * @brief Set player play mode
 *
 * @param mode Pointer to an int that stores the result,
 *               See PlayMode enum for more help
 *
 * @return See AudioErr structure
 */
int EspAudioPlaylistModeGet(int *mode);

/*
 * @brief Get current playlist id correspond to "playlistNameDef.h"
 *
 * @return
 *     - (<0)  Failed
 *     - (0)   playlist id
 */
int EspAudioCurListGet(void);

/*
 * @brief Set current playlist to given id and open it
 *
 * @param listId          Aimed playlist id corresponding to "playlistNameDef.h"
 * @param modifyExisting  If 1 means clean it and then open it, the same as fopen(path, "w+"), while if 0 means fopen(path, "r")
 *
 * @return
 *     - (<0)  Failed
 *     - (0)   Success
 */
int EspAudioPlaylistSet(int listId, int modifyExisting);

/*
 * @brief Set current playlist to given id and open it
 *
 * @param listId          Aimed playlist id corresponding to "playlistNameDef.h"
 * @param modifyExisting  If 1 means clean it and then open it, the same as fopen(path, "w+"), while if 0 means fopen(path, "r")
 *
 * @return
 *     - (<0)  Failed
 *     - (0)   Success
 */
 int EspAudioPlaylistSave(int listId, const char *uri);

/*
 * @brief Read a corresponding playlist
 *
 * @param listId      Aimed playlist id corresponding to "playlistNameDef.h"
 * @param buf         Buffer to be written
 * @param len         Length of data to be read, must be less than buffer size
 * @param offset      Offset bytes from the data header
 * @param isAttr      if 1 read attibution file, if 0 read list file, refer to "playlistNameDef.c" for the details
 *
 * @return
 *     - (<0)  Failed
 *     - (0)   Success
 */
 int EspAudioPlaylistRead(int listId, char *buf, int len, int offset, int isAttr);

/*
 * @brief Get playlist basic information
 *
 * @param listId      Aimed playlist id corresponding to "playlistNameDef.h"
 * @param *amount     The amount of the musics of the list
 * @param *size       The file size of the list
 *
 * @return
 *     - (<0)  Failed
 *     - (0)   Success
 */
int EspAudioPlaylistInfo(int listId, int *amount, int *size);

/*
 * @brief Close the corresponding playlist
 *
 * @param listId  Aimed playlist id corresponding to "playlistNameDef.h"
 *
 * @return
 *     - (<0)  Failed
 *     - (0)   Success
 */
 int EspAudioPlaylistClose(int listId);

/*
 * @brief Clear the corresponding playlist
 *
 * @param listId  Aimed playlist id corresponding to "playlistNameDef.h"
 *
 * @return
 *     - (<0)  Failed
 *     - (0)   Success
 */
 int EspAudioPlaylistClear (int listId);

#endif /* _ESP_AUDIO_H_ */
