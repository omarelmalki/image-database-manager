
/**
 * @file imgst_read.c
 * @brief Implements do_read function
 *
 */
#include "imgStore.h"
#include "image_content.h"
#include "error.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/********************************************************************//**
 * Reads the content of an image from an imgStore.
********************************************************************** */
int do_read(const char* img_id, int resolution, char** image_buffer, uint32_t* image_size, struct imgst_file* imgst_file)
{
    M_REQUIRE_NON_NULL(img_id);
    M_REQUIRE_NON_NULL(image_buffer);
    M_REQUIRE_NON_NULL_IMGST_FILE(imgst_file);
    M_CHECK_IMG_ID(img_id);
    M_REQUIRE(resolution < NB_RES && resolution >= 0, ERR_RESOLUTIONS, ERR_MESSAGES[ERR_RESOLUTIONS], NULL);


    size_t index = 0;
    M_EXIT_IF_ERR(find_img_id(&index, imgst_file, img_id));

    //if we don't have the image in this resolution, we make it.
    if(imgst_file->metadata[index].offset[resolution] == 0) {
        M_EXIT_IF_ERR(lazily_resize(resolution, imgst_file, index));
    }

    *image_size = imgst_file->metadata[index].size[resolution];

    *image_buffer = malloc(*image_size);
    M_EXIT_IF_NULL(*image_buffer, *image_size);

    // read original image into buffer
    M_EXIT_IF_ERR_DO_SOMETHING(
    read_disk_image(imgst_file->file, (void**) image_buffer,  *image_size, (long)imgst_file->metadata[index].offset[resolution]), {
        free(*image_buffer);
        *image_buffer = NULL;
    });

    return ERR_NONE;
}
