/**
 * @file imgStoreMgr.c
 * @brief imgStore Manager: command line interpretor for imgStore core commands.
 *
 * Image Database Management Tool
 *
 * @author Mia Primorac
 */

#include "util.h" // for _unused
#include "imgStore.h"
#include "image_content.h"
#include "error.h"

#include <stdlib.h>
#include <string.h>
#include <vips/vips.h> // for VIPS_INIT and vips shutdown

/********************************************************************//**
* A command is a function that we call from the command line
*********************************************************************** */
typedef int (*command)(int args, char* argv[]);

/********************************************************************//**
 * A command_mapping is used to map a command to its name as it will
 * be called in the command line.
 ********************************************************************** */
typedef struct {
    const char* name;
    const command cmd;
    const int nbr_compulsory_args;
} command_mapping;


/********************************************************************//**
 * Opens imgStore file and calls do_list command.
 ********************************************************************** */
int
do_list_cmd (int args _unused, char* argv[])
{
    const char* fileName = argv[1];
    M_CHECK_IMGSTR_NAME(fileName);
    struct imgst_file myfile;
    // opens file
    M_EXIT_IF_ERR(do_open(fileName, "rb", &myfile));

    // lists file
    do_list(&myfile, STDOUT);

    // closes file
    do_close(&myfile);

    return ERR_NONE;
}


#define MAX_ARGS 2
#define NBR_OPT_ARGS 3
typedef struct {
    const char* name;
    const size_t nbr_of_args;
    uint32_t bound;
    uint32_t args[MAX_ARGS];
    int error;
} optional_args ;
/********************************************************************//**
 * Prepares and calls do_create command.
********************************************************************** */
int
do_create_cmd (int args, char* argv[])
{
    const char* fileName = argv[1];
    M_CHECK_IMGSTR_NAME(fileName);

    // Default values
    uint32_t max_files   =  10;
    uint16_t thumb_res_x =  64;
    uint16_t thumb_res_y =  64;
    uint16_t small_res_x = 256;
    uint16_t small_res_y = 256;

    //parsing the optionnal arguments:
    if(args >= 3) {

        optional_args args_tab[NBR_OPT_ARGS] = {{"-max_files", 1, MAX_MAX_FILES, {max_files}, ERR_MAX_FILES},
            {"-thumb_res", 2, MAX_THUMB_RES, {(uint32_t)thumb_res_x, (uint32_t)thumb_res_y}, ERR_RESOLUTIONS},
            {"-small_res", 2, MAX_SMALL_RES, {(uint32_t)small_res_x, (uint32_t)small_res_y}, ERR_RESOLUTIONS}
        };
        size_t i = 2;
        while (i < (size_t)args) {
            int valid = 0;
            for(size_t j = 0; j < NBR_OPT_ARGS && !valid; j++) {
                if(!strcmp(argv[i], args_tab[j].name)) {
                    if(! ((size_t)args - i > args_tab[j].nbr_of_args)) {
                        return ERR_NOT_ENOUGH_ARGUMENTS;
                    }
                    for(size_t k = 0; k < args_tab[j].nbr_of_args; k++) {
                        uint32_t temp = atouint32(argv[++i]);
                        if(temp == 0 || temp > args_tab[j].bound) {
                            return args_tab[j].error;
                        }
                        args_tab[j].args[k] = temp;
                    }
                    valid = 1;
                    ++i;
                }
            }
            if(valid == 0) {
                return ERR_INVALID_ARGUMENT;
            }
        }

        //we retrierve what has been parsed (indexes are safe since they have been defined in the same scope):
        max_files = args_tab[0].args[0];
        thumb_res_x = (uint16_t) args_tab[1].args[0];
        thumb_res_y = (uint16_t) args_tab[1].args[1];
        small_res_x = (uint16_t) args_tab[2].args[0];
        small_res_y = (uint16_t) args_tab[2].args[1];

    }

    puts("Create");
    // initialize dbfile
    struct imgst_file dbfile = {.header={.max_files=max_files, .res_resized={thumb_res_x, thumb_res_y, small_res_x, small_res_y}}};
    // creates file and prints header
    int error_status = do_create(fileName, &dbfile);
    if (error_status == ERR_NONE) {
        print_header(&dbfile.header);
    }
    // closes file
    do_close(&dbfile);
    return error_status;
}

/********************************************************************//**
 * Displays some explanations.
 ********************************************************************** */
int help (int args _unused, char* argv[] _unused)
{
    puts("imgStoreMgr [COMMAND] [ARGUMENTS]");
    puts("\thelp: displays this help.");
    puts("\tlist <imgstore_filename>: list imgStore content.");
    puts("\tcreate <imgstore_filename> [options]: create a new imgStore.");
    puts("\t\toptions are:");
    puts("\t\t\t-max_files <MAX_FILES>: maximum number of files.");
    puts("\t\t\t\tdefault value is 10");
    puts("\t\t\t\tmaximum value is 100000");
    puts("\t\t\t-thumb_res <X_RES> <Y_RES>: resolution for thumbnail images.");
    puts("\t\t\t\tdefault value is 64x64");
    puts("\t\t\t\tmaximum value is 128x128");
    puts("\t\t\t-small_res <X_RES> <Y_RES>: resolution for small images.");
    puts("\t\t\t\tdefault value is 256x256");
    puts("\t\t\t\tmaximum value is 512x512");
    puts("\tread   <imgstore_filename> <imgID> [original|orig|thumbnail|thumb|small]:");
    puts("\t\tread an image from the imgStore and save it to a file.");
    puts("\t\tdefault resolution is \"original\".");
    puts("\tinsert <imgstore_filename> <imgID> <filename>: insert a new image in the imgStore.");
    puts("\tdelete <imgstore_filename> <imgID>: delete image imgID from imgStore.");
    puts("\tgc <imgstore_filename> <tmp imgstore_filename>: performs garbage collecting on imgStore. Requires a temporary filename for copying the imgStore.");
    return ERR_NONE;
}


/********************************************************************//**
 * Deletes an image from the imgStore.
 */
int do_delete_cmd (int args _unused, char* argv[])
{
    // checks arguments
    const char* fileName = argv[1];
    const char* img_id = argv[2];
    M_CHECK_IMGSTR_NAME(fileName);
    M_CHECK_IMG_ID(img_id);

    struct imgst_file imgst;
    // do_open will initialize the struct imgst_file
    M_EXIT_IF_ERR(do_open(fileName, "rb+", &imgst));

    // deletes image
    int err = do_delete(img_id, &imgst);
    // closes file
    do_close(&imgst);
    return err;
}

/********************************************************************//**
 * Deletes an image from the imgStore.
 */
int do_insert_cmd (int args _unused, char* argv[])
{
    // checks arguments
    const char* fileName = argv[1];
    const char* img_id = argv[2];
    const char* image_filename = argv[3];
    M_CHECK_IMGSTR_NAME(fileName);
    M_CHECK_IMG_ID(img_id);
    size_t length_image_filename = strlen(image_filename);
    M_REQUIRE(length_image_filename > 0, ERR_INVALID_ARGUMENT, "image filename is empty", NULL);
    M_REQUIRE(length_image_filename < FILENAME_MAX, ERR_INVALID_FILENAME, "image filename is too long", NULL);

    FILE* image_file = fopen(image_filename, "rb");
    M_REQUIRE_NON_NULL_CUSTOM_ERR(image_file, ERR_IO);
    M_IO_CHECK_WITH_CODE(
    fseek(image_file, 0L, SEEK_END) != 0, {
        fclose(image_file);
        image_file = NULL;
    });

    size_t file_size = (size_t) ftell(image_file);
    void* image_buffer = malloc(file_size);

    M_CHECK_WITH_CODE(
    image_buffer == NULL, {
        fclose(image_file);
        image_file = NULL;
    },
    ERR_OUT_OF_MEMORY);

    M_IO_CHECK_WITH_CODE(
    fseek(image_file, 0L, SEEK_SET) != 0, {
        fclose(image_file);
        image_file = NULL;
        free(image_buffer);
        image_buffer = NULL;
    });

    M_IO_CHECK_WITH_CODE(
    fread(image_buffer, file_size, 1, image_file) != 1, {
        fclose(image_file);
        image_file = NULL;
        free(image_buffer);
        image_buffer = NULL;
    });

    fclose(image_file);
    image_file = NULL;

    struct imgst_file imgst;
    // do_open will initialize the struct imgst_file
    M_EXIT_IF_ERR_DO_SOMETHING(
    do_open(fileName, "rb+", &imgst), {
        free(image_buffer);
        image_buffer = NULL;
    });
    // inserts image
    int err = do_insert(image_buffer, file_size, img_id, &imgst);
    free(image_buffer);
    image_buffer = NULL;
    // closes file
    do_close(&imgst);
    return err;
}

/********************************************************************//**
 * Reads the content of an image from a imgStore.
 */
int do_read_cmd(int args, char* argv[])
{

    // checks arguments
    const char* fileName = argv[1];
    const char* img_id = argv[2];
    M_EXIT_IF_TOO_LONG(fileName, MAX_IMGST_NAME);
    M_CHECK_IMG_ID(img_id);

    int resolution_code = RES_ORIG;
    if(args > 3) {
        const char* resolution = argv[3];
        resolution_code = resolution_atoi(resolution);
        M_REQUIRE(resolution_code != -1, ERR_RESOLUTIONS, "unrecognised resolution", NULL);
    }

    struct imgst_file imgst_file;
    M_EXIT_IF_ERR(do_open(fileName, "rb+", &imgst_file));

    char* image_buffer = NULL;
    uint32_t image_size = 0;
    M_EXIT_IF_ERR_DO_SOMETHING(
    do_read(img_id, resolution_code, &image_buffer, &image_size, &imgst_file),
    do_close(&imgst_file));

    char* image_name = NULL;
    M_EXIT_IF_ERR_DO_SOMETHING(
    create_name(img_id, resolution_code, &image_name), {
        do_close(&imgst_file);
        free(image_buffer);
        image_buffer = NULL;
    });

    FILE* image = fopen(image_name, "wb");
    M_IO_CHECK_WITH_CODE(
    image == NULL, {
        free(image_name);
        image_name = NULL;
        free(image_buffer);
        image_buffer = NULL;
        do_close(&imgst_file);
    });

    M_IO_CHECK_WITH_CODE(
    fwrite(image_buffer, image_size, 1, image) != 1, {
        fclose(image);
        image = NULL;
        free(image_name);
        image_name = NULL;
        free(image_buffer);
        image_buffer = NULL;
        do_close(&imgst_file);
    });

    fclose(image);
    image = NULL;
    free(image_name);
    image_name = NULL;
    free(image_buffer);
    image_buffer = NULL;
    do_close(&imgst_file);

    return ERR_NONE;
}

/**
 * Do some clean-up for imgStore file handling.
 */
int do_gc_cmd(int args _unused, char* argv[])
{
    // checks arguments
    const char* fileName = argv[1];
    const char* temp_fileName = argv[2];
    M_CHECK_IMGSTR_NAME(fileName);
    size_t length_temp_filename = strlen(temp_fileName);
    M_REQUIRE(length_temp_filename > 0, ERR_INVALID_ARGUMENT, "image filename is empty", NULL);
    M_REQUIRE(length_temp_filename < FILENAME_MAX, ERR_INVALID_FILENAME, "image filename is too long", NULL);

    return do_gbcollect (fileName, temp_fileName);
    //just make sure that files are closed and removed in do_gbcollect?
}


#define NBR_OF_CMDS 7
static const command_mapping commands[NBR_OF_CMDS] = {
    {"list", do_list_cmd, 1},
    {"create", do_create_cmd, 1},
    {"help", help, 0},
    {"delete", do_delete_cmd, 2},
    {"insert", do_insert_cmd, 3},
    {"read", do_read_cmd, 2},
    {"gc", do_gc_cmd, 2}
};
/********************************************************************//**
 * MAIN
 */
int main (int argc, char* argv[])
{
    int ret = 0;
    if (argc < 2) {
        ret = ERR_NOT_ENOUGH_ARGUMENTS;
    } else {
        // start vips
        if (VIPS_INIT(argv[0]) != 0) {
            ret = ERR_IMGLIB;
            vips_error_exit("Error while starting Vips");
        } else {
            argc--;
            argv++; // skips command call name
            char* cmd = argv[0];
            int called = 0;
            for(int i = 0; i < NBR_OF_CMDS && called == 0; i++) {
                if (!strcmp(cmd, commands[i].name)) {
                    if (argc < commands[i].nbr_compulsory_args + 1) {
                        ret = ERR_NOT_ENOUGH_ARGUMENTS;
                    } else {
                        ret = commands[i].cmd(argc, argv);
                    }
                    called = 1;
                }
            }
            if(!called) {
                ret = ERR_INVALID_COMMAND;
            }
            // ends vips
            vips_shutdown();
        }
    }
    if (ret) {
        fprintf(stderr, "ERROR: %s\n", ERR_MESSAGES[ret]);
        help(argc, argv);
    }

    return ret;
}
