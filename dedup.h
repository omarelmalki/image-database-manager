/**
 * @file dedup.h
 * @brief Header file to prototype do_name_and_content_dedup()
 *
 */
#pragma once
#include "imgStore.h"
#include <stdint.h>

/**
 * @brief De-duplicates an image at a specific index in a file if it was duplicated.
 *
 * @param file imgStore file
 * @param index position of the image in the metadata array of the file
 * @return int error code. 0 if no error.
 */
int do_name_and_content_dedup(const struct imgst_file* file, const uint32_t index);
