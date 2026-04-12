//#ifndef __BSP_ESP8266_H
//#define __BSP_ESP8266_H

//#include "main.h"
//#include <stdint.h>

///* 接收缓存大小 */
//#define ESP8266_RX_BUF_SIZE          (1024U)
//#define ESP8266_ASYNC_BUF_SIZE       (1024U)
//#define ESP8266_CMD_TIMEOUT_MS       (5000U)

///* Wi-Fi 参数 */
//#define WIFI_SSID                    "weiwei"
//#define WIFI_PASSWORD                "12345678"

///* OneNET 参数
// * 说明：
// * 1. 这里的 token 已经是 URL 编码后的形式
// * 2. 因为下面代码会把 token 当作 %s 参数传入 snprintf，
// *    所以 token 字符串里保留单个 %2F / %3D 即可
// * 3. 只有“直接把 token 写进 snprintf 的格式串”时，才需要写 %%2F
// */
//#define ONENET_HOST                  "mqtts.heclouds.com"
////#define ONENET_HOST                  "183.230.40.96"
//#define ONENET_PORT                  (1883U)

//#define ONENET_PRODUCT_ID            "o307B1Ac13"
//#define ONENET_DEVICE_NAME           "produce1"
////#define ONENET_TOKEN                 "version=2018-10-31&res=products%%2Fo307B1Ac13%%2Fdevices%%2Fproduce1&et=2199223456&method=md5&sign=2XAQOVtnM95dUgvlDGy7UA%%3D%%3D"

//#define ONENET_TOKEN                 "version=2018-10-31&res=products%2Fo307B1Ac13%2Fdevices%2Fproduce1&et=1893456000&method=md5&sign=MxYgF1Mvwn%2F5JLDIn4RNVg%3D%3D"
///* OneNET 系统 Topic */
//#define TOPIC_PROPERTY_POST          "$sys/o307B1Ac13/produce1/thing/property/post"
//#define TOPIC_PROPERTY_POST_REPLY    "$sys/o307B1Ac13/produce1/thing/property/post/reply"

//#define TOPIC_SERVICE_SET_PUMP       "$sys/o307B1Ac13/produce1/thing/service/SetPump/invoke"
//#define TOPIC_SERVICE_SET_LIGHT      "$sys/o307B1Ac13/produce1/thing/service/SetLight/invoke"
////#define TOPIC_SERVICE_SET_LIGHT      "$sys/o307B1Ac13/produce1/thing/service/LEDState/invoke"
//#define TOPIC_SERVICE_SET_BEEP       "$sys/o307B1Ac13/produce1/thing/service/SetBeep/invoke"

//#define TOPIC_SERVICE_SET_PUMP_REPLY  "$sys/o307B1Ac13/produce1/thing/service/SetPump/invoke_reply"
//#define TOPIC_SERVICE_SET_LIGHT_REPLY "$sys/o307B1Ac13/produce1/thing/service/SetLight/invoke_reply"
////#define TOPIC_SERVICE_SET_LIGHT_REPLY "$sys/o307B1Ac13/produce1/thing/service/LEDState/invoke_reply"
//#define TOPIC_SERVICE_SET_BEEP_REPLY  "$sys/o307B1Ac13/produce1/thing/service/SetBeep/invoke_reply"

///* 服务命令类型 */
//typedef enum
//{
//    ESP_SERVICE_NONE = 0,
//    ESP_SERVICE_SET_PUMP,
//    ESP_SERVICE_SET_LIGHT,
//    ESP_SERVICE_SET_BEEP
//} ESP_ServiceType_t;

///* 服务命令结构体 */
//typedef struct
//{
//    ESP_ServiceType_t type;   /* 服务类型 */
//    uint8_t value;            /* 输入参数 value，0/1 */
//    char id[32];              /* OneNET 下发的请求 id */
//} ESP_ServiceCmd_t;

///* 基础 AT */
//void BSP_ESP8266_Init(void);
//HAL_StatusTypeDef BSP_ESP8266_TestAT(void);
//HAL_StatusTypeDef BSP_ESP8266_SetStationMode(void);
//HAL_StatusTypeDef BSP_ESP8266_JoinAP(const char *ssid, const char *password);

///* OneNET 连接 */
//HAL_StatusTypeDef BSP_ESP8266_StartOneNET(void);

///* 上报属性 */
//HAL_StatusTypeDef BSP_ESP8266_PublishPropertyJson(const char *json_body);

///* 轮询接收服务调用 */
//uint8_t BSP_ESP8266_PollService(ESP_ServiceCmd_t *cmd);

///* 回复服务调用结果 */
//HAL_StatusTypeDef BSP_ESP8266_ReplyService(const ESP_ServiceCmd_t *cmd, uint8_t success);

///* 调试查看最近 AT 回包 */
//const char *BSP_ESP8266_GetLastRx(void);

//#endif

#ifndef __BSP_ESP8266_H
#define __BSP_ESP8266_H

#include "main.h"
#include <stdint.h>

#define ESP8266_RX_BUF_SIZE          (1024U)
#define ESP8266_ASYNC_BUF_SIZE       (2048U)
#define ESP8266_CMD_TIMEOUT_MS       (5000U)
#define ESP8266_IRQ_FIFO_SIZE        (2048U)

#define WIFI_SSID                    "weiwei"
#define WIFI_PASSWORD                "12345678"

#define ONENET_HOST                  "mqtts.heclouds.com"
#define ONENET_PORT                  (1883U)

#define ONENET_PRODUCT_ID            "o307B1Ac13"
#define ONENET_DEVICE_NAME           "produce1"
#define ONENET_TOKEN                 "version=2018-10-31&res=products%2Fo307B1Ac13%2Fdevices%2Fproduce1&et=1893456000&method=md5&sign=MxYgF1Mvwn%2F5JLDIn4RNVg%3D%3D"

#define TOPIC_PROPERTY_POST          "$sys/o307B1Ac13/produce1/thing/property/post"
#define TOPIC_PROPERTY_POST_REPLY    "$sys/o307B1Ac13/produce1/thing/property/post/reply"
#define TOPIC_SERVICE_SET_PUMP       "$sys/o307B1Ac13/produce1/thing/service/SetPump/invoke"
#define TOPIC_SERVICE_SET_LIGHT      "$sys/o307B1Ac13/produce1/thing/service/SetLight/invoke"
#define TOPIC_SERVICE_SET_BEEP       "$sys/o307B1Ac13/produce1/thing/service/SetBeep/invoke"

#define TOPIC_SERVICE_SET_PUMP_REPLY  "$sys/o307B1Ac13/produce1/thing/service/SetPump/invoke_reply"
#define TOPIC_SERVICE_SET_LIGHT_REPLY "$sys/o307B1Ac13/produce1/thing/service/SetLight/invoke_reply"
#define TOPIC_SERVICE_SET_BEEP_REPLY  "$sys/o307B1Ac13/produce1/thing/service/SetBeep/invoke_reply"

typedef enum
{
    ESP_SERVICE_NONE = 0,
    ESP_SERVICE_SET_PUMP,
    ESP_SERVICE_SET_LIGHT,
    ESP_SERVICE_SET_BEEP
} ESP_ServiceType_t;

typedef struct
{
    ESP_ServiceType_t type;
    uint8_t value;
    char id[32];
} ESP_ServiceCmd_t;

void BSP_ESP8266_Init(void);
HAL_StatusTypeDef BSP_ESP8266_TestAT(void);
HAL_StatusTypeDef BSP_ESP8266_SetStationMode(void);
HAL_StatusTypeDef BSP_ESP8266_JoinAP(const char *ssid, const char *password);
HAL_StatusTypeDef BSP_ESP8266_StartOneNET(void);
HAL_StatusTypeDef BSP_ESP8266_PublishPropertyJson(const char *json_body);
uint8_t BSP_ESP8266_PollService(ESP_ServiceCmd_t *cmd);
HAL_StatusTypeDef BSP_ESP8266_ReplyService(const ESP_ServiceCmd_t *cmd, uint8_t success);
const char *BSP_ESP8266_GetLastRx(void);

#endif

