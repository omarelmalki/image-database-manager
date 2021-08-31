/*
 * @file imgst_delete.c
 * @brief implementation of the function do_delete() related to the deletion of
 * an image from an image_store.
 *
 */
#include "imgStore.h"
#include "error.h"

#include <stdint.h> // for uint8_t
#include <stdio.h> // for sprintf


/**
 * @brief Deletes an image from a imgStore imgStore.
 *
 * Effectively, it only invalidates the is_valid field and updates the
 * metadata.  The raw data content is not erased, it stays where it
 * was (and  new content is always appended to the end; no garbage
 * collection).
 *
 * @param img_id The ID of the image to be deleted.
 * @param imgst_file The main in-memory data structure
 * @return Some error code. 0 if no error.
 */
int do_delete(const char* img_id, struct imgst_file* imgst_file)
{
    // check validity of arguments
    M_REQUIRE_NON_NULL(img_id);
    // file must be already open
    M_REQUIRE_NON_NULL_IMGST_FILE(imgst_file);
    // check correctness of img_id
    M_CHECK_IMG_ID(img_id);

    size_t index = 0;
    M_EXIT_IF_ERR(find_img_id(&index, imgst_file, img_id));
    imgst_file->metadata[index].is_valid = EMPTY;

    // writes updated metadata to disk
    M_EXIT_IF_ERR(update_disk_metadata(imgst_file, index));

    // updates header and writes it to disk
    imgst_file->header.num_files--;
    imgst_file->header.imgst_version++;
    M_EXIT_IF_ERR(update_disk_header(imgst_file));

    return ERR_NONE;
}
