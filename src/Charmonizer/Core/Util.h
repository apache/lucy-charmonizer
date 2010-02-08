/* Chaz/Core/Util.h -- miscellaneous utilities.
 */

#ifndef H_CHAZ_UTIL
#define H_CHAZ_UTIL 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>

extern int chaz_Util_verbosity;

/* Open a file (truncating if necessary) and write [content] to it.  Util_die() if
 * an error occurs.
 */
void
chaz_Util_write_file(const char *filename, const char *content);

/* Read an entire file into memory.
 */
char* 
chaz_Util_slurp_file(const char *file_path, size_t *len_ptr);

/* Return a newly allocated copy of a NULL-terminated string.
 */
char* 
chaz_Util_strdup(const char *string);

/* Get the length of a file (may overshoot on text files under DOS).
 */
long  
chaz_Util_flength(FILE *f);

/* Print an error message to stderr and exit.
 */
void  
chaz_Util_die(const char *format, ...);

/* Print an error message to stderr.
 */
void
chaz_Util_warn(const char *format, ...);

/* Attept to delete a file.  Don't error if the file wasn't there to begin
 * with.  Return 1 if it seems like the file is gone because an attempt to
 * open it for reading fails (this doesn't guarantee that the file is gone,
 * but it works well enough for our purposes).  Return 0 if we can still 
 * read the file.
 */
int
chaz_Util_remove_and_verify(const char *file_path);

/* Attempt to open a file for reading, then close it immediately.
 */
int
chaz_Util_can_open_file(const char *file_path);

#ifdef CHAZ_USE_SHORT_NAMES
  #define Util_verbosity              chaz_Util_verbosity 
  #define Util_write_file             chaz_Util_write_file 
  #define Util_slurp_file             chaz_Util_slurp_file 
  #define Util_flength                chaz_Util_flength 
  #define Util_die                    chaz_Util_die 
  #define Util_warn                   chaz_Util_warn 
  #define Util_strdup                 chaz_Util_strdup
  #define Util_remove_and_verify      chaz_Util_remove_and_verify 
  #define Util_can_open_file          chaz_Util_can_open_file 
#endif

#ifdef __cplusplus
}
#endif

#endif /* H_CHAZ_UTIL */

/**
 * Copyright 2006 The Apache Software Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

