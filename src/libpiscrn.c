// Adapted from https://github.com/AndrewFromMelbourne/raspi2png, Copyright (c)
// 2014 Andrew Duncan, MIT License

#define _GNU_SOURCE

#include "libpiscrn.h"

#include <math.h>
#include <png.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>

#include "bcm_host.h"

//-----------------------------------------------------------------------

#ifndef ALIGN_TO_16
#define ALIGN_TO_16(x) ((x + 15) & ~15)
#endif

const piscrn_screenshot_params piscrn_default_params = {
    .output = {.choice = PISCRN_OUTPUT_FILE, .fileName = "snapshot.png"},
    .displayNumber = 0,
    .compression = Z_DEFAULT_COMPRESSION,
    .requestedHeight = 0,
    .requestedWidth = 0,
    .delay = 0};

//-----------------------------------------------------------------------

typedef struct {
  char *buffer;
  size_t allocSize;
  size_t imgSize;
} piscrn_png_mem_ptr;

// Used as a libpng writer callback
// Writes png data into a dynamically allocated buffer
void piscrn_png_memory_write(png_structp png_ptr, png_bytep data,
                             png_size_t length) {
  piscrn_png_mem_ptr *p = (piscrn_png_mem_ptr *)png_get_io_ptr(png_ptr);

  if ((p->imgSize + length) > p->allocSize) {
    p->allocSize *= 2;
    p->buffer = realloc(p->buffer, p->allocSize);

    if (!p->buffer)
      png_error(png_ptr, "Write Error");
  }

  memcpy(p->buffer + p->imgSize, data, length);
  p->imgSize += length;
}

piscrn_error_code
piscrn_take_screenshot(piscrn_screenshot_params const *params) {

  int8_t dmxBytesPerPixel = 4;

  bcm_host_init();

  if (params->delay) {
    sleep(params->delay);
  }

  //-------------------------------------------------------------------

  DISPMANX_DISPLAY_HANDLE_T displayHandle =
      vc_dispmanx_display_open(params->displayNumber);

  if (displayHandle == 0) {
    return PISCRN_UNABLE_TO_OPEN_DISPLAY;
  }

  DISPMANX_MODEINFO_T modeInfo;
  int result = vc_dispmanx_display_get_info(displayHandle, &modeInfo);

  if (result != 0) {
    return PISCRN_UNABLE_TO_GET_DISPLAY_INFO;
  }

  int32_t pngWidth = modeInfo.width;
  int32_t pngHeight = modeInfo.height;

  if (params->requestedWidth > 0) {
    pngWidth = params->requestedWidth;

    if (params->requestedHeight == 0) {
      double numerator = modeInfo.height * params->requestedWidth;
      double denominator = modeInfo.width;

      pngHeight = (int32_t)ceil(numerator / denominator);
    }
  }

  if (params->requestedHeight > 0) {
    pngHeight = params->requestedHeight;

    if (params->requestedWidth == 0) {
      double numerator = modeInfo.width * params->requestedHeight;
      double denominator = modeInfo.height;

      pngWidth = (int32_t)ceil(numerator / denominator);
    }
  }

  //-------------------------------------------------------------------
  // only need to check low bit of modeInfo.transform (value of 1 or 3).
  // If the display is rotated either 90 or 270 degrees (value 1 or 3)
  // the width and height need to be transposed.

  int32_t dmxWidth = pngWidth;
  int32_t dmxHeight = pngHeight;

  if (modeInfo.transform & 1) {
    dmxWidth = pngHeight;
    dmxHeight = pngWidth;
  }

  int32_t dmxPitch = dmxBytesPerPixel * ALIGN_TO_16(dmxWidth);

  void *dmxImagePtr = malloc(dmxPitch * dmxHeight);

  if (dmxImagePtr == NULL) {
    return PISCRN_OOM;
  }

  //-------------------------------------------------------------------

  uint32_t vcImagePtr = 0;
  DISPMANX_RESOURCE_HANDLE_T resourceHandle;
  VC_IMAGE_TYPE_T imageType = VC_IMAGE_RGBA32;
  resourceHandle =
      vc_dispmanx_resource_create(imageType, dmxWidth, dmxHeight, &vcImagePtr);

  result =
      vc_dispmanx_snapshot(displayHandle, resourceHandle, DISPMANX_NO_ROTATE);

  if (result != 0) {
    vc_dispmanx_resource_delete(resourceHandle);
    vc_dispmanx_display_close(displayHandle);

    return PISCRN_DRIVER_ERROR;
  }

  VC_RECT_T rect;
  result = vc_dispmanx_rect_set(&rect, 0, 0, dmxWidth, dmxHeight);

  if (result != 0) {
    vc_dispmanx_resource_delete(resourceHandle);
    vc_dispmanx_display_close(displayHandle);

    return PISCRN_DRIVER_ERROR;
  }

  result = vc_dispmanx_resource_read_data(resourceHandle, &rect, dmxImagePtr,
                                          dmxPitch);

  if (result != 0) {
    vc_dispmanx_resource_delete(resourceHandle);
    vc_dispmanx_display_close(displayHandle);

    return PISCRN_DRIVER_ERROR;
  }

  vc_dispmanx_resource_delete(resourceHandle);
  vc_dispmanx_display_close(displayHandle);

  //-------------------------------------------------------------------
  // Convert from RGBA (32 bit) to RGB (24 bit)

  int8_t pngBytesPerPixel = 3;
  int32_t pngPitch = pngBytesPerPixel * pngWidth;
  void *pngImagePtr = malloc(pngPitch * pngHeight);

  int32_t j = 0;
  for (j = 0; j < pngHeight; j++) {
    int32_t dmxXoffset = 0;
    int32_t dmxYoffset = 0;

    switch (modeInfo.transform & 3) {
    case 0: // 0 degrees

      if (modeInfo.transform & 0x20000) // flip vertical
      {
        dmxYoffset = (dmxHeight - j - 1) * dmxPitch;
      } else {
        dmxYoffset = j * dmxPitch;
      }

      break;

    case 1: // 90 degrees

      if (modeInfo.transform & 0x20000) // flip vertical
      {
        dmxXoffset = j * dmxBytesPerPixel;
      } else {
        dmxXoffset = (dmxWidth - j - 1) * dmxBytesPerPixel;
      }

      break;

    case 2: // 180 degrees

      if (modeInfo.transform & 0x20000) // flip vertical
      {
        dmxYoffset = j * dmxPitch;
      } else {
        dmxYoffset = (dmxHeight - j - 1) * dmxPitch;
      }

      break;

    case 3: // 270 degrees

      if (modeInfo.transform & 0x20000) // flip vertical
      {
        dmxXoffset = (dmxWidth - j - 1) * dmxBytesPerPixel;
      } else {
        dmxXoffset = j * dmxBytesPerPixel;
      }

      break;
    }

    int32_t i = 0;
    for (i = 0; i < pngWidth; i++) {
      uint8_t *pngPixelPtr =
          pngImagePtr + (i * pngBytesPerPixel) + (j * pngPitch);

      switch (modeInfo.transform & 3) {
      case 0: // 0 degrees

        if (modeInfo.transform & 0x10000) // flip horizontal
        {
          dmxXoffset = (dmxWidth - i - 1) * dmxBytesPerPixel;
        } else {
          dmxXoffset = i * dmxBytesPerPixel;
        }

        break;

      case 1: // 90 degrees

        if (modeInfo.transform & 0x10000) // flip horizontal
        {
          dmxYoffset = (dmxHeight - i - 1) * dmxPitch;
        } else {
          dmxYoffset = i * dmxPitch;
        }

        break;

      case 2: // 180 degrees

        if (modeInfo.transform & 0x10000) // flip horizontal
        {
          dmxXoffset = i * dmxBytesPerPixel;
        } else {
          dmxXoffset = (dmxWidth - i - 1) * dmxBytesPerPixel;
        }

        break;

      case 3: // 270 degrees

        if (modeInfo.transform & 0x10000) // flip horizontal
        {
          dmxYoffset = i * dmxPitch;
        } else {
          dmxYoffset = (dmxHeight - i - 1) * dmxPitch;
        }

        break;
      }

      uint8_t *dmxPixelPtr = dmxImagePtr + dmxXoffset + dmxYoffset;

      memcpy(pngPixelPtr, dmxPixelPtr, 3);
    }
  }

  free(dmxImagePtr);
  dmxImagePtr = NULL;

  //-------------------------------------------------------------------

  png_structp pngPtr =
      png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if (pngPtr == NULL) {
    return PISCRN_LIBPNG_ERROR;
  }

  png_infop infoPtr = png_create_info_struct(pngPtr);

  if (infoPtr == NULL) {
    return PISCRN_LIBPNG_ERROR;
  }

  if (setjmp(png_jmpbuf(pngPtr))) {
    return PISCRN_LIBPNG_ERROR;
  }

  FILE *pngfp = NULL;
  piscrn_png_mem_ptr pngmem;

  if (params->output.choice == PISCRN_OUTPUT_STDOUT) {
    pngfp = stdout;
    png_init_io(pngPtr, pngfp);
  } else if (params->output.choice == PISCRN_OUTPUT_FILE) {
    pngfp = fopen(params->output.fileName, "wb");

    if (pngfp == NULL) {
      return PISCRN_UNABLE_TO_CREATE_FILE;
    }

    png_init_io(pngPtr, pngfp);
  } else if (params->output.choice == PISCRN_OUTPUT_MEMORY) {
    size_t min_png_size = 2 * 1024 * 1024; // 2MB
    pngmem.buffer = malloc(min_png_size);
    pngmem.allocSize = min_png_size;
    pngmem.imgSize = 0;

    png_set_write_fn(pngPtr, &pngmem, piscrn_png_memory_write, NULL);
  }

  int bit_depth = 8;
  png_set_IHDR(pngPtr, infoPtr, pngWidth, pngHeight, bit_depth,
               PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

  if (params->compression != Z_DEFAULT_COMPRESSION) {
    png_set_compression_level(pngPtr, params->compression);
  }

  png_set_filter(pngPtr, 0, PNG_FILTER_SUB);
  png_write_info(pngPtr, infoPtr);

  int y = 0;
  for (y = 0; y < pngHeight; y++) {
    png_write_row(pngPtr, pngImagePtr + (pngPitch * y));
  }

  png_write_end(pngPtr, NULL);
  png_destroy_write_struct(&pngPtr, &infoPtr);

  if (params->output.choice == PISCRN_OUTPUT_FILE) {
    fclose(pngfp);
  }

  if (params->output.choice == PISCRN_OUTPUT_MEMORY) {
    *params->output.memoryOut = pngmem.buffer;
    *params->output.sizeOut = pngmem.imgSize;
  }
  //-------------------------------------------------------------------

  free(pngImagePtr);
  pngImagePtr = NULL;

  return PISCRN_SUCCESS;
}
