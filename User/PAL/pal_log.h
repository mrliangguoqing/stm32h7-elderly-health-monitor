#ifndef __LOG_H_
#define __LOG_H_

#include <stdio.h>

/**
 * @brief 日志级别枚举定义
 */
enum PAL_LOG_LEVEL
{
    PAL_LOG_LEVEL_OFF = 0,   /* 关闭日志输出 */
    PAL_LOG_LEVEL_ERROR = 1, /* 错误日志 */
    PAL_LOG_LEVEL_INFO = 2,  /* 信息日志 */
    PAL_LOG_LEVEL_DEBUG = 3  /* 调试日志 */
};

#define PAL_LOG_DEBUG /* 定义日志级别为调试级别，启用调试日志输出 */

/* 默认日志级别 */
#ifndef PAL_LOG_LEVEL
#define PAL_LOG_LEVEL PAL_LOG_LEVEL_DEBUG /* 如果没有显式定义日志级别，默认为调试级别 */
#endif

/* 日志颜色宏定义 */
#define PAL_COLOR_RESET "\033[0m"    /* 重置颜色，用于恢复默认颜色 */
#define PAL_COLOR_ERROR "\033[0;31m" /* 红色，用于错误日志 */
#define PAL_COLOR_INFO "\033[0;32m"  /* 绿色，用于信息日志 */
#define PAL_COLOR_DEBUG "\033[0;34m" /* 蓝色，用于调试日志 */

/* 日志输出宏，只有在日志级别满足条件时才输出 */
#ifdef PAL_LOG_DEBUG
    #define PAL_LOG(level, format, ...) \
        do { \
            if (level <= PAL_LOG_LEVEL) { \
                const char *level_str = ""; /* 存储日志级别的字符串 */\
                const char *color = PAL_COLOR_RESET; /* 日志颜色的默认值为重置颜色 */ \
                switch (level) { \
                    case PAL_LOG_LEVEL_ERROR: level_str = "[E]"; color = PAL_COLOR_ERROR; break; /* 错误日志 */ \
                    case PAL_LOG_LEVEL_INFO: level_str = "[I]"; color = PAL_COLOR_INFO; break; /* 信息日志 */ \
                    case PAL_LOG_LEVEL_DEBUG: level_str = "[D]"; color = PAL_COLOR_DEBUG; break; /* 调试日志 */ \
                    default: break; \
                } \
                printf("%s%s: ", color, level_str); /* 打印日志级别及其颜色 */ \
                printf(format, ##__VA_ARGS__); /* 打印日志内容 */ \
                printf("%s\r\n",PAL_COLOR_RESET); /* 输出完日志后重置颜色 */ \
            } \
        } while (0)
#else
    /* 如果没有定义 PAL_LOG_DEBUG，LOG 宏什么都不做 */
    #define LOG(level, format, ...) /* 不输出任何日志 */
#endif

/* 函数声明 */
int fputc(int ch, FILE *f);

#endif /* __LOG_H_ */
