#pragma once

/* Shared library constructor */
#define URU_F_CTOR __attribute__((constructor))

/* Shared library destructor */
#define URU_F_DTOR __attribute__((destructor))

/* Paste two expanded tokens. Compiler need this */
#define __URU_TOKENPASTE_HELPER(x, y)     x ## y

/* Paste two expanded tokens */
#define URU_PP_TOKENPASTE(x, y)           __URU_TOKENPASTE_HELPER(x, y)

/* Unique value generator */
#ifdef __COUNTER__
    #define URU_PP_UNIQUE_ID __COUNTER__
#else
    #define URU_PP_UNIQUE_ID __LINE__
#endif

/* Creating unique identifiers, used for macros */
#define URU_PP_APPEND_UNIQUE_ID(x) URU_PP_TOKENPASTE(x, URU_PP_UNIQUE_ID)

/*
 * Define code which runs at global constructor phase
 */
#define URU_STATIC_INIT \
    static void URU_F_CTOR URU_PP_APPEND_UNIQUE_ID(URU_initializer_ctor)()

/*
 * Define code which runs at global destructor phase
 */
#define URU_STATIC_CLEANUP \
    static void URU_F_DTOR URU_PP_APPEND_UNIQUE_ID(URU_initializer_dtor)()
