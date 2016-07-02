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

/* Charmonizer/Core/Make.h
 */

#ifndef H_CHAZ_MAKE
#define H_CHAZ_MAKE

#ifdef __cplusplus
extern "C" {
#endif

#include "Charmonizer/Core/CFlags.h"

typedef struct chaz_MakeFile chaz_MakeFile;
typedef struct chaz_MakeVar chaz_MakeVar;
typedef struct chaz_MakeRule chaz_MakeRule;
typedef struct chaz_MakeBinary chaz_MakeBinary;

typedef void (*chaz_Make_list_files_callback_t)(const char *dir, char *file,
                                                void *context);

/** Initialize the environment.
 *
 * @param make_command Name of the make command. Auto-detect if NULL.
 */
void
chaz_Make_init(const char *make_command);

/** Clean up the environment.
 */
void
chaz_Make_clean_up(void);

/** Return the name of the detected 'make' executable.
 */
const char*
chaz_Make_get_make(void);

/** Return the type of shell used by the detected 'make' executable.
 */
int
chaz_Make_shell_type(void);

/** Recursively list files in a directory. For every file a callback is called
 * with the filename and a context variable.
 *
 * @param dir Directory to search in.
 * @param ext File extension to search for.
 * @param callback Callback to call for every matching file.
 * @param context Context variable to pass to callback.
 */
void
chaz_Make_list_files(const char *dir, const char *ext,
                     chaz_Make_list_files_callback_t callback, void *context);

/** MakeFile constructor.
 */
chaz_MakeFile*
chaz_MakeFile_new();

/** MakeFile destructor.
 */
void
chaz_MakeFile_destroy(chaz_MakeFile *makefile);

/** Add a variable to a makefile.
 *
 * @param makefile The makefile.
 * @param name Name of the variable.
 * @param value Value of the variable. Can be NULL if you want add content
 * later.
 * @return a MakeVar.
 */
chaz_MakeVar*
chaz_MakeFile_add_var(chaz_MakeFile *makefile, const char *name,
                      const char *value);

/** Add a rule to a makefile.
 *
 * @param makefile The makefile.
 * @param target The first target of the rule. Can be NULL if you want to add
 * targets later.
 * @param prereq The first prerequisite of the rule. Can be NULL if you want to
 * add prerequisites later.
 * @return a MakeRule.
 */
chaz_MakeRule*
chaz_MakeFile_add_rule(chaz_MakeFile *makefile, const char *target,
                       const char *prereq);

/** Return the rule for the 'clean' target.
 *
 * @param makefile The makefile.
 */
chaz_MakeRule*
chaz_MakeFile_clean_rule(chaz_MakeFile *makefile);

/** Return the rule for the 'distclean' target.
 *
 * @param makefile The makefile.
 */
chaz_MakeRule*
chaz_MakeFile_distclean_rule(chaz_MakeFile *makefile);

/** Add an executable. Returns a chaz_MakeBinary object.
 *
 * @param dir The target directory or NULL for the current directory.
 * @param basename The name of the executable without extension.
 */
chaz_MakeBinary*
chaz_MakeFile_add_exe(chaz_MakeFile *self, const char *dir,
                      const char *basename);

/** Add a shared library. The library will be built in the current directory.
 * Returns a chaz_MakeBinary object.
 *
 * @param dir The target directory or NULL for the current directory.
 * @param basename The name of the library without prefix and extension.
 * @param version The version of the library.
 * @param major_version The major version of the library.
 */
chaz_MakeBinary*
chaz_MakeFile_add_shared_lib(chaz_MakeFile *self, const char *dir,
                             const char *basename, const char *version,
                             const char *major_version);

/** Add a static library. The library will be built in the current directory.
 * Returns a chaz_MakeBinary object.
 *
 * @param dir The target directory or NULL for the current directory.
 * @param basename The name of the library without prefix and extension.
 */
chaz_MakeBinary*
chaz_MakeFile_add_static_lib(chaz_MakeFile *self, const char *dir,
                             const char *basename);

/** Add a rule to build the lemon parser generator.
 *
 * @param makefile The makefile.
 * @param dir The lemon directory.
 */
chaz_MakeBinary*
chaz_MakeFile_add_lemon_exe(chaz_MakeFile *makefile, const char *dir);

/** Add a rule for a lemon grammar.
 *
 * @param makefile The makefile.
 * @param base_name The filename of the grammar without extension.
 */
chaz_MakeRule*
chaz_MakeFile_add_lemon_grammar(chaz_MakeFile *makefile,
                                const char *base_name);

/** Write the makefile to a file named 'Makefile' in the current directory.
 *
 * @param makefile The makefile.
 */
void
chaz_MakeFile_write(chaz_MakeFile *makefile);

/** Append content to a makefile variable. The new content will be separated
 * from the existing content with whitespace.
 *
 * @param var The variable.
 * @param element The additional content.
 */
void
chaz_MakeVar_append(chaz_MakeVar *var, const char *element);

/** Add another target to a makefile rule.
 *
 * @param rule The rule.
 * @param target The additional rule.
 */
void
chaz_MakeRule_add_target(chaz_MakeRule *rule, const char *target);

/** Add another prerequisite to a makefile rule.
 *
 * @param rule The rule.
 * @param prereq The additional prerequisite.
 */
void
chaz_MakeRule_add_prereq(chaz_MakeRule *rule, const char *prereq);

/** Add a command to a rule.
 *
 * @param rule The rule.
 * @param command The additional command.
 */
void
chaz_MakeRule_add_command(chaz_MakeRule *rule, const char *command);

/** Add a command to be executed with a special runtime library path.
 *
 * @param rule The rule.
 * @param command The additional command.
 * @param ... NULL-terminated list of library directories.
 */
void
chaz_MakeRule_add_command_with_libpath(chaz_MakeRule *rule,
                                       const char *command, ...);

/** Add a command to remove one or more files.
 *
 * @param rule The rule.
 * @param files The list of files.
 */
void
chaz_MakeRule_add_rm_command(chaz_MakeRule *rule, const char *files);

/** Add a command to remove one or more directories.
 *
 * @param rule The rule.
 * @param dirs The list of directories.
 */
void
chaz_MakeRule_add_recursive_rm_command(chaz_MakeRule *rule, const char *dirs);

/** Add one or more commands to call another makefile recursively.
 *
 * @param rule The rule.
 * @param dir The directory in which to call the makefile.
 * @param target The target to call. Pass NULL for the default target.
 */
void
chaz_MakeRule_add_make_command(chaz_MakeRule *rule, const char *dir,
                               const char *target);

/** Add a source file for the binary.
 *
 * @param dir The source directory or NULL for the current directory.
 * @param filename The filename.
 */
void
chaz_MakeBinary_add_src_file(chaz_MakeBinary *self, const char *dir,
                             const char *filename);

/** Add all .c files in a directory as sources for the binary.
 *
 * @param path The path to the directory.
 */
void
chaz_MakeBinary_add_src_dir(chaz_MakeBinary *self, const char *path);

/** Add a prerequisite to the make rule of the binary.
 *
 * @param prereq The prerequisite.
 */
void
chaz_MakeBinary_add_prereq(chaz_MakeBinary *self, const char *prereq);

/** Return a list of all objects separated by space.
 */
char*
chaz_MakeBinary_obj_string(chaz_MakeBinary *self);

/** Accessor for target.
 */
const char*
chaz_MakeBinary_get_target(chaz_MakeBinary *self);

/** Accessor for compile flags.
 */
chaz_CFlags*
chaz_MakeBinary_get_compile_flags(chaz_MakeBinary *self);

/** Accessor for link flags.
 */
chaz_CFlags*
chaz_MakeBinary_get_link_flags(chaz_MakeBinary *self);

#ifdef __cplusplus
}
#endif

#endif /* H_CHAZ_MAKE */


