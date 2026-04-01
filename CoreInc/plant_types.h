#ifndef __PLANT_TYPES_H
#define __PLANT_TYPES_H

#include <stdint.h>

/* SensorData_t（传感器数据结构体）
 * 作用：统一保存系统采集到的环境参数
 * 答辩可说：该结构体用于集中管理植物养护系统的实时监测数据，
 * 便于显示、控制与云端上传共用同一份数据源。
 */
typedef struct
{
    float air_temp;          /* air_temp（空气温度，单位：℃） */
    float air_humi;          /* air_humi（空气湿度，单位：%RH） */
    float light_lux;         /* light_lux（光照强度，单位：lux） */
    float soil_moisture;     /* soil_moisture（土壤湿度，当前先按百分比逻辑使用） */
    float ph_value;          /* ph_value（pH值） */
} SensorData_t;

/* SystemParam_t（系统参数结构体）
 * 作用：保存阈值和自动控制使能参数
 * 答辩可说：该结构体用于保存系统的可配置参数，
 * 后续既可以通过本地菜单修改，也可以通过云端下发修改。
 */
typedef struct
{
    float soil_low_th;       /* soil_low_th（土壤湿度下限阈值） */
    float light_low_th;      /* light_low_th（光照下限阈值） */
    float ph_low_th;         /* ph_low_th（pH下限阈值） */
    float ph_high_th;        /* ph_high_th（pH上限阈值） */

    uint8_t auto_water_en;   /* auto_water_en（自动浇水使能，1=开启，0=关闭） */
    uint8_t auto_light_en;   /* auto_light_en（自动补光使能，1=开启，0=关闭） */
    uint8_t alarm_en;        /* alarm_en（报警使能，1=开启，0=关闭） */
} SystemParam_t;

/* SystemState_t（系统状态结构体）
 * 作用：保存当前执行机构和通信状态
 * 答辩可说：该结构体反映系统当前运行状态，
 * 便于界面显示、云端上传以及后续故障诊断。
 */
typedef struct
{
    uint8_t pump_on;         /* pump_on（水泵状态，1=开启，0=关闭） */
    uint8_t light_on;        /* light_on（补光灯状态，1=开启，0=关闭） */
    uint8_t beep_on;         /* beep_on（蜂鸣器状态，1=开启，0=关闭） */

    uint8_t wifi_ok;         /* wifi_ok（WiFi连接状态，1=正常，0=异常） */
    uint8_t cloud_ok;        /* cloud_ok（云平台连接状态，1=正常，0=异常） */
} SystemState_t;

#endif
