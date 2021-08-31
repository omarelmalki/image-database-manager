/**
 * @file imgStore.h
 * @brief Main header file for imgStore core library.
 *
 * Defines the format of the data structures that will be stored on the disk
 * and provides interface functions.
 *
 * The image imgStore starts with exactly one header structure
 * followed by exactly imgst_header.max_files metadata
 * structures. The actual content is not defined by these structures
 * because it should be stored as raw bytes appended at the end of the
 * imgStore file and addressed by offsets in the metadata structure.
 *
 * @author Mia Primorac
 */
#ifndef IMGSTOREPRJ_IMGSTORE_H
#define IMGSTOREPRJ_IMGSTORE_H

#include "error.h" /* not needed in this very file,
                    * but we provide it here, as it is required by
                    * all the functions of this lib.
                    */
#include <stdio.h> // for FILE
#include <stdint.h> // for uint32_t, uint64_t
#include <openssl/sha.h> // for SHA256_DIGEST_LENGTH

#define CAT_TXT "EPFL ImgStore binary"

/* constraints */
#define MAX_IMGST_NAME  31  // max. size of a ImgStore name
#define MAX_IMG_ID     127  // max. size of an image id
#define MAX_MAX_FILES 100000
#define MAX_THUMB_RES 128
#define MAX_SMALL_RES 512

/* For is_valid in imgst_metadata */
#define EMPTY 0
#define NON_EMPTY 1

// imgStore library internal codes for different image resolutions.
#define RES_THUMB 0
#define RES_SMALL 1
#define RES_ORIG  2
#define NB_RES    3

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Header containing the configuration information of imgStore
 *
 */
struct imgst_header {
    char imgst_name[MAX_IMGST_NAME+1]; // name of image database
    uint32_t imgst_version; // image database version, increased after each modification
    uint32_t num_files; // number of valid images in the database
    const uint32_t max_files; // maximum number of files, does not change after creation
    const uint16_t res_resized[2*(NB_RES-1)]; // maximum resolutions of different image resolutions, elements do not change after creation
    uint32_t unused_32;
    uint64_t unused_64;
};

/**
 * @brief Metadata of an image
 *
 */
struct img_metadata {
    char img_id[MAX_IMG_ID+1]; // image id
    unsigned char SHA[SHA256_DIGEST_LENGTH]; // hash code of the image
    uint32_t res_orig[2]; // resolution of the original image
    uint32_t size[NB_RES]; // size of the images of different resolutions in the file of the database
    uint64_t offset[NB_RES]; // positions of the images in the file of the database
    uint16_t is_valid; // indicates if the image is still used
    uint16_t unused_16;
};

/**
 * @brief Image file structure
 *
 */
struct imgst_file {
    FILE* file; // file containing everything in the disk
    struct imgst_header header; // header
    struct img_metadata* metadata; // metadata
};

/**
 * @brief Prints imgStore header informations.
 *
 * @param header The header to be displayed.
 */
void print_header(const struct imgst_header* header);

/**
 * @brief Prints image metadata informations.
 *
 * @param metadata The metadata of one image.
 */
void print_metadata (const struct img_metadata*  metadata);

/**
 * @brief Open imgStore file, read the header and all the metadata.
 *
 * @param imgst_filename Path to the imgStore file
 * @param open_mode Mode for fopen(), accepts only: "rb", "rb+"
 * @param imgst_file Structure for header, metadata and file pointer.
 * @return int Some error code. 0 if no error.
 */
int do_open (const char* imgst_filename, const char* open_mode, struct imgst_file* imgst_file);

/**
 * @brief Do some clean-up for imgStore file handling.
 *
 * @param imgst_file Structure for header, metadata and file pointer to be freed/closed.
 */
void do_close (struct imgst_file* imgst_file);

/**
 * @brief List of possible output modes for do_list
 *
 * @param imgst_file Structure for header, metadata and file pointer to be freed/closed.
 */
enum do_list_mode {STDOUT, JSON};


/**
 * @brief Displays imgStore metadata or returns it as a JSON object converted to char*.
 *
 * @param imgst_file In memory structure with header and metadata.
 * @param mode Wether we want to display the imgStore metadata (STDOUT) or receive it as a JSON object
 * converted to string (JSON).
 * @return NULL if mode is STDOUT, the JSON object, or an error string if mode is JSON, and
 * another error string if mode is unkown. The string must be freed by the caller.
 */
char* do_list(const struct imgst_file* imgst_file, const enum do_list_mode mode);

/**
 * @brief Creates the imgStore called imgst_filename. Writes the header and the
 *        preallocated empty metadata array to imgStore file.
 *
 * @param imgst_filename Path to the imgStore file
 * @param imgst_file In memory structure with header and metadata.
 * @return int Some error code. 0 if no error.
 */
int do_create(const char* imgst_filename, struct imgst_file* imgst_file);

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
int do_delete(const char* img_id, struct imgst_file* imgst_file);

/**
 * @brief Transforms resolution string to its int value.
 *
 * @param resolution The resolution string. Shall be "original",
 *        "orig", "thumbnail", "thumb" or "small".
 * @return The corresponding value or -1 if error.
 */
int resolution_atoi(const char* resolution);

/**
 * @brief Reads the content of an image from a imgStore.
 *
 * @param img_id The ID of the image to be read.
 * @param resolution The desired resolution for the image read.
 * @param image_buffer Location of the location of the image content
 * @param image_size Location of the image size variable
 * @param imgst_file The main in-memory data structure
 * @return Some error code. 0 if no error.
 */
int do_read(const char* img_id, int resolution, char** image_buffer, uint32_t* image_size, struct imgst_file* imgst_file);


/**
 * @brief Insert image in the imgStore file
 *
 * @param buffer Pointer to the raw image content
 * @param size Image size
 * @param img_id Image ID
 * @param imgst_file imgStore file
 * @return Some error code. 0 if no error.
 */
int do_insert(const char* buffer, size_t size, const char* img_id, struct imgst_file* imgst_file);

/**Do some clean-up for imgStore file handling.
 * @param imgst_path The path to the imgStore file
 * @param imgst_tmp_bkp_path The path to the a (to be created) temporary imgStore backup file
 * @return Some error code. 0 if no error.
 */
int do_gbcollect (const char *imgst_path, const char *imgst_tmp_bkp_path);


/********************************************************************//**
 * @brief  Attempts to find an img_id in an imst_file
 * @param index : the returned index
 * @param imgst_file : the imgst_file to be searched
 * @param img_id : the img_id from which we want the index
 * @return some error code. 0 if no error.
 */
int find_img_id(size_t* const index, const struct imgst_file* imgst_file, const char* img_id);

/********************************************************************//**
 * @brief  Creates the name of a picture according to conventions.
 * @param img_id: the name of the picture
 * @param resolution_code : the resolution we want the name to refer to
 * @param name : the created name. Must be freed after use.
 * @return some error code. 0 if no error.
 */
int create_name(const char* img_id, int resolution_code, char** name);

/**
 * @brief Writes the metadata of an image on disk
 *
 * @param imgst_file destination file
 * @param index index of the image
 * @return int Some error code. 0 if no error.
 */
int update_disk_metadata(struct imgst_file* imgst_file, size_t index);

/**
 * @brief Writes the header of the imgStore on disk
 *
 * @param imgst_file destination file
 * @return int Some error code. 0 if no error.
 */
int update_disk_header(struct imgst_file* imgst_file);

/**
 * @brief Writes an image at the end of a file and outputs its position in the file
 *
 * @param imgst_file destination file
 * @param buffer pointer on the image
 * @param size size of the image
 * @param next_position output: position of the image in the file after write
 * @return int Some error code. 0 if no error.
 */
int write_disk_image(struct imgst_file* imgst_file, void* buffer, size_t size, long* next_position);

/**
 * @brief Reads an image from a file
 *
 * @param imgst_file file
 * @param buffer output: image
 * @param size size of the image
 * @param offset position of the image in the file
 * @return int Some error code. 0 if no error.
 */
int read_disk_image(FILE* file, void** buffer, size_t size, long offset);

#ifdef __cplusplus
}
#endif
#endif
