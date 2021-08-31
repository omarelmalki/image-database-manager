/**
 * @file imgst_content.c
 * @brief imgStore library: lazily_resize implementation.
 */
#include <stdio.h>
#include <stdint.h>
#include "imgStore.h"
#include "image_content.h"
#include <vips/vips.h>
#include "error.h"
#include <stdlib.h>

/**
 * @brief Computes the shrinking factor (keeping aspect ratio)
 *
 * @param image The image to be resized.
 * @param max_width The maximum width allowed for resized creation.
 * @param max_height The maximum height allowed for resized creation.
 */
double shrink_value(const VipsImage *image,
                    int max_width,
                    int max_height);

/**
 * @brief Creates a new resized image and updates size and offset in the file.
 *
 * @param internal_code resolution code
 * @param imgst_file file
 * @param index index of image
 * @return int Some error code. 0 if no error.
 */
int resize_image(int internal_code, struct imgst_file* imgst_file, size_t index);

/********************************************************************//**
 * Creates a new variant of the specified image, only if it is absent from the file.
 * Creates a copy of the image at the end of the file and updates the metadata accordingly.
 */
int lazily_resize(int internal_code, struct imgst_file* imgst_file, size_t index)
{
    // check correctness of arguments
    M_REQUIRE_NON_NULL_IMGST_FILE(imgst_file);
    M_CHECK_IMGST_FILE_INDEX(imgst_file, index);
    // check resolution
    M_REQUIRE(internal_code < NB_RES && internal_code >= 0, ERR_RESOLUTIONS, ERR_MESSAGES[ERR_RESOLUTIONS], NULL);


    // if internal_code is RES_ORIG, do nothing and return ERR_NONE
    if (internal_code != RES_ORIG) {
        // if the offset is not 0, that means the image already exists in this resolution
        if (imgst_file->metadata[index].offset[internal_code] == 0) {
            // Resize the image using vips
            M_EXIT_IF_ERR(resize_image(internal_code, imgst_file, index));

            // updates matadata after resizing
            M_EXIT_IF_ERR(update_disk_metadata(imgst_file, index));
        }
    }
    return ERR_NONE;
}

/********************************************************************//**
 * Resizes the image and updates on disk the size and offset.
 */
int resize_image(int internal_code, struct imgst_file* imgst_file, size_t index)
{
    // original image object
    VipsObject *original = VIPS_OBJECT(vips_image_new());
    VipsImage **original_array = (VipsImage**) vips_object_local_array (original, 1);

    // allocates buffer with original size
    uint32_t size_orig =  imgst_file->metadata[index].size[RES_ORIG];
    void* buffer = malloc(size_orig);
    M_EXIT_IF_NULL(buffer, size_orig);
    read_disk_image(imgst_file->file, &buffer, size_orig, (long)imgst_file->metadata[index].offset[RES_ORIG]);

    // loads vips image from buffer
    M_IMGLIB_CHECK_WITH_CODE(
    vips_jpegload_buffer (buffer, size_orig, original_array, NULL) != 0, {
        g_object_unref(original);
        free(buffer);
        buffer = NULL;
    });


    // calulates scale factor
    const double ratio = shrink_value(original_array[0], imgst_file->header.res_resized[internal_code*2], imgst_file->header.res_resized[internal_code*2+1]);

    // resized image object
    VipsObject *resized = VIPS_OBJECT(vips_image_new());
    VipsImage **resized_array = (VipsImage**) vips_object_local_array (resized, 1);

    // resize image
    M_IMGLIB_CHECK_WITH_CODE(
    vips_resize(original_array[0], resized_array, ratio, NULL) != 0, {
        g_object_unref(resized);
        g_object_unref(original);
        free(buffer);
        buffer = NULL;
    });

    // saves resized image into a new buffer
    void* new_buffer = NULL;
    size_t new_size = 0;

    M_IMGLIB_CHECK_WITH_CODE(
    vips_jpegsave_buffer(resized_array[0], &new_buffer, &new_size, NULL) != 0, {
        g_object_unref(resized);
        g_object_unref(original);
        free(buffer);
        buffer = NULL;
    });

    // updates size in the matadata
    imgst_file->metadata[index].size[internal_code] = (uint32_t) new_size;

    // writes resized image at the end of file
    long next_position;
    M_EXIT_IF_ERR_DO_SOMETHING(
    write_disk_image(imgst_file, new_buffer, new_size, &next_position), {
        g_object_unref(resized);
        g_object_unref(original);
        g_free(new_buffer);
        new_buffer = NULL;
        free(buffer);
        buffer = NULL;
    });

    // updates the offset of the resized file in the metadata
    imgst_file->metadata[index].offset[internal_code] = (uint64_t) next_position;

    // dereference objects and free buffer
    g_object_unref(resized);
    g_object_unref(original);
    g_free(new_buffer);
    new_buffer = NULL;
    free(buffer);
    buffer = NULL;

    return ERR_NONE;
}

/********************************************************************//**
 * Computes the shrinking factor (keeping aspect ratio)
 */
double shrink_value(const VipsImage *image,
                    int max_width,
                    int max_height)
{
    const double h_shrink = (double) max_width  / (double) image->Xsize ;
    const double v_shrink = (double) max_height / (double) image->Ysize ;
    return h_shrink > v_shrink ? v_shrink : h_shrink ;
}

/********************************************************************//**
 * Get the resolution of an image
 */
int get_resolution(uint32_t* height, uint32_t* width, const char* image_buffer, size_t image_size)
{
    M_REQUIRE_NON_NULL(image_buffer);
    // image object
    VipsObject *loaded = VIPS_OBJECT(vips_image_new());
    VipsImage **loaded_image = (VipsImage**) vips_object_local_array (loaded, 1);

    // load vips image
    M_IMGLIB_CHECK_WITH_CODE(
    vips_jpegload_buffer((void*) image_buffer, image_size, loaded_image, NULL) != 0,
    g_object_unref(loaded));

    // get height and width
    *height = (uint32_t) loaded_image[0] ->  Ysize;
    *width = (uint32_t) loaded_image[0] ->  Xsize;
    // dereference object
    g_object_unref(loaded);

    return ERR_NONE;
}
