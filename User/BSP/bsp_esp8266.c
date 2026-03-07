#include "bsp_esp8266.h"
#include "bsp_dwt.h"

#include "ring_buffer.h"
#include "pal_log.h"

#include "cJSON.h"

#include <string.h>

char weather_json_buf[WEATHER_JSON_BUFFER_SIZE]; /* 提取出的完整的天气 JSON 字符串 */
WeatherInfo g_weather_info = {0};                /* 解析后的最终天气数据 */

/* ESP8266 接收状态机变量 */
static uint16_t g_json_idx = 0;    /* 当前 JSON 数据在缓冲区中的存储偏移量 */
static uint8_t g_json_started = 0; /* 标志位：是否已匹配到起始大括号 '{' */
static int16_t g_brace_cnt = 0;    /* 大括号平衡计数器，用于判断 JSON 嵌套是否结束 */

/* 内部函数 */
static void BSP_ESP8266_UsartSendString(char *_str);
static void BSP_ESP8266_WeatherReset(void);
static uint8_t BSP_ESP8266_WeatherRead(RingBuffer_t *rb, char *pDest, uint16_t maxLen);
static uint8_t BSP_ESP8266_WeatherParse(const char *json_str, WeatherInfo *info);

/**
 * @brief  ESP8266 初始化配置流程
 * @note   包含：测试、关闭回显、重启、模式设置、连接 WiFi、建立 TCP、进入透传
 * @retval 0: 配置成功; 1: 某个环节配置失败
 */
uint8_t BSP_ESP8266_Init(void)
{
    /* 测试模块通信 */
    if (BSP_ESP8266_SendCmd(ESP8266_AT, "OK", 3000) == 1)
    {
        PAL_LOG(PAL_LOG_LEVEL_ERROR, "AT 通信测试失败\r\n");
        return 1;
    }

    /* 关闭命令回显 (ATE0) */
    if (BSP_ESP8266_SendCmd(ESP8266_AT_ATE0, "OK", 3000) == 1)
    {
        PAL_LOG(PAL_LOG_LEVEL_ERROR, "ATE0 设置失败\r\n");
        return 1;
    }

    /* 软件复位模块 */
    if (BSP_ESP8266_SendCmd(ESP8266_AT_RST, "ready", 5000) == 1)
    {
        PAL_LOG(PAL_LOG_LEVEL_ERROR, "模块复位失败\r\n");
        return 1;
    }

    /* 设置为 Station 模式 */
    if (BSP_ESP8266_SendCmd(ESP8266_AT_CWMODE("1"), "OK", 3000) == 1)
    {
        PAL_LOG(PAL_LOG_LEVEL_ERROR, "模式设置失败\r\n");
        return 1;
    }

    /* 连接 WiFi (超时时间较长) */
    if (BSP_ESP8266_SendCmd(ESP8266_AT_CWJAP(WIFI_SSID,WIFI_PASS), "OK", 20000) == 1)
    {
        PAL_LOG(PAL_LOG_LEVEL_ERROR, "WiFi 连接失败\r\n");
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
    static uint16_t my_idx = 0;          /* 静态变量：记录 LineReader 的解析进度 */
    uint32_t start_tick = HAL_GetTick(); /* 记录开始时间点 */

    /* 通过串口发送指令 */
    BSP_ESP8266_UsartSendString(cmd);

    /* 在超时时间内循环检查缓冲区 */
    while (HAL_GetTick() - start_tick < timeout_ms)
    {
        /* 调用行读取函数，若返回 1 表示成功解析出一行以 \n 结尾的字符串 */
        if (RingBuffer_ReadLine(&g_uart_rx_fifo, line_buf, sizeof(line_buf), &my_idx) == 0)
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
        BSP_DWT_DelayMs(5);
    }

    /* 退出逻辑：发送退出透传序列 "+++" */
    /* 注意：退出透传通常需要前后各 1 秒的静默期 */
    BSP_DWT_DelayMs(1000);
    BSP_ESP8266_UsartSendString("+++");
    BSP_DWT_DelayMs(1000);

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

    /* 解析 JSON 字符串 */
    cJSON *root = cJSON_Parse(json_str);
    if (!root)
    {
        PAL_LOG(PAL_LOG_LEVEL_ERROR, "JSON 解析失败 : %s\n", cJSON_GetErrorPtr());
        return 1;
    }

    /* 获取 results 数组 */
    cJSON *results = cJSON_GetObjectItemCaseSensitive(root, "results");
    if (!cJSON_IsArray(results) || cJSON_GetArraySize(results) == 0)
    {
        cJSON_Delete(root);
        return 2;
    }

    /* 获取第一个结果对象 (results[0]) */
    cJSON *first_result = cJSON_GetArrayItem(results, 0);
    if (!cJSON_IsObject(first_result))
    {
        cJSON_Delete(root);
        return 2;
    }

    /* --- 定义通用的字符串字段安全拷贝宏 --- */
    /* 使用 sizeof(dst)-1 确保留出结束符空间，并手动补 \0 */
#define SAFE_COPY_FIELD(obj, key, dst)                           \
    do                                                           \
    {                                                            \
        cJSON *tmp = cJSON_GetObjectItemCaseSensitive(obj, key); \
        if (cJSON_IsString(tmp) && (tmp->valuestring != NULL))   \
        {                                                        \
            strncpy(dst, tmp->valuestring, sizeof(dst) - 1);     \
            dst[sizeof(dst) - 1] = '\0';                         \
        }                                                        \
    } while (0)

    /* ---------------- 解析城市信息 location ---------------- */
    cJSON *location = cJSON_GetObjectItemCaseSensitive(first_result, "location");
    if (cJSON_IsObject(location))
    {
        SAFE_COPY_FIELD(location, "id", info->id);
        SAFE_COPY_FIELD(location, "name", info->name);
        SAFE_COPY_FIELD(location, "country", info->country);
        SAFE_COPY_FIELD(location, "path", info->path);
        SAFE_COPY_FIELD(location, "timezone", info->timezone);
        SAFE_COPY_FIELD(location, "timezone_offset", info->timezone_offset);
    }

    /* ---------------- 解析 daily 数组 ---------------- */
    cJSON *daily_array = cJSON_GetObjectItemCaseSensitive(first_result, "daily");
    info->daily_count = 0; /* 初始化计数器 */

    if (cJSON_IsArray(daily_array))
    {
        int count = cJSON_GetArraySize(daily_array);
        /* 限制解析天数，防止 WeatherInfo 结构体中的 daily 数组溢出 */
        if (count > WEATHER_MAX_DAYS)
        {
            count = WEATHER_MAX_DAYS;
        }

        for (int i = 0; i < count; i++)
        {
            cJSON *day_item = cJSON_GetArrayItem(daily_array, i);
            if (!cJSON_IsObject(day_item))
            {
                continue;
            }

            WeatherDay *d = &info->daily[i];
            memset(d, 0, sizeof(WeatherDay)); /* 写入前清空当前天的数据 */

            SAFE_COPY_FIELD(day_item, "date", d->date);
            SAFE_COPY_FIELD(day_item, "text_day", d->text_day);
            SAFE_COPY_FIELD(day_item, "text_night", d->text_night);
            SAFE_COPY_FIELD(day_item, "code_day", d->code_day);
            SAFE_COPY_FIELD(day_item, "code_night", d->code_night);
            SAFE_COPY_FIELD(day_item, "high", d->high);
            SAFE_COPY_FIELD(day_item, "low", d->low);
            SAFE_COPY_FIELD(day_item, "rainfall", d->rainfall);
            SAFE_COPY_FIELD(day_item, "precip", d->precip);
            SAFE_COPY_FIELD(day_item, "wind_direction", d->wind_direction);
            SAFE_COPY_FIELD(day_item, "wind_direction_degree", d->wind_direction_degree);
            SAFE_COPY_FIELD(day_item, "wind_speed", d->wind_speed);
            SAFE_COPY_FIELD(day_item, "wind_scale", d->wind_scale);
            SAFE_COPY_FIELD(day_item, "humidity", d->humidity);

            info->daily_count++;
        }
    }

    /* ---------------- 解析最后更新时间 last_update ---------------- */
    SAFE_COPY_FIELD(first_result, "last_update", info->last_update);

#undef SAFE_COPY_FIELD /* 及时取消宏定义，防止污染其他函数 */

    /* 释放 cJSON 占用的堆内存 */
    cJSON_Delete(root);

    return 0; /* 解析成功 */
}

/**
 * @brief  通过串口打印解析后的天气信息 JSON 对象
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

    PAL_LOG(PAL_LOG_LEVEL_INFO, "========= 城市信息 =========\r\n");
    PAL_LOG(PAL_LOG_LEVEL_INFO, "城市ID：%s", info->id);
    PAL_LOG(PAL_LOG_LEVEL_INFO, "城市名称：%s", info->name);
    PAL_LOG(PAL_LOG_LEVEL_INFO, "国家：%s", info->country);
    PAL_LOG(PAL_LOG_LEVEL_INFO, "路径：%s", info->path);
    PAL_LOG(PAL_LOG_LEVEL_INFO, "时区：%s", info->timezone);
    PAL_LOG(PAL_LOG_LEVEL_INFO, "时区偏移：%s", info->timezone_offset);
    PAL_LOG(PAL_LOG_LEVEL_INFO, "最后更新时间：%s", info->last_update);

    PAL_LOG(PAL_LOG_LEVEL_INFO, "========= 每日天气 =========\r\n");
    for (int i = 0; i < info->daily_count; i++)
    {
        const WeatherDay *d = &info->daily[i];
        PAL_LOG(PAL_LOG_LEVEL_INFO, "---- 第 %d 天 ----", i + 1);
        PAL_LOG(PAL_LOG_LEVEL_INFO, "日期：%s", d->date);
        PAL_LOG(PAL_LOG_LEVEL_INFO, "白天天气：%s (代码：%s)", d->text_day, d->code_day);
        PAL_LOG(PAL_LOG_LEVEL_INFO, "夜间天气：%s (代码：%s)", d->text_night, d->code_night);
        PAL_LOG(PAL_LOG_LEVEL_INFO, "最高温度：%s ℃", d->high);
        PAL_LOG(PAL_LOG_LEVEL_INFO, "最低温度：%s ℃", d->low);
        PAL_LOG(PAL_LOG_LEVEL_INFO, "降雨量：%s mm", d->rainfall);
        PAL_LOG(PAL_LOG_LEVEL_INFO, "降水概率：%s %%", d->precip);
        PAL_LOG(PAL_LOG_LEVEL_INFO, "风向：%s (角度：%s)", d->wind_direction, d->wind_direction_degree);
        PAL_LOG(PAL_LOG_LEVEL_INFO, "风速：%s m/s", d->wind_speed);
        PAL_LOG(PAL_LOG_LEVEL_INFO, "风力等级：%s", d->wind_scale);
        PAL_LOG(PAL_LOG_LEVEL_INFO, "湿度：%s %%", d->humidity);
        PAL_LOG(PAL_LOG_LEVEL_INFO, "");
    }
}
