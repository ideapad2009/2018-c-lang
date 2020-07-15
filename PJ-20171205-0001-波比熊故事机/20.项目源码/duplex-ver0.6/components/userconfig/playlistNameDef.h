#ifndef __PLAYLISTNAMEDEF_H__
#define __PLAYLISTNAMEDEF_H__

#define PLAYLIST_NUM_OPENED_LISTS_MAX    4 //how many playlists can be opened simultaneously

typedef enum PlayListId {

    /*!< playlists in flash */
    //the following value must equal the respective flashUri-partition's SubType in partition table, and must >= 3 according to idf partition doc
    PLAYLIST_IN_FLASH_DEFAULT = 5, //reserved by system, do not delete
    PLAYLIST_IN_FLASH_02,
    //add your TF-Card playlist id before `PLAYLIST_IN_CARD_PRIMARY`

    /*!< playlists in TF-card */
    PLAYLIST_IN_CARD_PRIMARY, //reserved by system, do not change
    PLAYLIST_IN_CARD_02,
    PLAYLIST_IN_CARD_03,
    //add your TF-Card playlist id before `PLAYLIST_MAX`

    PLAYLIST_MAX,/* add your sd flash playlist before this */
} PlayListId;

typedef int (*ISCODECLIB)(const char *ext);//for system, do not delete

int getPlayListsNum(PlayListId id);
int getDefaultSdIndex();
int getDefaultFlashIndex();
int getMaxIndex();

#endif /* __PLAYLISTNAMEDEF_H__ */

