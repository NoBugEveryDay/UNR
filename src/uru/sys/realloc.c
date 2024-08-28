#include <uru/sys/realloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef URU_ENABLE_ASSERT
void* uru_realloc_debug(void* old_ptr, size_t old_size, size_t new_size, const char* file_name, int line_num, const char* func_name)
#else
void* uru_realloc_check(void* old_ptr, size_t old_size, size_t new_size)
#endif
{
#ifdef CUSTOMIZE_REALLOC
    if (new_size <= old_size) {
        return old_ptr;
    }
    void* new_prt = malloc(new_size);
    if (new_prt == NULL) {
#ifdef URU_ENABLE_ASSERT
        printf("Malloc memory failed @ %s[%d]:%s()\n", file_name, line_num, func_name);
#else
        printf("Malloc memory failed\n");
#endif
        exit(1);
    }
    if (old_ptr) {
        memcpy(new_prt, old_ptr, old_size);
        free(old_ptr);
    }
    memset(new_prt + old_size, 0, new_size - old_size);
    return new_prt;
#else
    void* new_prt = realloc(old_ptr, new_size);
    if (new_prt == NULL) {
#ifdef URU_ENABLE_ASSERT
        printf("Malloc memory failed @ %s[%d]:%s()\n", file_name, line_num, func_name);
#else
        printf("Malloc memory failed\n");
#endif
        exit(1);
    }
    memset(new_prt + old_size, 0, new_size - old_size);
    return new_prt;
#endif
}

#ifdef URU_ENABLE_ASSERT
void* uru_malloc_debug(size_t size, const char* file_name, int line_num, const char* func_name)
#else
void* uru_malloc_check(size_t size)
#endif
{
    void* ret = malloc(size);
    if (ret == NULL) {
#ifdef URU_ENABLE_ASSERT
        printf("Malloc memory failed @ %s[%d]:%s()\n", file_name, line_num, func_name);
#else
        printf("Malloc memory failed\n");
#endif
        exit(1);
    }
    return ret;
}

#ifdef URU_ENABLE_ASSERT
void* uru_calloc_debug(size_t nmemb, size_t size, const char* file_name, int line_num, const char* func_name)
#else
void* uru_calloc_check(size_t nmemb, size_t size)
#endif
{
    void* ret = calloc(nmemb, size);
    if (ret == NULL) {
#ifdef URU_ENABLE_ASSERT
        printf("Malloc memory failed @ %s[%d]:%s()\n", file_name, line_num, func_name);
#else
        printf("Malloc memory failed\n");
#endif
        exit(1);
    }
    return ret;
}