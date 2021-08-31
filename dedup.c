/**
 * @file dedup.c
 * @brief implementation of the function do_name_and_content_dedup() to de-duplicate images
 *
 */
#include <stdio.h>
#include "dedup.h"
#include <openssl/sha.h>
#include "error.h"
#include "imgStore.h"
#include <string.h>
#include <stdint.h>

/**
 * @brief Compares two SHA values
 *
 * @param SHA1
 * @param SHA2
 * @return int 0 if equal, -1 if not equal.
 */
int compare_sha(unsigned char SHA1[SHA256_DIGEST_LENGTH], unsigned char SHA2[SHA256_DIGEST_LENGTH]);

/********************************************************************//**
 * De-duplicates an image at a specific index in a file if it was duplicated.
 */
int do_name_and_content_dedup(const struct imgst_file* file, const uint32_t index)
{
    M_REQUIRE_NON_NULL(file);
    M_REQUIRE_NON_NULL(file->metadata);
    M_REQUIRE(index < file->header.max_files, ERR_INVALID_ARGUMENT, "index out of bounds", NULL);

    size_t i = 0;
    size_t valid = 0;
    int found = 0;
    while (i < file->header.max_files) {
        if (file->metadata[i].is_valid == NON_EMPTY) {
            if (i != index) {
                M_REQUIRE(strcmp(file->metadata[i].img_id, file->metadata[index].img_id), ERR_DUPLICATE_ID, ERR_MESSAGES[ERR_DUPLICATE_ID], NULL);
                if (found == 0 && (!compare_sha(file->metadata[i].SHA, file->metadata[index].SHA))) {
                    file->metadata[index].offset[RES_ORIG] = file->metadata[i].offset[RES_ORIG];
                    file->metadata[index].offset[RES_THUMB] = file->metadata[i].offset[RES_THUMB];
                    file->metadata[index].offset[RES_SMALL] = file->metadata[i].offset[RES_SMALL];
                    file->metadata[index].size[RES_SMALL] = file->metadata[i].size[RES_SMALL];
                    file->metadata[index].size[RES_THUMB] = file->metadata[i].size[RES_THUMB];
                    found = 1;
                }
            }
            valid++;
        }
        i++;
    }
    if (found == 0) {
        file->metadata[index].offset[RES_ORIG] = 0;
    }

    return ERR_NONE;
}

/********************************************************************//**
 * Compares two SHA values
 */
int compare_sha(unsigned char SHA1[SHA256_DIGEST_LENGTH], unsigned char SHA2[SHA256_DIGEST_LENGTH])
{
    for (size_t i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        if (SHA1[i] != SHA2[i]) return -1;
    }
    return 0;
}
