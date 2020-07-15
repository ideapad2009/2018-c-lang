#ifndef _FATFS_SAL_H_
#define _FATFS_SAL_H_

#ifdef __cplusplus
extern "C" {
#endif

int FatfsComboWrite(const void* buffer, int size, int count, FILE* stream);

#ifdef __cplusplus
}
#endif

#endif  //_FATFS_SAL_H_
