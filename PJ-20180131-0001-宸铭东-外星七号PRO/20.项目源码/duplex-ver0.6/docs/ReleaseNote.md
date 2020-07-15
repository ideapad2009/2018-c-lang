# Espressif Audio Platform

## Release 0.6 [Date:2017-12-22]
## New Features
- 1.修改mp3解码库
- 2.支持DLNA
- 3.支持Dueros
- 4.支持IDF3.0
- 5.解码库内存

## Resolved Issues
- 1.修复Ver0.5 已知Bug

## Known issues
- 1.有些M4A的解码会出错
- 2.不支持24bit的音乐


## Release 0.6-rc1 [Date:2017-10-20]
## New Features
- 1.Player内存优化
- 2.解码库优化

## Resolved Issues
- 1.修复Ver0.5 已知Bug

## Known issues
- 1.有些M4A的解码会出错
- 2.不支持24bit的音乐


## Release 0.5 [Date:2017-09-01]
## New Features
- 1.Player支持双核
- 2.Player独立功能的API
- 3.完善了Audio Error Number
- 4.新增M4A（moov）和MP4解码库
- 5.提示音支持了不被打断功能
- 6.支持了OPUS歌曲文件
- 7.支持opus的录音
- 8.支持客户定制解码库
- 9.支持WIFI和BT的切换，需要Flash size支持
- 10.优化部分内存

## Resolved Issues
- 1.修复了一些 BT 多次重连的问题
- 2.修复了smartconfig 成功率低的问题
- 3.修复了m4a crash的问题
- 4.修复了AAC小文件不能播放
- 5.修复了错误码异常的问题
- 6.修复了一些 BT 和WIFI下噪音问题
- 7.修复了smartconfig crash的问题
- 8.修复了i2s/live stopping的问题
- 9.修复了MP3小文件播放异常的问题
- 10.修复了Seek后时间不准的问题
- 11.修复了play、pause的问题
- 12.修改了Rec按键录音的方式，目前按下开始录音，松开开始回放
- 13.修复了一些 crash 的问题
- 14.优化DRAM使用内存

## Known issues
- 1.有些M4A的解码会出错
- 2.录音到SDcard有微弱杂音
- 3.不支持24bit的音乐
- 4.有的BT设备播放音乐会有杂音


## Release 0.5.0-rc2 [Date:2017-08-04]
## New Features
- 1.None

## Resolved Issues
- 1.修复了一些 crash 的问题
- 2.优化DRAM使用内存

## Known issues
- 1.有些M4A的解码会出错
- 2.录音到SDcard有微弱杂音
- 3.不支持24bit的音乐

## Release 0.5.0-rc1 [Date:2017-07-20]
## New Features
- 1.Player支持双核
- 2.Player独立功能的API
- 3.完善了Audio Error Number


## Resolved Issues
- 1.修复了一些 BT 多次重连的问题
- 2.修复了smartconfig 成功率低的问题
- 3.修复了m4a crash的问题
- 4.修复了AAC小文件不能播放
- 5.修复了错误码异常的问题

## Known issues
- 1.有些M4A的解码会出错
- 2.录音到SDcard有微弱杂音
- 3.不支持24bit的音乐


## Release 0.5.0 [Date:2017-06-13]
## New Features
- 1.新增M4A（moov）和MP4解码库
- 2.提示音支持了不被打断功能
- 3.支持了OPUS歌曲文件
- 4.支持opus的录音
- 5.支持客户定制解码库
- 6.支持WIFI和BT的切换
- 7.优化部分内存

## Resolved Issues
- 1.修复了一些 BT 和WIFI下噪音问题
- 2.修复了smartconfig crash的问题
- 3.修复了i2s/live stopping的问题
- 4.修复了MP3小文件播放异常的问题
- 5.修复了Seek后时间不准的问题
- 6.修复了play、pause的问题
- 7.修改了Rec按键录音的方式，目前按下开始录音，松开开始回放


## Known issues
- 1.有些M4A的解码会出错
- 2.录音到SDcard有微弱杂音
- 3.不支持24bit的音乐


## Release 0.2.0 [Date:2017-05-12]
## New Features
- 1.新增SeekByTime, GetPosByTime, GetSongInfo等接口
- 2.支持了单、双通道录音配置
- 3.新增了Player状态广播通知的功能
- 4.新增了APP 推送歌曲和频道列表的功能
- 5.新增了部分终端命令
- 6.新增了aux in功能，JP4-pin2--ON
- 7.支持SdCard和Flash Play list
- 8.支持SdCard和Flash提示音，参见userconfig/toneuri.c
- 9.支持系统状态指示灯
- 10.支持OTA升级
- 11.完善网络直播
- 12.支持替换Codec IC
- 13.支持SdCard热插拔

## Resolved Issues
- 1.完善网络直播
- 2.修复了WIFI（点播和直播）和SD卡 模式下aac引起的reset
- 3.修复了mp3杂音问题
- 4.修复了flac lost sync 问题
- 5.修复了aac播放失败的问题
- 6.修复了flac watchdog的问题
- 7.修复音量调节与打印不符的问题
- 8.修复了wav 数据不完整引起的噪音问题
- 9.修复了一些 BT 重启的问题
- 10.修复SD卡的一些问题

## Known issues
- 播放有的音乐会出现杂音



## Release 0.1.3 [Date:2017-03-28]
- 功能列表：
- 1.支持Play list,需要插入SDcard
- 2.支持提示音，参见userconfig/toneuri.c
- 3.支持系统状态指示灯
- 4.支持OTA升级
- 5.支持网络直播
- 6.支持Lrya32T_V2,LyraT_V3.1(WROAR)
- 7.支持长按Rec(Lyra32T_V2)/Play(LyraT_V3.1)录音存储于SDcard,松开按键，回放已录声音,16k/2channels/16bits
- 8.修复一些reset的问题
- 9.完善BT音质



## Release 0.1.0 [Date:2017-03-03]
- 功能列表：
- 1.支持SD card 歌曲列表顺序播放；
- 2.支持IOT APP 推送喜马拉雅点播内容，暂时不支持直播；
- 3.支持音量调节Vol＋／Vol－，长按Vol＋／Vol－ 可以触发上一首／下一首，
- 4.支持BT，A2DP／AVRCP
- 5.支持格式MP3／AAC／OGG／FLAC／WAV
- 6.Enable SET touch后可以使用Smartconfig