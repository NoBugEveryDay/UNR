/**
 * @file dev.h
 * @author Guangnan Feng (nobugday@gamil.com)
 * @brief The things defined in this header file are used when developing the program.
 * @version 1.0
 * @date 2023-12-28
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <uru/type/uru_status.h>
#include <stdio.h>
#include <stdlib.h>

#define URU_NOT_IMPLEMENTED_EXIT()                                                     \
    {                                                                                  \
        printf("NOT implemented yet! @ %s[%d]:%s().\n", __FILE__, __LINE__, __func__); \
        exit(URU_NOT_IMPLEMENT_EXIT);                                                  \
    }

#ifdef URU_ENABLE_ASSERT
    #define URU_ASSERT(condition)                                                               \
        {                                                                                       \
            if (__builtin_expect(!(condition), 0)) {                                            \
                printf("ERROR @ %s[%d]:%s() - %s\n", __FILE__, __LINE__, __func__, #condition); \
                fflush(stdout);                                                                 \
                exit(URU_ASSERT_EXIT);                                                          \
            }                                                                                   \
        }
    #define URU_ASSERT_REASON(condition, ...)                                   \
        {                                                                       \
            if (__builtin_expect(!(condition), 0)) {                            \
                printf("ERROR @ %s[%d]:%s() - ", __FILE__, __LINE__, __func__); \
                printf(__VA_ARGS__);                                            \
                printf("\n");                                                   \
                fflush(stdout);                                                 \
                exit(URU_ASSERT_EXIT);                                          \
            }                                                                   \
        }
#else
    #define URU_ASSERT(condition) {condition;}
    #define URU_ASSERT_REASON(condition, ...) {condition;}
#endif

#ifdef URU_ENABLE_LOG
    #define URU_LOG(fmt, ...)         \
        {                             \
            printf("[UNR INFO] ");  \
            printf(fmt, __VA_ARGS__); \
            fflush(stdout);           \
        }
#else
    #define URU_LOG(fmt, ...)
#endif
