# Flash Tone OTA的使用方法

## 1.使用步骤
- step1：

  修改分区表信息(partitions_esp_audio.csv)，分配tonestatus分区，大小只需要8K的flash空间
示例如下，下面分区表知道flash tone的大小为372K，tonestatus大小为8K，flashtone的subtype是0x04，tonestatus的subtype是0x06，这个是固定的。使用者必须先确定需要升级的flash tone文件的大小，分配安全的分区大小以供使用，重新编译partition固件，make partition-clean ,make partition

    nvs,      data, nvs,     0x9000,  0x4000,

    otadata,  data, ota,     0xd000,  0x2000

    phy_init, data, phy,     0xf000,  0x1000,

    wifi,  0,  ota_0, 0x10000, 2432K,

    bt,  0,  ota_1, 0x270000, 1216K,

    flashTone,  data,  0x04, 0x3A0000, 372K,

    tonestatus,  data,  0x06, , 8K,

    flashUri, data, 0x05, , 0x1000,

- step2：

  使用tools目录下的mk_audio_bin.py生成flash tone固件，放置云端服务器上，需要注意和老版本不一样的地方是flash tone附件最后加上了CRC32的校验值，以供升级完成校验时使用

-step3：

  修改ToneOtaService.h中的TONE_OTA_URL宏，指定升级固件存放的URL，示例：
```
  #define TONE_OTA_URL "http://192.168.3.3:8080/audio-esp.bin"
```

-step4：
在app_main中添加tone ota的服务，重新编译烧录，当连上路由器之后，会自动从服务器获取最新的flash tone固件
```
ToneOtaService* toneota = ToneOtaServiceCreate();
player->addService(player, (MediaService *) toneota);
```

## 2.API解释
```
//创建Tone OTA的服务，若无特殊修改动作，只需确保可以连接升级服务器的情况下调用到该接口即可完成升级动作，无须关心后面的4个API
ToneOtaService* ToneOtaServiceCreate(void);

//启动flash tone升级，传入升级固件的URL即可，该接口是阻塞的。返回的errno 参考TONE_OTA_PARTITION_NOT_FOUND等宏定义

esp_err_t esp_tone_ota_audio_begin(const char* url);

//设置OTA的升级状态，可以在升级过程中调用该接口设置升级的状态，调用示例
 esp_tone_ota_audio_set_status(OTA_STATUS_END | OTA_RES_PROGRESS);
表述设置升级状态进入END状态，阶段是OTA_RES_PROGRESS

esp_err_t esp_tone_ota_audio_set_status(uint32_t status);

//查询OTA的升级状态，可以在升级过程中调用该接口查询升级的状态，调用示例
ToneOtaStatus = esp_tone_ota_audio_get_status();ToneOtaStatus的高16bit表示升级的状态，低16bit表示该状态的阶段，参考宏定义#define OTA_STATUS_START   (1<<17)，#define OTA_RES_SUCCESS    (1<<0)等
uint32_t esp_tone_ota_audio_get_status(void);

//校验升级固件，校验通过返回ESP_OK
esp_err_t esp_tone_ota_audio_end(void);
```
