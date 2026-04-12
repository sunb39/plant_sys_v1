#ifndef __APP_CLOUD_H
#define __APP_CLOUD_H

#include "plant_types.h"
#include <stdint.h>

#define CLOUD_TX_BUF_SIZE    (1024U)   /* CLOUD_TX_BUF_SIZE（云端发送缓存区大小） */

/* APP_Cloud_Init（云端通信初始化）
 * 作用：初始化通信状态和缓存区
 */
void APP_Cloud_Init(SystemState_t *state);

/* APP_Cloud_Upload（云端数据上传）
 * 作用：将当前数据打包成待发送字符串
 */
void APP_Cloud_Upload(const SensorData_t *data, const SystemState_t *state);

/* APP_Cloud_Parse（云端报文解析）
 * 作用：预留接收命令解析接口
 */
void APP_Cloud_Parse(void);

/* APP_Cloud_GetLastPacket（获取最近一次待上传报文）
 * 作用：便于调试时查看打包结果
 */
const char *APP_Cloud_GetLastPacket(void);

#endif
