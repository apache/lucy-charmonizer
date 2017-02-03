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

#include <string.h>
#include <stdlib.h>
#include "Charmonizer/Core/CFlags.h"
#include "Charmonizer/Core/Compiler.h"
#include "Charmonizer/Core/Util.h"
#include "Charmonizer/Core/OperatingSystem.h"

struct chaz_CFlags {
    int   style;
    char *string;
};

chaz_CFlags*
chaz_CFlags_new(int style) {
    chaz_CFlags *flags = (chaz_CFlags*)malloc(sizeof(chaz_CFlags));
    flags->style  = style;
    flags->string = chaz_Util_strdup("");
    return flags;
}

void
chaz_CFlags_destroy(chaz_CFlags *flags) {
    free(flags->string);
    free(flags);
}

const char*
chaz_CFlags_get_string(chaz_CFlags *flags) {
    return flags->string;
}

void
chaz_CFlags_append(chaz_CFlags *flags, const char *string) {
    char *new_string;
    if (flags->string[0] == '\0') {
        new_string = chaz_Util_strdup(string);
    }
    else {
        new_string = chaz_Util_join(" ", flags->string, string, NULL);
    }
    free(flags->string);
    flags->string = new_string;
}

void
chaz_CFlags_clear(chaz_CFlags *flags) {
    if (flags->string[0] != '\0') {
        free(flags->string);
        flags->string = chaz_Util_strdup("");
    }
}

void
chaz_CFlags_set_output_obj(chaz_CFlags *flags, const char *filename) {
    const char *output;
    char *string;
    if (flags->style == CHAZ_CFLAGS_STYLE_MSVC) {
        output = "/c /Fo";
    }
    else {
        /* POSIX */
        output = "-c -o ";
    }
    string = chaz_Util_join("", output, filename, NULL);
    chaz_CFlags_append(flags, string);
    free(string);
}

void
chaz_CFlags_set_output_exe(chaz_CFlags *flags, const char *filename) {
    const char *output;
    char *string;
    if (flags->style == CHAZ_CFLAGS_STYLE_MSVC) {
        output = "/Fe";
    }
    else {
        /* POSIX */
        output = "-o ";
    }
    string = chaz_Util_join("", output, filename, NULL);
    chaz_CFlags_append(flags, string);
    free(string);
}

void
chaz_CFlags_add_define(chaz_CFlags *flags, const char *name,
                       const char *value) {
    const char *define;
    char *string;
    if (flags->style == CHAZ_CFLAGS_STYLE_MSVC) {
        define = "/D";
    }
    else {
        /* POSIX */
        define = "-D ";
    }
    if (value) {
        string = chaz_Util_join("", define, name, "=", value, NULL);
    }
    else {
        string = chaz_Util_join("", define, name, NULL);
    }
    chaz_CFlags_append(flags, string);
    free(string);
}

void
chaz_CFlags_add_include_dir(chaz_CFlags *flags, const char *dir) {
    const char *include;
    char *string;
    if (flags->style == CHAZ_CFLAGS_STYLE_MSVC)  {
        include = "/I ";
    }
    else {
        /* POSIX */
        include = "-I ";
    }
    string = chaz_Util_join("", include, dir, NULL);
    chaz_CFlags_append(flags, string);
    free(string);
}

void
chaz_CFlags_enable_optimization(chaz_CFlags *flags) {
    const char *string;
    if (flags->style == CHAZ_CFLAGS_STYLE_MSVC) {
        string = "/O2";
    }
    else if (flags->style == CHAZ_CFLAGS_STYLE_GNU) {
        string = "-O2";
    }
    else if (flags->style == CHAZ_CFLAGS_STYLE_SUN_C) {
        string = "-xO4";
    }
    else {
        /* POSIX */
        string = "-O 1";
    }
    chaz_CFlags_append(flags, string);
}

void
chaz_CFlags_enable_debugging(chaz_CFlags *flags) {
    if (flags->style == CHAZ_CFLAGS_STYLE_GNU
        || flags->style == CHAZ_CFLAGS_STYLE_SUN_C) {
        chaz_CFlags_append(flags, "-g");
    }
}

void
chaz_CFlags_disable_strict_aliasing(chaz_CFlags *flags) {
    if (flags->style == CHAZ_CFLAGS_STYLE_MSVC) {
        return;
    }
    else if (flags->style == CHAZ_CFLAGS_STYLE_GNU) {
        chaz_CFlags_append(flags, "-fno-strict-aliasing");
    }
    else if (flags->style == CHAZ_CFLAGS_STYLE_SUN_C) {
        chaz_CFlags_append(flags, "-xalias_level=any");
    }
    else {
        chaz_Util_die("Don't know how to disable strict aliasing with '%s'",
                      chaz_CC_get_cc());
    }
}

void
chaz_CFlags_set_warnings_as_errors(chaz_CFlags *flags) {
    const char *string;
    if (flags->style == CHAZ_CFLAGS_STYLE_MSVC) {
        string = "/WX";
    }
    else if (flags->style == CHAZ_CFLAGS_STYLE_GNU) {
        string = "-Werror";
    }
    else if (flags->style == CHAZ_CFLAGS_STYLE_SUN_C) {
        string = "-errwarn=%all";
    }
    else {
        chaz_Util_die("Don't know how to set warnings as errors with '%s'",
                      chaz_CC_get_cc());
    }
    chaz_CFlags_append(flags, string);
}

void
chaz_CFlags_compile_shared_library(chaz_CFlags *flags) {
    const char *string;
    if (flags->style == CHAZ_CFLAGS_STYLE_MSVC) {
        string = "/MD";
    }
    else if (flags->style == CHAZ_CFLAGS_STYLE_GNU) {
        int binary_format = chaz_CC_binary_format();
        if (binary_format == CHAZ_CC_BINFMT_MACHO) {
            string = "-fno-common";
        }
        else if (binary_format == CHAZ_CC_BINFMT_ELF) {
            string = "-fPIC";
        }
        else {
            /* MinGW. */
            return;
        }
    }
    else if (flags->style == CHAZ_CFLAGS_STYLE_SUN_C) {
        string = "-KPIC";
    }
    else {
        return;
    }
    chaz_CFlags_append(flags, string);
}

void
chaz_CFlags_hide_symbols(chaz_CFlags *flags) {
    if (flags->style == CHAZ_CFLAGS_STYLE_GNU) {
        if (chaz_CC_binary_format() != CHAZ_CC_BINFMT_PE) {
            chaz_CFlags_append(flags, "-fvisibility=hidden");
        }
    }
    else if (flags->style == CHAZ_CFLAGS_STYLE_SUN_C) {
        static int checked = 0;
        static int version_ge_550;

        if (!checked) {
            /* Requires Sun Studio 8. */
            version_ge_550 = chaz_CC_test_sun_c_version(">= 0x550");
            checked = 1;
        }

        if (version_ge_550) {
            chaz_CFlags_append(flags, "-xldscope=hidden");
        }
    }
}

void
chaz_CFlags_link_shared_library(chaz_CFlags *flags, const char *basename,
                                const char *version,
                                const char *major_version) {
    char *string = NULL;

    if (flags->style == CHAZ_CFLAGS_STYLE_MSVC) {
        string = chaz_Util_strdup("/DLL");
    }
    else if (flags->style == CHAZ_CFLAGS_STYLE_GNU) {
        int binary_format = chaz_CC_binary_format();

        if (binary_format == CHAZ_CC_BINFMT_MACHO) {
            string = chaz_Util_join(" ", "-dynamiclib", "-current_version",
                                    version, "-compatibility_version",
                                    major_version, NULL);
        }
        else if (binary_format == CHAZ_CC_BINFMT_ELF) {
            string = chaz_Util_join("", "-shared -Wl,-soname,lib", basename,
                                    ".so.", major_version, NULL);
        }
        else if (binary_format == CHAZ_CC_BINFMT_PE) {
            string = chaz_Util_join("", "-shared -Wl,--out-implib,lib",
                                    basename, "-", major_version, ".dll.a",
                                    NULL);
        }
    }
    else if (flags->style == CHAZ_CFLAGS_STYLE_SUN_C) {
        string = chaz_Util_join("", "-G -h lib", basename, ".so.",
                                major_version, NULL);
    }
    else {
        chaz_Util_die("Don't know how to link a shared library with '%s'",
                      chaz_CC_get_cc());
    }

    if (string) {
        chaz_CFlags_append(flags, string);
        free(string);
    }
}

void
chaz_CFlags_set_link_output(chaz_CFlags *flags, const char *filename) {
    const char *output;
    char *string;
    if (flags->style == CHAZ_CFLAGS_STYLE_MSVC) {
        output = "/OUT:";
    }
    else {
        output = "-o ";
    }
    string = chaz_Util_join("", output, filename, NULL);
    chaz_CFlags_append(flags, string);
    free(string);
}

void
chaz_CFlags_add_library_path(chaz_CFlags *flags, const char *directory) {
    const char *lib_path;
    char *string;
    if (flags->style == CHAZ_CFLAGS_STYLE_MSVC) {
        if (strcmp(directory, ".") == 0) {
            /* The MS linker searches the current directory by default. */
            return;
        }
        else {
            lib_path = "/LIBPATH:";
        }
    }
    else {
        lib_path = "-L ";
    }
    string = chaz_Util_join("", lib_path, directory, NULL);
    chaz_CFlags_append(flags, string);
    free(string);
}

void
chaz_CFlags_add_shared_lib(chaz_CFlags *flags, const char *dir,
                           const char *basename, const char *major_version) {
    int binfmt = chaz_CC_binary_format();
    char *filename;
    if (binfmt == CHAZ_CC_BINFMT_PE) {
        filename = chaz_CC_import_lib_filename(dir, basename, major_version);
    }
    else {
        filename = chaz_CC_shared_lib_filename(dir, basename, major_version);
    }
    chaz_CFlags_append(flags, filename);
    free(filename);
}

void
chaz_CFlags_add_external_lib(chaz_CFlags *flags, const char *library) {
    char *string;
    if (flags->style == CHAZ_CFLAGS_STYLE_MSVC) {
        string = chaz_Util_join("", library, ".lib", NULL);
    }
    else {
        string = chaz_Util_join(" ", "-l", library, NULL);
    }
    chaz_CFlags_append(flags, string);
    free(string);
}

void
chaz_CFlags_add_rpath(chaz_CFlags *flags, const char *path) {
    char *string;

    if (chaz_CC_binary_format() != CHAZ_CC_BINFMT_ELF) { return; }

    if (flags->style == CHAZ_CFLAGS_STYLE_GNU) {
        string = chaz_Util_join("", "-Wl,-rpath,", path, NULL);
    }
    else if (flags->style == CHAZ_CFLAGS_STYLE_SUN_C) {
        string = chaz_Util_join(" ", "-R", path, NULL);
    }
    else {
        chaz_Util_die("Don't know how to set rpath with '%s'",
                      chaz_CC_get_cc());
    }

    chaz_CFlags_append(flags, string);
    free(string);
}

void
chaz_CFlags_enable_code_coverage(chaz_CFlags *flags) {
    if (flags->style == CHAZ_CFLAGS_STYLE_GNU) {
        chaz_CFlags_append(flags, "--coverage");
    }
    else {
        chaz_Util_die("Don't know how to enable code coverage with '%s'",
                      chaz_CC_get_cc());
    }
}


