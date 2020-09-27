#ifndef PISCRN_H
#define PISCRN_H

#include <stdint.h>
#include <stddef.h>

typedef enum {
    PISCRN_SUCCESS,
    PISCRN_UNABLE_TO_OPEN_DISPLAY,
    PISCRN_UNABLE_TO_GET_DISPLAY_INFO,
    PISCRN_OOM,
    PISCRN_DRIVER_ERROR,
    PISCRN_LIBPNG_ERROR,
    PISCRN_UNABLE_TO_CREATE_FILE,
} piscrn_error_code;

typedef enum {
    PISCRN_OUTPUT_STDOUT,
    PISCRN_OUTPUT_FILE,
    PISCRN_OUTPUT_MEMORY
} piscrn_output_choice;

typedef struct {
    piscrn_output_choice choice;
    union {
        struct {
            const char** memoryOut;
            size_t* sizeOut;
        };
        const char* fileName;
    };
} piscrn_output_descriptor;

typedef struct {
    piscrn_output_descriptor output;
    uint32_t displayNumber;
    int compression;
    int32_t requestedHeight;
    int32_t requestedWidth;
    int delay;
} piscrn_screenshot_params;

const piscrn_screenshot_params piscrn_default_params;

piscrn_error_code
piscrn_take_screenshot(piscrn_screenshot_params const* params);

#endif