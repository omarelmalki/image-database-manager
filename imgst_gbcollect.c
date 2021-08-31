/**
 * @file imgst_gbcollect.c
 * @brief Implements do_gbcollect() method
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "imgStore.h"
#include "image_content.h"
#include "error.h"

int do_gbcollect(const char* imgst_name, const char* tmp_name)
{
    // check arguments
    M_REQUIRE_NON_NULL(imgst_name);
    M_REQUIRE_NON_NULL(tmp_name);
    M_CHECK_IMGSTR_NAME(imgst_name);
    size_t length_temp_filename = strlen(tmp_name);
    M_REQUIRE(length_temp_filename > 0, ERR_INVALID_ARGUMENT, "image filename is empty", NULL);
    M_REQUIRE(length_temp_filename < FILENAME_MAX, ERR_INVALID_FILENAME, "image filename is too long", NULL);

    // open file
    struct imgst_file imgst_file;
    M_EXIT_IF_ERR(do_open(imgst_name, "rb", &imgst_file));
    struct imgst_header header = imgst_file.header;

    // initialize temp file
    struct imgst_file temp_file = {
        .header={
            .max_files = header.max_files,
            .res_resized = {header.res_resized[0], header.res_resized[1], header.res_resized[2], header.res_resized[3]}
        }
    };
    M_EXIT_IF_ERR_DO_SOMETHING(do_create(tmp_name, &temp_file), do_close(&imgst_file));
    do_close(&temp_file);

    // open temp_file
    do_open(tmp_name, "rb+", &temp_file);
    size_t valid_images = 0;
    for (size_t i = 0; i < header.max_files; i++) {
        // check if is valid
        if (imgst_file.metadata[i].is_valid == NON_EMPTY) {
            // read
            char* image_buffer = NULL;
            uint32_t size_read = 0;
            M_EXIT_IF_ERR_DO_SOMETHING(do_read(imgst_file.metadata[i].img_id, RES_ORIG, &image_buffer, &size_read, &imgst_file), {
                do_close(&imgst_file);
                do_close(&temp_file);
            });
            // insert
            M_EXIT_IF_ERR_DO_SOMETHING(do_insert(image_buffer, size_read, imgst_file.metadata[i].img_id, &temp_file), {
                do_close(&imgst_file);
                do_close(&temp_file);
                free(image_buffer);
                image_buffer = NULL;
            });
            // lazily_resize
            for (int res = 0; res < NB_RES; res++) {
                if (imgst_file.metadata[i].offset[res] != 0) {
                    M_EXIT_IF_ERR_DO_SOMETHING(lazily_resize(res, &temp_file, valid_images), {
                        do_close(&imgst_file);
                        do_close(&temp_file);
                        free(image_buffer);
                        image_buffer = NULL;
                    });
                }
            }
            valid_images++;
            free(image_buffer);
            image_buffer = NULL;
        }
    }
    // close files and delete temporary file
    do_close(&imgst_file);
    do_close(&temp_file);
    M_IO_CHECK(remove(imgst_name), 0);
    M_IO_CHECK(rename(tmp_name, imgst_name), 0);
    return ERR_NONE;
}
