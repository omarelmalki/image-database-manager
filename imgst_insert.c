/**
 * @file imgst_insert.c
 * @brief imgStore library: do_insert implementation.
 */
#include "imgStore.h"
#include "dedup.h"
#include "image_content.h"
#include "error.h"

#include <stdio.h>
#include <stdint.h>
#include <openssl/sha.h>

/**
 * @brief Returns the index of an empty image slot in the metadata and updates it. Parameters are expected to be correct.
 *
 * @param buffer Pointer to the raw image content
 * @param size Image size
 * @param img_id Image ID
 * @param imgst_file imgStore file
 * @return size_t index.
 */
size_t find_empty_and_update_metadata(const char* buffer, size_t size, const char* img_id, const struct imgst_file* imgst_file);

/********************************************************************//**
 * Insert image in the imgStore file
 */
int do_insert(const char* buffer, size_t size, const char* img_id, struct imgst_file* imgst_file)
{
    //fprintf(stderr, "DEBUG: %d\n%s\n", size, img_id);
    // check validity of arguments
    M_REQUIRE_NON_NULL(buffer);
    M_REQUIRE_NON_NULL(img_id);
    // file must be already open
    M_REQUIRE_NON_NULL_IMGST_FILE(imgst_file);
    M_CHECK_IMG_ID(img_id);
    M_REQUIRE(imgst_file->header.num_files < imgst_file->header.max_files, ERR_FULL_IMGSTORE, ERR_MESSAGES[ERR_FULL_IMGSTORE], NULL);

    const uint32_t index = (uint32_t) find_empty_and_update_metadata(buffer, size, img_id, imgst_file);
    M_EXIT_IF_ERR(do_name_and_content_dedup(imgst_file, index));

    // if there is no duplicate image, write image at the end of file
    if (imgst_file->metadata[index].offset[RES_ORIG] == 0) {
        long offset;
        M_EXIT_IF_ERR(write_disk_image(imgst_file, (void*)buffer, size, &offset));
        imgst_file->metadata[index].offset[RES_ORIG] = (uint64_t)offset;
        imgst_file->metadata[index].offset[RES_THUMB] = 0;
        imgst_file->metadata[index].size[RES_THUMB] = 0;
        imgst_file->metadata[index].offset[RES_SMALL] = 0;
        imgst_file->metadata[index].size[RES_SMALL] = 0;
    }
    M_EXIT_IF_ERR(get_resolution(&imgst_file->metadata[index].res_orig[1], &imgst_file->metadata[index].res_orig[0], buffer, size));

    // updates header
    imgst_file->header.num_files++;
    imgst_file->header.imgst_version++;
    M_EXIT_IF_ERR(update_disk_header(imgst_file));
    // updates metadata
    imgst_file->metadata[index].is_valid = NON_EMPTY;
    M_EXIT_IF_ERR(update_disk_metadata(imgst_file, index));

    return ERR_NONE;
}

/********************************************************************//**
  * Returns the index of an empty image slot in the metadata and updates it. Parameters are expected to be correct.
  */
size_t find_empty_and_update_metadata(const char* buffer, size_t size, const char* img_id, const struct imgst_file* imgst_file)
{
    size_t i = 0;
    int found = 0;
    while (i < imgst_file->header.max_files && found == 0) {
        if (imgst_file->metadata[i].is_valid == EMPTY) {
            SHA256((unsigned char *) buffer, size, imgst_file->metadata[i].SHA);
            strncpy(imgst_file->metadata[i].img_id, img_id, MAX_IMG_ID);
            imgst_file->metadata[i].size[RES_ORIG] = (uint32_t)size;
            found = 1;
        }
        i++;
    }
    return i-1;
}
