#define CHAZ_USE_SHORT_NAMES

#include <stdlib.h>
#include <string.h>

#include "Charmonizer/Core/Dir.h"

#include "Charmonizer/Core/Compiler.h"
#include "Charmonizer/Core/HeaderChecker.h"
#include "Charmonizer/Core/ConfWriter.h"
#include "Charmonizer/Core/OperatingSystem.h"
#include "Charmonizer/Core/Util.h"

static chaz_bool_t mkdir_available = false;
static chaz_bool_t rmdir_available = false;
static chaz_bool_t initialized     = false;
int    Dir_mkdir_num_args = 0;
static char mkdir_command[7];
char *Dir_mkdir_command = mkdir_command;

/* Source code for standard POSIX mkdir */
static char posix_mkdir_code[] = METAQUOTE
    #include <%s>
    int main(int argc, char **argv) {
        if (argc != 2) { return 1; }
        if (mkdir(argv[1], 0777) != 0) { return 2; }
        return 0;
    }
METAQUOTE;

/* Source code for Windows _mkdir. */
static char win_mkdir_code[] = METAQUOTE
    #include <direct.h>
    int main(int argc, char **argv) {
        if (argc != 2) { return 1; }
        if (_mkdir(argv[1]) != 0) { return 2; }
        return 0;
    }
METAQUOTE;

/* Source code for rmdir. */
static char rmdir_code[] = METAQUOTE
    #include <%s>
    int main(int argc, char **argv) {
        if (argc != 2) { return 1; }
        if (rmdir(argv[1]) != 0) { return 2; }
        return 0;
    }
METAQUOTE;

static chaz_bool_t
S_try_init_posix_mkdir(char *header)
{
    size_t needed = sizeof(posix_mkdir_code) + 30;
    char *code_buf = (char*)malloc(needed);

    /* Attempt compilation. */
    sprintf(code_buf, posix_mkdir_code, header);
    mkdir_available = CC_compile_exe("_charm_mkdir.c", "_charm_mkdir", 
        code_buf, strlen(code_buf));

    /* Set vars on success. */
    if (mkdir_available) {
        strcpy(mkdir_command, "mkdir");
        if (strcmp(header, "direct.h") == 0) {
            Dir_mkdir_num_args = 1;
        }
        else {
            Dir_mkdir_num_args = 2;
        }
    }

    free(code_buf);
    return mkdir_available;
}

static chaz_bool_t
S_try_init_win_mkdir()
{
    mkdir_available = CC_compile_exe("_charm_mkdir.c", "_charm_mkdir", 
        win_mkdir_code, strlen(win_mkdir_code));
    if (mkdir_available) {
        strcpy(mkdir_command, "_mkdir");
        Dir_mkdir_num_args = 1;
    }
    return mkdir_available;
}

static void
S_init_mkdir()
{
    if (Util_verbosity) {
        printf("Attempting to compile _charm_mkdir utility...\n");
    }
    if (HeadCheck_check_header("windows.h")) {
        if (S_try_init_win_mkdir())               { return; }
        if (S_try_init_posix_mkdir("direct.h"))   { return; }
    }
    else {
        if (S_try_init_posix_mkdir("sys/stat.h")) { return; }
    }
}

static chaz_bool_t
S_try_init_rmdir(char *header) 
{
    size_t needed = sizeof(posix_mkdir_code) + 30;
    char *code_buf = (char*)malloc(needed);
    sprintf(code_buf, rmdir_code, header);
    rmdir_available = CC_compile_exe("_charm_rmdir.c", "_charm_rmdir", 
        code_buf, strlen(code_buf));
    free(code_buf);
    return rmdir_available;
}

static void
S_init_rmdir()
{
    if (Util_verbosity) {
        printf("Attempting to compile _charm_rmdir utility...\n");
    }
    if (S_try_init_rmdir("unistd.h"))   { return; }
    if (S_try_init_rmdir("dirent.h"))   { return; }
    if (S_try_init_rmdir("direct.h"))   { return; }
}

/* Compile _charm_mkdir and _charm_rmdir. */
void
Dir_init(void)
{
    if (!initialized) { 
        initialized = true;
        S_init_mkdir();
        S_init_rmdir();
    }
}

void
Dir_clean_up(void)
{
    if (!Util_remove_and_verify("_charm_mkdir")) {
        Util_die("Failed to remove '_charm_mkdir'");
    }
    if (!Util_remove_and_verify("_charm_rmdir")) {
        Util_die("Failed to remove '_charm_rmdir'");
    }
}

chaz_bool_t
Dir_mkdir(const char *filepath)
{
    if (!initialized) { Dir_init(); }
    return OS_run_local("_charm_mkdir ", filepath, NULL);
}

chaz_bool_t
Dir_rmdir(const char *filepath)
{
    if (!initialized) { Dir_init(); }
    return OS_run_local("_charm_rmdir ", filepath, NULL);
}

/**
 * Copyright 2009 The Apache Software Foundation
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

