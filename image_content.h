/**
 * @file image_content.h
 * @brief Header file to prototype functions to manipulate content
 *
 */
#pragma once
#include <stdio.h>
#include <stdint.h>
#include "imgStore.h"

/**
 * @brief Creates a new variant of the specified image, only if it is absent from the file.
 * Creates a copy of the image at the end of the file and updates the metadata accordingly.
 *
 * @param internal_code internal code corresponding to one of the derived resolution of the image (see imgStore.h)
 * @param imgst_file imgStore file that we are working with
 * @param index position of the image to treat
 * @return int Some error code. 0 if no error.
 */
int lazily_resize (int internal_code, struct imgst_file* imgst_file, size_t index);

/**
 * @brief Get the resolution of an image
 *
 * @param height height
 * @param width width
 * @param image_buffer Pointer on a JPEG image
 * @param image_size memory size of the image
 * @return int Some error code. 0 if no error.
 */
int get_resolution(uint32_t* height, uint32_t* width, const char* image_buffer, size_t image_size);
