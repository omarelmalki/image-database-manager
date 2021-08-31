#pragma once

/**
 * @file error.h
 * @brief Error codes for PPS course (CS-212)
 *
 * @author E. Bugnion, J.-C. Chappelier, V. Rousset
 * @date 2016-2021
 */

#ifdef DEBUG
#include <stdio.h> // for fprintf
#endif
#include <string.h> // strerror()
#include <errno.h>  // errno

#ifdef __cplusplus
extern "C" {
#endif

// ======================================================================
/**
 * @brief internal error codes.
 *
 */
typedef enum {
    ERR_CLANG_TYPE_FIX = -1, // this stupid value is to fix type to be int instead of unsigned on some compilers (e.g. clang version 8.0)
    ERR_NONE = 0, // no error

    ERR_IO,
    ERR_OUT_OF_MEMORY,
    ERR_NOT_ENOUGH_ARGUMENTS,
    ERR_INVALID_FILENAME,
    ERR_INVALID_COMMAND,
    ERR_INVALID_ARGUMENT,
    ERR_MAX_FILES,
    ERR_RESOLUTIONS,
    ERR_INVALID_IMGID,
    ERR_FULL_IMGSTORE,
    ERR_FILE_NOT_FOUND,
    NOT_IMPLEMENTED,
    ERR_DUPLICATE_ID,
    ERR_IMGLIB,
    ERR_DEBUG,

    NB_ERR // not an actual error but to have the total number of errors
} error_code;

// ======================================================================
/*
 * Helpers (macros)
 */

// ----------------------------------------------------------------------
/**
 * @brief debug_print macro is usefull to print message in DEBUG mode only.
 */
#ifdef DEBUG
#define debug_print(fmt, ...) \
    fprintf(stderr, __FILE__ ":%d:%s(): " fmt "\n", \
       __LINE__, __func__, __VA_ARGS__)
#else
#define debug_print(fmt, ...) \
    do {} while(0)
#endif

// ----------------------------------------------------------------------
/**
 * @brief M_EXIT macro is usefull to return an error code from a function with a debug message.
 *        Example usage:
 *           M_EXIT(ERR_INVALID_ARGUMENT, "unable to do something decent with value %lu", i);
 */
#define M_EXIT(error_code, fmt, ...)  \
    do { \
        debug_print(fmt, __VA_ARGS__); \
        return error_code; \
    } while(0)

// ----------------------------------------------------------------------
/**
 * @brief M_EXIT_ERR_NOMSG macro is a kind of M_EXIT, indicating (in debug mode)
 *        only the error message corresponding to the error code.
 *        Example usage:
 *            M_EXIT_ERR_NOMSG(ERR_INVALID_ARGUMENT);
 */
#define M_EXIT_ERR_NOMSG(error_code)   \
    M_EXIT(error_code, "error: %s", ERR_MESSAGES[error_code - ERR_NONE])

// ----------------------------------------------------------------------
/**
 * @brief M_EXIT_IF_ERR_DO_SOMETHING macro is a shortcut for M_EXIT_ERR on some call,
 *        which does something before exiting;
 *        i.e. if call's return value is different from ERR_NONE, first
 *        executing some code (typically to disallocated ressources)
 *        and tjem displaying the call which causes the error before exiting.
 *        Example usage:
 *            M_EXIT_IF_ERR_DO_SOMETHING(foo(a, b), free(p));
 */
#define M_EXIT_IF_ERR_DO_SOMETHING(call_one, call_two) \
    do { \
        const error_code retVal = call_one; \
        if (retVal != ERR_NONE) { \
            call_two; \
            M_EXIT_ERR(retVal, ", when calling: %s", #call_one); \
        } \
    } while(0)

// ----------------------------------------------------------------------
/**
 * @brief M_EXIT_ERR macro is a kind of M_EXIT, indicating (in debug mode)
 *        the error message corresponding to the error code, followed
 *        by the message passed in argument.

 *        Example usage:
 *            M_EXIT_ERR(ERR_INVALID_ARGUMENT, "unable to generate from %lu", i);
 */
#define M_EXIT_ERR(error_code, fmt, ...)   \
    M_EXIT(error_code, "error: %s" fmt, ERR_MESSAGES[error_code - ERR_NONE], __VA_ARGS__)

// ----------------------------------------------------------------------
/**
 * @brief M_EXIT_IF macro calls M_EXIT_ERR only if provided test is true.
 *        Example usage:
 *            M_EXIT_IF(i > 3, ERR_INVALID_ARGUMENT, "third argument value (%lu) is too high (> 3)", i);
 */
#define M_EXIT_IF(test, error_code, fmt, ...)   \
    do { \
        if (test) { \
            M_EXIT_ERR(error_code, fmt, __VA_ARGS__); \
        } \
    } while(0)

// ----------------------------------------------------------------------
/**
 * @brief M_EXIT_IF_ERR_MSG macro is a shortcut for M_EXIT_IF testing if some error occured,
 *        i.e. if error `ret` is different from ERR_NONE, and displaying some explaination.
 *        Without explaination, see M_EXIT_IF_ERR below.
 *        Example usage:
 *            M_EXIT_IF_ERR_MSG(foo(a, b), "calling foo()");
 */
#define M_EXIT_IF_ERR_MSG(ret, name)   \
   do { \
        const error_code retVal = ret; \
        M_EXIT_IF(retVal != ERR_NONE, retVal, "%s", name);   \
    } while(0)

// ----------------------------------------------------------------------
/**
 * @brief M_EXIT_IF_ERR macro is like M_EXIT_IF_ERR_MSG but where the message is the full call.
 *        Example usage:
 *            M_EXIT_IF_ERR(foo(a, b));
 */
#define M_EXIT_IF_ERR(ret)   \
  M_EXIT_IF_ERR_MSG(ret, ", when calling: " #ret)

// ----------------------------------------------------------------------
/**
 * @brief M_EXIT_IF_NULL macro is usefull to warn (and stop) when NULL pointers are detected.
 *        size is typically the allocation size.
 *        Example usage:
 *            M_EXIT_IF_NULL(p = malloc(size), size);
 */
#define M_EXIT_IF_NULL(var, size)   \
    M_EXIT_IF((var) == NULL, ERR_OUT_OF_MEMORY, ", cannot allocate %zu bytes for %s", size, #var)

// ----------------------------------------------------------------------
/**
 * @brief M_EXIT_IF_TOO_LONG macro is usefull to warn (and stop) when a string is more than size characters long.
 *        Example usage:
 *            M_EXIT_IF_TOO_LONG(answer, INPUT_MAX_SIZE);
 */
#define M_EXIT_IF_TOO_LONG(var, size)   \
    M_EXIT_IF(strlen(var) > size, ERR_INVALID_ARGUMENT, ", given %s is larger than %d bytes", #var, size)

// ----------------------------------------------------------------------
/**
 * @brief M_EXIT_IF_TRAILING macro is usefull to test whether some trailing character(s) remain(s) in file.
 *        Example usage:
 *            M_EXIT_IF_TRAILING(file);
 */
#define M_EXIT_IF_TRAILING(file)   \
    M_EXIT_IF(getc(file) != '\n', ERR_INVALID_ARGUMENT, ", trailing chars on \"%s\"", #file)

// ----------------------------------------------------------------------
/**
 * @brief M_EXIT_IF_TRAILING_WITH_CODE macro is similar to M_EXIT_IF_TRAILING but allows to
 *        run some code before exiting.
 *        Example usage:
 *           M_EXIT_IF_TRAILING_WITH_CODE(stdin, { free(something); } );
 */
#define M_EXIT_IF_TRAILING_WITH_CODE(file, code)   \
    do { \
        if (getc(file) != '\n') { \
            code; \
            M_EXIT_ERR(ERR_INVALID_ARGUMENT, ", trailing chars on \"%s\"", #file); \
        } \
    } while(0)

// ----------------------------------------------------------------------
/**
 * @brief M_REQUIRE macro is similar to M_EXIT_IF but with two differences:
 *         1) making use of the NEGATION of the test (thus "require")
 *         2) making use of M_EXIT rather than M_EXIT_ERR
 *        Example usage:
 *            M_REQUIRE(i <= 3, ERR_INVALID_ARGUMENT, "input value (%lu) is too high (> 3)", i);
 */
#define M_REQUIRE(test, error_code, fmt, ...)   \
    do { \
        if (!(test)) { \
             M_EXIT(error_code, fmt, __VA_ARGS__); \
        } \
    } while(0)

// ----------------------------------------------------------------------
/**
 * @brief M_REQUIRE_NO_ERR macro is a M_REQUIRE test on a return.
 *        This might be useful to return an error if one occured.
 *        Example usage:
 *            M_REQUIRE_NO_ERR(ERR_INVALID_ARGUMENT);
 */
#define M_REQUIRE_NO_ERR(error_code) \
    M_REQUIRE(error_code == 0, error_code, "%s %s", "Recieved error code: ", ERR_MESSAGES[error_code - ERR_NONE])

// ----------------------------------------------------------------------
/**
 * @brief M_REQUIRE_NO_ERRNO macro is a M_REQUIRE test on errno.
 *        This might be useful to change errno into our own error code.
 *        Example usage:
 *            M_REQUIRE_NO_ERRNO(ERR_INVALID_ARGUMENT);
 */
#define M_REQUIRE_NO_ERRNO(error_code) \
    M_REQUIRE(errno == 0, error_code, "%s", strerror(errno))

// ----------------------------------------------------------------------
/**
 * @brief M_REQUIRE_NON_NULL_CUSTOM_ERR macro is usefull to check for non NULL argument.
 *        Example usage:
 *            M_REQUIRE_NON_NULL_CUSTOM_ERR(key, ERR_IO);
 */
#define M_REQUIRE_NON_NULL_CUSTOM_ERR(arg, error_code) \
    M_REQUIRE(arg != NULL, error_code, "parameter %s is NULL", #arg)

// ----------------------------------------------------------------------
/**
 * @brief M_REQUIRE_NON_NULL macro is a shortcut for
 *        M_REQUIRE_NON_NULL_CUSTOM_ERR returning ERR_INVALID_ARGUMENT.
 *        Example usage:
 *            M_REQUIRE_NON_NULL(key);
 */
#define M_REQUIRE_NON_NULL(arg) \
    M_REQUIRE_NON_NULL_CUSTOM_ERR(arg, ERR_INVALID_ARGUMENT)

// ----------------------------------------------------------------------
/**
 * @brief M_CHECK_WITH_CODE macro checks a condition, executes code and returns error_code
 *
 */
#define M_CHECK_WITH_CODE(test, code, error_code) \
    do { \
        if (test) { \
            code; \
            M_EXIT_ERR_NOMSG(error_code); \
        } \
    } while(0)

// ----------------------------------------------------------------------
/**
 * @brief M_IMGLIB_CHECK_WITH_CODE macro is a shortcut for M_CHECK_WITH_CODE
 *        returning ERR_IMGLIB
 *
 */
#define M_IMGLIB_CHECK_WITH_CODE(test, code) \
    M_CHECK_WITH_CODE(test, code, ERR_IMGLIB)

// ----------------------------------------------------------------------
/**
 * @brief M_IO_CHECK_WITH_CODE macro is a shortcut for M_CHECK_WITH_CODE
 *        returning ERR_IO
 *
 */
#define M_IO_CHECK_WITH_CODE(test, code) \
    M_CHECK_WITH_CODE(test, code, ERR_IO)

// ----------------------------------------------------------------------
/**
 * @brief M_CHECK_IMGSTR_NAME macro verifies the correctness of the filename
 *
 */
#define M_CHECK_IMGSTR_NAME(fileName) \
    M_REQUIRE(strlen(fileName) <= MAX_IMGST_NAME, ERR_INVALID_FILENAME, "Name too long", NULL); \
    M_REQUIRE(strlen(fileName) > 0, ERR_INVALID_FILENAME, "Empty Name", NULL)

// ----------------------------------------------------------------------
/**
 * @brief M_CHECK_IMG_ID macro verifies the correctness of the image id
 *
 */
#define M_CHECK_IMG_ID(img_id) \
    M_REQUIRE(strlen(img_id) <= MAX_IMG_ID, ERR_INVALID_IMGID, "Image ID too long", NULL); \
    M_REQUIRE(strlen(img_id) > 0, ERR_INVALID_IMGID, "Empty image ID", NULL)

// ----------------------------------------------------------------------
/**
 * @brief M_REQUIRE_NON_NULL_IMGST_FILE macro verifies the validity of an imgst_file given as argument
 *
 */
#define M_REQUIRE_NON_NULL_IMGST_FILE(imgst_file) \
    M_REQUIRE_NON_NULL(imgst_file); \
    M_REQUIRE_NON_NULL(imgst_file->metadata); \
    M_REQUIRE_NON_NULL(imgst_file->file)

// ----------------------------------------------------------------------
/**
 * @brief M_CHECK_IMGST_FILE_INDEX macro verifies the validity of an image of index of an imgst_file given as argument
 *
 */
#define M_CHECK_IMGST_FILE_INDEX(imgst_file, index) \
    M_REQUIRE(index < imgst_file->header.max_files, ERR_INVALID_ARGUMENT, "index out of bounds", NULL); \
    M_REQUIRE(imgst_file->metadata[index].is_valid == NON_EMPTY, ERR_INVALID_ARGUMENT, "invalid index", NULL)

// ----------------------------------------------------------------------
/**
 * @brief M_IMGLIB_CHECK macro is a shortcut for
 *        M_EXIT_IF returning ERR_IMGLIB.
 */
#define M_IMGLIB_CHECK(ret, expected) \
    M_EXIT_IF((ret) != (expected), ERR_IMGLIB, ERR_MESSAGES[ERR_IMGLIB], NULL)

// ----------------------------------------------------------------------
/**
 * @brief M_IMGLIB_CHECK macro is a shortcut for
 *        M_EXIT_IF returning ERR_IMGLIB.
 */
#define M_VIPS_CHECK(ret) \
    M_IMGLIB_CHECK(ret, 0)

// ----------------------------------------------------------------------
/**
 * @brief M_IO_CHECK macro is a shortcut for
 *        M_EXIT_IF returning ERR_IO.
 *
 */
#define M_IO_CHECK(ret, expected) \
    M_EXIT_IF((ret) != (expected), ERR_IO, ERR_MESSAGES[ERR_IO], NULL)

// ======================================================================
/**
* @brief internal error messages. defined in error.c
*
*/
extern
const char* const ERR_MESSAGES[];

#ifdef __cplusplus
}
#endif
