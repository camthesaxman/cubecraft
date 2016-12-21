#include "global.h"

#ifndef NDEBUG

//Restore the real versions of these functions
#undef malloc
#undef realloc
#undef memalign

static void enter_text_mode(void)
{
    GXRModeObj *videoMode;
    void *framebuffer;
    
    VIDEO_Init();
    videoMode = VIDEO_GetPreferredMode(NULL);
    framebuffer = MEM_K0_TO_K1(SYS_AllocateFramebuffer(videoMode));
    CON_InitEx(videoMode, 20, 30, videoMode->fbWidth - 40, videoMode->xfbHeight - 60);
    VIDEO_Configure(videoMode);
    VIDEO_SetNextFramebuffer(framebuffer);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
}

static void pause(void)
{
    puts("Press the A button to exit...");
    while (1)
    {
        PAD_ScanPads();
        if (PAD_ButtonsDown(0) & PAD_BUTTON_A)
            exit(0);
    }
}

void assert_fail(const char *expression, const char *function, const char *filename, int line)
{
    enter_text_mode();
    puts("***ERROR: ASSERTION FAILED***");
    printf("expression (%s)\n", expression);
    printf("function: %s(), file: %s, line: %i\n", function, filename, line);
    pause();
}

void *checked_malloc(size_t size, const char *function, const char *filename, int line)
{
    void *ptr = malloc(size);
    
    if (ptr == NULL)
    {
        enter_text_mode();
        puts("***ERROR: MEMORY ALLOCATION FAILURE***");
        printf("malloc() attempted to allocate %u bytes\n", size);
        printf("function: %s(), file: %s, line: %i\n", function, filename, line);
        pause();
    }
    return ptr;
}

void *checked_realloc(void *ptr, size_t new_size, const char *function, const char *filename, int line)
{
    void *newPtr = realloc(ptr, new_size);
    
    if (newPtr == NULL)
    {
        enter_text_mode();
        puts("***ERROR: MEMORY ALLOCATION FAILURE***");
        printf("realloc() attempted to resize 0x%08X to %u bytes\n", (unsigned int)ptr, new_size);
        printf("function: %s(), file: %s, line: %i\n", function, filename, line);
        pause();
    }
    return newPtr;
}

void *checked_memalign(size_t alignment, size_t size, const char *function, const char *filename, int line)
{
    void *ptr = memalign(alignment, size);
    
    if (ptr == NULL)
    {
        enter_text_mode();
        puts("***ERROR: MEMORY ALLOCATION FAILURE***");
        printf("memalign() attempted to allocate %u bytes, aligned at %u\n", size, alignment);
        printf("function: %s(), file: %s, line: %i\n", function, filename, line);
        pause();
    }
    return ptr;
}

#endif