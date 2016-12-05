#ifndef GUARD_DEBUG_H
#define GUARD_DEBUG_H

#ifdef NDEBUG
#define assert(expression)
#else
#define malloc(size) checked_malloc(size, __FUNCTION__, __FILE__, __LINE__)
#define realloc(ptr, new_size) checked_realloc(ptr, new_size, __FUNCTION__, __FILE__, __LINE__)
#define memalign(alignment, size) checked_memalign(alignment, size, __FUNCTION__, __FILE__, __LINE__)
#define assert(expression) ((void)((expression) || (assert_fail(#expression, __FUNCTION__, __FILE__, __LINE__), 0)))
void assert_fail(const char *expression, const char *function, const char *filename, int line);
void *checked_malloc(size_t size, const char *function, const char *filename, int line);
void *checked_realloc(void *ptr, size_t new_size, const char *function, const char *filename, int line);
void *checked_memalign(size_t alignment, size_t size, const char *function, const char *filename, int line);
#endif //NDEBUG

#endif //GUARD_DEBUG_H
