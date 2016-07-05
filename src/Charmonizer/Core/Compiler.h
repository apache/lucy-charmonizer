/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* Charmonizer/Core/Compiler.h
 */

#ifndef H_CHAZ_COMPILER
#define H_CHAZ_COMPILER

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include "Charmonizer/Core/Defines.h"
#include "Charmonizer/Core/CFlags.h"

#define CHAZ_CC_BINFMT_ELF      1
#define CHAZ_CC_BINFMT_MACHO    2
#define CHAZ_CC_BINFMT_PE       3

/* Attempt to compile and link an executable.  Return true if the executable
 * file exists after the attempt.
 */
int
chaz_CC_compile_exe(const char *source_path, const char *exe_path,
                    const char *code);

/* Attempt to compile an object file.  Return true if the object file
 * exists after the attempt.
 */
int
chaz_CC_compile_obj(const char *source_path, const char *obj_path,
                    const char *code);

/* Attempt to compile the supplied source code and return true if the
 * effort succeeds.
 */
int
chaz_CC_test_compile(const char *source);

/* Attempt to compile and link the supplied source code and return true if
 * the effort succeeds.
 */
int
chaz_CC_test_link(const char *source);

/* Attempt to compile the supplied source code.  If successful, capture the
 * output of the program and return a pointer to a newly allocated buffer.
 * If the compilation fails, return NULL.  The length of the captured
 * output will be placed into the integer pointed to by [output_len].
 */
char*
chaz_CC_capture_output(const char *source, size_t *output_len);

/** Return true if macro is defined.
 */
int
chaz_CC_has_macro(const char *macro);

/** Initialize the compiler environment.
 */
void
chaz_CC_init(const char *cc_command, const char *cflags);

/* Clean up the environment.
 */
void
chaz_CC_clean_up(void);

/* Accessor for the compiler executable's string representation.
 */
const char*
chaz_CC_get_cc(void);

/* Accessor for `cflags`.
 */
const char*
chaz_CC_get_cflags(void);

/* Accessor for `extra_cflags`.
 */
chaz_CFlags*
chaz_CC_get_extra_cflags(void);

/* Accessor for `temp_cflags`.
 */
chaz_CFlags*
chaz_CC_get_temp_cflags(void);

/* Return a new CFlags object.
 */
chaz_CFlags*
chaz_CC_new_cflags(void);

/* Return the binary format.
 */
int
chaz_CC_binary_format(void);

/* Return the extension for an executable.
 */
const char*
chaz_CC_exe_ext(void);

/* Return the extension for a shared (dynamic) library.
 */
const char*
chaz_CC_shared_lib_ext(void);

/* Return the extension for a static library.
 */
const char*
chaz_CC_static_lib_ext(void);

/* Return the extension for an import library (Windows).
 */
const char*
chaz_CC_import_lib_ext(void);

/* Return the extension for a compiled object.
 */
const char*
chaz_CC_obj_ext(void);

int
chaz_CC_gcc_version_num(void);

const char*
chaz_CC_gcc_version(void);

int
chaz_CC_msvc_version_num(void);

int
chaz_CC_sun_c_version_num(void);

int
chaz_CC_is_cygwin(void);

int
chaz_CC_is_mingw(void);

const char*
chaz_CC_link_command(void);

/* Create a command for building a static library.
 *
 * @param target The target library filename.
 * @param objects The list of object files to be archived in the library.
 */
char*
chaz_CC_format_archiver_command(const char *target, const char *objects);

/* Returns a "ranlib" command if valid.
 *
 * @param target The library filename.
 */
char*
chaz_CC_format_ranlib_command(const char *target);

/** Returns the filename for a shared library.
 *
 * @param dir The target directory or NULL for the current directory.
 * @param basename The name of the library without prefix and extension.
 * @param version The library version.
 */
char*
chaz_CC_shared_lib_filename(const char *dir, const char *basename,
                            const char *version);

/** Returns the filename for an import library.
 *
 * @param dir The target directory or NULL for the current directory.
 * @param basename The name of the library without prefix and extension.
 * @param version The library version.
 */
char*
chaz_CC_import_lib_filename(const char *dir, const char *basename,
                            const char *version);

/** Returns the filename for an MSVC export file.
 *
 * @param dir The target directory or NULL for the current directory.
 * @param basename The name of the library without prefix and extension.
 * @param version The library version.
 */
char*
chaz_CC_export_filename(const char *dir, const char *basename,
                        const char *version);

/** Returns the filename for a static library.
 *
 * @param dir The target directory or NULL for the current directory.
 * @param basename The name of the library without prefix and extension.
 */
char*
chaz_CC_static_lib_filename(const char *dir, const char *basename);

#ifdef __cplusplus
}
#endif

#endif /* H_CHAZ_COMPILER */


