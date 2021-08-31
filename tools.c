/* ** NOTE: undocumented in Doxygen
 * @file tools.c
 * @brief implementation of several tool functions for imgStore
 *
 * @author Mia Primorac
 */

#include "imgStore.h"
#include "error.h"

#include <stdint.h> // for uint8_t
#include <stdio.h> // for sprintf
#include <openssl/sha.h> // for SHA256_DIGEST_LENGTH
#include <stdlib.h>
#include <inttypes.h> // for PRI...

/********************************************************************//**
 * Human-readable SHA
 */
static void
sha_to_string (const unsigned char* SHA,
               char* sha_string)
{
    if (SHA == NULL) {
        return;
    }
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        sprintf(&sha_string[i*2], "%02x", SHA[i]);
    }

    sha_string[2*SHA256_DIGEST_LENGTH] = '\0';
}

/********************************************************************//**
 * imgStore header display.
 */
void print_header(const struct imgst_header* header)
{
    if(header != NULL) {
        puts("*****************************************");
        puts("**********IMGSTORE HEADER START**********");
        printf("TYPE: %31s\n", header->imgst_name);
        printf("VERSION: %" PRIu32 "\n", header->imgst_version);
        printf("IMAGE COUNT: %" PRIu32 "\t\tMAX IMAGES: %" PRIu32 "\n", header->num_files, header->max_files);
        printf("THUMBNAIL: %" PRIu16 " x %" PRIu16 "\tSMALL: %" PRIu16 " x %" PRIu16 "\n", header->res_resized[RES_THUMB*2],header->res_resized[RES_THUMB*2 + 1], header->res_resized[RES_SMALL*2], header->res_resized[RES_SMALL*2+1]);
        puts("***********IMGSTORE HEADER END***********");
        puts("*****************************************");
    }
}

/********************************************************************//**
 * Metadata display.
 */
void print_metadata (const struct img_metadata* metadata)
{
    if(metadata != NULL) {
        char sha_printable[2*SHA256_DIGEST_LENGTH+1];
        sha_to_string(metadata->SHA, sha_printable);
        printf("IMAGE ID: %s\n", metadata->img_id);
        printf("SHA: %s\n", sha_printable);
        printf("VALID: %" PRIu16 "\n", metadata->is_valid);
        printf("UNUSED: %" PRIu16 "\n", metadata->unused_16);
        printf("OFFSET ORIG. : %" PRIu64 "\t\tSIZE ORIG. : %" PRIu32 "\n", metadata->offset[RES_ORIG], metadata->size[RES_ORIG]);
        printf("OFFSET THUMB.: %" PRIu64 "\t\tSIZE THUMB.: %" PRIu32 "\n", metadata->offset[RES_THUMB], metadata->size[RES_THUMB]);
        printf("OFFSET SMALL : %" PRIu64 "\t\tSIZE SMALL : %" PRIu32 "\n", metadata->offset[RES_SMALL], metadata->size[RES_SMALL]);
        printf("ORIGINAL: %" PRIu32 " x %" PRIu32 "\n", metadata->res_orig[0], metadata->res_orig[1]);
        puts("*****************************************");
    }
}

/********************************************************************//**
 * Open imgStore file, read the header and all the metadata.
 */
int do_open (const char* imgst_filename, const char* open_mode, struct imgst_file* imgst_file)
{
    M_REQUIRE_NON_NULL(imgst_filename);
    M_REQUIRE_NON_NULL(open_mode);
    M_REQUIRE_NON_NULL(imgst_file);
    M_REQUIRE((strcmp(open_mode, "rb") || strcmp(open_mode, "rb+")), ERR_INVALID_ARGUMENT, "open mode should be 'rb' or 'rb+'", NULL);

    // Open file
    imgst_file->file = fopen(imgst_filename, open_mode);
    M_REQUIRE_NON_NULL_CUSTOM_ERR(imgst_file->file, ERR_IO);

    // Read header
    M_IO_CHECK_WITH_CODE(
    fread(&imgst_file->header, sizeof(struct imgst_header), 1, imgst_file->file) != 1, {
        fclose(imgst_file->file);
        imgst_file->file = NULL;
    });

    // Initialises the metadata
    imgst_file->metadata = calloc(imgst_file->header.max_files, sizeof(struct img_metadata));
    M_CHECK_WITH_CODE(
    imgst_file->metadata == NULL, {
        fclose(imgst_file->file);
        imgst_file->file = NULL;
    },
    ERR_OUT_OF_MEMORY);

    // Read metadata
    M_IO_CHECK_WITH_CODE(
    fread(imgst_file->metadata, sizeof(struct img_metadata), imgst_file->header.max_files, imgst_file->file) != imgst_file->header.max_files,
    do_close(imgst_file));

    return ERR_NONE;
}

/********************************************************************//**
 * Do some clean-up for imgStore file handling.
 */
void do_close (struct imgst_file* imgst_file)
{
    // close file
    if (imgst_file->file != NULL) fclose(imgst_file->file);
    imgst_file-> file = NULL;

    // free metadata
    if (imgst_file->metadata != NULL) free(imgst_file-> metadata);
    imgst_file->metadata = NULL;
}

/********************************************************************//**
 * Transforms resolution string to its int value.
 */
int resolution_atoi(const char* resolution)
{
    if (resolution == NULL) return -1;
    if (!strcmp(resolution, "original") || !strcmp(resolution, "orig")) return RES_ORIG;
    if (!strcmp(resolution, "thumbnail") || !strcmp(resolution, "thumb")) return RES_THUMB;
    if (!strcmp(resolution, "small")) return RES_SMALL;
    return -1;
}
/********************************************************************//**
 * Attempts to find an img_id in an imst_file
 */
int find_img_id(size_t* const index, const struct imgst_file* imgst_file, const char* img_id)
{
    size_t i = 0;
    int found = 0;
    uint32_t valid_read = 0;
    while(i < imgst_file->header.max_files && found == 0 && valid_read < imgst_file -> header.num_files) {
        if(imgst_file-> metadata[i].is_valid == NON_EMPTY) {
            valid_read++;
            if(!strcmp(imgst_file->metadata[i].img_id, img_id)) {
                found = 1;
                *index = i;
            }
        }
        i++;
    }
    if(found == 0) return ERR_FILE_NOT_FOUND;
    return ERR_NONE;
}

#define THUMB_STR "_thumb"
#define SMALL_STR "_small"
#define ORIG_STR "_orig"
#define MAX_RES_NAME 6 // length of the longest string above
/********************************************************************//**
 * Creates the name of a picture according to conventions.
 */
int create_name(const char* img_id, int resolution_code, char** name)
{
    M_REQUIRE_NON_NULL(img_id);
    M_CHECK_IMG_ID(img_id);
    M_REQUIRE(resolution_code < NB_RES && resolution_code >= 0, ERR_RESOLUTIONS, ERR_MESSAGES[ERR_RESOLUTIONS], NULL);

    char* res = calloc(MAX_RES_NAME + 1, 1);
    M_EXIT_IF_NULL(res, MAX_RES_NAME + 1);
    switch(resolution_code) {
    case RES_THUMB:
        strcpy(res, THUMB_STR);
        break;
    case RES_SMALL:
        strcpy(res, SMALL_STR);
        break;
    case RES_ORIG:
        strcpy(res, ORIG_STR);
        break;
    };
    *name = calloc(MAX_IMG_ID + MAX_RES_NAME + 5, 1);
    M_CHECK_WITH_CODE(
    *name == NULL, {
        free(res);
        res = NULL;
    },
    ERR_OUT_OF_MEMORY);

    strcat(*name, img_id);
    strcat(*name, res);
    strcat(*name, ".jpg");

    free(res);
    return ERR_NONE;
}

/********************************************************************//**
* Writes the metadata of an image on disk
*/
int update_disk_metadata(struct imgst_file* imgst_file, size_t index)
{
    // calculates offset of image
    long offset = (long)(sizeof(imgst_file->header) + index * sizeof(imgst_file->metadata[index]));
    if (offset < 0) return ERR_IO; // size is bigger than maximum value for a long int

    // updates metadata
    M_IO_CHECK(fseek(imgst_file->file, offset, SEEK_SET), 0);
    M_IO_CHECK(fwrite(&(imgst_file->metadata[index]), sizeof(imgst_file->metadata[index]), 1, imgst_file->file), 1);
    return ERR_NONE;
}

/********************************************************************//**
* Writes the header of the imgStore on disk
*/
int update_disk_header(struct imgst_file* imgst_file)
{
    M_IO_CHECK(fseek(imgst_file->file, 0, SEEK_SET), 0);
    M_IO_CHECK(fwrite(&imgst_file->header, sizeof(imgst_file->header), 1, imgst_file->file), 1);
    return ERR_NONE;
}

/********************************************************************//**
* Writes an image at the end of a file and outputs its position in the file
*/
int write_disk_image(struct imgst_file* imgst_file, void* buffer, size_t size, long* next_position)
{
    // writes resized image at the end of file
    M_IO_CHECK(fseek(imgst_file->file, 0L, SEEK_END), 0);

    // saves the new position of resized image
    *next_position = ftell(imgst_file->file);
    M_IO_CHECK(fwrite(buffer, size, 1, imgst_file->file), 1);
    return ERR_NONE;
}

/********************************************************************//**
* Reads an image from a file
*/
int read_disk_image(FILE* file, void** buffer, size_t size, long offset)
{
    // read original image into buffer
    M_IO_CHECK(fseek(file, offset, SEEK_SET), 0);
    M_IO_CHECK(fread(*buffer, size, 1, file), 1);
    return ERR_NONE;
}
