/* Copyright © 2021 Piepacker Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef LIBRETRO_SCRIPT_H_
#define LIBRETRO_SCRIPT_H_

#include <libretro.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef RETRO_SCRIPT_API
#   if defined(_WIN32) || defined(__CYGWIN__) || defined(__MINGW32__) 
#        ifdef __GNUC__
#            define RETRO_SCRIPT_API RETRO_CALLCONV __attribute__((__dllexport__))
#        else
#            define RETRO_SCRIPT_API RETRO_CALLCONV __declspec(dllexport)
#        endif
#    else
#        if defined(__GNUC__) && __GNUC__ >= 4
#            define RETRO_SCRIPT_API RETRO_CALLCONV __attribute__((__visibility__("default")))
#        else
#            define RETRO_SCRIPT_API RETRO_CALLCONV
#        endif
#    endif
#endif

// incremented only for backward-incompatible changes
#define RETRO_SCRIPT_API_VERSION         1

// this should be called once each time a new core is loaded, before any of the 
// following functions are called.
// returns the RETRO_SCRIPT_API_VERSION
RETRO_SCRIPT_API uint32_t retro_script_init();

// this only needs to be called once before closing the front-end.
// there is otherwise no need to call it when a core is unloaded or loaded.
RETRO_SCRIPT_API void retro_script_deinit();

// returns error text if an error occured, or nullptr if no error.
RETRO_SCRIPT_API const char* retro_script_get_error();

#define RETRO_SCRIPT_DECLT(name) decl_##name##_t
#define RETRO_SCRIPT_INTERCEPT(rtype, name, ...) \
typedef rtype (RETRO_CALLCONV *RETRO_SCRIPT_DECLT(name))(__VA_ARGS__); \
RETRO_SCRIPT_API RETRO_SCRIPT_DECLT(name) retro_script_intercept_##name(RETRO_SCRIPT_DECLT(name));

// these intercept replacements should all be called before retro_init
// example usage: core.retro_set_environment = retro_script_intercept_retro_set_environment(core.retro_set_environment)
RETRO_SCRIPT_INTERCEPT(void, retro_set_environment, retro_environment_t);
RETRO_SCRIPT_INTERCEPT(void*, retro_get_memory_data, unsigned id);
RETRO_SCRIPT_INTERCEPT(size_t, retro_get_memory_size, unsigned id);
RETRO_SCRIPT_INTERCEPT(void, retro_init, void);
RETRO_SCRIPT_INTERCEPT(void, retro_deinit, void);
RETRO_SCRIPT_INTERCEPT(void, retro_run, void);
RETRO_SCRIPT_INTERCEPT(void, retro_set_input_poll, retro_input_poll_t);
RETRO_SCRIPT_INTERCEPT(void, retro_set_input_state, retro_input_state_t);

typedef uint32_t retro_script_id_t;

// loads a lua script, returning a handle.
// returns 0 if script load fails.
RETRO_SCRIPT_API retro_script_id_t retro_script_load_lua(const char* path_to_script);

// loads a script, returning a handle.
// the provided callback is called before the script executes, to allow the
// front-end to modify the lua context, e.g. to add functionality.
// During the callback, the shallowest element on the stack will be the `retro` table.
// It is advised to add a subfield to the retro table for the front-end, e.g. `retro.ludo` or `retro.arch`, etc.
// callback should return 1 if successful, 0 on error.
// returns 0 if script load fails.
struct lua_State;
typedef int (RETRO_CALLCONV *retro_script_setup_lua_t)(struct lua_State* L);
RETRO_SCRIPT_API retro_script_id_t retro_script_load_lua_special(const char* path_to_script, retro_script_setup_lua_t);

// set callback to be invoked on a lua error during pcall.
// preferably, should not print anything, should just manipulate the error on the stack and return.
typedef int (*lua_CFunction) (struct lua_State *L);
RETRO_SCRIPT_API void retro_script_set_lua_error_handler(lua_CFunction);
RETRO_SCRIPT_API lua_CFunction retro_script_get_lua_error_handler(void);

// invoked if a lua error from a script reaches top-level without being caught.
// default is to print the error.
typedef void (*retro_script_lua_uncaught_error_cb) (retro_script_id_t script_id, int lua_status_code, const char* error_msg);
RETRO_SCRIPT_API void retro_script_set_lua_uncaught_error_handler(retro_script_lua_uncaught_error_cb cb);

#ifdef __cplusplus
}
#endif

#endif