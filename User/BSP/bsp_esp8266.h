#ifndef __BSP_ESP8266_H
#define __BSP_ESP8266_H

#include "usart.h"

#define DEBUG_UART huart1   /* 调试串口 */
#define ESP8266_UART huart2 /* ESP8266 连接的串口 */

/* 天气数据 JSON 原始字符串缓冲区大小 */
#define WEATHER_JSON_BUFFER_SIZE 1024

/* ---------------- 天气解析相关配置 ---------------- */
#define WEATHER_MAX_DAYS 3 /* 可获取的天气天数 */

/* ---------------- WiFi 账号信息配置 ---------------- */
#define WIFI_SSID "WIFI_SSID" /* 目标 WiFi 的 SSID 名称 */
#define WIFI_PASS "WIFI_PASS" /* 目标 WiFi 的连接密码 */

/* ---------------- 心知天气 API 配置 ---------------- */
#define SENIVERSE_API_PRIVATE_KEY "SCPHbIqbzgs8Nlqdr" /* 心知天气的用户私钥 */
#define SENIVERSE_LOCATION "Nanjing"                  /* 查询天气的城市地点 */

/* ---------------- 常用基础 AT 指令 ---------------- */
#define ESP8266_AT "AT\r\n"         /* 握手指令：检查模块是否正常响应 */
#define ESP8266_AT_ATE0 "ATE0\r\n"  /* 关闭回显：执行命令时不返回输入的命令字符串 */
#define ESP8266_AT_RST "AT+RST\r\n" /* 软件复位：模块重新启动 */
#define ESP8266_AT_GMR "AT+GMR\r\n" /* 版本查询：获取固件版本信息 */

/* ---------------- WiFi 模式与连接指令 ---------------- */
/* 设置工作模式：1-Station, 2-AP, 3-Station+AP */
#define ESP8266_AT_CWMODE(mode) "AT+CWMODE=" mode "\r\n"

/* 连接指定 WiFi：需传入 SSID 和密码 */
#define ESP8266_AT_CWJAP(ssid, password) "AT+CWJAP=\"" ssid "\",\"" password "\"\r\n"

/* 断开当前 WiFi 连接 */
#define ESP8266_AT_CWQAP "AT+CWQAP\r\n"

/* ---------------- 网络连接与透传相关 ---------------- */
/* 建立 TCP 链路：连接心知天气服务器的 80 端口 */
#define ESP8266_AT_CIPSTART_TCP "AT+CIPSTART=\"TCP\",\"api.seniverse.com\",80\r\n"

/* 开启单路连接模式 (CIPMUX=0) */
#define ESP8266_AT_CIPMUX_0 "AT+CIPMUX=0\r\n"

/* 开启透传模式：进入透明传输状态 */
#define ESP8266_AT_CIPMODE_1 "AT+CIPMODE=1\r\n"

/* 准备发送数据：进入透传后开始接收数据流 */
#define ESP8266_AT_CIPSEND "AT+CIPSEND\r\n"

/* ---------------- HTTP 业务请求报文 ---------------- */
/* * 心知天气 v3 接口请求字符串
 * 包含：私钥、地点、语言(en)、单位(c)、起始天数(0)、天数(3)
 * 注意：Connection: close 表示获取完数据后服务器主动断开连接，防止占用
 */
#define ESP8266_AT_HTTP_GET_SENIVERSE                                                                                                             \
    "GET /v3/weather/daily.json?key=" SENIVERSE_API_PRIVATE_KEY "&location=" SENIVERSE_LOCATION "&language=en&unit=c&start=0&days=3 HTTP/1.1\r\n" \
    "Host: api.seniverse.com\r\n"                                                                                                                 \
    "Connection: close\r\n\r\n"

/* ---------------- NTP 网络时间相关指令 ---------------- */

/* * 配置 SNTP 服务器
 * 参数说明:
 * 1: 开启 SNTP
 * 8: 东八区 (北京时间)
 * "ntp1.aliyun.com": 阿里云 NTP 服务器地址
 */
#define ESP8266_AT_CIPSNTPCFG "AT+CIPSNTPCFG=1,8,\"ntp1.aliyun.com\"\r\n"

/* 查询当前经过 SNTP 同步后的网络时间 */
#define ESP8266_AT_CIPSNTPTIME "AT+CIPSNTPTIME?\r\n"

/**
 * @brief  天气数据结构体
 */
typedef struct
{
    char date[16];                 /* 日期 */
    char text_day[32];             /* 白天天气描述 */
    char text_night[32];           /* 夜间天气描述 */
    char code_day[8];              /* 白天天气代码 */
    char code_night[8];            /* 夜间天气代码 */
    char high[8];                  /* 最高温度 */
    char low[8];                   /* 最低温度 */
    char rainfall[16];             /* 降雨量 */
    char precip[16];               /* 降水概率 */
    char wind_direction[8];        /* 风向 */
    char wind_direction_degree[8]; /* 风向角度 */
    char wind_speed[16];           /* 风速 */
    char wind_scale[8];            /* 风力等级 */
    char humidity[8];              /* 湿度 */
} WeatherDay;

/**
 * @brief  完整的天气信息结构体，包括城市信息和 daily 数据
 */
typedef struct
{
    char id[32];                        /* 城市ID */
    char name[32];                      /* 城市名称 */
    char country[8];                    /* 国家 */
    char path[64];                      /* 路径 */
    char timezone[32];                  /* 时区 */
    char timezone_offset[8];            /* 时区偏移 */
    WeatherDay daily[WEATHER_MAX_DAYS]; /* 存储每天的天气数据 */
    int daily_count;                    /* 实际解析的天数 */
    char last_update[32];               /* 最后更新时间 */
} WeatherInfo;

/**
 * @brief  网络时间信息结构体
 * @note   用于存储从 ESP8266 NTP 服务器解析后的日期与时间数据
 */
typedef struct
{
    uint16_t year;  /* 年份：如 2026 */
    uint8_t month;  /* 月份：1 - 12 */
    uint8_t day;    /* 日期：1 - 31 */
    uint8_t hour;   /* 小时：0 - 23 (24小时制) */
    uint8_t minute; /* 分钟：0 - 59 */
    uint8_t second; /* 秒钟：0 - 59 */
    uint8_t week;   /* 星期：1 - 7 (1:Mon, 2:Tue, 3:Wed, 4:Thu, 5:Fri, 6:Sat, 7:Sun) */
} NetTime_t;

/* 函数声明 */
uint8_t BSP_ESP8266_Init(void);
uint8_t BSP_ESP8266_SendCmd(char *cmd, char *ack, uint32_t timeout_ms);
uint8_t BSP_ESP8266_WeatherUpdate(WeatherInfo *info);
void BSP_ESP8266_Weather_Print(const WeatherInfo *info);
uint8_t BSP_ESP8266_SyncTime(NetTime_t *time);
void BSP_ESP8266_Time_Print(const NetTime_t *time);

extern WeatherInfo g_weather_info; /* 解析后的最终天气数据 */
extern NetTime_t g_net_time;       /* 解析后的时间数据 */

#endif
