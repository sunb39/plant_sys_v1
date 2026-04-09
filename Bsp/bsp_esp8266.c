#include "bsp_esp8266.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bsp_oled.h"
/* 最近一次 AT 回包 */
static char g_esp8266_rx_buf[ESP8266_RX_BUF_SIZE];

/* 异步消息缓存（用于接收 +MQTTSUBRECV） */
static char g_async_buf[ESP8266_ASYNC_BUF_SIZE];
static uint16_t g_async_len = 0U;

/* 等待结果类型 */
typedef enum
{
    ESP_WAIT_TIMEOUT = 0,
    ESP_WAIT_OK,
    ESP_WAIT_ERROR
} ESP_WaitResult_t;

/* 清空最近一次 AT 回包 */
static void BSP_ESP8266_ClearRxBuf(void)
{
    memset(g_esp8266_rx_buf, 0, sizeof(g_esp8266_rx_buf));
}

/* 清空异步缓冲 */
static void BSP_ESP8266_ClearAsyncBuf(void)
{
    memset(g_async_buf, 0, sizeof(g_async_buf));
    g_async_len = 0U;
}

/* 串口发字符串 */
static HAL_StatusTypeDef BSP_ESP8266_SendString(const char *str)
{
    if (str == 0)
    {
        return HAL_ERROR;
    }

    return HAL_UART_Transmit(&huart1,
                             (uint8_t *)str,
                             (uint16_t)strlen(str),
                             ESP8266_CMD_TIMEOUT_MS);
}

/* 把 UART 中残留的异步数据吸走，放到异步缓存 */
static void BSP_ESP8266_DrainUartToAsync(void)
{
    uint8_t ch = 0U;

    while (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE) != RESET)
    {
        ch = (uint8_t)(huart1.Instance->DR & 0xFFU);

        if (g_async_len < (ESP8266_ASYNC_BUF_SIZE - 1U))
        {
            g_async_buf[g_async_len++] = (char)ch;
            g_async_buf[g_async_len] = '\0';
        }
        else
        {
            /* 缓冲满了就清掉，避免一直脏数据 */
            BSP_ESP8266_ClearAsyncBuf();
        }
    }
}

/* 等待 AT 响应
 * 说明：
 * 1. ok1 / ok2 任一匹配则判成功
 * 2. err1 / err2 任一匹配则判失败
 */
static ESP_WaitResult_t BSP_ESP8266_WaitResult(const char *ok1,
                                               const char *ok2,
                                               const char *err1,
                                               const char *err2,
                                               uint32_t timeout_ms)
{
    uint32_t start_tick = HAL_GetTick();
    uint16_t rx_len = 0U;
    uint8_t ch = 0U;

    BSP_ESP8266_ClearRxBuf();

    while ((HAL_GetTick() - start_tick) < timeout_ms)
    {
        if (HAL_UART_Receive(&huart1, &ch, 1U, 50U) == HAL_OK)
        {
            if (rx_len < (ESP8266_RX_BUF_SIZE - 1U))
            {
                g_esp8266_rx_buf[rx_len++] = (char)ch;
                g_esp8266_rx_buf[rx_len] = '\0';
            }

            if ((err1 != 0) && (strstr(g_esp8266_rx_buf, err1) != 0))
            {
                return ESP_WAIT_ERROR;
            }

            if ((err2 != 0) && (strstr(g_esp8266_rx_buf, err2) != 0))
            {
                return ESP_WAIT_ERROR;
            }

            if ((ok1 != 0) && (strstr(g_esp8266_rx_buf, ok1) != 0))
            {
                return ESP_WAIT_OK;
            }

            if ((ok2 != 0) && (strstr(g_esp8266_rx_buf, ok2) != 0))
            {
                return ESP_WAIT_OK;
            }
        }
    }

    return ESP_WAIT_TIMEOUT;
}

/* 发送 AT 并等待成功 */
static HAL_StatusTypeDef BSP_ESP8266_SendCmdOK(const char *cmd,
                                               const char *ok1,
                                               const char *ok2,
                                               const char *err1,
                                               const char *err2,
                                               uint32_t timeout_ms)
{
    ESP_WaitResult_t ret;

    /* 先把异步脏数据吸走，避免影响当前 AT */
    BSP_ESP8266_DrainUartToAsync();

    if (BSP_ESP8266_SendString(cmd) != HAL_OK)
    {
        return HAL_ERROR;
    }

    ret = BSP_ESP8266_WaitResult(ok1, ok2, err1, err2, timeout_ms);
    return (ret == ESP_WAIT_OK) ? HAL_OK : HAL_ERROR;
}

/* MQTTPUBRAW 发布原始 JSON
 * 说明：
 * 1. 先发 AT+MQTTPUBRAW
 * 2. 等到 '>'
 * 3. 再发原始 JSON 正文
 */
static HAL_StatusTypeDef BSP_ESP8266_MqttPubRaw(const char *topic, const char *payload)
{
    char cmd[256];
    uint16_t len;

    if ((topic == 0) || (payload == 0))
    {
        return HAL_ERROR;
    }

    len = (uint16_t)strlen(payload);

    snprintf(cmd,
             sizeof(cmd),
             "AT+MQTTPUBRAW=0,\"%s\",%u,0,0\r\n",
             topic,
             len);

    if (BSP_ESP8266_SendCmdOK(cmd, ">", 0, "ERROR", 0, 3000U) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (BSP_ESP8266_SendString(payload) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (BSP_ESP8266_WaitResult("+MQTTPUB:OK", "OK", "+MQTTPUB:FAIL", "ERROR", 5000U) != ESP_WAIT_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/* 从文本中取出 "id":"xxx" */
static uint8_t BSP_ESP8266_ParseId(const char *src, char *out, uint16_t out_size)
{
    const char *p;
    uint16_t i = 0U;

    if ((src == 0) || (out == 0) || (out_size == 0U))
    {
        return 0U;
    }

    p = strstr(src, "\"id\":\"");
    if (p == 0)
    {
        return 0U;
    }

    p += 6; /* 跳过 "id":" */

    while ((*p != '\0') && (*p != '"') && (i < (out_size - 1U)))
    {
        out[i++] = *p++;
    }

    out[i] = '\0';
    return (i > 0U) ? 1U : 0U;
}

/* 从文本中取出 "value":0/1 */
static uint8_t BSP_ESP8266_ParseValue01(const char *src, uint8_t *value)
{
    const char *p;

    if ((src == 0) || (value == 0))
    {
        return 0U;
    }

    p = strstr(src, "\"value\":");
    if (p == 0)
    {
        return 0U;
    }

    p += 8; /* 跳过 "value": */

    while ((*p == ' ') || (*p == '\t'))
    {
        p++;
    }

    if (*p == '1')
    {
        *value = 1U;
        return 1U;
    }

    if (*p == '0')
    {
        *value = 0U;
        return 1U;
    }

    return 0U;
}

/* 试图从异步缓存中提取服务命令 */
static uint8_t BSP_ESP8266_TryExtractService(const char *topic,
                                             ESP_ServiceType_t type,
                                             ESP_ServiceCmd_t *cmd)
{
    if ((topic == 0) || (cmd == 0))
    {
        return 0U;
    }

    if (strstr(g_async_buf, topic) == 0)
    {
        return 0U;
    }

    memset(cmd, 0, sizeof(ESP_ServiceCmd_t));
    cmd->type = type;

    if (BSP_ESP8266_ParseId(g_async_buf, cmd->id, sizeof(cmd->id)) == 0U)
    {
        strcpy(cmd->id, "0");
    }

    if (BSP_ESP8266_ParseValue01(g_async_buf, &cmd->value) == 0U)
    {
        cmd->value = 0U;
    }

    /* 提取后清空，避免重复处理 */
    BSP_ESP8266_ClearAsyncBuf();
    return 1U;
}

/* 初始化 */
void BSP_ESP8266_Init(void)
{
    BSP_ESP8266_ClearRxBuf();
    BSP_ESP8266_ClearAsyncBuf();
}

/* 测试 AT */
HAL_StatusTypeDef BSP_ESP8266_TestAT(void)
{
    BSP_ESP8266_SendCmdOK("ATE0\r\n", "OK", 0, "ERROR", 0, 1000U);
    return BSP_ESP8266_SendCmdOK("AT\r\n", "OK", 0, "ERROR", 0, 1000U);
}

/* STA 模式 */
HAL_StatusTypeDef BSP_ESP8266_SetStationMode(void)
{
    return BSP_ESP8266_SendCmdOK("AT+CWMODE=1\r\n", "OK", 0, "ERROR", 0, 2000U);
}

/* 连热点 */
HAL_StatusTypeDef BSP_ESP8266_JoinAP(const char *ssid, const char *password)
{
    char cmd[128];

    if ((ssid == 0) || (password == 0))
    {
        return HAL_ERROR;
    }

    snprintf(cmd,
             sizeof(cmd),
             "AT+CWJAP=\"%s\",\"%s\"\r\n",
             ssid,
             password);

    return BSP_ESP8266_SendCmdOK(cmd,
                                 "WIFI GOT IP",
                                 "OK",
                                 "+CWJAP:",
                                 "ERROR",
                                 20000U);
}

/* 启动 OneNET 连接
 * 说明：
 * 1. AT 测试
 * 2. 连 Wi-Fi
 * 3. 配置 MQTT
 * 4. 连接 OneNET
 * 5. 订阅属性回复和三个服务调用 topic
 */
//HAL_StatusTypeDef BSP_ESP8266_StartOneNET(void)
//{
//    char cmd[512];

//    if (BSP_ESP8266_TestAT() != HAL_OK)
//    {
//        return HAL_ERROR;
//    }

//    /* 先尽量清理旧 MQTT 连接，失败也不退出 */
//    BSP_ESP8266_SendCmdOK("AT+MQTTCLEAN=0\r\n", "OK", 0, "ERROR", 0, 1000U);

//    if (BSP_ESP8266_SetStationMode() != HAL_OK)
//    {
//        return HAL_ERROR;
//    }

//    if (BSP_ESP8266_JoinAP(WIFI_SSID, WIFI_PASSWORD) != HAL_OK)
//    {
//        return HAL_ERROR;
//    }

//    /* MQTT 用户配置
//     * 这里 client_id = 设备名 produce1
//     * username  = 产品ID
//     * password  = token
//     */
//    snprintf(cmd,
//             sizeof(cmd),
//             "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n",
//             ONENET_DEVICE_NAME,
//             ONENET_PRODUCT_ID,
//             ONENET_TOKEN);

//    if (BSP_ESP8266_SendCmdOK(cmd, "OK", 0, "ERROR", 0, 3000U) != HAL_OK)
//    {
//        return HAL_ERROR;
//    }

//    if (BSP_ESP8266_SendCmdOK("AT+MQTTCONNCFG=0,120,0,\"\",\"\",0,0\r\n",
//                              "OK",
//                              0,
//                              "ERROR",
//                              0,
//                              3000U) != HAL_OK)
//    {
//        return HAL_ERROR;
//    }

////    snprintf(cmd,
////             sizeof(cmd),
////             "AT+MQTTCONN=0,\"%s\",%u,1\r\n",
////             ONENET_HOST,
////             ONENET_PORT);

////    if (BSP_ESP8266_SendCmdOK(cmd,
////                              "+MQTTCONNECTED",
////                              "OK",
////                              "+MQTTDISCONNECTED",
////                              "ERROR",
////                              10000U) != HAL_OK)
////    {
////        return HAL_ERROR;
////    }
//	snprintf(cmd,
//         sizeof(cmd),
//         "AT+MQTTCONN=0,\"%s\",%u,1\r\n",
//         ONENET_HOST,
//         ONENET_PORT);

///* 第一步：先发命令 */
//if (BSP_ESP8266_SendString(cmd) != HAL_OK)
//{
//    return HAL_ERROR;
//}

///* 第二步：先等 AT 命令本身是否接受
// * 注意：这里的 OK 只表示 AT 命令已被模块接受，
// * 还不代表 MQTT 会话已经真正建立成功。
// */
//if (BSP_ESP8266_WaitResult("OK", 0, "ERROR", 0, 3000U) != ESP_WAIT_OK)
//{
//    return HAL_ERROR;
//}

///* 第三步：再单独等待真正的 MQTT 连接结果
// * 必须等到 +MQTTCONNECTED 才算真的连上。
// */
//if (BSP_ESP8266_WaitResult("+MQTTCONNECTED",
//                           0,
//                           "+MQTTDISCONNECTED",
//                           "ERROR",
//                           10000U) != ESP_WAIT_OK)
//{
//    return HAL_ERROR;
//}
//    if (BSP_ESP8266_SendCmdOK("AT+MQTTSUB=0,\"" TOPIC_PROPERTY_POST_REPLY "\",0\r\n",
//                              "OK", 0, "ERROR", 0, 3000U) != HAL_OK)
//    {
//        return HAL_ERROR;
//    }

//    if (BSP_ESP8266_SendCmdOK("AT+MQTTSUB=0,\"" TOPIC_SERVICE_SET_PUMP "\",0\r\n",
//                              "OK", 0, "ERROR", 0, 3000U) != HAL_OK)
//    {
//        return HAL_ERROR;
//    }

//    if (BSP_ESP8266_SendCmdOK("AT+MQTTSUB=0,\"" TOPIC_SERVICE_SET_LIGHT "\",0\r\n",
//                              "OK", 0, "ERROR", 0, 3000U) != HAL_OK)
//    {
//        return HAL_ERROR;
//    }

//    if (BSP_ESP8266_SendCmdOK("AT+MQTTSUB=0,\"" TOPIC_SERVICE_SET_BEEP "\",0\r\n",
//                              "OK", 0, "ERROR", 0, 3000U) != HAL_OK)
//    {
//        return HAL_ERROR;
//    }

//    BSP_ESP8266_ClearAsyncBuf();
//    return HAL_OK;
//}
HAL_StatusTypeDef BSP_ESP8266_StartOneNET(void)
{
    char cmd[512];

    /* 【终极绝杀1】：每次连云前，直接让 ESP8266 硬件重启！清空一切脏连接！ */
    BSP_OLED_ShowString(0, 4, "0. RST ESP...   "); BSP_OLED_Refresh();
    BSP_ESP8266_SendString("AT+RST\r\n");
    HAL_Delay(2000); /* 给模块2秒钟的开机时间 */
    
    /* 关回显，同步波特率 */
    BSP_ESP8266_SendCmdOK("ATE0\r\n", "OK", 0, "ERROR", 0, 1000U);

    BSP_OLED_ShowString(0, 4, "2. JOIN WIFI... "); BSP_OLED_Refresh();
    if (BSP_ESP8266_SetStationMode() != HAL_OK) return HAL_ERROR;
    if (BSP_ESP8266_JoinAP(WIFI_SSID, WIFI_PASSWORD) != HAL_OK) return HAL_ERROR;

    /* 给 WiFi 1秒钟的缓冲时间，防止模块抽风 */
    HAL_Delay(1000);

    BSP_OLED_ShowString(0, 4, "3. MQTT CFG...  "); BSP_OLED_Refresh();
    snprintf(cmd, sizeof(cmd),
             "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n",
             ONENET_DEVICE_NAME, ONENET_PRODUCT_ID, ONENET_TOKEN);
    if (BSP_ESP8266_SendCmdOK(cmd, "OK", 0, "ERROR", 0, 3000U) != HAL_OK)
    {
        BSP_OLED_ShowString(0, 4, "ERR: MQTT CFG   "); BSP_OLED_Refresh();
        return HAL_ERROR;
    }

    BSP_OLED_ShowString(0, 4, "4. MQTT CONN... "); BSP_OLED_Refresh();
    /* 【终极绝杀2】：加回 CONNCFG，设置 120秒心跳，并且 0 代表清除历史会话（OneNET强制要求） */
    if (BSP_ESP8266_SendCmdOK("AT+MQTTCONNCFG=0,120,0,\"\",\"\",0,0\r\n", "OK", 0, "ERROR", 0, 3000U) != HAL_OK)
    {
        BSP_OLED_ShowString(0, 4, "ERR: CONNCFG    "); BSP_OLED_Refresh();
        return HAL_ERROR;
    }

    /* 使用你之前测试没问题的直连 IP */
    snprintf(cmd, sizeof(cmd), "AT+MQTTCONN=0,\"%s\",%u,0\r\n", ONENET_HOST, ONENET_PORT);
    
    if (BSP_ESP8266_SendCmdOK(cmd, "+MQTTCONNECTED", "OK", "ERROR", "+MQTTDISCONNECTED", 15000U) != HAL_OK)
    {
        char err_buf[17] = {0};
        snprintf(err_buf, sizeof(err_buf), "E:%.14s", g_esp8266_rx_buf); 
        for(int i=0; i<16; i++) { if(err_buf[i]=='\r' || err_buf[i]=='\n') err_buf[i]=' '; }
        BSP_OLED_ShowString(0, 4, err_buf); 
        BSP_OLED_Refresh();
        HAL_Delay(4000);
        return HAL_ERROR;
    }
    
    BSP_OLED_ShowString(0, 4, "5. MQTT SUB...  "); BSP_OLED_Refresh();
    if (BSP_ESP8266_SendCmdOK("AT+MQTTSUB=0,\"" TOPIC_PROPERTY_POST_REPLY "\",0\r\n", "OK", 0, "ERROR", 0, 3000U) != HAL_OK) return HAL_ERROR;
    if (BSP_ESP8266_SendCmdOK("AT+MQTTSUB=0,\"" TOPIC_SERVICE_SET_PUMP "\",0\r\n", "OK", 0, "ERROR", 0, 3000U) != HAL_OK) return HAL_ERROR;
    if (BSP_ESP8266_SendCmdOK("AT+MQTTSUB=0,\"" TOPIC_SERVICE_SET_LIGHT "\",0\r\n", "OK", 0, "ERROR", 0, 3000U) != HAL_OK) return HAL_ERROR;
    if (BSP_ESP8266_SendCmdOK("AT+MQTTSUB=0,\"" TOPIC_SERVICE_SET_BEEP "\",0\r\n", "OK", 0, "ERROR", 0, 3000U) != HAL_OK) return HAL_ERROR;

    BSP_ESP8266_ClearAsyncBuf();
    return HAL_OK;
}
/* 发布属性 */
HAL_StatusTypeDef BSP_ESP8266_PublishPropertyJson(const char *json_body)
{
    return BSP_ESP8266_MqttPubRaw(TOPIC_PROPERTY_POST, json_body);
}

/* 回复服务执行结果 */
HAL_StatusTypeDef BSP_ESP8266_ReplyService(const ESP_ServiceCmd_t *cmd, uint8_t success)
{
    char topic[128];
    char payload[128];

    if (cmd == 0)
    {
        return HAL_ERROR;
    }

    switch (cmd->type)
    {
        case ESP_SERVICE_SET_PUMP:
            strcpy(topic, TOPIC_SERVICE_SET_PUMP_REPLY);
            break;

        case ESP_SERVICE_SET_LIGHT:
            strcpy(topic, TOPIC_SERVICE_SET_LIGHT_REPLY);
            break;

        case ESP_SERVICE_SET_BEEP:
            strcpy(topic, TOPIC_SERVICE_SET_BEEP_REPLY);
            break;

        default:
            return HAL_ERROR;
    }

    snprintf(payload,
             sizeof(payload),
             "{\"id\":\"%s\",\"code\":%u,\"msg\":\"%s\"}",
             cmd->id,
             (success != 0U) ? 200U : 500U,
             (success != 0U) ? "success" : "failed");

    return BSP_ESP8266_MqttPubRaw(topic, payload);
}

/* 轮询服务调用
 * 说明：
 * 1. 不用中断，主循环里轮询 USART1 是否有新字节
 * 2. 只处理服务调用 topic
 * 3. 属性 post/reply 消息只缓存，不做动作
 */
uint8_t BSP_ESP8266_PollService(ESP_ServiceCmd_t *cmd)
{
    BSP_ESP8266_DrainUartToAsync();

    if (g_async_len == 0U)
    {
        return 0U;
    }

    if (strstr(g_async_buf, TOPIC_PROPERTY_POST_REPLY) != 0)
    {
        /* 属性上报回复，不需要动作，清掉即可 */
        BSP_ESP8266_ClearAsyncBuf();
        return 0U;
    }

    if (BSP_ESP8266_TryExtractService(TOPIC_SERVICE_SET_PUMP, ESP_SERVICE_SET_PUMP, cmd) != 0U)
    {
        return 1U;
    }

    if (BSP_ESP8266_TryExtractService(TOPIC_SERVICE_SET_LIGHT, ESP_SERVICE_SET_LIGHT, cmd) != 0U)
    {
        return 1U;
    }

    if (BSP_ESP8266_TryExtractService(TOPIC_SERVICE_SET_BEEP, ESP_SERVICE_SET_BEEP, cmd) != 0U)
    {
        return 1U;
    }

    /* 缓冲太长，说明里面是无用字符或脏数据，清掉 */
    if (g_async_len > 900U)
    {
        BSP_ESP8266_ClearAsyncBuf();
    }

    return 0U;
}

const char *BSP_ESP8266_GetLastRx(void)
{
    return g_esp8266_rx_buf;

}

