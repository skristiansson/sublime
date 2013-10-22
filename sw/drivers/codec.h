#ifndef _CODEC_H_
#define _CODEC_H_
#include <config.h>

#if (CODEC_DRIVER == ssm2603)
#define codec_init	ssm2603_init
#endif

extern void codec_init(void);

#endif
