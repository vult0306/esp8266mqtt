#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__
#include "os_type.h"

#define USE_OPTIMIZE_PRINTF
typedef struct{
    int sod;
    int eod;
    char dur;
    char gap;
    bool scheduled;
} USRCFG;
int ICACHE_FLASH_ATTR USRCFG_Save();
extern USRCFG usrCfg;
extern bool load_config_done;
#endif

