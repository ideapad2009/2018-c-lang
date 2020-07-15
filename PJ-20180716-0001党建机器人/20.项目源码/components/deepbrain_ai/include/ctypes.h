#ifndef _C_TYPES_H_
#define _C_TYPES_H_

#define BUFFER_SIZE_8 	 8
#define BUFFER_SIZE_16 	 16
#define BUFFER_SIZE_32 	 32
#define BUFFER_SIZE_64 	 64
#define BUFFER_SIZE_128  128
#define BUFFER_SIZE_256  256
#define BUFFER_SIZE_512  512
#define BUFFER_SIZE_1024 1024
#define BUFFER_SIZE_KB(num) (BUFFER_SIZE_1024*num)

typedef char sint8_t;
typedef unsigned char uint8_t;
typedef signed short int int16_t;
typedef unsigned short int uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t; 

#if __WORDSIZE == 64

typedef signed long int int64_t;
typedef unsigned long int uint64_t;

#else

typedef signed long long int int64_t;
typedef unsigned long long int uint64_t;

#endif

#endif

