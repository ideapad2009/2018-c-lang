# ESP-Audio Project

ESP-Audio project is an official development framework for the [ESP32](https://espressif.com/en) chip.


This project is a fully integrated solution which supports multi-source (e.g. Http / Flash / TF-Card / BT) and multi-format (e.g. aac / m4a / mp3 / ogg / opus / amr) music playing with some simple APIs.   
Along with some music-playing related features such as music-tone (can automatically interrupt and resume the music) and playlist and DLNA / AIRPLAY protocol and Duer OS and re-sampling (e.g. 16K sample rate to 48K) and smartconfig, it will be very easy to develop a audio project.

## Feature List

### Hardware
- BT & BLE
- Wi-Fi
- TF-Card
- External RAM
- Auxiliary / Button / LED

### Software
- Music (TF-card / Flash / Http)
    - playing (OPUS / AMR / WAV / AAC / M4A / MP3 / OGG)
    - recording (OPUS / AMR / WAV)
- Protocol
    - DLNA
    - AIRPLAY
- Cloud
    - Duer OS
- Playlist (integrated with Next or Previous method)
- Tone (automatically interrupt and resume the music)
- OTA (Over-The-Air software upgrade)
- Re-sampling (e.g. 16K sample rate to 48K)
- Smartconfig & Blufi
- Shell (uart terminal)

### Restriction
- Can not play 24bits music directly, but can be upgraged to 32 bits using audio re-sampling API.
- BT and Wi-Fi and ESP-Audio can not coexist without PSRAM (external RAM) because of the lack of RAM. 

## Quick Usage Example
```c
//initialize the player
EspAudioInit(&playerConfig, &player);
//support mp3 music
EspAudioCodecLibAdd(CODEC_DECODER, 10 * 1024, "mp3", MP3Open, MP3Process, MP3Close, MP3TriggerStop);
//initialize Wi-Fi and connect
deviceController->enableWifi(deviceController);
//Play the music
EspAudioPlay("http://iot.espressif.com/file/example.mp3");
```
With these simple APIs, users can develop their own audio project on any ESP32 based board.

***

## Getting Started

### Preparation
ESP-IDF and related environment should be done to build the ESP-Audio project and to generate the binaries as well as to `make flash`.  

Documents and hardware guides are also available on following links.
1. Refer to the [ESP-IDF README](https://github.com/espressif/esp-idf "ESP-IDF doc") on github to set up the environment.
2. Refer to the [ESP-IDF Programming Guide](http://esp-idf.readthedocs.io/en/latest/index.html "ESP-32 IDF system") for information about the ESP-IDF system (including hardware and software).  

    > Please pay attention to [Partition Tables](http://esp-idf.readthedocs.io/en/latest/api-guides/partition-tables.html "Partition Tables Section") section.  
    > This is at which the Flash tone and Flash playlist as well as the OTA binaries locate

3. Refer to [ESP32 documents](http://espressif.com/en/support/download/documents?keys=&field_type_tid%5B%5D=13 "ESP32 documents") for datasheet and technical references, etc.

## Developing ESP-Audio Project
Refer to [ESP-Audio Programming Guide](docs/ESP-Audio_Programming_Guide.md "Audio Programming Guide").

[Build](http://esp-idf.readthedocs.io/en/latest/get-started/index.html#build-and-flash "build section in 'ESP-IDF Programming Guide'") this project and generate the final binary files to be downloaded.

## Downloading Firmware
Once the binary files are generated, these files should be downloaded into the board.  
>Check out the schematics in */esp-audio-app/docs* directory.  

When using *ESP32_LyraXX* board, please follow the following steps to download the firmware:

1. Press the *BOOT* button and the *RST* button simultaneously.
2. Release the *RST* button.
3. Release the *BOOT* button.

***

## Resources

- ESP32 Home Page: https://espressif.com/en
- ESP-IDF: https://github.com/espressif/esp-idf
- ESP-IDF Programming Guide: http://esp-idf.readthedocs.io/en/latest/index.html
- Espressif Documents: https://espressif.com/en/support/download/documents

### Audiopedia
This cyclopedia is about the general ideas and structure of ESP-Audio project for users who are interested in the general implementation of the ESP-Audio system. 

#### [Audiopedia Link](docs/Audiopedia/Audiopedia.md "Audiopedia")

