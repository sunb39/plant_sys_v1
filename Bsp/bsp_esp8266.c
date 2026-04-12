////#include "bsp_esp8266.h"
////#include "usart.h"
////#include <stdio.h>
////#include <string.h>
////#include <stdlib.h>
////#include "bsp_oled.h"
/////* 最近一次 AT 回包 */
////static char g_esp8266_rx_buf[ESP8266_RX_BUF_SIZE];

/////* 异步消息缓存（用于接收 +MQTTSUBRECV） */
////static char g_async_buf[ESP8266_ASYNC_BUF_SIZE];
////static uint16_t g_async_len = 0U;

/////* 等待结果类型 */
////typedef enum
////{
////    ESP_WAIT_TIMEOUT = 0,
////    ESP_WAIT_OK,
////    ESP_WAIT_ERROR
////} ESP_WaitResult_t;

/////* 清空最近一次 AT 回包 */
////static void BSP_ESP8266_ClearRxBuf(void)
////{
////    memset(g_esp8266_rx_buf, 0, sizeof(g_esp8266_rx_buf));
////}

/////* 清空异步缓冲 */
////static void BSP_ESP8266_ClearAsyncBuf(void)
////{
////    memset(g_async_buf, 0, sizeof(g_async_buf));
////    g_async_len = 0U;
////}

/////* 串口发字符串 */
////static HAL_StatusTypeDef BSP_ESP8266_SendString(const char *str)
////{
////    if (str == 0)
////    {
////        return HAL_ERROR;
////    }

////    return HAL_UART_Transmit(&huart1,
////                             (uint8_t *)str,
////                             (uint16_t)strlen(str),
////                             ESP8266_CMD_TIMEOUT_MS);
////}

/////* 把 UART 中残留的异步数据吸走，放到异步缓存 */
////static void BSP_ESP8266_DrainUartToAsync(void)
////{
////    uint8_t ch = 0U;

////    /* 【防假死补丁】：如果单片机处理太慢导致串口塞满（ORE溢出），强制清除标志位，让它继续接收 */
////    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_ORE) != RESET)
////    {
////        __HAL_UART_CLEAR_OREFLAG(&huart1);
////    }

////    while (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE) != RESET)
////    {
////        ch = (uint8_t)(huart1.Instance->DR & 0xFFU);

////        if (g_async_len < (ESP8266_ASYNC_BUF_SIZE - 1U))
////        {
////            g_async_buf[g_async_len++] = (char)ch;
////            g_async_buf[g_async_len] = '\0';
////        }
////        else
////        {
////            /* 缓冲满了就清掉，避免一直脏数据 */
////            BSP_ESP8266_ClearAsyncBuf();
////        }
////    }
////}

/////* 等待 AT 响应
//// * 说明：
//// * 1. ok1 / ok2 任一匹配则判成功
//// * 2. err1 / err2 任一匹配则判失败
//// */
////static ESP_WaitResult_t BSP_ESP8266_WaitResult(const char *ok1,
////                                               const char *ok2,
////                                               const char *err1,
////                                               const char *err2,
////                                               uint32_t timeout_ms)
////{
////    uint32_t start_tick = HAL_GetTick();
////    uint16_t rx_len = 0U;
////    uint8_t ch = 0U;

////    BSP_ESP8266_ClearRxBuf();

////    while ((HAL_GetTick() - start_tick) < timeout_ms)
////    {
////        if (HAL_UART_Receive(&huart1, &ch, 1U, 50U) == HAL_OK)
////        {
////            if (rx_len < (ESP8266_RX_BUF_SIZE - 1U))
////            {
////                g_esp8266_rx_buf[rx_len++] = (char)ch;
////                g_esp8266_rx_buf[rx_len] = '\0';
////            }

////            if ((err1 != 0) && (strstr(g_esp8266_rx_buf, err1) != 0))
////            {
////                return ESP_WAIT_ERROR;
////            }

////            if ((err2 != 0) && (strstr(g_esp8266_rx_buf, err2) != 0))
////            {
////                return ESP_WAIT_ERROR;
////            }

////            if ((ok1 != 0) && (strstr(g_esp8266_rx_buf, ok1) != 0))
////            {
////                return ESP_WAIT_OK;
////            }

////            if ((ok2 != 0) && (strstr(g_esp8266_rx_buf, ok2) != 0))
////            {
////                return ESP_WAIT_OK;
////            }
////        }
////    }

////    return ESP_WAIT_TIMEOUT;
////}

/////* 发送 AT 并等待成功 */
////static HAL_StatusTypeDef BSP_ESP8266_SendCmdOK(const char *cmd,
////                                               const char *ok1,
////                                               const char *ok2,
////                                               const char *err1,
////                                               const char *err2,
////                                               uint32_t timeout_ms)
////{
////    ESP_WaitResult_t ret;

////    /* 先把异步脏数据吸走，避免影响当前 AT */
////    BSP_ESP8266_DrainUartToAsync();

////    if (BSP_ESP8266_SendString(cmd) != HAL_OK)
////    {
////        return HAL_ERROR;
////    }

////    ret = BSP_ESP8266_WaitResult(ok1, ok2, err1, err2, timeout_ms);
////    return (ret == ESP_WAIT_OK) ? HAL_OK : HAL_ERROR;
////}

/////* MQTTPUBRAW 发布原始 JSON
//// * 说明：
//// * 1. 先发 AT+MQTTPUBRAW
//// * 2. 等到 '>'
//// * 3. 再发原始 JSON 正文
//// */
////static HAL_StatusTypeDef BSP_ESP8266_MqttPubRaw(const char *topic, const char *payload)
////{
////    char cmd[256];
////    uint16_t len;

////    if ((topic == 0) || (payload == 0))
////    {
////        return HAL_ERROR;
////    }

////    len = (uint16_t)strlen(payload);

////    snprintf(cmd,
////             sizeof(cmd),
////             "AT+MQTTPUBRAW=0,\"%s\",%u,0,0\r\n",
////             topic,
////             len);

////    if (BSP_ESP8266_SendCmdOK(cmd, ">", 0, "ERROR", 0, 3000U) != HAL_OK)
////    {
////        return HAL_ERROR;
////    }

////    if (BSP_ESP8266_SendString(payload) != HAL_OK)
////    {
////        return HAL_ERROR;
////    }

////    if (BSP_ESP8266_WaitResult("+MQTTPUB:OK", "OK", "+MQTTPUB:FAIL", "ERROR", 5000U) != ESP_WAIT_OK)
////    {
////        return HAL_ERROR;
////    }

////    return HAL_OK;
////}

/////* 从文本中取出 "id":"xxx" */
////static uint8_t BSP_ESP8266_ParseId(const char *src, char *out, uint16_t out_size)
////{
////    const char *p;
////    uint16_t i = 0U;

////    if ((src == 0) || (out == 0) || (out_size == 0U))
////    {
////        return 0U;
////    }

////    p = strstr(src, "\"id\":\"");
////    if (p == 0)
////    {
////        return 0U;
////    }

////    p += 6; /* 跳过 "id":" */

////    while ((*p != '\0') && (*p != '"') && (i < (out_size - 1U)))
////    {
////        out[i++] = *p++;
////    }

////    out[i] = '\0';
////    return (i > 0U) ? 1U : 0U;
////}

/////* 从文本中取出 "value":0/1 */
////static uint8_t BSP_ESP8266_ParseValue01(const char *src, uint8_t *value)
////{
////    const char *p;

////    if ((src == 0) || (value == 0))
////    {
////        return 0U;
////    }

////    p = strstr(src, "\"value\":");
////    if (p == 0)
////    {
////        return 0U;
////    }

////    p += 8; /* 跳过 "value": */

////    while ((*p == ' ') || (*p == '\t'))
////    {
////        p++;
////    }

////    if (*p == '1')
////    {
////        *value = 1U;
////        return 1U;
////    }

////    if (*p == '0')
////    {
////        *value = 0U;
////        return 1U;
////    }

////    return 0U;
////}

///////* 试图从异步缓存中提取服务命令 */
//////static uint8_t BSP_ESP8266_TryExtractService(const char *topic,
//////                                             ESP_ServiceType_t type,
//////                                             ESP_ServiceCmd_t *cmd)
//////{
//////    if ((topic == 0) || (cmd == 0))
//////    {
//////        return 0U;
//////    }

//////    if (strstr(g_async_buf, topic) == 0)
//////    {
//////        return 0U;
//////    }

//////    memset(cmd, 0, sizeof(ESP_ServiceCmd_t));
//////    cmd->type = type;

//////    if (BSP_ESP8266_ParseId(g_async_buf, cmd->id, sizeof(cmd->id)) == 0U)
//////    {
//////        strcpy(cmd->id, "0");
//////    }

//////    if (BSP_ESP8266_ParseValue01(g_async_buf, &cmd->value) == 0U)
//////    {
//////        cmd->value = 0U;
//////    }

//////    /* 提取后清空，避免重复处理 */
//////    BSP_ESP8266_ClearAsyncBuf();
//////    return 1U;
//////}


///////* 试图从异步缓存中提取服务命令（解决粘包导致 ID 提取错乱问题） */
//////static uint8_t BSP_ESP8266_TryExtractService(const char *topic,
//////                                             ESP_ServiceType_t type,
//////                                             ESP_ServiceCmd_t *cmd)
//////{
//////    char *p_topic;

//////    if ((topic == 0) || (cmd == 0))
//////    {
//////        return 0U;
//////    }

//////    /* 1. 找到服务主题在缓存中的具体位置，并记录下指针 */
//////    p_topic = strstr(g_async_buf, topic);
//////    if (p_topic == 0)
//////    {
//////        return 0U;
//////    }

//////    memset(cmd, 0, sizeof(ESP_ServiceCmd_t));
//////    cmd->type = type;

//////    /* 2. 【核心修复】：必须从 p_topic 的位置往后找 ID 和 Value！
//////     * 绝不能传入 g_async_buf 从头找，否则会误抓到属性上报 reply 的 ID ("123") */
//////    if (BSP_ESP8266_ParseId(p_topic, cmd->id, sizeof(cmd->id)) == 0U)
//////    {
//////        strcpy(cmd->id, "0");
//////    }

//////    if (BSP_ESP8266_ParseValue01(p_topic, &cmd->value) == 0U)
//////    {
//////        cmd->value = 0U;
//////    }

//////    /* 3. 提取完成后清空缓存，避免重复处理 */
////////    BSP_ESP8266_ClearAsyncBuf();
//////	memset(p_topic, ' ', strlen(topic));
//////    return 1U;
//////}


/////* 试图从异步缓存中提取服务命令（极简直接提取版 - 已修复编译错误） */
////static uint8_t BSP_ESP8266_TryExtractService(const char *topic,
////                                             ESP_ServiceType_t type,
////                                             ESP_ServiceCmd_t *cmd)
////{
////    char *p_topic;
////    char *p_val;
////    char *p_id;

////    if ((topic == 0) || (cmd == 0))
////    {
////        return 0U;
////    }

////    /* 1. 找到服务主题在缓存中的具体位置 */
////    p_topic = strstr((char *)g_async_buf, topic);
////    if (p_topic == 0)
////    {
////        return 0U;
////    }

////    /* 初始化结构体并赋值类型 */
////    memset(cmd, 0, sizeof(ESP_ServiceCmd_t));
////    cmd->type = type;

////    /* 2. 寻找 "value": 提取 0 或 1 */
////    p_val = strstr(p_topic, "\"value\":");
////    if (p_val != 0)
////    {
////        // 向后偏移 8 个字符，直接判断是不是 '1'
////        if (p_val[8] == '1') {
////            cmd->value = 1U;
////        } else {
////            cmd->value = 0U;
////        }
////    }

////    /* 3. 寻找 "id":" 提取消息 ID (用于正确回复云端) */
////    p_id = strstr(p_topic, "\"id\":\"");
////    if (p_id != 0)
////    {
////        p_id += 6; /* 跳过 "id":" 这个字符串前缀 */
////        int i = 0;
////        /* 挨个抄写 ID 字符，直到遇到下一个双引号或装满 */
////        while ((p_id[i] != '\"') && (i < sizeof(cmd->id) - 1))
////        {
////            cmd->id[i] = p_id[i];
////            i++;
////        }
////        cmd->id[i] = '\0'; // 加上字符串结尾
////    }
////    else
////    {
////        strcpy(cmd->id, "0"); // 找不到则默认给 0
////    }

////    /* 4. 提取完成后，把这部分主题抹除成空格，避免死循环重复触发 */
////    memset(p_topic, ' ', strlen(topic));
////    
////    return 1U;
////}

/////* 初始化 */
////void BSP_ESP8266_Init(void)
////{
////    BSP_ESP8266_ClearRxBuf();
////    BSP_ESP8266_ClearAsyncBuf();
////}

/////* 测试 AT */
////HAL_StatusTypeDef BSP_ESP8266_TestAT(void)
////{
////    BSP_ESP8266_SendCmdOK("ATE0\r\n", "OK", 0, "ERROR", 0, 1000U);
////    return BSP_ESP8266_SendCmdOK("AT\r\n", "OK", 0, "ERROR", 0, 1000U);
////}

/////* STA 模式 */
////HAL_StatusTypeDef BSP_ESP8266_SetStationMode(void)
////{
////    return BSP_ESP8266_SendCmdOK("AT+CWMODE=1\r\n", "OK", 0, "ERROR", 0, 2000U);
////}

/////* 连热点 */
////HAL_StatusTypeDef BSP_ESP8266_JoinAP(const char *ssid, const char *password)
////{
////    char cmd[128];

////    if ((ssid == 0) || (password == 0))
////    {
////        return HAL_ERROR;
////    }

////    snprintf(cmd,
////             sizeof(cmd),
////             "AT+CWJAP=\"%s\",\"%s\"\r\n",
////             ssid,
////             password);

////    return BSP_ESP8266_SendCmdOK(cmd,
////                                 "WIFI GOT IP",
////                                 "OK",
////                                 "+CWJAP:",
////                                 "ERROR",
////                                 20000U);
////}

/////* 启动 OneNET 连接
//// * 说明：
//// * 1. AT 测试
//// * 2. 连 Wi-Fi
//// * 3. 配置 MQTT
//// * 4. 连接 OneNET
//// * 5. 订阅属性回复和三个服务调用 topic
//// */
//////HAL_StatusTypeDef BSP_ESP8266_StartOneNET(void)
//////{
//////    char cmd[512];

//////    if (BSP_ESP8266_TestAT() != HAL_OK)
//////    {
//////        return HAL_ERROR;
//////    }

//////    /* 先尽量清理旧 MQTT 连接，失败也不退出 */
//////    BSP_ESP8266_SendCmdOK("AT+MQTTCLEAN=0\r\n", "OK", 0, "ERROR", 0, 1000U);

//////    if (BSP_ESP8266_SetStationMode() != HAL_OK)
//////    {
//////        return HAL_ERROR;
//////    }

//////    if (BSP_ESP8266_JoinAP(WIFI_SSID, WIFI_PASSWORD) != HAL_OK)
//////    {
//////        return HAL_ERROR;
//////    }

//////    /* MQTT 用户配置
//////     * 这里 client_id = 设备名 produce1
//////     * username  = 产品ID
//////     * password  = token
//////     */
//////    snprintf(cmd,
//////             sizeof(cmd),
//////             "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n",
//////             ONENET_DEVICE_NAME,
//////             ONENET_PRODUCT_ID,
//////             ONENET_TOKEN);

//////    if (BSP_ESP8266_SendCmdOK(cmd, "OK", 0, "ERROR", 0, 3000U) != HAL_OK)
//////    {
//////        return HAL_ERROR;
//////    }

//////    if (BSP_ESP8266_SendCmdOK("AT+MQTTCONNCFG=0,120,0,\"\",\"\",0,0\r\n",
//////                              "OK",
//////                              0,
//////                              "ERROR",
//////                              0,
//////                              3000U) != HAL_OK)
//////    {
//////        return HAL_ERROR;
//////    }

////////    snprintf(cmd,
////////             sizeof(cmd),
////////             "AT+MQTTCONN=0,\"%s\",%u,1\r\n",
////////             ONENET_HOST,
////////             ONENET_PORT);

////////    if (BSP_ESP8266_SendCmdOK(cmd,
////////                              "+MQTTCONNECTED",
////////                              "OK",
////////                              "+MQTTDISCONNECTED",
////////                              "ERROR",
////////                              10000U) != HAL_OK)
////////    {
////////        return HAL_ERROR;
////////    }
//////	snprintf(cmd,
//////         sizeof(cmd),
//////         "AT+MQTTCONN=0,\"%s\",%u,1\r\n",
//////         ONENET_HOST,
//////         ONENET_PORT);

///////* 第一步：先发命令 */
//////if (BSP_ESP8266_SendString(cmd) != HAL_OK)
//////{
//////    return HAL_ERROR;
//////}

///////* 第二步：先等 AT 命令本身是否接受
////// * 注意：这里的 OK 只表示 AT 命令已被模块接受，
////// * 还不代表 MQTT 会话已经真正建立成功。
////// */
//////if (BSP_ESP8266_WaitResult("OK", 0, "ERROR", 0, 3000U) != ESP_WAIT_OK)
//////{
//////    return HAL_ERROR;
//////}

///////* 第三步：再单独等待真正的 MQTT 连接结果
////// * 必须等到 +MQTTCONNECTED 才算真的连上。
////// */
//////if (BSP_ESP8266_WaitResult("+MQTTCONNECTED",
//////                           0,
//////                           "+MQTTDISCONNECTED",
//////                           "ERROR",
//////                           10000U) != ESP_WAIT_OK)
//////{
//////    return HAL_ERROR;
//////}
//////    if (BSP_ESP8266_SendCmdOK("AT+MQTTSUB=0,\"" TOPIC_PROPERTY_POST_REPLY "\",0\r\n",
//////                              "OK", 0, "ERROR", 0, 3000U) != HAL_OK)
//////    {
//////        return HAL_ERROR;
//////    }

//////    if (BSP_ESP8266_SendCmdOK("AT+MQTTSUB=0,\"" TOPIC_SERVICE_SET_PUMP "\",0\r\n",
//////                              "OK", 0, "ERROR", 0, 3000U) != HAL_OK)
//////    {
//////        return HAL_ERROR;
//////    }

//////    if (BSP_ESP8266_SendCmdOK("AT+MQTTSUB=0,\"" TOPIC_SERVICE_SET_LIGHT "\",0\r\n",
//////                              "OK", 0, "ERROR", 0, 3000U) != HAL_OK)
//////    {
//////        return HAL_ERROR;
//////    }

//////    if (BSP_ESP8266_SendCmdOK("AT+MQTTSUB=0,\"" TOPIC_SERVICE_SET_BEEP "\",0\r\n",
//////                              "OK", 0, "ERROR", 0, 3000U) != HAL_OK)
//////    {
//////        return HAL_ERROR;
//////    }

//////    BSP_ESP8266_ClearAsyncBuf();
//////    return HAL_OK;
//////}
////HAL_StatusTypeDef BSP_ESP8266_StartOneNET(void)
////{
////    char cmd[512];

////    /* 【终极绝杀1】：每次连云前，直接让 ESP8266 硬件重启！清空一切脏连接！ */
////    BSP_OLED_ShowString(0, 4, "0. RST ESP...   "); BSP_OLED_Refresh();
////    BSP_ESP8266_SendString("AT+RST\r\n");
////    HAL_Delay(2000); /* 给模块2秒钟的开机时间 */
////    
////    /* 关回显，同步波特率 */
////    BSP_ESP8266_SendCmdOK("ATE0\r\n", "OK", 0, "ERROR", 0, 1000U);

////    BSP_OLED_ShowString(0, 4, "2. JOIN WIFI... "); BSP_OLED_Refresh();
////    if (BSP_ESP8266_SetStationMode() != HAL_OK) return HAL_ERROR;
////    if (BSP_ESP8266_JoinAP(WIFI_SSID, WIFI_PASSWORD) != HAL_OK) return HAL_ERROR;

////    /* 给 WiFi 1秒钟的缓冲时间，防止模块抽风 */
////    HAL_Delay(1000);

////    BSP_OLED_ShowString(0, 4, "3. MQTT CFG...  "); BSP_OLED_Refresh();
////    snprintf(cmd, sizeof(cmd),
////             "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n",
////             ONENET_DEVICE_NAME, ONENET_PRODUCT_ID, ONENET_TOKEN);
////    if (BSP_ESP8266_SendCmdOK(cmd, "OK", 0, "ERROR", 0, 3000U) != HAL_OK)
////    {
////        BSP_OLED_ShowString(0, 4, "ERR: MQTT CFG   "); BSP_OLED_Refresh();
////        return HAL_ERROR;
////    }

////    BSP_OLED_ShowString(0, 4, "4. MQTT CONN... "); BSP_OLED_Refresh();
////    /* 【终极绝杀2】：加回 CONNCFG，设置 120秒心跳，并且 0 代表清除历史会话（OneNET强制要求） */
////    if (BSP_ESP8266_SendCmdOK("AT+MQTTCONNCFG=0,120,0,\"\",\"\",0,0\r\n", "OK", 0, "ERROR", 0, 3000U) != HAL_OK)
////    {
////        BSP_OLED_ShowString(0, 4, "ERR: CONNCFG    "); BSP_OLED_Refresh();
////        return HAL_ERROR;
////    }

////    /* 使用你之前测试没问题的直连 IP */
////    snprintf(cmd, sizeof(cmd), "AT+MQTTCONN=0,\"%s\",%u,0\r\n", ONENET_HOST, ONENET_PORT);
////    
////    if (BSP_ESP8266_SendCmdOK(cmd, "+MQTTCONNECTED", "OK", "ERROR", "+MQTTDISCONNECTED", 15000U) != HAL_OK)
////    {
////        char err_buf[17] = {0};
////        snprintf(err_buf, sizeof(err_buf), "E:%.14s", g_esp8266_rx_buf); 
////        for(int i=0; i<16; i++) { if(err_buf[i]=='\r' || err_buf[i]=='\n') err_buf[i]=' '; }
////        BSP_OLED_ShowString(0, 4, err_buf); 
////        BSP_OLED_Refresh();
////        HAL_Delay(4000);
////        return HAL_ERROR;
////    }
////    
////    BSP_OLED_ShowString(0, 4, "5. MQTT SUB...  "); BSP_OLED_Refresh();
////    if (BSP_ESP8266_SendCmdOK("AT+MQTTSUB=0,\"" TOPIC_PROPERTY_POST_REPLY "\",0\r\n", "OK", 0, "ERROR", 0, 3000U) != HAL_OK) return HAL_ERROR;
////    if (BSP_ESP8266_SendCmdOK("AT+MQTTSUB=0,\"" TOPIC_SERVICE_SET_PUMP "\",0\r\n", "OK", 0, "ERROR", 0, 3000U) != HAL_OK) return HAL_ERROR;
////    if (BSP_ESP8266_SendCmdOK("AT+MQTTSUB=0,\"" TOPIC_SERVICE_SET_LIGHT "\",0\r\n", "OK", 0, "ERROR", 0, 3000U) != HAL_OK) return HAL_ERROR;
////    if (BSP_ESP8266_SendCmdOK("AT+MQTTSUB=0,\"" TOPIC_SERVICE_SET_BEEP "\",0\r\n", "OK", 0, "ERROR", 0, 3000U) != HAL_OK) return HAL_ERROR;

////    BSP_ESP8266_ClearAsyncBuf();
////    return HAL_OK;
////}
/////* 发布属性 */
////HAL_StatusTypeDef BSP_ESP8266_PublishPropertyJson(const char *json_body)
////{
////    return BSP_ESP8266_MqttPubRaw(TOPIC_PROPERTY_POST, json_body);
////}

/////* 回复服务执行结果 */
////HAL_StatusTypeDef BSP_ESP8266_ReplyService(const ESP_ServiceCmd_t *cmd, uint8_t success)
////{
////    char topic[128];
////    char payload[128];

////    if (cmd == 0)
////    {
////        return HAL_ERROR;
////    }

////    switch (cmd->type)
////    {
////        case ESP_SERVICE_SET_PUMP:
////            strcpy(topic, TOPIC_SERVICE_SET_PUMP_REPLY);
////            break;

////        case ESP_SERVICE_SET_LIGHT:
////            strcpy(topic, TOPIC_SERVICE_SET_LIGHT_REPLY);
////            break;

////        case ESP_SERVICE_SET_BEEP:
////            strcpy(topic, TOPIC_SERVICE_SET_BEEP_REPLY);
////            break;

////        default:
////            return HAL_ERROR;
////    }

////    // 【核心修复】：为 JSON 强行塞入 ,"data":{} 字段，满足 OneNET 的怪癖！
////    snprintf(payload,
////             sizeof(payload),
////             "{\"id\":\"%s\",\"code\":%u,\"msg\":\"%s\",\"data\":{}}",
////             cmd->id,
////             (success != 0U) ? 200U : 500U,
////             (success != 0U) ? "success" : "failed");

////    return BSP_ESP8266_MqttPubRaw(topic, payload);
////}

///////* 轮询服务调用
////// * 说明：
////// * 1. 不用中断，主循环里轮询 USART1 是否有新字节
////// * 2. 只处理服务调用 topic
////// * 3. 属性 post/reply 消息只缓存，不做动作
////// */
//////uint8_t BSP_ESP8266_PollService(ESP_ServiceCmd_t *cmd)
//////{
//////    BSP_ESP8266_DrainUartToAsync();

//////    if (g_async_len == 0U)
//////    {
//////        return 0U;
//////    }

//////    if (strstr(g_async_buf, TOPIC_PROPERTY_POST_REPLY) != 0)
//////    {
//////        /* 属性上报回复，不需要动作，清掉即可 */
//////        BSP_ESP8266_ClearAsyncBuf();
//////        return 0U;
//////    }

//////    if (BSP_ESP8266_TryExtractService(TOPIC_SERVICE_SET_PUMP, ESP_SERVICE_SET_PUMP, cmd) != 0U)
//////    {
//////        return 1U;
//////    }

//////    if (BSP_ESP8266_TryExtractService(TOPIC_SERVICE_SET_LIGHT, ESP_SERVICE_SET_LIGHT, cmd) != 0U)
//////    {
//////        return 1U;
//////    }

//////    if (BSP_ESP8266_TryExtractService(TOPIC_SERVICE_SET_BEEP, ESP_SERVICE_SET_BEEP, cmd) != 0U)
//////    {
//////        return 1U;
//////    }

//////    /* 缓冲太长，说明里面是无用字符或脏数据，清掉 */
//////    if (g_async_len > 900U)
//////    {
//////        BSP_ESP8266_ClearAsyncBuf();
//////    }

//////    return 0U;
//////}

//////const char *BSP_ESP8266_GetLastRx(void)
//////{
//////    return g_esp8266_rx_buf;

//////}
//////uint8_t BSP_ESP8266_PollService(ESP_ServiceCmd_t *cmd)
//////{
//////    BSP_ESP8266_DrainUartToAsync();

//////    if (g_async_len == 0U)
//////    {
//////        return 0U;
//////    }

//////    /* 1. 【核心修复】：必须优先检查有没有服务下发！
//////     * 即便发生了粘包，只要提取到服务，就优先处理服务。
//////     */
//////    if (BSP_ESP8266_TryExtractService(TOPIC_SERVICE_SET_PUMP, ESP_SERVICE_SET_PUMP, cmd))
//////    {
//////        return 1U;
//////    }
//////    else if (BSP_ESP8266_TryExtractService(TOPIC_SERVICE_SET_LIGHT, ESP_SERVICE_SET_LIGHT, cmd))
//////    {
//////        return 1U;
//////    }
//////    else if (BSP_ESP8266_TryExtractService(TOPIC_SERVICE_SET_BEEP, ESP_SERVICE_SET_BEEP, cmd))
//////    {
//////        return 1U;
//////    }

////////    /* 2. 如果找了一圈发现没有服务命令，再去处理无关痛痒的 reply 消息 */
////////    char *p_reply = strstr(g_async_buf, TOPIC_PROPERTY_POST_REPLY);
////////    if (p_reply != 0)
////////    {
////////        /* 同样使用空格抹除，绝不清空整个缓存 */
////////        memset(p_reply, ' ', strlen(TOPIC_PROPERTY_POST_REPLY));
////////        return 0U;
////////    }

//////    /* 3. 安全兜底：只有当缓存真的快满了（比如大于 800 字节），才被动清空一次防止溢出 */
//////    if (g_async_len >= (ESP8266_ASYNC_BUF_SIZE - 256))
//////    {
//////        BSP_ESP8266_ClearAsyncBuf();
//////    }

//////    return 0U;
//////}

////uint8_t BSP_ESP8266_PollService(ESP_ServiceCmd_t *cmd)
////{
////    BSP_ESP8266_DrainUartToAsync();

////    if (g_async_len == 0U)
////    {
////        return 0U;
////    }

////    /* 【核心修复】：放弃使用超长宏定义匹配！
////     * 直接搜索极短的核心关键字。这样不仅能绕过宏定义拼错的坑，
////     * 还能完美应对单片机卡顿时“漏接”了前面几个字母的问题！
////     */
////    if (BSP_ESP8266_TryExtractService("SetPump/invoke", ESP_SERVICE_SET_PUMP, cmd))
////    {
////        return 1U;
////    }
////    else if (BSP_ESP8266_TryExtractService("SetLight/invoke", ESP_SERVICE_SET_LIGHT, cmd))
////    {
////        return 1U;
////    }
////    else if (BSP_ESP8266_TryExtractService("SetBeep/invoke", ESP_SERVICE_SET_BEEP, cmd))
////    {
////        return 1U;
////    }

////    /* 安全兜底：只有当缓存真的快满了，才被动清空一次防止溢出 */
////    if (g_async_len >= (ESP8266_ASYNC_BUF_SIZE - 256))
////    {
////        BSP_ESP8266_ClearAsyncBuf();
////    }

////    return 0U;
////}


//#include "bsp_esp8266.h"
//#include "usart.h"
//#include <stdio.h>
//#include <string.h>
//#include <stdlib.h>
//#include "bsp_oled.h"
///* ? AT ? */
//static char g_esp8266_rx_buf[ESP8266_RX_BUF_SIZE];

///* ??棨? +MQTTSUBRECV */
//static char g_async_buf[ESP8266_ASYNC_BUF_SIZE];
//static uint16_t g_async_len = 0U;

///* ? */
//typedef enum
//{
//    ESP_WAIT_TIMEOUT = 0,
//    ESP_WAIT_OK,
//    ESP_WAIT_ERROR
//} ESP_WaitResult_t;

///* ? AT ? */
//static void BSP_ESP8266_ClearRxBuf(void)
//{
//    memset(g_esp8266_rx_buf, 0, sizeof(g_esp8266_rx_buf));
//}

///* ? */
//static void BSP_ESP8266_ClearAsyncBuf(void)
//{
//    memset(g_async_buf, 0, sizeof(g_async_buf));
//    g_async_len = 0U;
//}

///* 向异步缓冲区追加单个字符 */
//static void BSP_ESP8266_AppendAsyncChar(char ch)
//{
//    if (g_async_len < (ESP8266_ASYNC_BUF_SIZE - 1U))
//    {
//        g_async_buf[g_async_len++] = ch;
//        g_async_buf[g_async_len] = '\0';
//    }
//    else
//    {
//        BSP_ESP8266_ClearAsyncBuf();
//    }
//}

///* 向异步缓冲区追加字符串 */
//static void BSP_ESP8266_AppendAsyncText(const char *text)
//{
//    if (text == 0)
//    {
//        return;
//    }

//    while (*text != '\0')
//    {
//        BSP_ESP8266_AppendAsyncChar(*text++);
//    }
//}

///* ?? */
//static HAL_StatusTypeDef BSP_ESP8266_SendString(const char *str)
//{
//    if (str == 0)
//    {
//        return HAL_ERROR;
//    }

//    return HAL_UART_Transmit(&huart1,
//                             (uint8_t *)str,
//                             (uint16_t)strlen(str),
//                             ESP8266_CMD_TIMEOUT_MS);
//}

///*  UART в???? */
//static void BSP_ESP8266_DrainUartToAsync(void)
//{
//    uint8_t ch = 0U;

//    /* ??′ORE??λ */
//    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_ORE) != RESET)
//    {
//        __HAL_UART_CLEAR_OREFLAG(&huart1);
//    }

//    while (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE) != RESET)
//    {
//        ch = (uint8_t)(huart1.Instance->DR & 0xFFU);

//        if (g_async_len < (ESP8266_ASYNC_BUF_SIZE - 1U))
//        {
//            g_async_buf[g_async_len++] = (char)ch;
//            g_async_buf[g_async_len] = '\0';
//        }
//        else
//        {
//            /* ??? */
//            BSP_ESP8266_ClearAsyncBuf();
//        }
//    }
//}

///* ? AT ?
// * ?
// * 1. ok1 / ok2 ??г?
// * 2. err1 / err2 ???
// */
//static ESP_WaitResult_t BSP_ESP8266_WaitResult(const char *ok1,
//                                               const char *ok2,
//                                               const char *err1,
//                                               const char *err2,
//                                               uint32_t timeout_ms)
//{
//    uint32_t start_tick = HAL_GetTick();
//    uint16_t rx_len = 0U;
//    uint8_t ch = 0U;
//    char line_buf[256];
//    uint16_t line_len = 0U;

//    BSP_ESP8266_ClearRxBuf();

//    while ((HAL_GetTick() - start_tick) < timeout_ms)
//    {
//        if (HAL_UART_Receive(&huart1, &ch, 1U, 50U) == HAL_OK)
//        {
//            if (rx_len < (ESP8266_RX_BUF_SIZE - 1U))
//            {
//                g_esp8266_rx_buf[rx_len++] = (char)ch;
//                g_esp8266_rx_buf[rx_len] = '\0';
//            }

//            if (line_len < (sizeof(line_buf) - 1U))
//            {
//                line_buf[line_len++] = (char)ch;
//                line_buf[line_len] = '\0';
//            }
//            else
//            {
//                line_len = 0U;
//                line_buf[0] = '\0';
//            }

//            if (((char)ch == '\n') || ((char)ch == '\r'))
//            {
//                if (strstr(line_buf, "+MQTTSUBRECV:") != 0)
//                {
//                    BSP_ESP8266_AppendAsyncText(line_buf);
//                }

//                line_len = 0U;
//                line_buf[0] = '\0';
//            }

//            if ((err1 != 0) && (strstr(g_esp8266_rx_buf, err1) != 0))
//            {
//                return ESP_WAIT_ERROR;
//            }

//            if ((err2 != 0) && (strstr(g_esp8266_rx_buf, err2) != 0))
//            {
//                return ESP_WAIT_ERROR;
//            }

//            if ((ok1 != 0) && (strstr(g_esp8266_rx_buf, ok1) != 0))
//            {
//                return ESP_WAIT_OK;
//            }

//            if ((ok2 != 0) && (strstr(g_esp8266_rx_buf, ok2) != 0))
//            {
//                return ESP_WAIT_OK;
//            }
//        }
//    }

//    return ESP_WAIT_TIMEOUT;
//}

///*  AT ?? */
//static HAL_StatusTypeDef BSP_ESP8266_SendCmdOK(const char *cmd,
//                                               const char *ok1,
//                                               const char *ok2,
//                                               const char *err1,
//                                               const char *err2,
//                                               uint32_t timeout_ms)
//{
//    ESP_WaitResult_t ret;

//    /* ?????? AT */
//    BSP_ESP8266_DrainUartToAsync();

//    if (BSP_ESP8266_SendString(cmd) != HAL_OK)
//    {
//        return HAL_ERROR;
//    }

//    ret = BSP_ESP8266_WaitResult(ok1, ok2, err1, err2, timeout_ms);
//    return (ret == ESP_WAIT_OK) ? HAL_OK : HAL_ERROR;
//}

///* MQTTPUBRAW ?? JSON
// * ?
// * 1. ? AT+MQTTPUBRAW
// * 2. ? '>'
// * 3. ??? JSON 
// */
//static HAL_StatusTypeDef BSP_ESP8266_MqttPubRaw(const char *topic, const char *payload)
//{
//    char cmd[256];
//    uint16_t len;

//    if ((topic == 0) || (payload == 0))
//    {
//        return HAL_ERROR;
//    }

//    len = (uint16_t)strlen(payload);

//    snprintf(cmd,
//             sizeof(cmd),
//             "AT+MQTTPUBRAW=0,\"%s\",%u,0,0\r\n",
//             topic,
//             len);

//    if (BSP_ESP8266_SendCmdOK(cmd, ">", 0, "ERROR", 0, 3000U) != HAL_OK)
//    {
//        return HAL_ERROR;
//    }

//    if (BSP_ESP8266_SendString(payload) != HAL_OK)
//    {
//        return HAL_ERROR;
//    }

//    if (BSP_ESP8266_WaitResult("+MQTTPUB:OK", "OK", "+MQTTPUB:FAIL", "ERROR", 5000U) != ESP_WAIT_OK)
//    {
//        return HAL_ERROR;
//    }

//    return HAL_OK;
//}

///* ?? "id":"xxx" */
//static uint8_t BSP_ESP8266_ParseId(const char *src, char *out, uint16_t out_size)
//{
//    const char *p;
//    uint16_t i = 0U;

//    if ((src == 0) || (out == 0) || (out_size == 0U))
//    {
//        return 0U;
//    }

//    p = strstr(src, "\"id\":\"");
//    if (p == 0)
//    {
//        return 0U;
//    }

//    p += 6; /*  "id":" */

//    while ((*p != '\0') && (*p != '"') && (i < (out_size - 1U)))
//    {
//        out[i++] = *p++;
//    }

//    out[i] = '\0';
//    return (i > 0U) ? 1U : 0U;
//}

///* ?? "value":0/1 */
//static uint8_t BSP_ESP8266_ParseValue01(const char *src, uint8_t *value)
//{
//    const char *p;

//    if ((src == 0) || (value == 0))
//    {
//        return 0U;
//    }

//    p = strstr(src, "\"value\":");
//    if (p == 0)
//    {
//        return 0U;
//    }

//    p += 8; /*  "value": */

//    while ((*p == ' ') || (*p == '\t'))
//    {
//        p++;
//    }

//    if (*p == '1')
//    {
//        *value = 1U;
//        return 1U;
//    }

//    if (*p == '0')
//    {
//        *value = 0U;
//        return 1U;
//    }

//    return 0U;
//}

/////* ??? */
////static uint8_t BSP_ESP8266_TryExtractService(const char *topic,
////                                             ESP_ServiceType_t type,
////                                             ESP_ServiceCmd_t *cmd)
////{
////    if ((topic == 0) || (cmd == 0))
////    {
////        return 0U;
////    }

////    if (strstr(g_async_buf, topic) == 0)
////    {
////        return 0U;
////    }

////    memset(cmd, 0, sizeof(ESP_ServiceCmd_t));
////    cmd->type = type;

////    if (BSP_ESP8266_ParseId(g_async_buf, cmd->id, sizeof(cmd->id)) == 0U)
////    {
////        strcpy(cmd->id, "0");
////    }

////    if (BSP_ESP8266_ParseValue01(g_async_buf, &cmd->value) == 0U)
////    {
////        cmd->value = 0U;
////    }

////    /* ??? */
////    BSP_ESP8266_ClearAsyncBuf();
////    return 1U;
////}


/////* ????? ID ?? */
////static uint8_t BSP_ESP8266_TryExtractService(const char *topic,
////                                             ESP_ServiceType_t type,
////                                             ESP_ServiceCmd_t *cmd)
////{
////    char *p_topic;

////    if ((topic == 0) || (cmd == 0))
////    {
////        return 0U;
////    }

////    /* 1. ??е?λ??? */
////    p_topic = strstr(g_async_buf, topic);
////    if (p_topic == 0)
////    {
////        return 0U;
////    }

////    memset(cmd, 0, sizeof(ESP_ServiceCmd_t));
////    cmd->type = type;

////    /* 2. ? p_topic λ ID  Value
////     * ? g_async_buf ???? reply  ID ("123") */
////    if (BSP_ESP8266_ParseId(p_topic, cmd->id, sizeof(cmd->id)) == 0U)
////    {
////        strcpy(cmd->id, "0");
////    }

////    if (BSP_ESP8266_ParseValue01(p_topic, &cmd->value) == 0U)
////    {
////        cmd->value = 0U;
////    }

////    /* 3. ???棬? */
//////    BSP_ESP8266_ClearAsyncBuf();
////	memset(p_topic, ' ', strlen(topic));
////    return 1U;
////}


///* ?????? - ? */
//static uint8_t BSP_ESP8266_TryExtractService(const char *topic,
//                                             ESP_ServiceType_t type,
//                                             ESP_ServiceCmd_t *cmd)
//{
//    char *p_topic;
//    char *p_val;
//    char *p_id;

//    if ((topic == 0) || (cmd == 0))
//    {
//        return 0U;
//    }

//    /* 1. ??е?λ */
//    p_topic = strstr((char *)g_async_buf, topic);
//    if (p_topic == 0)
//    {
//        return 0U;
//    }

//    /* ??岢? */
//    memset(cmd, 0, sizeof(ESP_ServiceCmd_t));
//    cmd->type = type;

//    /* 2. ? "value": ? 0  1 */
//    p_val = strstr(p_topic, "\"value\":");
//    if (p_val != 0)
//    {
//        // ? 8 ??ж? '1'
//        if (p_val[8] == '1') {
//            cmd->value = 1U;
//        } else {
//            cmd->value = 0U;
//        }
//    }

//    /* 3. ? "id":" ?? ID (???) */
//    p_id = strstr(p_topic, "\"id\":\"");
//    if (p_id != 0)
//    {
//        p_id += 6; /*  "id":" ??? */
//        int i = 0;
//        /* д ID ?????? */
//        while ((p_id[i] != '\"') && (i < sizeof(cmd->id) - 1))
//        {
//            cmd->id[i] = p_id[i];
//            i++;
//        }
//        cmd->id[i] = '\0'; // ?β
//    }
//    else
//    {
//        strcpy(cmd->id, "0"); // ??? 0
//    }

//    /* 4. ???????????? */
//    memset(p_topic, ' ', strlen(topic));
//    
//    return 1U;
//}

///* ? */
//void BSP_ESP8266_Init(void)
//{
//    BSP_ESP8266_ClearRxBuf();
//    BSP_ESP8266_ClearAsyncBuf();
//}

///*  AT */
//HAL_StatusTypeDef BSP_ESP8266_TestAT(void)
//{
//    BSP_ESP8266_SendCmdOK("ATE0\r\n", "OK", 0, "ERROR", 0, 1000U);
//    return BSP_ESP8266_SendCmdOK("AT\r\n", "OK", 0, "ERROR", 0, 1000U);
//}

///* STA ?? */
//HAL_StatusTypeDef BSP_ESP8266_SetStationMode(void)
//{
//    return BSP_ESP8266_SendCmdOK("AT+CWMODE=1\r\n", "OK", 0, "ERROR", 0, 2000U);
//}

///* ? */
//HAL_StatusTypeDef BSP_ESP8266_JoinAP(const char *ssid, const char *password)
//{
//    char cmd[128];

//    if ((ssid == 0) || (password == 0))
//    {
//        return HAL_ERROR;
//    }

//    snprintf(cmd,
//             sizeof(cmd),
//             "AT+CWJAP=\"%s\",\"%s\"\r\n",
//             ssid,
//             password);

//    return BSP_ESP8266_SendCmdOK(cmd,
//                                 "WIFI GOT IP",
//                                 "OK",
//                                 "+CWJAP:",
//                                 "ERROR",
//                                 20000U);
//}

///*  OneNET 
// * ?
// * 1. AT 
// * 2.  Wi-Fi
// * 3.  MQTT
// * 4.  OneNET
// * 5. ?? topic
// */
////HAL_StatusTypeDef BSP_ESP8266_StartOneNET(void)
////{
////    char cmd[512];

////    if (BSP_ESP8266_TestAT() != HAL_OK)
////    {
////        return HAL_ERROR;
////    }

////    /* ? MQTT ???? */
////    BSP_ESP8266_SendCmdOK("AT+MQTTCLEAN=0\r\n", "OK", 0, "ERROR", 0, 1000U);

////    if (BSP_ESP8266_SetStationMode() != HAL_OK)
////    {
////        return HAL_ERROR;
////    }

////    if (BSP_ESP8266_JoinAP(WIFI_SSID, WIFI_PASSWORD) != HAL_OK)
////    {
////        return HAL_ERROR;
////    }

////    /* MQTT ?
////     *  client_id = 豸 produce1
////     * username  = ?ID
////     * password  = token
////     */
////    snprintf(cmd,
////             sizeof(cmd),
////             "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n",
////             ONENET_DEVICE_NAME,
////             ONENET_PRODUCT_ID,
////             ONENET_TOKEN);

////    if (BSP_ESP8266_SendCmdOK(cmd, "OK", 0, "ERROR", 0, 3000U) != HAL_OK)
////    {
////        return HAL_ERROR;
////    }

////    if (BSP_ESP8266_SendCmdOK("AT+MQTTCONNCFG=0,120,0,\"\",\"\",0,0\r\n",
////                              "OK",
////                              0,
////                              "ERROR",
////                              0,
////                              3000U) != HAL_OK)
////    {
////        return HAL_ERROR;
////    }

//////    snprintf(cmd,
//////             sizeof(cmd),
//////             "AT+MQTTCONN=0,\"%s\",%u,1\r\n",
//////             ONENET_HOST,
//////             ONENET_PORT);

//////    if (BSP_ESP8266_SendCmdOK(cmd,
//////                              "+MQTTCONNECTED",
//////                              "OK",
//////                              "+MQTTDISCONNECTED",
//////                              "ERROR",
//////                              10000U) != HAL_OK)
//////    {
//////        return HAL_ERROR;
//////    }
////	snprintf(cmd,
////         sizeof(cmd),
////         "AT+MQTTCONN=0,\"%s\",%u,1\r\n",
////         ONENET_HOST,
////         ONENET_PORT);

/////* ?? */
////if (BSP_ESP8266_SendString(cmd) != HAL_OK)
////{
////    return HAL_ERROR;
////}

/////* ?? AT ??
//// * ?? OK ?? AT ???
//// *  MQTT ???
//// */
////if (BSP_ESP8266_WaitResult("OK", 0, "ERROR", 0, 3000U) != ESP_WAIT_OK)
////{
////    return HAL_ERROR;
////}

/////* ?? MQTT ?
//// * ? +MQTTCONNECTED ?
//// */
////if (BSP_ESP8266_WaitResult("+MQTTCONNECTED",
////                           0,
////                           "+MQTTDISCONNECTED",
////                           "ERROR",
////                           10000U) != ESP_WAIT_OK)
////{
////    return HAL_ERROR;
////}
////    if (BSP_ESP8266_SendCmdOK("AT+MQTTSUB=0,\"" TOPIC_PROPERTY_POST_REPLY "\",0\r\n",
////                              "OK", 0, "ERROR", 0, 3000U) != HAL_OK)
////    {
////        return HAL_ERROR;
////    }

////    if (BSP_ESP8266_SendCmdOK("AT+MQTTSUB=0,\"" TOPIC_SERVICE_SET_PUMP "\",0\r\n",
////                              "OK", 0, "ERROR", 0, 3000U) != HAL_OK)
////    {
////        return HAL_ERROR;
////    }

////    if (BSP_ESP8266_SendCmdOK("AT+MQTTSUB=0,\"" TOPIC_SERVICE_SET_LIGHT "\",0\r\n",
////                              "OK", 0, "ERROR", 0, 3000U) != HAL_OK)
////    {
////        return HAL_ERROR;
////    }

////    if (BSP_ESP8266_SendCmdOK("AT+MQTTSUB=0,\"" TOPIC_SERVICE_SET_BEEP "\",0\r\n",
////                              "OK", 0, "ERROR", 0, 3000U) != HAL_OK)
////    {
////        return HAL_ERROR;
////    }

////    BSP_ESP8266_ClearAsyncBuf();
////    return HAL_OK;
////}
//HAL_StatusTypeDef BSP_ESP8266_StartOneNET(void)
//{
//    char cmd[512];

//    /* ??1??? ESP8266 ??? */
//    BSP_OLED_ShowString(0, 4, "0. RST ESP...   "); BSP_OLED_Refresh();
//    BSP_ESP8266_SendString("AT+RST\r\n");
//    HAL_Delay(2000); /* ?2??? */
//    
//    /* ??? */
//    BSP_ESP8266_SendCmdOK("ATE0\r\n", "OK", 0, "ERROR", 0, 1000U);

//    BSP_OLED_ShowString(0, 4, "2. JOIN WIFI... "); BSP_OLED_Refresh();
//    if (BSP_ESP8266_SetStationMode() != HAL_OK) return HAL_ERROR;
//    if (BSP_ESP8266_JoinAP(WIFI_SSID, WIFI_PASSWORD) != HAL_OK) return HAL_ERROR;

//    /*  WiFi 1?????? */
//    HAL_Delay(1000);

//    BSP_OLED_ShowString(0, 4, "3. MQTT CFG...  "); BSP_OLED_Refresh();
//    snprintf(cmd, sizeof(cmd),
//             "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n",
//             ONENET_DEVICE_NAME, ONENET_PRODUCT_ID, ONENET_TOKEN);
//    if (BSP_ESP8266_SendCmdOK(cmd, "OK", 0, "ERROR", 0, 3000U) != HAL_OK)
//    {
//        BSP_OLED_ShowString(0, 4, "ERR: MQTT CFG   "); BSP_OLED_Refresh();
//        return HAL_ERROR;
//    }

//    BSP_OLED_ShowString(0, 4, "4. MQTT CONN... "); BSP_OLED_Refresh();
//    /* ??2? CONNCFG 120 0 ??OneNET?? */
//    if (BSP_ESP8266_SendCmdOK("AT+MQTTCONNCFG=0,120,0,\"\",\"\",0,0\r\n", "OK", 0, "ERROR", 0, 3000U) != HAL_OK)
//    {
//        BSP_OLED_ShowString(0, 4, "ERR: CONNCFG    "); BSP_OLED_Refresh();
//        return HAL_ERROR;
//    }

//    /* ????? IP */
//    snprintf(cmd, sizeof(cmd), "AT+MQTTCONN=0,\"%s\",%u,0\r\n", ONENET_HOST, ONENET_PORT);
//    
//    if (BSP_ESP8266_SendCmdOK(cmd, "+MQTTCONNECTED", "OK", "ERROR", "+MQTTDISCONNECTED", 15000U) != HAL_OK)
//    {
//        char err_buf[17] = {0};
//        snprintf(err_buf, sizeof(err_buf), "E:%.14s", g_esp8266_rx_buf); 
//        for(int i=0; i<16; i++) { if(err_buf[i]=='\r' || err_buf[i]=='\n') err_buf[i]=' '; }
//        BSP_OLED_ShowString(0, 4, err_buf); 
//        BSP_OLED_Refresh();
//        HAL_Delay(4000);
//        return HAL_ERROR;
//    }
//    
//    BSP_OLED_ShowString(0, 4, "5. MQTT SUB...  "); BSP_OLED_Refresh();
//    if (BSP_ESP8266_SendCmdOK("AT+MQTTSUB=0,\"" TOPIC_PROPERTY_POST_REPLY "\",0\r\n", "OK", 0, "ERROR", 0, 3000U) != HAL_OK) return HAL_ERROR;
//    if (BSP_ESP8266_SendCmdOK("AT+MQTTSUB=0,\"" TOPIC_SERVICE_SET_PUMP "\",0\r\n", "OK", 0, "ERROR", 0, 3000U) != HAL_OK) return HAL_ERROR;
//    if (BSP_ESP8266_SendCmdOK("AT+MQTTSUB=0,\"" TOPIC_SERVICE_SET_LIGHT "\",0\r\n", "OK", 0, "ERROR", 0, 3000U) != HAL_OK) return HAL_ERROR;
//    if (BSP_ESP8266_SendCmdOK("AT+MQTTSUB=0,\"" TOPIC_SERVICE_SET_BEEP "\",0\r\n", "OK", 0, "ERROR", 0, 3000U) != HAL_OK) return HAL_ERROR;

//    BSP_ESP8266_ClearAsyncBuf();
//    return HAL_OK;
//}
///*  */
//HAL_StatusTypeDef BSP_ESP8266_PublishPropertyJson(const char *json_body)
//{
//    return BSP_ESP8266_MqttPubRaw(TOPIC_PROPERTY_POST, json_body);
//}

///* ??н */
//HAL_StatusTypeDef BSP_ESP8266_ReplyService(const ESP_ServiceCmd_t *cmd, uint8_t success)
//{
//    char topic[128];
//    char payload[128];

//    if (cmd == 0)
//    {
//        return HAL_ERROR;
//    }

//    switch (cmd->type)
//    {
//        case ESP_SERVICE_SET_PUMP:
//            strcpy(topic, TOPIC_SERVICE_SET_PUMP_REPLY);
//            break;

//        case ESP_SERVICE_SET_LIGHT:
//            strcpy(topic, TOPIC_SERVICE_SET_LIGHT_REPLY);
//            break;

//        case ESP_SERVICE_SET_BEEP:
//            strcpy(topic, TOPIC_SERVICE_SET_BEEP_REPLY);
//            break;

//        default:
//            return HAL_ERROR;
//    }

//    // ?? JSON ? ,"data":{} ?Σ OneNET ???
//    snprintf(payload,
//             sizeof(payload),
//             "{\"id\":\"%s\",\"code\":%u,\"msg\":\"%s\",\"data\":{}}",
//             cmd->id,
//             (success != 0U) ? 200U : 500U,
//             (success != 0U) ? "success" : "failed");

//    return BSP_ESP8266_MqttPubRaw(topic, payload);
//}

/////* ?
//// * ?
//// * 1. ж??? USART1 ??
//// * 2. ? topic
//// * 3.  post/reply ??棬
//// */
////uint8_t BSP_ESP8266_PollService(ESP_ServiceCmd_t *cmd)
////{
////    BSP_ESP8266_DrainUartToAsync();

////    if (g_async_len == 0U)
////    {
////        return 0U;
////    }

////    if (strstr(g_async_buf, TOPIC_PROPERTY_POST_REPLY) != 0)
////    {
////        /* ??? */
////        BSP_ESP8266_ClearAsyncBuf();
////        return 0U;
////    }

////    if (BSP_ESP8266_TryExtractService(TOPIC_SERVICE_SET_PUMP, ESP_SERVICE_SET_PUMP, cmd) != 0U)
////    {
////        return 1U;
////    }

////    if (BSP_ESP8266_TryExtractService(TOPIC_SERVICE_SET_LIGHT, ESP_SERVICE_SET_LIGHT, cmd) != 0U)
////    {
////        return 1U;
////    }

////    if (BSP_ESP8266_TryExtractService(TOPIC_SERVICE_SET_BEEP, ESP_SERVICE_SET_BEEP, cmd) != 0U)
////    {
////        return 1U;
////    }

////    /* ???? */
////    if (g_async_len > 900U)
////    {
////        BSP_ESP8266_ClearAsyncBuf();
////    }

////    return 0U;
////}

////const char *BSP_ESP8266_GetLastRx(void)
////{
////    return g_esp8266_rx_buf;

////}
////uint8_t BSP_ESP8266_PollService(ESP_ServiceCmd_t *cmd)
////{
////    BSP_ESP8266_DrainUartToAsync();

////    if (g_async_len == 0U)
////    {
////        return 0U;
////    }

////    /* 1. ???з·
////     * ????????
////     */
////    if (BSP_ESP8266_TryExtractService(TOPIC_SERVICE_SET_PUMP, ESP_SERVICE_SET_PUMP, cmd))
////    {
////        return 1U;
////    }
////    else if (BSP_ESP8266_TryExtractService(TOPIC_SERVICE_SET_LIGHT, ESP_SERVICE_SET_LIGHT, cmd))
////    {
////        return 1U;
////    }
////    else if (BSP_ESP8266_TryExtractService(TOPIC_SERVICE_SET_BEEP, ESP_SERVICE_SET_BEEP, cmd))
////    {
////        return 1U;
////    }

//////    /* 2. ???з???? reply ? */
//////    char *p_reply = strstr(g_async_buf, TOPIC_PROPERTY_POST_REPLY);
//////    if (p_reply != 0)
//////    {
//////        /* ????? */
//////        memset(p_reply, ' ', strlen(TOPIC_PROPERTY_POST_REPLY));
//////        return 0U;
//////    }

////    /* 3. ???е?? 800 ????η? */
////    if (g_async_len >= (ESP8266_ASYNC_BUF_SIZE - 256))
////    {
////        BSP_ESP8266_ClearAsyncBuf();
////    }

////    return 0U;
////}

//uint8_t BSP_ESP8266_PollService(ESP_ServiceCmd_t *cmd)
//{
//    BSP_ESP8266_DrainUartToAsync();

//    if (g_async_len == 0U)
//    {
//        return 0U;
//    }

//    /* 先匹配完整主题，匹配不到再用短关键字兜底 */
//    if (BSP_ESP8266_TryExtractService(TOPIC_SERVICE_SET_PUMP, ESP_SERVICE_SET_PUMP, cmd))
//    {
//        return 1U;
//    }
//    else if (BSP_ESP8266_TryExtractService(TOPIC_SERVICE_SET_LIGHT, ESP_SERVICE_SET_LIGHT, cmd))
//    {
//        return 1U;
//    }
//    else if (BSP_ESP8266_TryExtractService(TOPIC_SERVICE_SET_BEEP, ESP_SERVICE_SET_BEEP, cmd))
//    {
//        return 1U;
//    }
//    else if (BSP_ESP8266_TryExtractService("SetPump/invoke", ESP_SERVICE_SET_PUMP, cmd))
//    {
//        return 1U;
//    }
//    else if (BSP_ESP8266_TryExtractService("SetLight/invoke", ESP_SERVICE_SET_LIGHT, cmd))
//    {
//        return 1U;
//    }
//    else if (BSP_ESP8266_TryExtractService("SetBeep/invoke", ESP_SERVICE_SET_BEEP, cmd))
//    {
//        return 1U;
//    }

//    /* 属性上报回复不需要动作；不要整包清空，只在缓存极大时兜底清空 */
//    if (g_async_len >= (ESP8266_ASYNC_BUF_SIZE - 128U))
//    {
//        BSP_ESP8266_ClearAsyncBuf();
//    }

//    return 0U;
//}

#include "bsp_esp8266.h"
#include "usart.h"
#include "bsp_oled.h"
#include <stdio.h>
#include <string.h>

static char g_esp8266_rx_buf[ESP8266_RX_BUF_SIZE];
static uint16_t g_esp8266_rx_len = 0U;

static char g_async_buf[ESP8266_ASYNC_BUF_SIZE];
static uint16_t g_async_len = 0U;

static volatile uint8_t  g_uart_irq_fifo[ESP8266_IRQ_FIFO_SIZE];
static volatile uint16_t g_uart_irq_head = 0U;
static volatile uint16_t g_uart_irq_tail = 0U;
static volatile uint8_t  g_uart_irq_overflow = 0U;
static uint8_t g_uart_rx_byte = 0U;

typedef enum
{
    ESP_WAIT_TIMEOUT = 0,
    ESP_WAIT_OK,
    ESP_WAIT_ERROR
} ESP_WaitResult_t;

static void BSP_ESP8266_ClearCmdBuf(void)
{
    memset(g_esp8266_rx_buf, 0, sizeof(g_esp8266_rx_buf));
    g_esp8266_rx_len = 0U;
}

static void BSP_ESP8266_ClearAsyncBuf(void)
{
    memset(g_async_buf, 0, sizeof(g_async_buf));
    g_async_len = 0U;
}

static void BSP_ESP8266_ClearIrqFifo(void)
{
    __disable_irq();
    g_uart_irq_head = 0U;
    g_uart_irq_tail = 0U;
    g_uart_irq_overflow = 0U;
    __enable_irq();
}

static void BSP_ESP8266_StartReceiveIT(void)
{
    (void)HAL_UART_Receive_IT(&huart1, &g_uart_rx_byte, 1U);
}

static void BSP_ESP8266_IrqPushByte(uint8_t ch)
{
    uint16_t next = (uint16_t)((g_uart_irq_head + 1U) % ESP8266_IRQ_FIFO_SIZE);

    if (next == g_uart_irq_tail)
    {
        g_uart_irq_overflow = 1U;
        g_uart_irq_tail = (uint16_t)((g_uart_irq_tail + 1U) % ESP8266_IRQ_FIFO_SIZE);
    }

    g_uart_irq_fifo[g_uart_irq_head] = ch;
    g_uart_irq_head = next;
}

static uint8_t BSP_ESP8266_IrqPopByte(uint8_t *out)
{
    if ((out == 0) || (g_uart_irq_tail == g_uart_irq_head))
    {
        return 0U;
    }

    *out = g_uart_irq_fifo[g_uart_irq_tail];
    g_uart_irq_tail = (uint16_t)((g_uart_irq_tail + 1U) % ESP8266_IRQ_FIFO_SIZE);
    return 1U;
}

static void BSP_ESP8266_AppendRolling(char *buf, uint16_t *len, uint16_t size, char ch)
{
    if ((buf == 0) || (len == 0) || (size < 2U))
    {
        return;
    }

    if (*len >= (size - 1U))
    {
        memmove(buf, buf + 1, (size_t)(size - 2U));
        *len = (uint16_t)(size - 2U);
    }

    buf[*len] = ch;
    (*len)++;
    buf[*len] = '\0';
}

static void BSP_ESP8266_FeedIncoming(void)
{
    uint8_t ch = 0U;

    while (BSP_ESP8266_IrqPopByte(&ch) != 0U)
    {
        BSP_ESP8266_AppendRolling(g_esp8266_rx_buf,
                                  &g_esp8266_rx_len,
                                  ESP8266_RX_BUF_SIZE,
                                  (char)ch);

        BSP_ESP8266_AppendRolling(g_async_buf,
                                  &g_async_len,
                                  ESP8266_ASYNC_BUF_SIZE,
                                  (char)ch);
    }

    if (g_uart_irq_overflow != 0U)
    {
        g_uart_irq_overflow = 0U;
        BSP_ESP8266_AppendRolling(g_esp8266_rx_buf,
                                  &g_esp8266_rx_len,
                                  ESP8266_RX_BUF_SIZE,
                                  '!');
        BSP_ESP8266_AppendRolling(g_async_buf,
                                  &g_async_len,
                                  ESP8266_ASYNC_BUF_SIZE,
                                  '!');
    }
}

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

    p += 6;

    while ((*p != '\0') && (*p != '"') && (i < (out_size - 1U)))
    {
        out[i++] = *p++;
    }

    out[i] = '\0';
    return (i > 0U) ? 1U : 0U;
}

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

    p += 8;
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

static void BSP_ESP8266_RemoveAsyncPrefix(uint16_t remove_len)
{
    if (remove_len == 0U)
    {
        return;
    }

    if (remove_len >= g_async_len)
    {
        BSP_ESP8266_ClearAsyncBuf();
        return;
    }

    memmove(g_async_buf,
            g_async_buf + remove_len,
            (size_t)(g_async_len - remove_len + 1U));
    g_async_len = (uint16_t)(g_async_len - remove_len);
}

static uint8_t BSP_ESP8266_TryDiscardTopic(const char *topic)
{
    char *p_topic;
    char *p_end;
    uint16_t remove_len;

    if (topic == 0)
    {
        return 0U;
    }

    p_topic = strstr(g_async_buf, topic);
    if (p_topic == 0)
    {
        return 0U;
    }

    p_end = strstr(p_topic, "}}\r\n");
    if (p_end != 0)
    {
        remove_len = (uint16_t)((p_end + 4) - g_async_buf);
        BSP_ESP8266_RemoveAsyncPrefix(remove_len);
        return 1U;
    }

    p_end = strstr(p_topic, "}}");
    if (p_end != 0)
    {
        remove_len = (uint16_t)((p_end + 2) - g_async_buf);
        BSP_ESP8266_RemoveAsyncPrefix(remove_len);
        return 1U;
    }

    return 0U;
}

static uint8_t BSP_ESP8266_TryExtractService(const char *topic,
                                             ESP_ServiceType_t type,
                                             ESP_ServiceCmd_t *cmd)
{
    char *p_topic;
    char *p_payload;
    char *p_end;
    uint16_t remove_len;

    if ((topic == 0) || (cmd == 0))
    {
        return 0U;
    }

    p_topic = strstr(g_async_buf, topic);
    if (p_topic == 0)
    {
        return 0U;
    }

    p_payload = strchr(p_topic, '{');
    if (p_payload == 0)
    {
        return 0U;
    }

    p_end = strstr(p_payload, "}}\r\n");
    if (p_end == 0)
    {
        p_end = strstr(p_payload, "}}");
        if (p_end == 0)
        {
            return 0U;
        }
        remove_len = (uint16_t)((p_end + 2) - g_async_buf);
    }
    else
    {
        remove_len = (uint16_t)((p_end + 4) - g_async_buf);
    }

    memset(cmd, 0, sizeof(ESP_ServiceCmd_t));
    cmd->type = type;

    if (BSP_ESP8266_ParseId(p_payload, cmd->id, sizeof(cmd->id)) == 0U)
    {
        strcpy(cmd->id, "0");
    }

    if (BSP_ESP8266_ParseValue01(p_payload, &cmd->value) == 0U)
    {
        cmd->value = 0U;
    }

    BSP_ESP8266_RemoveAsyncPrefix(remove_len);
    return 1U;
}

static ESP_WaitResult_t BSP_ESP8266_WaitResult(const char *ok1,
                                               const char *ok2,
                                               const char *err1,
                                               const char *err2,
                                               uint32_t timeout_ms)
{
    uint32_t start_tick = HAL_GetTick();

    while ((HAL_GetTick() - start_tick) < timeout_ms)
    {
        BSP_ESP8266_FeedIncoming();

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

        HAL_Delay(1);
    }

    return ESP_WAIT_TIMEOUT;
}

static HAL_StatusTypeDef BSP_ESP8266_SendCmdOK(const char *cmd,
                                               const char *ok1,
                                               const char *ok2,
                                               const char *err1,
                                               const char *err2,
                                               uint32_t timeout_ms)
{
    ESP_WaitResult_t ret;

    BSP_ESP8266_FeedIncoming();
    BSP_ESP8266_ClearCmdBuf();

    if (BSP_ESP8266_SendString(cmd) != HAL_OK)
    {
        return HAL_ERROR;
    }

    ret = BSP_ESP8266_WaitResult(ok1, ok2, err1, err2, timeout_ms);
    return (ret == ESP_WAIT_OK) ? HAL_OK : HAL_ERROR;
}

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

void BSP_ESP8266_Init(void)
{
    BSP_ESP8266_ClearCmdBuf();
    BSP_ESP8266_ClearAsyncBuf();
    BSP_ESP8266_ClearIrqFifo();
    BSP_ESP8266_StartReceiveIT();
}

HAL_StatusTypeDef BSP_ESP8266_TestAT(void)
{
    (void)BSP_ESP8266_SendCmdOK("ATE0\r\n", "OK", 0, "ERROR", 0, 1000U);
    return BSP_ESP8266_SendCmdOK("AT\r\n", "OK", 0, "ERROR", 0, 1000U);
}

HAL_StatusTypeDef BSP_ESP8266_SetStationMode(void)
{
    return BSP_ESP8266_SendCmdOK("AT+CWMODE=1\r\n", "OK", 0, "ERROR", 0, 2000U);
}

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

//HAL_StatusTypeDef BSP_ESP8266_StartOneNET(void)
//{
//    char cmd[512];

//    BSP_OLED_ShowString(0, 4, "0. RST ESP...   ");
//    BSP_OLED_Refresh();
////    (void)BSP_ESP8266_SendString("AT+RST\r\n");
////    HAL_Delay(2000);

//    (void)BSP_ESP8266_SendCmdOK("ATE0\r\n", "OK", 0, "ERROR", 0, 1000U);

//    BSP_OLED_ShowString(0, 4, "2. JOIN WIFI... ");
//    BSP_OLED_Refresh();
//    if (BSP_ESP8266_SetStationMode() != HAL_OK)
//    {
//        return HAL_ERROR;
//    }
//    if (BSP_ESP8266_JoinAP(WIFI_SSID, WIFI_PASSWORD) != HAL_OK)
//    {
//        return HAL_ERROR;
//    }

//    HAL_Delay(1000);
//	
//	/* 先清理旧 MQTT 会话，失败也不直接退出 */
//BSP_ESP8266_SendCmdOK("AT+MQTTCLEAN=0\r\n",
//                      "OK",
//                      0,
//                      "ERROR",
//                      0,
//                      1000U);
//HAL_Delay(200);
//	
//    BSP_OLED_ShowString(0, 4, "3. MQTT CFG...  ");
//    BSP_OLED_Refresh();
//    snprintf(cmd,
//             sizeof(cmd),
//             "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n",
//             ONENET_DEVICE_NAME,
//             ONENET_PRODUCT_ID,
//             ONENET_TOKEN);
//    if (BSP_ESP8266_SendCmdOK(cmd, "OK", 0, "ERROR", 0, 3000U) != HAL_OK)
//    {
//        BSP_OLED_ShowString(0, 4, "ERR: MQTT CFG  ");
//        BSP_OLED_Refresh();
//        return HAL_ERROR;
//    }

//    BSP_OLED_ShowString(0, 4, "4. MQTT CONN... ");
//    BSP_OLED_Refresh();
//    if (BSP_ESP8266_SendCmdOK("AT+MQTTCONNCFG=0,120,0,\"\",\"\",0,0\r\n",
//                              "OK",
//                              0,
//                              "ERROR",
//                              0,
//                              3000U) != HAL_OK)
//    {
//        BSP_OLED_ShowString(0, 4, "ERR: CONNCFG   ");
//        BSP_OLED_Refresh();
//        return HAL_ERROR;
//    }

//    snprintf(cmd,
//             sizeof(cmd),
//             "AT+MQTTCONN=0,\"%s\",%u,0\r\n",
//             ONENET_HOST,
//             ONENET_PORT);

//    if (BSP_ESP8266_SendCmdOK(cmd,
//                              "+MQTTCONNECTED",
//                              "OK",
//                              "ERROR",
//                              "+MQTTDISCONNECTED",
//                              15000U) != HAL_OK)
//    {
//        char err_buf[17] = {0};
//        uint8_t i;

//        snprintf(err_buf, sizeof(err_buf), "E:%.14s", g_esp8266_rx_buf);
//        for (i = 0U; i < 16U; i++)
//        {
//            if ((err_buf[i] == '\r') || (err_buf[i] == '\n'))
//            {
//                err_buf[i] = ' ';
//            }
//        }
//        BSP_OLED_ShowString(0, 4, err_buf);
//        BSP_OLED_Refresh();
//        HAL_Delay(4000);
//        return HAL_ERROR;
//    }

//    BSP_OLED_ShowString(0, 4, "5. MQTT SUB...  ");
//    BSP_OLED_Refresh();
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


//HAL_StatusTypeDef BSP_ESP8266_StartOneNET(void)
//{
//    char cmd[512];

//    if (BSP_ESP8266_TestAT() != HAL_OK)
//    {
//        return HAL_ERROR;
//    }

//    /* 清理旧 MQTT 会话，失败也不立刻退出 */
//    (void)BSP_ESP8266_SendCmdOK("AT+MQTTCLEAN=0\r\n", "OK", 0, "ERROR", 0, 1000U);

//    if (BSP_ESP8266_SetStationMode() != HAL_OK)
//    {
//        return HAL_ERROR;
//    }

//    if (BSP_ESP8266_JoinAP(WIFI_SSID, WIFI_PASSWORD) != HAL_OK)
//    {
//        return HAL_ERROR;
//    }

//    snprintf(cmd, sizeof(cmd),
//             "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n",
//             ONENET_DEVICE_NAME,
//             ONENET_PRODUCT_ID,
//             ONENET_TOKEN);
//    if (BSP_ESP8266_SendCmdOK(cmd, "OK", 0, "ERROR", 0, 3000U) != HAL_OK)
//    {
//        return HAL_ERROR;
//    }

//    /* MQTT 连接参数 */
//if (BSP_ESP8266_SendCmdOK("AT+MQTTCONNCFG=0,120,0,\"\",\"\",0,0\r\n",
//                          "OK",
//                          0,
//                          "ERROR",
//                          0,
//                          3000U) != HAL_OK)
//{
//    BSP_OLED_ShowString(0, 4, "ERR: CONNCFG    ");
//    BSP_OLED_Refresh();
//    return HAL_ERROR;
//}

///* 发起 MQTT 连接
// * 注意：这版固件很多时候只返回 OK，不稳定输出 +MQTTCONNECTED
// * 所以后面改成：先认 OK，再用订阅是否成功作为最终验证
// */
//snprintf(cmd,
//         sizeof(cmd),
//         "AT+MQTTCONN=0,\"%s\",%u,0\r\n",
//         ONENET_HOST,
//         ONENET_PORT);

//if (BSP_ESP8266_SendCmdOK(cmd,
//                          "OK",
//                          0,
//                          "ERROR",
//                          "+MQTTDISCONNECTED",
//                          10000U) != HAL_OK)
//{
//    BSP_OLED_ShowString(0, 4, "ERR: MQTT CONN ");
//    BSP_OLED_Refresh();
//    return HAL_ERROR;
//}

///* 用订阅成功作为真正连上云的判据 */
//if (BSP_ESP8266_SendCmdOK("AT+MQTTSUB=0,\"" TOPIC_PROPERTY_POST_REPLY "\",0\r\n",
//                          "OK", 0, "ERROR", 0, 3000U) != HAL_OK)
//{
//    BSP_OLED_ShowString(0, 4, "ERR: SUB REPLY  ");
//    BSP_OLED_Refresh();
//    return HAL_ERROR;
//}

//if (BSP_ESP8266_SendCmdOK("AT+MQTTSUB=0,\"" TOPIC_SERVICE_SET_PUMP "\",0\r\n",
//                          "OK", 0, "ERROR", 0, 3000U) != HAL_OK)
//{
//    BSP_OLED_ShowString(0, 4, "ERR: SUB PUMP   ");
//    BSP_OLED_Refresh();
//    return HAL_ERROR;
//}

//if (BSP_ESP8266_SendCmdOK("AT+MQTTSUB=0,\"" TOPIC_SERVICE_SET_LIGHT "\",0\r\n",
//                          "OK", 0, "ERROR", 0, 3000U) != HAL_OK)
//{
//    BSP_OLED_ShowString(0, 4, "ERR: SUB LIGHT  ");
//    BSP_OLED_Refresh();
//    return HAL_ERROR;
//}

//if (BSP_ESP8266_SendCmdOK("AT+MQTTSUB=0,\"" TOPIC_SERVICE_SET_BEEP "\",0\r\n",
//                          "OK", 0, "ERROR", 0, 3000U) != HAL_OK)
//{
//    BSP_OLED_ShowString(0, 4, "ERR: SUB BEEP   ");
//    BSP_OLED_Refresh();
//    return HAL_ERROR;
//}

//BSP_ESP8266_ClearAsyncBuf();
//return HAL_OK;
//}


HAL_StatusTypeDef BSP_ESP8266_StartOneNET(void)
{
    char cmd[512];

    /* 1. 测试 AT */
    BSP_OLED_ShowString(0, 4, "1. TEST AT...   ");
    BSP_OLED_Refresh();
    if (BSP_ESP8266_TestAT() != HAL_OK)
    {
        BSP_OLED_ShowString(0, 4, "ERR: TEST AT   ");
        BSP_OLED_Refresh();
        return HAL_ERROR;
    }

    /* 2. 清理旧 MQTT 会话
     * 说明：失败不直接退出，尽量兼容模块当前状态
     */
    BSP_OLED_ShowString(0, 4, "2. CLEAN MQTT  ");
    BSP_OLED_Refresh();
    (void)BSP_ESP8266_SendCmdOK("AT+MQTTCLEAN=0\r\n", "OK", 0, "ERROR", 0, 1000U);

    /* 3. 设置 STA 模式 */
    BSP_OLED_ShowString(0, 4, "3. STA MODE... ");
    BSP_OLED_Refresh();
    if (BSP_ESP8266_SetStationMode() != HAL_OK)
    {
        BSP_OLED_ShowString(0, 4, "ERR: STA MODE  ");
        BSP_OLED_Refresh();
        return HAL_ERROR;
    }

    /* 4. 连接 WiFi */
    BSP_OLED_ShowString(0, 4, "4. JOIN WIFI.. ");
    BSP_OLED_Refresh();
    if (BSP_ESP8266_JoinAP(WIFI_SSID, WIFI_PASSWORD) != HAL_OK)
    {
        BSP_OLED_ShowString(0, 4, "ERR: JOIN WIFI ");
        BSP_OLED_Refresh();
        return HAL_ERROR;
    }

    /* 5. 配置 MQTT 用户信息 */
    BSP_OLED_ShowString(0, 4, "5. MQTT USER.. ");
    BSP_OLED_Refresh();
    snprintf(cmd, sizeof(cmd),
             "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n",
             ONENET_DEVICE_NAME,
             ONENET_PRODUCT_ID,
             ONENET_TOKEN);
    if (BSP_ESP8266_SendCmdOK(cmd, "OK", 0, "ERROR", 0, 3000U) != HAL_OK)
    {
        BSP_OLED_ShowString(0, 4, "ERR: USERCFG   ");
        BSP_OLED_Refresh();
        return HAL_ERROR;
    }

    /* 6. MQTT 连接参数 */
    BSP_OLED_ShowString(0, 4, "6. MQTT CFG... ");
    BSP_OLED_Refresh();
    if (BSP_ESP8266_SendCmdOK("AT+MQTTCONNCFG=0,120,0,\"\",\"\",0,0\r\n",
                              "OK",
                              0,
                              "ERROR",
                              0,
                              3000U) != HAL_OK)
    {
        BSP_OLED_ShowString(0, 4, "ERR: CONNCFG   ");
        BSP_OLED_Refresh();
        return HAL_ERROR;
    }

    /* 7. 发起 MQTT 连接
     * 说明：
     * 1. 这版固件很多时候只返回 OK，不稳定输出 +MQTTCONNECTED
     * 2. 因此这里只先认 OK
     * 3. 后面再用订阅是否成功作为真正连上云的判据
     */
    BSP_OLED_ShowString(0, 4, "7. MQTT CONN.. ");
    BSP_OLED_Refresh();
    snprintf(cmd,
             sizeof(cmd),
             "AT+MQTTCONN=0,\"%s\",%u,0\r\n",
             ONENET_HOST,
             ONENET_PORT);

    if (BSP_ESP8266_SendCmdOK(cmd,
                              "OK",
                              0,
                              "ERROR",
                              "+MQTTDISCONNECTED",
                              10000U) != HAL_OK)
    {
        BSP_OLED_ShowString(0, 4, "ERR: MQTT CONN ");
        BSP_OLED_Refresh();
        return HAL_ERROR;
    }

    /* 8. 订阅属性上报回复主题 */
    BSP_OLED_ShowString(0, 4, "8. SUB REPLY.. ");
    BSP_OLED_Refresh();
    if (BSP_ESP8266_SendCmdOK("AT+MQTTSUB=0,\"" TOPIC_PROPERTY_POST_REPLY "\",0\r\n",
                              "OK", 0, "ERROR", 0, 3000U) != HAL_OK)
    {
        BSP_OLED_ShowString(0, 4, "ERR: SUB REPLY ");
        BSP_OLED_Refresh();
        return HAL_ERROR;
    }

    /* 9. 订阅水泵服务主题 */
    BSP_OLED_ShowString(0, 4, "9. SUB PUMP... ");
    BSP_OLED_Refresh();
    if (BSP_ESP8266_SendCmdOK("AT+MQTTSUB=0,\"" TOPIC_SERVICE_SET_PUMP "\",0\r\n",
                              "OK", 0, "ERROR", 0, 3000U) != HAL_OK)
    {
        BSP_OLED_ShowString(0, 4, "ERR: SUB PUMP  ");
        BSP_OLED_Refresh();
        return HAL_ERROR;
    }

    /* 10. 订阅补光灯服务主题 */
    BSP_OLED_ShowString(0, 4, "10.SUB LIGHT.. ");
    BSP_OLED_Refresh();
    if (BSP_ESP8266_SendCmdOK("AT+MQTTSUB=0,\"" TOPIC_SERVICE_SET_LIGHT "\",0\r\n",
                              "OK", 0, "ERROR", 0, 3000U) != HAL_OK)
    {
        BSP_OLED_ShowString(0, 4, "ERR: SUB LIGHT ");
        BSP_OLED_Refresh();
        return HAL_ERROR;
    }

    /* 11. 订阅蜂鸣器服务主题 */
    BSP_OLED_ShowString(0, 4, "11.SUB BEEP... ");
    BSP_OLED_Refresh();
    if (BSP_ESP8266_SendCmdOK("AT+MQTTSUB=0,\"" TOPIC_SERVICE_SET_BEEP "\",0\r\n",
                              "OK", 0, "ERROR", 0, 3000U) != HAL_OK)
    {
        BSP_OLED_ShowString(0, 4, "ERR: SUB BEEP  ");
        BSP_OLED_Refresh();
        return HAL_ERROR;
    }

    /* 12. 清空异步缓冲，避免旧消息干扰 */
    BSP_ESP8266_ClearAsyncBuf();

    BSP_OLED_ShowString(0, 4, "CLOUD READY    ");
    BSP_OLED_Refresh();

    return HAL_OK;
}


HAL_StatusTypeDef BSP_ESP8266_PublishPropertyJson(const char *json_body)
{
    return BSP_ESP8266_MqttPubRaw(TOPIC_PROPERTY_POST, json_body);
}

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
             "{\"id\":\"%s\",\"code\":%u,\"msg\":\"%s\",\"data\":{}}",
             cmd->id,
             (success != 0U) ? 200U : 500U,
             (success != 0U) ? "success" : "failed");

    return BSP_ESP8266_MqttPubRaw(topic, payload);
}

uint8_t BSP_ESP8266_PollService(ESP_ServiceCmd_t *cmd)
{
	if (cmd == 0)
    {
        return 0U;
    }


    BSP_ESP8266_FeedIncoming();

    if (g_async_len == 0U)
    {
        return 0U;
    }

    (void)BSP_ESP8266_TryDiscardTopic(TOPIC_PROPERTY_POST_REPLY);

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

    if (g_async_len >= (ESP8266_ASYNC_BUF_SIZE - 64U))
    {
        BSP_ESP8266_RemoveAsyncPrefix((uint16_t)(g_async_len / 2U));
    }
	
	
    return 0U;
}

const char *BSP_ESP8266_GetLastRx(void)
{
    BSP_ESP8266_FeedIncoming();
    return g_esp8266_rx_buf;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if ((huart != 0) && (huart->Instance == USART1))
    {
        BSP_ESP8266_IrqPushByte(g_uart_rx_byte);
        BSP_ESP8266_StartReceiveIT();
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    volatile uint32_t temp;

    if ((huart == 0) || (huart->Instance != USART1))
    {
        return;
    }

    temp = huart->Instance->SR;
    temp = huart->Instance->DR;
    (void)temp;

    __HAL_UART_CLEAR_OREFLAG(huart);
    BSP_ESP8266_StartReceiveIT();
}



