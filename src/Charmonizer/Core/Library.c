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
#include "Charmonizer/Core/Library.h"
#include "Charmonizer/Core/Compiler.h"
#include "Charmonizer/Core/Util.h"
#include "Charmonizer/Core/OperatingSystem.h"

struct chaz_Lib {
    char *name;
    char *version;
    char *major_version;
    int   is_static;
    int   is_shared;
};

static char*
S_build_filename(chaz_Lib *lib, const char *version, const char *ext);

static const char*
S_get_prefix(void);

chaz_Lib*
chaz_Lib_new_shared(const char *name, const char *version,
                    const char *major_version) {
    chaz_Lib *lib = (chaz_Lib*)malloc(sizeof(chaz_Lib));
    lib->name          = chaz_Util_strdup(name);
    lib->version       = chaz_Util_strdup(version);
    lib->major_version = chaz_Util_strdup(major_version);
    lib->is_shared = 1;
    lib->is_static = 0;
    return lib;
}

chaz_Lib*
chaz_Lib_new_static(const char *name) {
    chaz_Lib *lib = (chaz_Lib*)malloc(sizeof(chaz_Lib));
    lib->name          = chaz_Util_strdup(name);
    lib->version       = NULL;
    lib->major_version = NULL;
    lib->is_shared = 0;
    lib->is_static = 1;
    return lib;
}

void
chaz_Lib_destroy(chaz_Lib *lib) {
    free(lib->name);
    free(lib->version);
    free(lib->major_version);
    free(lib);
}

const char*
chaz_Lib_get_name(chaz_Lib *lib) {
    return lib->name;
}

const char*
chaz_Lib_get_version(chaz_Lib *lib) {
    return lib->version;
}

const char*
chaz_Lib_get_major_version(chaz_Lib *lib) {
    return lib->major_version;
}

int
chaz_Lib_is_shared (chaz_Lib *lib) {
    return lib->is_shared;
}

int
chaz_Lib_is_static (chaz_Lib *lib) {
    return lib->is_static;
}

char*
chaz_Lib_filename(chaz_Lib *lib) {
    if (lib->is_static) {
        return chaz_Lib_no_version_filename(lib);
    }
    else {
        const char *ext = chaz_CC_shared_lib_ext();
        if (chaz_CC_binary_format() == CHAZ_CC_BINFMT_PE) {
            return S_build_filename(lib, lib->major_version, ext);
        }
        else {
            return S_build_filename(lib, lib->version, ext);
        }
    }
}

char*
chaz_Lib_major_version_filename(chaz_Lib *lib) {
    if (lib->is_static) {
        return chaz_Lib_no_version_filename(lib);
    }
    else {
        const char *ext = chaz_CC_shared_lib_ext();
        return S_build_filename(lib, lib->major_version, ext);
    }
}

char*
chaz_Lib_no_version_filename(chaz_Lib *lib) {
    const char *prefix = S_get_prefix();
    const char *ext = lib->is_shared
                      ? chaz_CC_shared_lib_ext()
                      : chaz_CC_static_lib_ext();
    return chaz_Util_join("", prefix, lib->name, ext, NULL);
}

char*
chaz_Lib_implib_filename(chaz_Lib *lib) {
    const char *ext = chaz_CC_import_lib_ext();
    return S_build_filename(lib, lib->major_version, ext);
}

char*
chaz_Lib_export_filename(chaz_Lib *lib) {
    return S_build_filename(lib, lib->major_version, ".exp");
}

static char*
S_build_filename(chaz_Lib *lib, const char *version, const char *ext) {
    const char *prefix = S_get_prefix();
    int binary_format = chaz_CC_binary_format();

    if (binary_format == CHAZ_CC_BINFMT_PE) {
        return chaz_Util_join("", prefix, lib->name, "-", version, ext, NULL);
    }
    else if (binary_format == CHAZ_CC_BINFMT_MACHO) {
        return chaz_Util_join("", prefix, lib->name, ".", version, ext, NULL);
    }
    else if (binary_format == CHAZ_CC_BINFMT_ELF) {
        return chaz_Util_join("", prefix, lib->name, ext, ".", version, NULL);
    }
    else {
        chaz_Util_die("Unsupported binary format");
        return NULL;
    }
}

static const char*
S_get_prefix() {
    if (chaz_CC_msvc_version_num()) {
        return "";
    }
    /* TODO: Readd Cygwin detection. */
    /*else if (chaz_OS_is_cygwin()) {
        return "cyg";
    }*/
    else {
        return "lib";
    }
}


