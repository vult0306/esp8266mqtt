/* main.c -- MQTT client example
*
* Copyright (c) 2014-2015, Tuan PM <tuanpm at live dot com>
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* * Neither the name of Redis nor the names of its contributors may be used
* to endorse or promote products derived from this software without
* specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ets_sys.h"
#include "driver/uart.h"
#include "osapi.h"
#include "mqtt.h"
#include "wifi.h"
#include "config.h"
#include "debug.h"
#include "gpio.h"
#include "user_interface.h"
#include "mem.h"
#include "sntp.h"
#include "user_config.h"

#define ON 1
#define OFF 0
#define DEBUG OFF
#define MAX_TOPIC 4
#define TOPIC_LENGTH 10
static char GARDEN[] = "/garden";
static char GARDEN_FB[] = "/garden_fb";
static char SCHEDULE[] = "/schedule";
static char SCHEDULE_FB[] = "/schedule_fb";
static char STOP_SCHEDULE[] = "/stop_schedule";
static char STOP_SCHEDULE_FB[] = "/stop_schedule_fb";
static char COUNTDOWN[] = "/countdown";
#if (DEBUG==ON)
static char DEBUG_INFO[] = "/debug";
#endif
MQTT_Client mqttClient;
MQTT_Client* mqttClientptr=NULL;
typedef unsigned long u32_t;
static ETSTimer sntp_timer, sntp_timer1;
static os_timer_t timer1; //use to periodically publish port status
bool watering=FALSE;        // flag to indicate water is already taken
bool schedule_busy_updating_flag=FALSE;
bool  load_config_done;
void publish_watering_process(MQTT_Client *args, int watering_process)
{
    char *processc = (char*)os_zalloc(5);
    MQTT_Client* client = (MQTT_Client*)args;
    os_memset(processc,0,sizeof(processc));
    processc[0] = (char)((watering_process/1000) + 48);
    processc[1] = (char)((watering_process%1000)/100 + 48);
    processc[2] = (char)((watering_process%100)/10 + 48);
    processc[3] = (char)((watering_process%10) + 48);

    //countdown to the next pumping
    MQTT_Publish(client, COUNTDOWN, processc, strlen(processc), 0, 0);
    os_free(processc);
}

void sntpfn()
{
    u32_t ts = 0;
    char *realtime;
    int min,hour;
    ts = sntp_get_current_timestamp();
    if (ts == 0) {
        os_printf("did not get a valid time from sntp server\n");
    } else {
            realtime = sntp_get_real_time(ts);
            hour = strtol(realtime+11, (char**)NULL, 10);
            min = strtol(realtime+14, (char**)NULL, 10);
#if (DEBUG==ON)
    os_printf("Start internet connection at: %s\n", sntp_get_real_time(ts));
#endif
            os_timer_disarm(&sntp_timer);
            MQTT_Connect(&mqttClient);
    }
}

void everyday_watering()
{
    u32_t ts = 0;
    char *realtime;
    char *text = (char*)os_zalloc(40);
    char hh,mm,ss;
    int mmmm,current_pos_in_period;
    ts = sntp_get_current_timestamp();
    realtime = sntp_get_real_time(ts);

#if (DEBUG==ON)
    os_printf("daily timer: %s\n", sntp_get_real_time(ts));
#endif

    if (ts == 0) {
        os_printf("did not get a valid time from sntp server\n");
    } else {
        //123456789123456789123456789
        //         1        2
        //Sun Dec 16 17:49:55 2018
        hh = strtol(realtime+11, (char**)NULL, 10);
        mm = strtol(realtime+14, (char**)NULL, 10);
        ss = strtol(realtime+17, (char**)NULL, 10);
        mmmm = hh*60 + mm;
#if (DEBUG==ON)
        os_printf("hh:mm:mmmm: %d:%d:%d\n",hh,mm,mmmm);
#endif
        if((load_config_done)&&(schedule_busy_updating_flag!=TRUE)&&(usrCfg.scheduled==TRUE)&&(mqttClientptr!=NULL)){
            if( (mmmm > usrCfg.sod) && (mmmm < usrCfg.eod) ){
            //sod                                                             eod
            //---------||---------||---------||---------||---------||---------||
            //dur| gap || 
            //---|-----||---|-----||---|-----||---|-----||---|-----||---|-----||
            //---------------------------|current_pos_in_period
            //--------------------||sod_2_now
                current_pos_in_period = (mmmm - usrCfg.sod) - (mmmm - usrCfg.sod)/(usrCfg.dur + usrCfg.gap)*(usrCfg.dur + usrCfg.gap);
#if (DEBUG==ON)
                os_printf("sod_2_now : %d\n",mmmm - usrCfg.sod);
                os_printf("durgap : %d\n",usrCfg.dur + usrCfg.gap);
                os_printf("repeated : %d\n",(mmmm - usrCfg.sod)/(usrCfg.dur + usrCfg.gap));
                os_printf("current_pos_in_period : %d\n",current_pos_in_period);
#endif
                if(current_pos_in_period < usrCfg.dur){
                    if(!watering){
                        //yes, we are in watering
                        GPIO_OUTPUT_SET(4, 1); // turn on watering
                        MQTT_Publish(mqttClientptr, COUNTDOWN, "0", 1, 0, 0);
                        watering = TRUE;
#if (DEBUG==ON)
                            //123456789012345678901234567890
                        text="turn on watering at: ";
                        os_memcpy(text+20,realtime,sizeof(realtime));
                        MQTT_Publish(mqttClientptr, DEBUG_INFO, text, strlen(text), 0, 0);
#endif
                    }
                    else{
                        MQTT_Publish(mqttClientptr, GARDEN_FB, "1", 1, 0, 0);
                    }
                }
                else{
                    if(watering){
                        watering = FALSE;
                        GPIO_OUTPUT_SET(4, 0); // turn off watering
#if (DEBUG==ON)
                            //123456789012345678901234567890
                        text="turn off watering at: ";
                        os_memcpy(text+21,realtime,sizeof(realtime));
                        MQTT_Publish(mqttClientptr, DEBUG_INFO, text, strlen(text), 0, 0);
#endif
                    }
                    else{
                        MQTT_Publish(mqttClientptr, GARDEN_FB, "0", 1, 0, 0);
                        publish_watering_process(mqttClientptr,(usrCfg.dur+usrCfg.gap)*60-(current_pos_in_period*60+ss));
                    }
                }
            }
        }
    }
}

void wifiConnectCb(uint8_t status)
{
    if(status == STATION_GOT_IP){
        sntp_setservername(0, "pool.ntp.org");        // set sntp server after got ip address
        if(sntp_set_timezone(7)){
            sntp_init();
        }
        os_timer_disarm(&sntp_timer);
        os_timer_setfn(&sntp_timer, (os_timer_func_t *)sntpfn, NULL);
        os_timer_arm(&sntp_timer, 1000, 1);//1s
    } else {
          MQTT_Disconnect(&mqttClient);
    }
}

void mqttConnectedCb(uint32_t *args)
{
    //MQTT_Client* client = (MQTT_Client*)args;
    mqttClientptr = (MQTT_Client*)args;
    MQTT_Client* client = mqttClientptr;
#if (DEBUG==ON)
    INFO("MQTT: Connected\r\n");
#endif
    MQTT_Subscribe(client, GARDEN, 1);
    MQTT_Subscribe(client, SCHEDULE, 1);
    MQTT_Subscribe(client, STOP_SCHEDULE, 1);
//    MQTT_Subscribe(client, "/mqtt/topic/1", 1);
//    MQTT_Subscribe(client, "/mqtt/topic/2", 2);
//    MQTT_Publish(client, "/mqtt/topic/0", "hello0", 6, 0, 0);
//    MQTT_Publish(client, "/mqtt/topic/1", "hello1", 6, 1, 0);
//    MQTT_Publish(client, "/mqtt/topic/2", "hello2", 6, 2, 0);
}

void mqttDisconnectedCb(uint32_t *args)
{
    MQTT_Client* client = (MQTT_Client*)args;
#if (DEBUG==ON)
    INFO("MQTT: Disconnected\r\n");
#endif
}

void mqttPublishedCb(uint32_t *args)
{
    MQTT_Client* client = (MQTT_Client*)args;
#if (DEBUG==ON)
    INFO("MQTT: Published\r\n");
#endif    
}

void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
    char    *topicBuf = (char*)os_zalloc(topic_len+1),
            *dataBuf = (char*)os_zalloc(data_len+1);
    char    *schedule = (char*)os_zalloc(17);
    int temp;
    MQTT_Client* client = (MQTT_Client*)args;

    os_memcpy(topicBuf, topic, topic_len);
    topicBuf[topic_len] = 0;

    os_memcpy(dataBuf, data, data_len);
    dataBuf[data_len] = 0;
#if (DEBUG==ON)
    INFO("Send topic: %s, data: %s \r\n", topicBuf, dataBuf);
#endif
    //manually control is available only when scheduler is turn off
    if( !strcmp(topicBuf,GARDEN) && (!usrCfg.scheduled)){
        if( dataBuf[0] != '0' ){
            GPIO_OUTPUT_SET(4, 1); // turn on watering
            MQTT_Publish(client, GARDEN_FB, dataBuf, data_len, 0, 0); //
        }
        else{
            GPIO_OUTPUT_SET(4, 0); // turn off watering
            MQTT_Publish(client, GARDEN_FB, dataBuf, data_len, 0, 0); //
        }
    }
    //schedule set up
    else if( !strcmp(topicBuf,SCHEDULE) ){
        //          1         2         3         4 
        //1234567890123456789012345678901234567890123456789 
        //hhmm/hhmm/mm/mm
        //sod/eod/dur/gap

        // set busy updating flag first
        schedule_busy_updating_flag = TRUE;
        // schedule_published = FALSE;         // we haven't published this update yet

        temp = strtol(dataBuf, (char**)NULL, 10);
        usrCfg.sod = (temp/100)*60 + (temp%100);//convert to mmmm
        temp = strtol(dataBuf+5, (char**)NULL, 10); 
        usrCfg.eod = (temp/100)*60 + (temp%100);//convert to mmmm
        usrCfg.dur = strtol(dataBuf+10, (char**)NULL, 10);
        usrCfg.gap = strtol(dataBuf+13, (char**)NULL, 10);
        //store this update to flash asap
#if (DEBUG==ON)
    INFO("\r\n Schedule updated ...\r\n");
    os_printf("usrCfg.sod : %d\n",usrCfg.sod);
    os_printf("usrCfg.sod : %d\n",usrCfg.eod);
    os_printf("usrCfg.dur : %d\n",usrCfg.dur);
    os_printf("usrCfg.gap : %d\n",usrCfg.gap);
#endif
        if (!USRCFG_Save()){
            //the change has been flashed
            schedule_busy_updating_flag = FALSE;
        }
                  //          1         2         3         4 
                  //1234567890123456789012345678901234567890123456789 
        schedule = "hh:mm/hh:mm/mm/mm";
        os_memcpy(schedule, dataBuf, 2);
        os_memcpy(schedule+3, dataBuf+2, 2);
        os_memcpy(schedule+6, dataBuf+5, 2);
        os_memcpy(schedule+9, dataBuf+7, 2);
        os_memcpy(schedule+12, dataBuf+10, 2);
        os_memcpy(schedule+15, dataBuf+13, 2);

        topicBuf = SCHEDULE_FB;
        MQTT_Publish(client, topicBuf, schedule, 17, 0, 0); //
    }
    //scheduler turn on/off
    else if( !strcmp(topicBuf,STOP_SCHEDULE) ){
        schedule_busy_updating_flag = TRUE;
        if( dataBuf[0] != '0' ){
            usrCfg.scheduled = TRUE;
            if (!USRCFG_Save()){
                //the change has been flashed
                schedule_busy_updating_flag = FALSE;
            }
            MQTT_Publish(client, STOP_SCHEDULE_FB, dataBuf, data_len, 0, 0); //
        }
        else{
            usrCfg.scheduled = FALSE;
            if (!USRCFG_Save()){
                //the change has been flashed
                schedule_busy_updating_flag = FALSE;
            }
            MQTT_Publish(client, STOP_SCHEDULE_FB, dataBuf, data_len, 0, 0); //
        }
    }
    os_free(topicBuf);
    os_free(dataBuf);
}


/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
 *******************************************************************************/
uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set(void)
{
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            break;
        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            break;
        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}


void user_init(void)
{
    uart_init(BIT_RATE_115200, BIT_RATE_115200);
    os_delay_us(5000);

    CFG_Load();

    WIFI_Connect(sysCfg.sta_ssid, sysCfg.sta_pwd, wifiConnectCb);

    MQTT_InitConnection(&mqttClient, sysCfg.mqtt_host, sysCfg.mqtt_port, sysCfg.security);
    //MQTT_InitConnection(&mqttClient, "192.168.11.122", 1880, 0);

    MQTT_InitClient(&mqttClient, sysCfg.device_id, sysCfg.mqtt_user, sysCfg.mqtt_pass, sysCfg.mqtt_keepalive, 1);
    //MQTT_InitClient(&mqttClient, "client_id", "user", "pass", 120, 1);

    MQTT_InitLWT(&mqttClient, "/lwt", "offline", 0, 0);
    MQTT_OnConnected(&mqttClient, mqttConnectedCb);
    MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);
    MQTT_OnPublished(&mqttClient, mqttPublishedCb);
    MQTT_OnData(&mqttClient, mqttDataCb);

    //watering at every 06:00 or 18:00
    os_timer_disarm(&sntp_timer1);
    os_timer_setfn(&sntp_timer1, (os_timer_func_t *)everyday_watering, NULL);
    os_timer_arm(&sntp_timer1, 1000, 1);//1s
#if (DEBUG==ON)
    os_printf("usrCfg.sod : %d\n",usrCfg.sod);
    os_printf("usrCfg.sod : %d\n",usrCfg.eod);
    os_printf("usrCfg.dur : %d\n",usrCfg.dur);
    os_printf("usrCfg.gap : %d\n",usrCfg.gap);
    INFO("\r\nSystem started ...\r\n");
#endif
}
