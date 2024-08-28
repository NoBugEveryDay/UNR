#include <stddef.h>

/**
 * @brief Reallocate `new_size` bytes of memory.
 *        Copy the contents of `old_ptr` to the new memory location.
 *        Set the remaining bytes to 0.
 *        Free the old memory location.
 * @details On some system, there are some problems with native `realloc`,
 *          Thus we define this function.
 */

#ifdef URU_ENABLE_ASSERT
    void* uru_malloc_debug(size_t size, const char *file_name, int line_num, const char* func_name);
    void* uru_calloc_debug(size_t nmemb, size_t size, const char *file_name, int line_num, const char* func_name);
    void* uru_realloc_debug(void* old_ptr, size_t old_size, size_t new_size, const char *file_name, int line_num, const char* func_name);
    #define uru_malloc(size) uru_malloc_debug(size, __FILE__, __LINE__, __func__)
    #define uru_calloc(nmemb, size) uru_calloc_debug(nmemb, size, __FILE__, __LINE__, __func__)
    #define uru_realloc(old_ptr, old_size, new_size) uru_realloc_debug(old_ptr, old_size, new_size, __FILE__, __LINE__, __func__)
#else
    void* uru_malloc_check(size_t size);
    void* uru_calloc_check(size_t nmemb, size_t size);
    void* uru_realloc_check(void* old_ptr, size_t old_size, size_t new_size);
    #define uru_malloc(size) uru_malloc_check(size)
    #define uru_calloc(nmemb, size) uru_calloc_check(nmemb, size)
    #define uru_realloc(old_ptr, old_size, new_size) uru_realloc_check(old_ptr, old_size, new_size)
#endif