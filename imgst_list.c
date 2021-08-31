/**
 * @file imgst_list.c
 * @brief Implements do_list function
 *
 */
#include "imgStore.h"
#include "error.h"
#include <json-c/json.h>
#include <stdio.h>



/********************************************************************//**
 * Displays imgStore metadata or returns it as a JSON object converted to char*.
 */
char* do_list(const struct imgst_file* imgst_file, const enum do_list_mode mode)
{
    // check correctness of arguments
    if(imgst_file == NULL) return NULL;

    if(mode == STDOUT) {
        // prints header and metadata of valid files
        print_header(&imgst_file->header);
        if (imgst_file->header.num_files == 0) {
            puts("<< empty imgStore >>");
        } else {
            size_t i = 0;
            size_t valid = 0;
            while (valid < imgst_file->header.num_files && i < imgst_file->header.max_files) {
                if (imgst_file->metadata[i].is_valid == NON_EMPTY) {
                    print_metadata(&imgst_file->metadata[i]);
                    valid++;
                }
                i++;
            }
        }
        return NULL;
    } else if (mode == JSON) {
        json_object* array = NULL;
        array = json_object_new_array();


        size_t i = 0;
        uint32_t valid_read = 0;
        while(i < imgst_file->header.max_files && valid_read < imgst_file -> header.num_files) {
            if(imgst_file-> metadata[i].is_valid == NON_EMPTY) {
                valid_read++;
                //no return type in version 0.12.1 :
                json_object_array_add(array, json_object_new_string(imgst_file-> metadata[i].img_id));
            }
            i++;
        }
        json_object* obj = json_object_new_object();
        //no return type in version 0.12.1 :
        json_object_object_add(obj, "Images", array);

        const char* array_id = json_object_to_json_string(obj);
        char* res = calloc(strlen(array_id) + 1, 1);
        strcpy(res, array_id);

        while(json_object_put(obj) != 1) {}
        return res;

    } else {
        char* res = calloc(strlen("unimplemented do_list output mode") + 1, 1);
        strcpy(res, "unimplemented do_list output mode");
        return res;
    }
}
