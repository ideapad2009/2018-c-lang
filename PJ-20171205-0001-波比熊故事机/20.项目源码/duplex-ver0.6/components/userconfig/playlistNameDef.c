#include "playlistNameDef.h"

/* -------------- sd-card playlist -------------- */
/*
    About playlist file system:

    Simply store all of the data that `EspAudioPlaylistSave()` passed in sequential order.
    But if want to use `EspAudioNext()` or `EspAudioPrev()`, then every uri in `EspAudioPlaylistSave()` must end with '\n'.
    The `PLAYLIST_IN_CARD_PRIMARY` list already has '\n' ending by default.
*/
const char *SdListNames[] = {//less than 80 characters
    "/playlist_primary.ini",
    "/playlist_02.ini",
    "/playlist_03.ini",
    //add user's lists
};

/*
    About attribution file system:

    The attribution file stored all the music's offset value in `sizeof(int)` format, demonstrating as follow:

    (0x00000000)(0x00000012)(0x00000100)(EOF)

    which means the first music uri starts at 0x0 bytes in the respective playlist, and the second one starts at 0x12 bytes,
        and the last one starts at 0x100 bytes,
        and this whole attribution file is `3 * sizeof(int) = 12` bytes in total.

    Aside: this file designed only for supporting shuffle-mode (random mode) and won't be used in other occasion,
            but pay attention that if player is in shuffle mode, then the contents of this file will be modified and won't recover.
            Flash playlist doesn't support shuffle-mode (random mode)

    Using `EspAudioPlaylistRead()` to read attribution file data when the last param is 1
*/
const char *SdListAttrNames[] = {//"attr" is short for attribution, name length must less than 80 characters
    //the file names must NOT be the same as `SdListNames[]`
    "/listAttri_file.ini",
    "/listAttri_02.ini",
    "/listAttri_03.ini",
    //add user's attribution files
};



/* -------------- flash playlist ---------------- */

/*
    About playlist file system:

    The same with sd-card playlist.
    But do not support shuffle-mode (random mode).
*/
const char *FlashListNames[] = {//less than 15 characters
    "HttpLists",
    "HttpLists2",
     //add user's playlists
};
/* These are not attribution files, only storing the playlists' file size */
const char *FlashListSizeNames[] = {//less than 15 characters
    "HLOffset",
    "HLOffset2",
    //add user's files
};


int getPlayListsNum(PlayListId id)
{
    if (id >= PLAYLIST_IN_CARD_PRIMARY && id < PLAYLIST_MAX)
        return sizeof(SdListNames) / sizeof(char*);
    else if (id >= PLAYLIST_IN_FLASH_DEFAULT)
        return sizeof(FlashListNames) / sizeof(char*);
    else
        return 0;
}

int getDefaultSdIndex()
{
    return PLAYLIST_IN_CARD_PRIMARY;
}

int getDefaultFlashIndex()
{
    return PLAYLIST_IN_FLASH_DEFAULT;
}

int getMaxIndex()
{
    return PLAYLIST_MAX;
}
