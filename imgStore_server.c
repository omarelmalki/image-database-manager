/**
 * @file imgStore_server.c
 * @brief webserver code for imgStore
 *
 * @copyright Copyright (c) 2021
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include "mongoose.h"
#include "imgStore.h"
#include "error.h"
#include "util.h"
#include <vips/vips.h> // for VIPS_INIT and vips shutdown


/*
 * Launch it with: ./imgStore_server NAME.jpg
 * Then with a browser go to
 *     http://localhost:8000/original
 * to see the original image, and to
 *     http://localhost:8000/thumbnail
 * to see its thumbnail version.
 */

// ======================================================================
// http server
static const char *s_root_dir = ".";
static const char *s_listening_address = "http://localhost:8000";
// Handle interrupts, like Ctrl-C
static int s_signo;
static void signal_handler(int signo)
{
    s_signo = signo;
}

/**
 * variables needed for handling
 */
// struct imgst_file
static struct imgst_file imgst_file;

/********************************************************************//**
* A handler is a function that we use to handle an http call
*********************************************************************** */
typedef void(*handler)(struct mg_connection *nc, struct mg_http_message *hm);

/********************************************************************//**
 * A handler_mapping is used to map a handler to its name as it will
 * be called in the uri.
 ********************************************************************** */
typedef struct {
    const char* uri;
    const handler cmd;
    const char* type;
} handler_mapping;

/**
 * @brief Error message routine
 *
 * @param nc struct mg_correction
 * @param error to print
 */
static void mg_error_msg(struct mg_connection* nc, int error)
{
    mg_http_reply(nc, 500, "",
                  "Error: %s", ERR_MESSAGES[error]);
}

/**
 * @brief Handles a list call
 *
 * @param nc struct mg_connection connection that received a list call event
 */
static void handle_list_call(struct mg_connection *nc, struct mg_http_message *hm _unused)
{
    char* imgstore_json = do_list(&imgst_file, JSON);
    mg_http_reply(nc, 200,
                  "Content-Type: application/json\r\n", imgstore_json, NULL);
    free(imgstore_json);
}


#define MAX_RES_NAME 6 // double check
/**
 * @brief Handles a read call
 *
 * @param nc struct mg_connection connection that received a list call event
 */
static void handle_read_call(struct mg_connection *nc, struct mg_http_message *hm)
{
    // initialize name and img_id
    char res_name[MAX_RES_NAME + 1] = "";
    char img_id[MAX_IMG_ID + 1] = "";

    // get arguments
    int res_l = mg_http_get_var(&hm->query, "res", res_name, MAX_RES_NAME);
    int img_id_l = mg_http_get_var(&hm->query, "img_id", img_id, MAX_IMG_ID);

    // check validity of arguments
    if (res_l > 0 && img_id_l > 0) {
        // read
        int resolution_code = resolution_atoi(res_name);
        if (resolution_code == -1) {
            mg_error_msg(nc, ERR_RESOLUTIONS);
        }
        char* image_buffer = NULL;
        uint32_t image_size = 0;
        int err_read = do_read(img_id, resolution_code, &image_buffer, &image_size, &imgst_file);
        if (err_read != ERR_NONE) {
            mg_error_msg(nc, err_read);
        } else {
            mg_printf(
            nc,
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: %d\r\n"
            "Content-Type: image/jpeg\r\n\r\n",
            image_size
            );
            mg_send(nc, image_buffer, image_size);
            free(image_buffer);
            image_buffer = NULL;
        }
    } else {
        mg_error_msg(nc, ERR_INVALID_ARGUMENT);
    }
}


static void handle_delete_call(struct mg_connection *nc, struct mg_http_message *hm)
{
    // initialize img_id
    char img_id[MAX_IMG_ID + 1] = "";
    // get img_id
    int img_id_l = mg_http_get_var(&hm->query, "img_id", img_id, MAX_IMG_ID);
    // check arguments
    if(img_id_l > 0) {
        // delete
        int err_delete = do_delete(img_id, &imgst_file);
        if(err_delete != ERR_NONE) {
            mg_error_msg(nc, err_delete);
        } else {
            mg_printf(nc,
                      "HTTP/1.1 302 Found\r\n"
                      "Location: %s/index.html\r\n\r\n",
                      s_listening_address);
            nc->is_draining = 1;
        }
    } else {
        mg_error_msg(nc, ERR_INVALID_IMGID);
    }
}


#define MAX_OFFSET 20 // 2^64
#define MAX_FILENAME 200 // maximal length retained in mg_http_upload (needed to have the same file name)
#define DIRECTORY "/tmp"
#define DIRECTORY_SIZE 4
static void handle_insert_call(struct mg_connection *nc, struct mg_http_message *hm)
{
    // if there is still data to insert, upload
    if (hm->body.len > 0) {
        mg_http_upload(nc, hm, DIRECTORY);
        // else call do_insert
    } else {
        // initialize arguments
        char offset[MAX_OFFSET + 1] = "", name[MAX_FILENAME] = "", path[MAX_FILENAME + DIRECTORY_SIZE + 1] = "";

        // get arguments
        int offset_l = mg_http_get_var(&hm->query, "offset", offset, MAX_OFFSET);
        int name_l = mg_http_get_var(&hm->query, "name", name, MAX_FILENAME);

        // check arguments
        if (offset_l > 0 && name_l > 0) {
            // insert
            uint32_t image_size = atouint32(offset);
            void* buffer = malloc(image_size);
            snprintf(path, sizeof(path), "%s/%s", DIRECTORY, name);
            if(name_l > MAX_IMG_ID) {
                mg_error_msg(nc, ERR_INVALID_IMGID);
                if (remove(path) != 0) mg_error_msg(nc, ERR_IO);
            } else {
                if (buffer == NULL) {
                    mg_error_msg(nc, ERR_OUT_OF_MEMORY);
                    if (remove(path) != 0) mg_error_msg(nc, ERR_IO);
                } else {
                    FILE* image_file = fopen(path, "rb+");
                    if(image_file == NULL) {
                        free(buffer);
                        buffer = NULL;
                        if (remove(path) != 0) mg_error_msg(nc, ERR_IO);
                        mg_error_msg(nc, ERR_IO);
                    } else {
                        if(fread(buffer, image_size, 1, image_file) != 1) {
                            free(buffer);
                            buffer = NULL;
                            fclose(image_file);
                            mg_error_msg(nc, ERR_IO);
                            if (remove(path) != 0) mg_error_msg(nc, ERR_IO);
                        } else {
                            fclose(image_file);
                            image_file = NULL;
                            if (remove(path) != 0) mg_error_msg(nc, ERR_IO);
                            int err = do_insert(buffer, image_size, name, &imgst_file);
                            free(buffer);
                            buffer = NULL;
                            if (err != ERR_NONE) {
                                mg_error_msg(nc, err);
                            } else {
                                mg_printf(nc,
                                          "HTTP/1.1 302 Found\r\n"
                                          "Location: %s/index.html\r\n\r\n",
                                          s_listening_address);
                                nc->is_draining = 1;
                            }
                        }
                    }
                }
            }
        } else {
            mg_error_msg(nc, ERR_INVALID_ARGUMENT);
        }
    }
}



#define NBR_OF_HANDLERS 4
static const handler_mapping handler_mappings[NBR_OF_HANDLERS] = {
    {"/imgStore/list", handle_list_call, "GET"},
    {"/imgStore/read", handle_read_call, "GET"},
    {"/imgStore/delete", handle_delete_call, "GET"},
    {"/imgStore/insert", handle_insert_call, "POST"}
};
// ======================================================================
/**
 * @brief Handles server events (eg HTTP requests) and deals with the different urls.
 *
 * For more check https://cesanta.com/docs/#event-handler-function
 */
static void imgst_event_handler(struct mg_connection *nc,
                                int ev,
                                void *ev_data,
                                void *fn_data
                               )
{
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    int matched = 0;
    switch (ev) {
    case MG_EV_HTTP_MSG:
        for(int i = 0; i < NBR_OF_HANDLERS && !matched; i++) {
            if (mg_http_match_uri(hm, handler_mappings[i].uri) && !mg_vcmp(&hm->method, handler_mappings[i].type)) {
                handler_mappings[i].cmd(nc, hm);
                ++matched;
            }
        }
        if(!matched) {
            struct mg_http_serve_opts opts = {.root_dir = s_root_dir};
            mg_http_serve_dir(nc, ev_data, &opts);
        }
    }
    (void) fn_data;
}

// ======================================================================
int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "%s", ERR_MESSAGES[ERR_NOT_ENOUGH_ARGUMENTS]);
        return EXIT_FAILURE;
    } else if (argc > 2) {
        fprintf(stderr, "%s", ERR_MESSAGES[ERR_INVALID_ARGUMENT]);
        return EXIT_FAILURE;
    }
    const char* imgStore_filename = argv[1];

    /* Create server */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    if (mg_http_listen(&mgr, s_listening_address, imgst_event_handler, NULL) == NULL) {
        fprintf(stderr, "Error starting server on address %s\n", s_listening_address);
        return EXIT_FAILURE;
    }
    /* Start */
    if (VIPS_INIT(argv[0]) != 0) {
        mg_mgr_free(&mgr);
        vips_error_exit("Error while starting Vips");
    }
    int err = ERR_NONE;
    if ((err = do_open(imgStore_filename, "rb+", &imgst_file)) != ERR_NONE) {
        fprintf(stderr, "%s", ERR_MESSAGES[err]);
        vips_shutdown();
        mg_mgr_free(&mgr);
        return EXIT_FAILURE;
    }

    printf("Starting imgStore server on %s\n", s_listening_address);
    print_header(&imgst_file.header);

    /* Poll */
    while (s_signo == 0) mg_mgr_poll(&mgr, 500);

    /* Cleanup */
    mg_mgr_free(&mgr);

    /* Exit */
    vips_shutdown();
    do_close(&imgst_file);
    printf("Exiting on signal %d", s_signo);

    return EXIT_SUCCESS;
}
