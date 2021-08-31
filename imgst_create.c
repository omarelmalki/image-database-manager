/**
 * @file imgst_create.c
 * @brief imgStore library: do_create implementation.
 */
#include <stdio.h>
#include "imgStore.h"
#include "error.h"
#include <string.h> // for strncpy
#include <stdlib.h>

/********************************************************************//**
 * Creates the imgStore called imgst_filename. Writes the header and the
 * preallocated empty metadata array to imgStore file.
 */
int do_create(const char* filename, struct imgst_file* DBFILE)
{
    // check correctness of arguments
    M_REQUIRE_NON_NULL(filename);
    M_REQUIRE_NON_NULL(DBFILE);

    // Sets the DB header name
    strncpy(DBFILE->header.imgst_name, CAT_TXT,  MAX_IMGST_NAME);
    DBFILE->header.imgst_name[MAX_IMGST_NAME] = '\0';

    // Sets the rest of the header
    DBFILE->header.imgst_version = 0;
    DBFILE->header.num_files = 0;
    DBFILE->header.unused_32 = 0;
    DBFILE->header.unused_64 = 0;

    // Initialises & allocate the metadata, is_valid is initialized to 0 (EMPTY) through calloc
    DBFILE->metadata = calloc(DBFILE->header.max_files, sizeof(struct img_metadata));
    M_EXIT_IF_NULL(DBFILE->metadata, (DBFILE->header.max_files*sizeof(struct img_metadata)));

    // Opens file
    DBFILE->file = NULL;
    DBFILE->file = fopen(filename, "wb");
    M_IO_CHECK_WITH_CODE(
    DBFILE->file == NULL, {
        free(DBFILE->metadata);
        DBFILE->metadata = NULL;
    });

    size_t total_written = 0;

    // Write header
    M_IO_CHECK_WITH_CODE(
    fwrite(&DBFILE->header, sizeof(DBFILE->header), 1, DBFILE->file) != 1,
    do_close(DBFILE));
    total_written += 1;

    // write metadata
    M_IO_CHECK_WITH_CODE(
    fwrite(DBFILE->metadata, sizeof(struct img_metadata), DBFILE->header.max_files, DBFILE->file) != DBFILE->header.max_files,
    do_close(DBFILE));
    total_written += DBFILE->header.max_files;

    printf("%zu item(s) written\n", total_written);
    return ERR_NONE;
}
