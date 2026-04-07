#include "bsp_esp8266.h"
#include "bsp_delay.h"
#include "bsp_at24cxx.h"

#include "ring_buffer.h"
#include "pal_log.h"

#include "cJSON.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

wifi_config_t wifi_config = {WIFI_SSID, WIFI_PASS}; /* 存储 WIFI 的配置信息 */

char weather_json_buf[WEATHER_JSON_BUFFER_SIZE]; /* 提取出的完整的天气 JSON 字符串 */
WeatherInfo g_weather_info = {0};                /* 解析后的最终天气数据 */

NetTime_t g_net_time = {0}; /* 解析后的时间数据 */

/* ESP8266 接收状态机变量 */
static uint16_t g_json_idx = 0;    /* 当前 JSON 数据在缓冲区中的存储偏移量 */
static uint8_t g_json_started = 0; /* 标志位：是否已匹配到起始大括号 '{' */
static int16_t g_brace_cnt = 0;    /* 大括号平衡计数器，用于判断 JSON 嵌套是否结束 */

/* 内部函数 */
static void BSP_ESP8266_UsartSendString(char *_str);
static void BSP_ESP8266_WeatherReset(void);
static uint8_t BSP_ESP8266_WeatherRead(RingBuffer_t *rb, char *pDest, uint16_t maxLen);
static uint8_t BSP_ESP8266_WeatherParse(const char *json_str, WeatherInfo *info);
static uint8_t BSP_ESP8266_Time_Parse(const char *input, NetTime_t *time);

/**
 * @brief  ESP8266 初始化配置流程
 * @note   包含：测试、关闭回显、重启、模式设置、连接 WiFi、建立 TCP、进入透传
 * @retval 0: 配置成功; 1: 某个环节配置失败
 */
uint8_t BSP_ESP8266_Init(void)
{
    char buffer[128] = {0};

    BSP_DelayMs(1000); /* 延时 1S 等待 ESP8266 稳定 */

    /* 测试模块通信 */
    if (BSP_ESP8266_SendCmd(ESP8266_AT, "OK", 3000) == 1)
    {
        PAL_LOG(PAL_LOG_LEVEL_ERROR, "AT 通信测试失败\r\n");
        return 1;
    }

    /* 软件复位模块 */
    if (BSP_ESP8266_SendCmd(ESP8266_AT_RST, "ready", 5000) == 1)
    {
        PAL_LOG(PAL_LOG_LEVEL_ERROR, "模块复位失败\r\n");
        return 1;
    }

    /* 关闭命令回显 (ATE0) */
    if (BSP_ESP8266_SendCmd(ESP8266_AT_ATE0, "OK", 3000) == 1)
    {
        PAL_LOG(PAL_LOG_LEVEL_ERROR, "ATE0 设置失败\r\n");
        return 1;
    }

    /* 设置为 Station 模式 */
    if (BSP_ESP8266_SendCmd(ESP8266_AT_CWMODE("1"), "OK", 3000) == 1)
    {
        PAL_LOG(PAL_LOG_LEVEL_ERROR, "模式设置失败\r\n");
        return 1;
    }

    /* 加载 WIFI 配置信息 */
    BSP_ESP8266_LoadWiFiConfig(&wifi_config);
    snprintf(buffer, sizeof(wifi_config), ESP8266_AT_CWJAP, wifi_config.wifi_ssid, wifi_config.wifi_pwd);

    /* 连接 WiFi (超时时间较长) */
    if (BSP_ESP8266_SendCmd(buffer, "OK", 20000) == 1)
    {
        PAL_LOG(PAL_LOG_LEVEL_ERROR, "WiFi 连接失败\r\n");
        return 1;
    }

    /* 配置 SNTP 服务器 */
    if (BSP_ESP8266_SendCmd(ESP8266_AT_CIPSNTPCFG, "OK", 3000) == 1)
    {
        PAL_LOG(PAL_LOG_LEVEL_ERROR, "SNTP 服务器配置失败\r\n");
        return 1;
    }

    /* 设置单连接模式 */
    if (BSP_ESP8266_SendCmd(ESP8266_AT_CIPMUX_0, "OK", 3000) == 1)
    {
        PAL_LOG(PAL_LOG_LEVEL_ERROR, "CIPMUX 设置失败\r\n");
        return 1;
    }

    /* 建立 TCP 连接 */
    if (BSP_ESP8266_SendCmd(ESP8266_AT_CIPSTART_TCP, "OK", 3000) == 1)
    {
        PAL_LOG(PAL_LOG_LEVEL_ERROR, "TCP 连接建立失败\r\n");
        return 1;
    }

    /* 开启透传模式 (Transmission Mode) */
    if (BSP_ESP8266_SendCmd(ESP8266_AT_CIPMODE_1, "OK", 3000) == 1)
    {
        PAL_LOG(PAL_LOG_LEVEL_ERROR, "透传模式开启失败\r\n");
        return 1;
    }

    /* 开始发送指令，进入透传状态 */
    if (BSP_ESP8266_SendCmd(ESP8266_AT_CIPSEND, "OK", 3000) == 1)
    {
        PAL_LOG(PAL_LOG_LEVEL_ERROR, "进入透传状态失败\r\n");
        return 1;
    }

    PAL_LOG(PAL_LOG_LEVEL_INFO, "ESP8266 配置成功并已进入透传模式\r\n");

    if (BSP_ESP8266_WeatherUpdate(&g_weather_info) == 1)
    {
        return 1;
    }

    BSP_ESP8266_Weather_Print(&g_weather_info);

    if (BSP_ESP8266_SyncTime(&g_net_time) == 1)
    {
        PAL_LOG(PAL_LOG_LEVEL_ERROR, "请求网络时间失败\r\n");
        return 1;
    }

    BSP_ESP8266_Time_Print(&g_net_time);

    return 0;
}

/**
 * @brief  从 EEPROM 加载 WiFi 配置信息
 * @param  dest_config: 指向用于存放配置数据的结构体指针
 * @retval 加载结果: 0-成功加载有效数据, 1-数据无效并已恢复默认值
 */
uint8_t BSP_ESP8266_LoadWiFiConfig(wifi_config_t *dest_config)
{
    AT24Cxx_ReadData(0x0000, (uint8_t *)dest_config, sizeof(wifi_config_t)); /* 从 EEPROM 读取原始数据 */

    if ((uint8_t)dest_config->wifi_ssid[0] == 0xFF || dest_config->wifi_ssid[0] == '\0') /* 数据校验 */
    {
        /* 默认值 */
        strcpy(dest_config->wifi_ssid, WIFI_SSID);
        strcpy(dest_config->wifi_pwd, WIFI_PASS);
        return 1;
    }

    /* 确保字符串强制结尾 */
    dest_config->wifi_ssid[31] = '\0';
    dest_config->wifi_pwd[31] = '\0';

    return 0;
}

/**
 * @brief  更新并持久化保存新的 WiFi 配置
 * @param  new_config: 指向包含新 SSID 和密码的配置结构体指针
 * @retval 0-成功, 1-失败
 */
uint8_t BSP_ESP8266_UpdateWiFiConfig(const wifi_config_t *new_config)
{
    memcpy(&wifi_config, new_config, sizeof(wifi_config_t));                            /* 更新内存中的全局变量 */
    if (AT24Cxx_WriteData(0x0000, (uint8_t *)&wifi_config, sizeof(wifi_config_t)) == 1) /* 持久化存储到 EEPROM */
    {
        return 1;
    }

    return 0;
}

/**
 * @brief  通过串口向 ESP8266 发送字符串（AT 指令或数据）
 * @param  _str: 待发送的字符串指针
 * @retval None
 */
static void BSP_ESP8266_UsartSendString(char *_str)
{
    if (_str == NULL)
    {
        return;
    }

    HAL_UART_Transmit(&ESP8266_UART, (uint8_t *)_str, strlen(_str), 100);

    PAL_LOG(PAL_LOG_LEVEL_INFO, "ESP8266 发送指令 : %s", _str);
}

/**
 * @brief  向 ESP8266 发送指令并等待指定的响应内容
 * @param  cmd: 待发送的 AT 指令字符串
 * @param  ack: 期望收到的响应关键字（如 "OK", "READY"）
 * @param  timeout_ms: 等待响应的最大超时时间（毫秒）
 * @retval 0: 成功匹配到目标响应; 1: 发生错误或超时
 */
uint8_t BSP_ESP8266_SendCmd(char *cmd, char *ack, uint32_t timeout_ms)
{
    char line_buf[256];                  /* 用于暂存从环形缓冲区读出的一行数据 */
    static uint16_t idx = 0;             /* 静态变量：记录 RingBuffer_ReadLine 的解析进度 */
    uint32_t start_tick = HAL_GetTick(); /* 记录超时计数的起始时间 */

    /* 通过串口发送指令 */
    BSP_ESP8266_UsartSendString(cmd);

    /* 在超时时间内循环检查缓冲区 */
    while (HAL_GetTick() - start_tick < timeout_ms)
    {
        /* 调用行读取函数，若返回 1 表示成功解析出一行以 \n 结尾的字符串 */
        if (RingBuffer_ReadLine(&g_uart_rx_fifo, line_buf, sizeof(line_buf), &idx) == 0)
        {
            /* 打印调试信息：显示当前处理的一行内容 */
            PAL_LOG(PAL_LOG_LEVEL_INFO, "ESP8266 处理相应 : %s", line_buf);

            /* 检查当前行是否包含期望的关键字 (ack) */
            if (strstr(line_buf, ack) != NULL)
            {
                PAL_LOG(PAL_LOG_LEVEL_INFO, "ESP8266 检测到目标响应 : %s", ack);
                return 0; /* 匹配成功，返回 0 */
            }

            /* 检查是否收到模块报错标志 */
            if (strstr(line_buf, "ERROR") != NULL)
            {
                return 1; /* 模块返回错误，提前终止 */
            }
        }
    }

    /* 循环结束仍未匹配，判定为超时 */
    PAL_LOG(PAL_LOG_LEVEL_INFO, "ESP8266 等待响应超时");
    return 1;
}

/**
 * @brief  全流程获取并更新天气数据（包含请求、等待、解析、退出透传）
 * @param  info: 指向存储结果的结构体指针
 * @retval 0: 更新成功; 1: 更新失败或超时
 */
uint8_t BSP_ESP8266_WeatherUpdate(WeatherInfo *info)
{
    uint32_t start_time;
    uint8_t get_json_success = 0;

    /* 重置解析器状态，确保缓冲区干净 */
    BSP_ESP8266_WeatherReset();

    /* 发送 HTTP GET 请求 (假设此时已在透传模式或直接发送) */
    BSP_ESP8266_UsartSendString(ESP8266_AT_HTTP_GET_SENIVERSE);
    PAL_LOG(PAL_LOG_LEVEL_INFO, "等待 JSON 数据中...\r\n");

    /* 在 5 秒超时时间内不断尝试读取 JSON */
    start_time = HAL_GetTick();
    while (HAL_GetTick() - start_time < 5000)
    {
        /* 尝试从环形缓冲区提取完整 JSON */
        if (BSP_ESP8266_WeatherRead(&g_uart_rx_fifo, weather_json_buf, WEATHER_JSON_BUFFER_SIZE) == 0)
        {
            PAL_LOG(PAL_LOG_LEVEL_INFO, "成功获取完整 JSON 数据!\r\n");
            PAL_LOG(PAL_LOG_LEVEL_INFO, "内容: %s\n", weather_json_buf);

            /* 解析并填充结构体 */
            if (BSP_ESP8266_WeatherParse(weather_json_buf, info) == 0)
            {
                get_json_success = 1;
                break;
            }
        }
        BSP_DelayMs(5);
    }

    /* 退出逻辑：发送退出透传序列 "+++" */
    /* 注意：退出透传通常需要前后各 1 秒的静默期 */
    BSP_DelayMs(1000);
    BSP_ESP8266_UsartSendString("+++");
    BSP_DelayMs(1000);

    /* 握手确认：确保模块回到了 AT 模式 */
    if (BSP_ESP8266_SendCmd(ESP8266_AT, "OK", 2000) != 0)
    {
        PAL_LOG(PAL_LOG_LEVEL_ERROR, "退出透传失败或模块无响应\r\n");
    }

    if (get_json_success)
    {
        PAL_LOG(PAL_LOG_LEVEL_INFO, "天气更新任务完成\r\n");
        return 0;
    }
    else
    {
        PAL_LOG(PAL_LOG_LEVEL_ERROR, "天气更新失败\r\n");
        return 1;
    }
}

/**
 * @brief  重置 ESP8266 JSON 解析状态
 * @note   在每次发起新的 HTTP 请求前调用，确保解析从零开始
 * @param  None
 * @retval None
 */
static void BSP_ESP8266_WeatherReset(void)
{
    /* 恢复状态机到初始状态 */
    g_json_idx = 0;     /* 清零索引 */
    g_json_started = 0; /* 关闭开始标志 */
    g_brace_cnt = 0;    /* 大括号计数归零 */

    /* 同步清空底层环形缓冲区，防止残余数据干扰后续解析 */
    RingBuffer_Clear(&g_uart_rx_fifo);

    PAL_LOG(PAL_LOG_LEVEL_INFO, "ESP8266 JSON 解析状态重置");
}

/**
 * @brief  从环形缓冲区中提取完整的 JSON 对象
 * @param  rb: 指向环形缓冲区结构体的指针
 * @param  pDest: 存储提取出的 JSON 字符串的目标缓冲区
 * @param  maxLen: 目标缓冲区的最大允许长度
 * @retval 0: 成功提取到一个完整的 JSON 帧; 1: 数据不完整或解析中
 */
static uint8_t BSP_ESP8266_WeatherRead(RingBuffer_t *rb, char *pDest, uint16_t maxLen)
{
    /* 遍历环形缓冲区中所有未读字节 */
    while (RingBuffer_Available(rb))
    {
        char c = (char)RingBuffer_Read(rb);

        /* 寻找 JSON 起始字符 '{' */
        if (!g_json_started)
        {
            if (c == '{')
            {
                g_json_started = 1; /* 锁定起始状态 */
                g_brace_cnt = 1;    /* 初始化大括号平衡计数器 */
                g_json_idx = 0;     /* 重置目标缓冲区写入索引 */
                pDest[g_json_idx++] = c;
            }
            continue; /* 未发现起始符前，跳过所有非 '{' 字符 */
        }

        /* 收集数据并进行嵌套计数 */
        if (g_json_idx < maxLen - 1)
        {
            pDest[g_json_idx++] = c;

            /* 核心逻辑：记录嵌套深度，确保提取的是完整的 JSON 对象而非碎片 */
            if (c == '{')
            {
                g_brace_cnt++;
            }
            else if (c == '}')
            {
                g_brace_cnt--;
            }

            /* 匹配成功：当计数器归零时，说明最外层大括号已闭合 */
            if (g_brace_cnt == 0)
            {
                pDest[g_json_idx] = '\0';   /* 添加字符串结束符 */
                BSP_ESP8266_WeatherReset(); /* 成功提取后手动重置解析状态 */
                return 0;                   /* 返回成功标志 */
            }
        }
        else
        {
            /* 溢出保护：若单帧 JSON 长度超过 maxLen，则报错并重置，防止内存越界 */
            PAL_LOG(PAL_LOG_LEVEL_ERROR, "ESP8266 环形缓冲区溢出");
            BSP_ESP8266_WeatherReset();
            return 1;
        }
    }

    /* 缓冲区已读空，但 JSON 尚未闭合（等待后续数据） */
    return 1;
}

/**
 * @brief  从 cJSON 对象中获取指定字段并转换为整型
 * @param  obj: 指向待解析的 cJSON 对象节点
 * @param  key: 字段键名 (Key)
 * @retval 字段对应的整数值 (若字段不存在或非字符串类型则返回 0)
 * @note   该函数专门处理 JSON 中以字符串形式存储的数字 (使用 atoi 转换)
 */
static int cJSON_GetFieldAsInt(cJSON *obj, const char *key)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (cJSON_IsString(item) && item->valuestring)
    {
        return atoi(item->valuestring);
    }

    return 0;
}

/**
 * @brief  从 cJSON 对象中获取指定字段并转换为浮点型
 * @param  obj: 指向待解析的 cJSON 对象节点
 * @param  key: 字段键名 (Key)
 * @retval 转换后的 float 型数值 (若字段不存在或非字符串类型则返回 0.0f)
 * @note   该函数封装了字符串转浮点数逻辑，主要用于处理 JSON 中以字符串格式存储的数值
 */
static float cJSON_GetFieldAsFloat(cJSON *obj, const char *key)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (cJSON_IsString(item) && item->valuestring)
    {
        return (float)atof(item->valuestring);
    }

    return 0.0f;
}

/**
 * @brief  解析完整的天气 JSON 报文
 * @param  json_str: 原始 JSON 字符串
 * @param  info: 指向存储解析结果的 WeatherInfo 结构体
 * @retval 0: 成功; 1: JSON 格式错误; 2: 关键字段缺失
 */
static uint8_t BSP_ESP8266_WeatherParse(const char *json_str, WeatherInfo *info)
{
    if (json_str == NULL || info == NULL)
    {
        return 1;
    }

    cJSON *root = cJSON_Parse(json_str);
    if (!root)
    {
        PAL_LOG(PAL_LOG_LEVEL_ERROR, "JSON 解析失败");
        return 1;
    }

    cJSON *results = cJSON_GetObjectItemCaseSensitive(root, "results");
    cJSON *first_result = cJSON_GetArrayItem(results, 0);
    if (!first_result)
    {
        cJSON_Delete(root);
        return 2;
    }

    /* 宏 用于拷贝字符串字段 */
#define COPY_STR(obj, key, dst)                                  \
    do                                                           \
    {                                                            \
        cJSON *tmp = cJSON_GetObjectItemCaseSensitive(obj, key); \
        if (cJSON_IsString(tmp) && tmp->valuestring)             \
        {                                                        \
            strncpy(dst, tmp->valuestring, sizeof(dst) - 1);     \
            dst[sizeof(dst) - 1] = '\0';                         \
        }                                                        \
    } while (0)

    /* 解析城市信息 */
    cJSON *loc = cJSON_GetObjectItemCaseSensitive(first_result, "location");
    COPY_STR(loc, "id", info->id);
    COPY_STR(loc, "name", info->name);
    COPY_STR(loc, "country", info->country);
    COPY_STR(loc, "path", info->path);
    COPY_STR(loc, "timezone", info->timezone);
    COPY_STR(loc, "timezone_offset", info->timezone_offset);

    /* 解析每日天气 */
    cJSON *daily_arr = cJSON_GetObjectItemCaseSensitive(first_result, "daily");
    info->daily_count = 0;

    int count = cJSON_GetArraySize(daily_arr);
    if (count > WEATHER_MAX_DAYS)
    {
        count = WEATHER_MAX_DAYS; /* 限制解析天数，防止 WeatherInfo 结构体中的 daily 数组溢出 */
    }

    for (int i = 0; i < count; i++)
    {
        cJSON *item = cJSON_GetArrayItem(daily_arr, i);
        WeatherDay *d = &info->daily[i];

        /* 字符串类 */
        COPY_STR(item, "date", d->date);
        COPY_STR(item, "text_day", d->text_day);
        COPY_STR(item, "text_night", d->text_night);
        COPY_STR(item, "wind_direction", d->wind_direction);

        /* 数值类 调用转换函数 */
        d->code_day = cJSON_GetFieldAsInt(item, "code_day");
        d->code_night = cJSON_GetFieldAsInt(item, "code_night");
        d->high = cJSON_GetFieldAsInt(item, "high");
        d->low = cJSON_GetFieldAsInt(item, "low");
        d->humidity = cJSON_GetFieldAsInt(item, "humidity");
        d->wind_scale = cJSON_GetFieldAsInt(item, "wind_scale");
        d->wind_direction_degree = cJSON_GetFieldAsInt(item, "wind_direction_degree");

        d->rainfall = cJSON_GetFieldAsFloat(item, "rainfall");
        d->precip = cJSON_GetFieldAsFloat(item, "precip");
        d->wind_speed = cJSON_GetFieldAsFloat(item, "wind_speed");

        info->daily_count++;
    }

    /* 解析最后更新时间 */
    COPY_STR(first_result, "last_update", info->last_update);

#undef COPY_STR

    /* 释放 cJSON 占用的堆内存 */
    cJSON_Delete(root);

    return 0; /* 解析成功 */
}

/**
 * @brief  通过串口打印解析后的数值化天气信息
 * @param  info 存储解析结果的 WeatherInfo 结构体
 * @retval None
 */
void BSP_ESP8266_Weather_Print(const WeatherInfo *info)
{
    if (info == NULL)
    {
        PAL_LOG(PAL_LOG_LEVEL_ERROR, "天气信息为空");
        return;
    }

    PAL_LOG(PAL_LOG_LEVEL_INFO, "========= 城市信息 =========");
    PAL_LOG(PAL_LOG_LEVEL_INFO, "城市ID：%s", info->id);
    PAL_LOG(PAL_LOG_LEVEL_INFO, "城市名称：%s", info->name);
    PAL_LOG(PAL_LOG_LEVEL_INFO, "国家：%s", info->country);
    PAL_LOG(PAL_LOG_LEVEL_INFO, "路径：%s", info->path);
    PAL_LOG(PAL_LOG_LEVEL_INFO, "时区：%s", info->timezone);
    PAL_LOG(PAL_LOG_LEVEL_INFO, "时区偏移：%s", info->timezone_offset);
    PAL_LOG(PAL_LOG_LEVEL_INFO, "最后更新时间：%s", info->last_update);

    PAL_LOG(PAL_LOG_LEVEL_INFO, "========= 每日天气 =========");
    for (int i = 0; i < info->daily_count; i++)
    {
        const WeatherDay *d = &info->daily[i];
        PAL_LOG(PAL_LOG_LEVEL_INFO, "---- 第 %d 天 ----", i + 1);
        PAL_LOG(PAL_LOG_LEVEL_INFO, "日期：%s", d->date);
        PAL_LOG(PAL_LOG_LEVEL_INFO, "白天天气：%s (代码：%d)", d->text_day, d->code_day);
        PAL_LOG(PAL_LOG_LEVEL_INFO, "夜间天气：%s (代码：%d)", d->text_night, d->code_night);
        PAL_LOG(PAL_LOG_LEVEL_INFO, "最高温度：%d ℃", d->high);
        PAL_LOG(PAL_LOG_LEVEL_INFO, "最低温度：%d ℃", d->low);
        PAL_LOG(PAL_LOG_LEVEL_INFO, "降雨量：%.2f mm", d->rainfall);
        PAL_LOG(PAL_LOG_LEVEL_INFO, "降水概率：%.2f %%", d->precip);
        PAL_LOG(PAL_LOG_LEVEL_INFO, "风向：%s (角度：%d)", d->wind_direction, d->wind_direction_degree);
        PAL_LOG(PAL_LOG_LEVEL_INFO, "风速：%.2f m/s", d->wind_speed);
        PAL_LOG(PAL_LOG_LEVEL_INFO, "风力等级：%d", d->wind_scale);
        PAL_LOG(PAL_LOG_LEVEL_INFO, "湿度：%d %%", d->humidity);
    }
}

/**
 * @brief  解析 ESP8266 返回的 NTP 时间字符串
 * @param  input: 原始字符串 (例如 "+CIPSNTPTIME:Sun Mar 08 11:02:27 2026")
 * @param  time:  指向存储结果的结构体
 * @retval 0: 成功; 1: 格式不匹配
 */
static uint8_t BSP_ESP8266_Time_Parse(const char *input, NetTime_t *time)
{
    char month_str[10];
    char week_str[10];
    int res;

    /* 查找有效负载起始位置 */
    const char *p = strstr(input, "+CIPSNTPTIME:");
    if (p == NULL)
    {
        return 1;
    }
    p += 13; /* 跳过 "+CIPSNTPTIME:" 前缀 */

    /* 使用 sscanf 提取原始字符串格式: Sun Mar 08 11:02:27 2026 */
    /* %3s 匹配前三个字符作为字符串 */
    res = sscanf(p, "%3s %3s %hhd %hhd:%hhd:%hhd %hd",
                 week_str,
                 month_str,
                 &time->day,
                 &time->hour,
                 &time->minute,
                 &time->second,
                 &time->year);

    if (res != 7)
    {
        return 1;
    }

    /* 转换月份字符串为数字 (1-12) */
    const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    time->month = 0;
    for (int i = 0; i < 12; i++)
    {
        if (strcmp(month_str, months[i]) == 0)
        {
            time->month = (uint8_t)(i + 1);
            break;
        }
    }

    /* 转换星期字符串为数字 (1-7) */
    /* 1:Mon, 2:Tue, 3:Wed, 4:Thu, 5:Fri, 6:Sat, 7:Sun */
    const char *weeks[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    time->week = 0xFF; /* 默认无效值 */
    for (int j = 0; j < 7; j++)
    {
        if (strcmp(week_str, weeks[j]) == 0)
        {
            time->week = (uint8_t)(j + 1);
            break;
        }
    }

    /* 最终合法性检查 */
    if (time->month == 0 || time->week == 0xFF)
    {
        return 1;
    }

    return 0;
}

/**
 * @brief  向 ESP8266 请求并同步当前网络时间
 * @param  time: 指向 NetTime_t 结构体的指针，用于存储解析后的时间数据
 * @retval 0: 成功获取并解析时间; 1: 发生超时或响应格式错误
 */
uint8_t BSP_ESP8266_SyncTime(NetTime_t *time)
{
    char line_buf[128];      /* 用于暂存从环形缓冲区读出的一行数据 */
    static uint16_t idx = 0; /* 静态变量：记录 RingBuffer_ReadLine 的解析进度 */
    uint32_t start_tick;     /* 记录超时计数的起始时间 */

    /* 发送 SNTP 查询指令 */
    BSP_ESP8266_UsartSendString(ESP8266_AT_CIPSNTPTIME);

    start_tick = HAL_GetTick();
    while (HAL_GetTick() - start_tick < 2000)
    {
        if (RingBuffer_ReadLine(&g_uart_rx_fifo, line_buf, sizeof(line_buf), &idx) == 0)
        {
            /* 只要读到时间前缀，立即解析 */
            if (strstr(line_buf, "+CIPSNTPTIME:") != NULL)
            {
                return BSP_ESP8266_Time_Parse(line_buf, time);
            }

            /* 如果读到 ERROR，说明 NTP 还没同步成功，提前退出 */
            if (strstr(line_buf, "ERROR") != NULL)
            {
                return 1;
            }
        }
    }
    return 1; /* 超时 */
}

/**
 * @brief  格式化并打印解析后的网络时间
 * @param  time: 指向已填充数据的 NetTime_t 结构体
 * @retval 无
 */
void BSP_ESP8266_Time_Print(const NetTime_t *time)
{
    /* 星期数字对应字符串映射表 */
    const char *week_str[] = {"Invalid", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    uint8_t w_idx = time->week;

    if (time == NULL)
    {
        PAL_LOG(PAL_LOG_LEVEL_ERROR, "时间结构体指针为空");
        return;
    }

    /* 基础合法性校验：防止打印出 00-00 这种错误数据 */
    if (w_idx < 1 || w_idx > 7)
    {
        w_idx = 0; /* 指向 "Invalid" */
    }

    PAL_LOG(PAL_LOG_LEVEL_INFO, "========= 网络同步时间 =========");

    /* 格式化输出示例：2026-03-08 11:02:27 (Sun) */
    PAL_LOG(PAL_LOG_LEVEL_INFO, "当前时间：%04d-%02d-%02d %02d:%02d:%02d (%s)",
            time->year,
            time->month,
            time->day,
            time->hour,
            time->minute,
            time->second,
            week_str[w_idx]);

    PAL_LOG(PAL_LOG_LEVEL_INFO, "================================");
}
