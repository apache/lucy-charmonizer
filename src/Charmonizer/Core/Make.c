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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "Charmonizer/Core/Make.h"
#include "Charmonizer/Core/CFlags.h"
#include "Charmonizer/Core/CLI.h"
#include "Charmonizer/Core/Compiler.h"
#include "Charmonizer/Core/OperatingSystem.h"
#include "Charmonizer/Core/Util.h"

#define CHAZ_MAKEBINARY_EXE         1
#define CHAZ_MAKEBINARY_STATIC_LIB  2
#define CHAZ_MAKEBINARY_SHARED_LIB  3

struct chaz_MakeVar {
    char   *name;
    char   *value;
    size_t  num_elements;
};

struct chaz_MakeRule {
    char *targets;
    char *prereqs;
    char *commands;
};

struct chaz_MakeBinary {
    chaz_MakeRule  *rule;  /* Owned by MakeBinary. */

    chaz_MakeVar   *obj_var;
    char           *obj_dollar_var;
    char          **sources;  /* List of all sources. */
    size_t          num_sources;
    char          **single_sources;  /* Only sources from add_src_file. */
    size_t          num_single_sources;
    char          **dirs;
    size_t          num_dirs;

    chaz_MakeVar   *cflags_var;
    chaz_CFlags    *cflags;

    chaz_MakeVar   *ldflags_var;
    chaz_CFlags    *ldflags;
};

struct chaz_MakeFile {
    chaz_MakeVar    **vars;
    size_t            num_vars;
    chaz_MakeRule   **rules;
    size_t            num_rules;
    char            **install_dirs;
    size_t            num_install_dirs;
    chaz_MakeRule    *install;
    chaz_MakeRule    *clean;
    chaz_MakeRule    *distclean;
    chaz_MakeBinary **binaries;
    size_t            num_binaries;
};

typedef struct {
    chaz_MakeBinary         *binary;
    chaz_Make_file_filter_t  filter;
    void                    *filter_ctx;
} chaz_MakeBinaryContext;

/* Static vars. */
static struct {
    chaz_CLI *cli;
    char     *make_command;
    int       shell_type;
    int       supports_pattern_rules;
} chaz_Make = {
    NULL, NULL,
    0, 0
};

/* Detect make command.
 *
 * The argument list must be a NULL-terminated series of different spellings
 * of `make`, which will be auditioned in the order they are supplied.  Here
 * are several possibilities:
 *
 *      make
 *      gmake
 *      nmake
 *      dmake
 */
static int
S_chaz_Make_detect(const char *make1, ...);

static int
S_chaz_Make_audition(const char *make);

static chaz_MakeBinary*
S_chaz_MakeFile_add_binary(chaz_MakeFile *self, int type, const char *basename,
                           const char *target);

static void
S_chaz_MakeFile_add_install_dir(chaz_MakeFile *self, const char *dir);

static void
S_chaz_MakeFile_write_install_vars(FILE *out);

static void
S_chaz_MakeFile_write_binary_rules(chaz_MakeBinary *binary, FILE *out);

static void
S_chaz_MakeFile_write_object_rules(char **sources, const char *command,
                                   FILE *out);

static void
S_chaz_MakeFile_write_pattern_rules(char **dirs, const char *command,
                                    FILE *out);

static chaz_MakeRule*
S_chaz_MakeRule_new(const char *target, const char *prereq);

static void
S_chaz_MakeRule_destroy(chaz_MakeRule *self);

static void
S_chaz_MakeRule_write(chaz_MakeRule *self, FILE *out);

static void
S_chaz_MakeBinary_destroy(chaz_MakeBinary *self);

static void
S_chaz_MakeBinary_list_files_callback(const char *dir, char *file,
                                      void *context);
static void
S_chaz_MakeBinary_do_add_src_file(chaz_MakeBinary *self, const char *path);

/** Return the path to the object file for a source file.
 *
 * @param path The path to the source file.
 */
static char*
S_chaz_MakeBinary_obj_path(const char *src_path);

void
chaz_Make_init(chaz_CLI *cli) {
    const char *make_command = chaz_CLI_strval(cli, "make");

    chaz_Make.cli        = cli;
    chaz_Make.shell_type = chaz_OS_shell_type();

    if (make_command) {
        if (!S_chaz_Make_detect(make_command, NULL)) {
            chaz_Util_warn("Make utility '%s' doesn't appear to work",
                           make_command);
        }
    }
    else {
        int succeeded = 0;

        /* mingw32-make seems to try to run commands under both cmd.exe
         * and sh.exe. Not sure about dmake.
         */
        if (chaz_Make.shell_type == CHAZ_OS_POSIX) {
            succeeded = S_chaz_Make_detect("make", "gmake", "dmake",
                                           "mingw32-make", NULL);
        }
        else if (chaz_Make.shell_type == CHAZ_OS_CMD_EXE) {
            succeeded = S_chaz_Make_detect("nmake", "dmake", "mingw32-make",
                                           NULL);
        }

        if (!succeeded) {
            chaz_Util_warn("No working make utility found");
        }
        else if (chaz_Util_verbosity) {
            printf("Detected make utility '%s'\n", chaz_Make.make_command);
        }
    }
}

void
chaz_Make_clean_up(void) {
    free(chaz_Make.make_command);
}

const char*
chaz_Make_get_make(void) {
    return chaz_Make.make_command;
}

int
chaz_Make_shell_type(void) {
    return chaz_Make.shell_type;
}

static int
S_chaz_Make_detect(const char *make1, ...) {
    va_list args;
    const char *candidate;
    int found = 0;
    const char makefile_content[] =
        "foo:\n"
        "\t@echo 643490c943525d19\n"
        "\n"
        "%.ext:\n"
        "\t@echo 8f4ef20576b070d5\n";
    chaz_Util_write_file("_charm_Makefile", makefile_content);

    /* Audition candidates. */
    found = S_chaz_Make_audition(make1);
    va_start(args, make1);
    while (!found && (NULL != (candidate = va_arg(args, const char*)))) {
        found = S_chaz_Make_audition(candidate);
    }
    va_end(args);

    chaz_Util_remove_and_verify("_charm_Makefile");

    return found;
}

static int
S_chaz_Make_audition(const char *make) {
    int succeeded = 0;
    char *command = chaz_Util_join(" ", make, "-f", "_charm_Makefile", NULL);

    chaz_Util_remove_and_verify("_charm_foo");
    chaz_OS_run_redirected(command, "_charm_foo");
    if (chaz_Util_can_open_file("_charm_foo")) {
        size_t len;
        char *content = chaz_Util_slurp_file("_charm_foo", &len);
        if (content != NULL && strstr(content, "643490c943525d19") != NULL) {
            succeeded = 1;
        }
        free(content);
    }
    chaz_Util_remove_and_verify("_charm_foo");
    free(command);

    if (succeeded) {
        chaz_Make.make_command = chaz_Util_strdup(make);

        command = chaz_Util_join(" ", make, "-f", "_charm_Makefile", "foo.ext",
                                 NULL);
        chaz_OS_run_redirected(command, "_charm_foo");
        if (chaz_Util_can_open_file("_charm_foo")) {
            size_t len;
            char *content = chaz_Util_slurp_file("_charm_foo", &len);
            if (content != NULL
                && strstr(content, "8f4ef20576b070d5") != NULL
               ) {
                chaz_Make.supports_pattern_rules = 1;
            }
            free(content);
        }
        chaz_Util_remove_and_verify("_charm_foo");
        free(command);
    }

    return succeeded;
}

chaz_MakeFile*
chaz_MakeFile_new() {
    chaz_MakeFile *self = (chaz_MakeFile*)calloc(1, sizeof(chaz_MakeFile));
    char *generated;

    self->vars         = (chaz_MakeVar**)calloc(1, sizeof(chaz_MakeVar*));
    self->rules        = (chaz_MakeRule**)calloc(1, sizeof(chaz_MakeRule*));
    self->install_dirs = (char**)calloc(1, sizeof(char*));

    self->install   = S_chaz_MakeRule_new("install", "all");
    self->clean     = S_chaz_MakeRule_new("clean", NULL);
    self->distclean = S_chaz_MakeRule_new("distclean", "clean");

    self->binaries = (chaz_MakeBinary**)calloc(1, sizeof(chaz_MakeBinary*));

    /* MSVC leaves .obj files around when creating executables. */
    generated = chaz_Util_join("", "charmonizer", chaz_OS_exe_ext(),
                               " charmonizer.obj charmony.h Makefile", NULL);
    chaz_MakeRule_add_rm_command(self->distclean, generated);

    free(generated);
    return self;
}

void
chaz_MakeFile_destroy(chaz_MakeFile *self) {
    size_t i;

    for (i = 0; self->vars[i]; i++) {
        chaz_MakeVar *var = self->vars[i];
        free(var->name);
        free(var->value);
        free(var);
    }
    free(self->vars);

    for (i = 0; self->rules[i]; i++) {
        S_chaz_MakeRule_destroy(self->rules[i]);
    }
    free(self->rules);

    for (i = 0; self->install_dirs[i]; i++) {
        free(self->install_dirs[i]);
    }
    free(self->install_dirs);

    for (i = 0; self->binaries[i]; i++) {
        S_chaz_MakeBinary_destroy(self->binaries[i]);
    }
    free(self->binaries);

    S_chaz_MakeRule_destroy(self->install);
    S_chaz_MakeRule_destroy(self->clean);
    S_chaz_MakeRule_destroy(self->distclean);

    free(self);
}

chaz_MakeVar*
chaz_MakeFile_add_var(chaz_MakeFile *self, const char *name,
                      const char *value) {
    chaz_MakeVar  *var      = (chaz_MakeVar*)malloc(sizeof(chaz_MakeVar));
    chaz_MakeVar **vars     = self->vars;
    size_t         num_vars = self->num_vars + 1;

    var->name         = chaz_Util_strdup(name);
    var->value        = chaz_Util_strdup("");
    var->num_elements = 0;

    if (value) { chaz_MakeVar_append(var, value); }

    vars = (chaz_MakeVar**)realloc(vars,
                                   (num_vars + 1) * sizeof(chaz_MakeVar*));
    vars[num_vars-1] = var;
    vars[num_vars]   = NULL;
    self->vars = vars;
    self->num_vars = num_vars;

    return var;
}

chaz_MakeRule*
chaz_MakeFile_add_rule(chaz_MakeFile *self, const char *target,
                       const char *prereq) {
    chaz_MakeRule  *rule      = S_chaz_MakeRule_new(target, prereq);
    chaz_MakeRule **rules     = self->rules;
    size_t          num_rules = self->num_rules + 1;

    rules = (chaz_MakeRule**)realloc(rules,
                                     (num_rules + 1) * sizeof(chaz_MakeRule*));
    rules[num_rules-1] = rule;
    rules[num_rules]   = NULL;
    self->rules = rules;
    self->num_rules = num_rules;

    return rule;
}

chaz_MakeRule*
chaz_MakeFile_install_rule(chaz_MakeFile *self) {
    return self->install;
}

chaz_MakeRule*
chaz_MakeFile_clean_rule(chaz_MakeFile *self) {
    return self->clean;
}

chaz_MakeRule*
chaz_MakeFile_distclean_rule(chaz_MakeFile *self) {
    return self->distclean;
}

chaz_MakeBinary*
chaz_MakeFile_add_exe(chaz_MakeFile *self, const char *dir,
                      const char *basename, int installed) {
    const char *exe_ext = chaz_CC_exe_ext();
    char *target;
    char *command;
    chaz_MakeBinary *binary;
    chaz_CFlags *ldflags;

    if (dir == NULL || strcmp(dir, ".") == 0) {
        target = chaz_Util_join("", basename, exe_ext, NULL);
    }
    else {
        const char *dir_sep = chaz_OS_dir_sep();
        target = chaz_Util_join("", dir, dir_sep, basename, exe_ext, NULL);
    }

    binary = S_chaz_MakeFile_add_binary(self, CHAZ_MAKEBINARY_EXE, basename,
                                        target);

    ldflags = chaz_CC_new_cflags();
    if (chaz_CC_is_msvc()) {
        chaz_CFlags_append(ldflags, "/nologo");
    }
    chaz_CFlags_set_link_output(ldflags, "$@");

    /* Objects must come before flags since flags may contain libraries. */
    command = chaz_Util_join("", "$(LINK) ", chaz_CFlags_get_string(ldflags),
                             " ", binary->obj_dollar_var,
                             " $(", binary->ldflags_var->name, ") ", NULL);
    chaz_MakeRule_add_command(binary->rule, command);

    if (installed) {
        chaz_MakeFile_install(self, target, "$(BINDIR)", NULL);
    }

    chaz_CFlags_destroy(ldflags);
    free(command);
    free(target);
    return binary;
}

chaz_MakeBinary*
chaz_MakeFile_add_shared_lib(chaz_MakeFile *self, const char *dir,
                             const char *basename, const char *version,
                             const char *major_version, int installed) {
    int binfmt = chaz_CC_binary_format();
    char *path, *vpath, *mpath;
    char *command;
    chaz_MakeBinary *binary;
    chaz_CFlags *ldflags;

    path = chaz_CC_shared_lib_filename(dir, basename, NULL);
    if (binfmt == CHAZ_CC_BINFMT_PE) {
        vpath = chaz_CC_shared_lib_filename(dir, basename, major_version);
        mpath = NULL;
    }
    else {
        vpath = chaz_CC_shared_lib_filename(dir, basename, version);
        mpath = chaz_CC_shared_lib_filename(dir, basename, major_version);
    }

    binary = S_chaz_MakeFile_add_binary(self, CHAZ_MAKEBINARY_SHARED_LIB,
                                        basename, vpath);

    chaz_CFlags_compile_shared_library(binary->cflags);

    ldflags = chaz_CC_new_cflags();
    if (chaz_CC_is_msvc()) {
        chaz_CFlags_append(ldflags, "/nologo");
    }
    chaz_CFlags_set_link_output(ldflags, "$@");
    chaz_CFlags_link_shared_library(ldflags, basename, version, major_version);
    if (binfmt == CHAZ_CC_BINFMT_MACHO) {
        /* Set temporary install name with full path on Darwin. */
        char *install_name = chaz_Util_join("", "-install_name \"$(CURDIR)/",
                                            mpath, "\"", NULL);
        chaz_CFlags_append(ldflags, install_name);
        free(install_name);
    }

    command = chaz_Util_join("", "$(LINK) ", chaz_CFlags_get_string(ldflags),
                             " ", binary->obj_dollar_var,
                             " $(", binary->ldflags_var->name, ")", NULL);
    chaz_MakeRule_add_command(binary->rule, command);
    free(command);

    if (installed) {
        const char *root = binfmt == CHAZ_CC_BINFMT_PE
                           ? "$(BINDIR)" : "$(LIBDIR)";
        chaz_MakeFile_install(self, vpath, root, NULL);
    }

    /* Add symlinks. */
    if (binfmt == CHAZ_CC_BINFMT_ELF || binfmt == CHAZ_CC_BINFMT_MACHO) {
        char *name  = chaz_CC_shared_lib_filename(NULL, basename, NULL);
        char *vname = chaz_CC_shared_lib_filename(NULL, basename, version);
        char *mname = chaz_CC_shared_lib_filename(NULL, basename,
                                                  major_version);
        const char *ltarget = binfmt == CHAZ_CC_BINFMT_MACHO ? vname : mname;

        command = chaz_Util_join(" ", "ln -sf", vname, mpath, NULL);
        chaz_MakeRule_add_command(binary->rule, command);
        free(command);

        command = chaz_Util_join(" ", "ln -sf", ltarget, path, NULL);
        chaz_MakeRule_add_command(binary->rule, command);
        free(command);

        if (installed) {
            command = chaz_Util_join("", "ln -sf ", vname,
                                     " \"$(LIBDIR)/", mname, "\"",
                                     NULL);
            chaz_MakeRule_add_command(self->install, command);
            free(command);

            command = chaz_Util_join("", "ln -sf ", ltarget,
                                     " \"$(LIBDIR)/", name, "\"",
                                     NULL);
            chaz_MakeRule_add_command(self->install, command);
            free(command);

            if (binfmt == CHAZ_CC_BINFMT_MACHO) {
                /* Change install name to installation path. */
                command = chaz_Util_join("",
                    "install_name_tool",
                    " -id \"$(LIBDIR)/", mname, "\"",
                    " \"$(LIBDIR)/", vname, "\"",
                    NULL
                );
                chaz_MakeRule_add_command(self->install, command);
                free(command);
            }
        }

        chaz_MakeRule_add_rm_command(self->clean, mpath);
        chaz_MakeRule_add_rm_command(self->clean, path);

        free(mname);
        free(vname);
        free(name);
    }

    if (binfmt == CHAZ_CC_BINFMT_PE) {
        /* Import library. */
        char *filename = chaz_CC_import_lib_filename(dir, basename,
                                                     major_version);
        if (installed) {
            chaz_MakeFile_install(self, filename, "$(LIBDIR)", NULL);
        }
        chaz_MakeRule_add_rm_command(self->clean, filename);
        free(filename);
    }

    if (chaz_CC_is_msvc()) {
        /* Remove export file. */
        char *filename = chaz_CC_export_filename(dir, basename, major_version);
        chaz_MakeRule_add_rm_command(self->clean, filename);
        free(filename);
    }

    chaz_CFlags_destroy(ldflags);
    free(mpath);
    free(vpath);
    free(path);
    return binary;
}

chaz_MakeBinary*
chaz_MakeFile_add_static_lib(chaz_MakeFile *self, const char *dir,
                             const char *basename, int installed) {
    char *target = chaz_CC_static_lib_filename(dir, basename);
    char *command;
    chaz_MakeBinary *binary
        = S_chaz_MakeFile_add_binary(self, CHAZ_MAKEBINARY_STATIC_LIB,
                                     basename, target);

    command = chaz_CC_format_archiver_command("$@", binary->obj_dollar_var);
    chaz_MakeRule_add_command(binary->rule, command);

    if (installed) {
        chaz_MakeFile_install(self, target, "$(LIBDIR)", NULL);
    }

    free(command);
    free(target);
    return binary;
}

static chaz_MakeBinary*
S_chaz_MakeFile_add_binary(chaz_MakeFile *self, int type, const char *basename,
                           const char *target) {
    chaz_MakeBinary *binary
        = (chaz_MakeBinary*)calloc(1, sizeof(chaz_MakeBinary));
    const char *suffix;
    char *uc_base = chaz_Util_strdup(basename);
    char *binary_var_name;
    char *obj_var_name;
    char *cflags_var_name;
    char *ldflags_var_name;
    char *obj_dollar_var;
    size_t i;
    size_t num_binaries;
    size_t alloc_size;
    chaz_MakeBinary **binaries;

    switch (type) {
        case CHAZ_MAKEBINARY_EXE:        suffix = "EXE";        break;
        case CHAZ_MAKEBINARY_STATIC_LIB: suffix = "STATIC_LIB"; break;
        case CHAZ_MAKEBINARY_SHARED_LIB: suffix = "SHARED_LIB"; break;
        default:
            chaz_Util_die("Unknown binary type %d", type);
            return NULL;
    }

    for (i = 0; uc_base[i] != '\0'; i++) {
        uc_base[i] = toupper((unsigned char)uc_base[i]);
    }

    binary_var_name  = chaz_Util_join("_", uc_base, suffix, NULL);
    obj_var_name     = chaz_Util_join("_", uc_base, suffix, "OBJS", NULL);
    cflags_var_name  = chaz_Util_join("_", uc_base, suffix, "CFLAGS", NULL);
    ldflags_var_name = chaz_Util_join("_", uc_base, suffix, "LDFLAGS", NULL);
    obj_dollar_var   = chaz_Util_join("", "$(", obj_var_name, ")", NULL);

    chaz_MakeFile_add_var(self, binary_var_name, target);

    binary->rule           = S_chaz_MakeRule_new(target, obj_dollar_var);
    binary->obj_var        = chaz_MakeFile_add_var(self, obj_var_name, NULL);
    binary->obj_dollar_var = obj_dollar_var;
    binary->sources        = (char**)calloc(1, sizeof(char*));
    binary->single_sources = (char**)calloc(1, sizeof(char*));
    binary->dirs           = (char**)calloc(1, sizeof(char*));

    binary->cflags_var  = chaz_MakeFile_add_var(self, cflags_var_name, NULL);
    binary->cflags      = chaz_CC_new_cflags();
    binary->ldflags_var = chaz_MakeFile_add_var(self, ldflags_var_name, NULL);
    binary->ldflags     = chaz_CC_new_cflags();

    chaz_MakeRule_add_rm_command(self->clean, obj_dollar_var);
    chaz_MakeRule_add_rm_command(self->clean, target);

    num_binaries = self->num_binaries;
    alloc_size   = (num_binaries + 2) * sizeof(chaz_MakeBinary*);
    binaries     = (chaz_MakeBinary**)realloc(self->binaries, alloc_size);
    binaries[num_binaries]   = binary;
    binaries[num_binaries+1] = NULL;
    self->binaries     = binaries;
    self->num_binaries = num_binaries + 1;

    free(ldflags_var_name);
    free(cflags_var_name);
    free(obj_var_name);
    free(binary_var_name);
    free(uc_base);
    return binary;
}

chaz_MakeBinary*
chaz_MakeFile_add_lemon_exe(chaz_MakeFile *self, const char *dir) {
    chaz_MakeBinary *exe = chaz_MakeFile_add_exe(self, dir, "lemon", 0);
    chaz_MakeBinary_add_src_file(exe, dir, "lemon.c");
    return exe;
}

chaz_MakeRule*
chaz_MakeFile_add_lemon_grammar(chaz_MakeFile *self,
                                const char *base_name) {
    char *c_file  = chaz_Util_join(".", base_name, "c", NULL);
    char *h_file  = chaz_Util_join(".", base_name, "h", NULL);
    char *y_file  = chaz_Util_join(".", base_name, "y", NULL);
    char *command = chaz_Util_join(" ", "$(LEMON_EXE) -q", y_file, NULL);

    chaz_MakeRule *rule = chaz_MakeFile_add_rule(self, c_file, y_file);
    chaz_MakeRule *clean_rule = chaz_MakeFile_clean_rule(self);

    chaz_MakeRule_add_prereq(rule, "$(LEMON_EXE)");
    chaz_MakeRule_add_command(rule, command);

    chaz_MakeRule_add_rm_command(clean_rule, h_file);
    chaz_MakeRule_add_rm_command(clean_rule, c_file);

    free(c_file);
    free(h_file);
    free(y_file);
    free(command);
    return rule;
}

void
chaz_MakeFile_install(chaz_MakeFile *self, const char *src, const char *root,
                      const char *dest) {
    char *path;
    char *command;

    if (dest) {
        path = chaz_Util_join(chaz_OS_dir_sep(), root, dest, NULL);
    }
    else {
        path = chaz_Util_strdup(root);
    }

    S_chaz_MakeFile_add_install_dir(self, path);

    if (chaz_Make.shell_type == CHAZ_OS_POSIX) {
        command = chaz_Util_join("", "cp -f ", src, " \"", path, "\"", NULL);
    }
    else if (chaz_Make.shell_type == CHAZ_OS_CMD_EXE) {
        command = chaz_Util_join("", "copy /y ", src, " \"", path, "\" >nul",
                                 NULL);
    }
    else {
        chaz_Util_die("Unsupported shell type: %d", chaz_Make.shell_type);
    }

    chaz_MakeRule_add_command(self->install, command);

    free(command);
    free(path);
}

void
chaz_MakeFile_install_dir(chaz_MakeFile *self, const char *src,
                          const char *root, const char *dest) {
    char *path;
    char *command;

    if (dest) {
        path = chaz_Util_join(chaz_OS_dir_sep(), root, dest, NULL);
    }
    else {
        path = chaz_Util_strdup(root);
    }

    S_chaz_MakeFile_add_install_dir(self, path);

    if (chaz_Make.shell_type == CHAZ_OS_POSIX) {
        command = chaz_Util_join("", "cp -Rf ", src, "/* \"", path, "\"",
                                 NULL);
    }
    else if (chaz_Make.shell_type == CHAZ_OS_CMD_EXE) {
        command = chaz_Util_join("", "xcopy /seiy ", src, " \"", path,
                                 "\" >nul", NULL);
    }
    else {
        chaz_Util_die("Unsupported shell type: %d", chaz_Make.shell_type);
    }

    chaz_MakeRule_add_command(self->install, command);

    free(command);
    free(path);
}

void
chaz_MakeFile_install_pkgconfig(chaz_MakeFile *self, const char *name,
                                const char *version, const char *content) {
    size_t size;
    const char *p;
    char *q;
    char *escaped;
    char *command;

    if (chaz_OS_shell_type() != CHAZ_OS_POSIX) { return; }

    /* Escape content for POSIX printf utility. */

    for (p = content, size = 0; *p != '\0'; p++) {
        switch (*p) {
            case '%':  size += 2; break;
            case '\\': size += 2; break;
            case '\'': size += 4; break;
            case '\n': size += 2; break;
            default:   size += 1; break;
        }
    }

    escaped = (char*)malloc(size + 1);

    for (p = content, q = escaped; *p != '\0'; p++) {
        switch (*p) {
            case '%':  memcpy(q, "%%",    2); q += 2; break;
            case '\\': memcpy(q, "\\\\",  2); q += 2; break;
            case '\'': memcpy(q, "\\047", 4); q += 4; break;
            case '\n': memcpy(q, "\\n",   2); q += 2; break;
            default: *q++ = *p; break;
        }
    }

    *q++ = '\0';

    S_chaz_MakeFile_add_install_dir(self, "$(LIBDIR)/pkgconfig");

    command = chaz_Util_join("",
        "printf '"
            "libdir=$(LIBDIR)\\n"
            "version=", version, "\\n"
            "\\n",
            escaped,
        "' >\"$(LIBDIR)/pkgconfig/", name, ".pc\"",
        NULL
    );
    chaz_MakeRule_add_command(self->install, command);

    free(command);
    free(escaped);
}

static void
S_chaz_MakeFile_add_install_dir(chaz_MakeFile *self, const char *dir) {
    size_t   i;
    size_t   num_install_dirs;
    size_t   alloc_size;
    char   **install_dirs;

    for (i = 0; self->install_dirs[i]; i++) {
        if (strcmp(dir, self->install_dirs[i]) == 0) {
            return;
        }
    }

    num_install_dirs = self->num_install_dirs;
    alloc_size       = (num_install_dirs + 2) * sizeof(char*);
    install_dirs     = (char**)realloc(self->install_dirs, alloc_size);
    install_dirs[num_install_dirs]   = chaz_Util_strdup(dir);
    install_dirs[num_install_dirs+1] = NULL;
    self->install_dirs     = install_dirs;
    self->num_install_dirs = num_install_dirs + 1;
}

void
chaz_MakeFile_write(chaz_MakeFile *self) {
    FILE   *out;
    size_t  i;

    out = fopen("Makefile", "w");
    if (!out) {
        chaz_Util_die("Can't open Makefile\n");
    }

    if (chaz_Make.shell_type == CHAZ_OS_CMD_EXE) {
        /* Make sure that mingw32-make uses the cmd.exe shell. */
        fprintf(out, "SHELL = cmd\n");
    }

    fprintf(out, "CC = %s\n", chaz_CC_get_cc());
    fprintf(out, "LINK = %s\n", chaz_CC_link_command());

    S_chaz_MakeFile_write_install_vars(out);

    /* Finalize binary vars. */
    for (i = 0; self->binaries[i]; i++) {
        chaz_MakeBinary *binary = self->binaries[i];
        const char *flags;

        flags = chaz_CFlags_get_string(binary->cflags);
        chaz_MakeVar_append(binary->cflags_var, flags);
        flags = chaz_CFlags_get_string(binary->ldflags);
        chaz_MakeVar_append(binary->ldflags_var, flags);
    }

    for (i = 0; self->vars[i]; i++) {
        chaz_MakeVar *var = self->vars[i];
        fprintf(out, "%s = %s\n", var->name, var->value);
    }
    fprintf(out, "\n");

    for (i = 0; self->rules[i]; i++) {
        S_chaz_MakeRule_write(self->rules[i], out);
    }

    for (i = 0; self->binaries[i]; i++) {
        S_chaz_MakeFile_write_binary_rules(self->binaries[i], out);
    }

    if (self->num_install_dirs) {
        /* Prepend mkdir commands. */
        chaz_MakeRule *dummy = S_chaz_MakeRule_new(NULL, NULL);
        char *commands;

        for (i = 0; self->install_dirs[i]; i++) {
            chaz_MakeRule_add_mkdir_command(dummy, self->install_dirs[i]);
        }

        commands = chaz_Util_join("", dummy->commands, self->install->commands,
                                  NULL);
        free(self->install->commands);
        self->install->commands = commands;

        S_chaz_MakeRule_destroy(dummy);
    }

    S_chaz_MakeRule_write(self->install, out);
    S_chaz_MakeRule_write(self->clean, out);
    S_chaz_MakeRule_write(self->distclean, out);

    /* Suffix rule for .c files. */
    if (chaz_CC_is_msvc()) {
        fprintf(out, ".c.obj :\n");
        fprintf(out, "\t$(CC) /nologo $(CFLAGS) /c $< /Fo$@\n\n");
    }
    else {
        fprintf(out, ".c.o :\n");
        fprintf(out, "\t$(CC) $(CFLAGS) -c $< -o $@\n\n");
    }

    fclose(out);
}

static void
S_chaz_MakeFile_write_install_vars(FILE *out) {
    const char *dir_sep = chaz_OS_dir_sep();
    const char *strval;

    strval = chaz_CLI_strval(chaz_Make.cli, "prefix");
    fprintf(out, "PREFIX = %s\n", strval ? strval : "/usr/local");

    strval = chaz_CLI_strval(chaz_Make.cli, "bindir");
    if (strval) {
        fprintf(out, "BINDIR = %s\n", strval);
    }
    else {
        fprintf(out, "BINDIR = $(PREFIX)%sbin\n", dir_sep);
    }

    strval = chaz_CLI_strval(chaz_Make.cli, "datarootdir");
    if (strval) {
        fprintf(out, "DATAROOTDIR = %s\n", strval);
    }
    else {
        fprintf(out, "DATAROOTDIR = $(PREFIX)%sshare\n", dir_sep);
    }

    strval = chaz_CLI_strval(chaz_Make.cli, "datadir");
    fprintf(out, "DATADIR = %s\n", strval ? strval : "$(DATAROOTDIR)");

    strval = chaz_CLI_strval(chaz_Make.cli, "libdir");
    if (strval) {
        fprintf(out, "LIBDIR = %s\n", strval);
    }
    else {
        fprintf(out, "LIBDIR = $(PREFIX)%slib\n", dir_sep);
    }

    strval = chaz_CLI_strval(chaz_Make.cli, "mandir");
    if (strval) {
        fprintf(out, "MANDIR = %s\n", strval);
    }
    else {
        fprintf(out, "MANDIR = $(DATAROOTDIR)%sman\n", dir_sep);
    }
}

static void
S_chaz_MakeFile_write_binary_rules(chaz_MakeBinary *binary, FILE *out) {
    const char *cflags_string;

    S_chaz_MakeRule_write(binary->rule, out);

    cflags_string = chaz_CFlags_get_string(binary->cflags);

    /* Write rules to compile with custom flags. */
    if (cflags_string[0] != '\0') {
        char *dollar_var = chaz_Util_join("", "$(", binary->cflags_var->name,
                                          ")", NULL);

        if (!chaz_Make.supports_pattern_rules
            || chaz_Make.shell_type == CHAZ_OS_CMD_EXE) {
            /* Write a rule for each object file. This is needed for make
             * utilities that don't support pattern rules but also for
             * mingw32-make which has problems with pattern rules and
             * backslash directory separators.
             */
            S_chaz_MakeFile_write_object_rules(binary->sources, dollar_var,
                                               out);
        }
        else {
            /* Write a pattern rule for each directory. */
            S_chaz_MakeFile_write_pattern_rules(binary->dirs, dollar_var, out);
            /* Write a rule for each object added with add_src_file. */
            S_chaz_MakeFile_write_object_rules(binary->single_sources,
                                               dollar_var, out);
        }

        free(dollar_var);
    }
}

static void
S_chaz_MakeFile_write_object_rules(char **sources, const char *cflags,
                                   FILE *out) {
    const char *cc = chaz_CC_is_msvc() ? "$(CC) /nologo" : "$(CC)";
    chaz_CFlags *output_cflags = chaz_CC_new_cflags();
    const char *output_cflags_string;
    size_t i;

    chaz_CFlags_set_output_obj(output_cflags, "$@");
    output_cflags_string = chaz_CFlags_get_string(output_cflags);

    for (i = 0; sources[i]; i++) {
        const char *source = sources[i];
        char *obj_path = S_chaz_MakeBinary_obj_path(source);
        chaz_MakeRule *rule;
        char *command;

        if (obj_path == NULL) { continue; }

        rule = S_chaz_MakeRule_new(obj_path, source);
        command = chaz_Util_join(" ", cc, "$(CFLAGS)", cflags, source,
                                 output_cflags_string, NULL);
        chaz_MakeRule_add_command(rule, command);
        S_chaz_MakeRule_write(rule, out);

        free(command);
        S_chaz_MakeRule_destroy(rule);
        free(obj_path);
    }

    chaz_CFlags_destroy(output_cflags);
}

static void
S_chaz_MakeFile_write_pattern_rules(char **dirs, const char *cflags,
                                    FILE *out) {
    const char *cc = chaz_CC_is_msvc() ? "$(CC) /nologo" : "$(CC)";
    const char *obj_ext = chaz_CC_obj_ext();
    const char *dir_sep = chaz_OS_dir_sep();
    chaz_CFlags *output_cflags = chaz_CC_new_cflags();
    const char *output_cflags_string;
    char *command;
    size_t i;

    chaz_CFlags_set_output_obj(output_cflags, "$@");
    output_cflags_string = chaz_CFlags_get_string(output_cflags);
    command  = chaz_Util_join(" ", cc, "$(CFLAGS)", cflags, "$<",
                              output_cflags_string, NULL);

    for (i = 0; dirs[i]; i++) {
        const char *dir = dirs[i];
        char *target = chaz_Util_join("", dir, dir_sep, "%", obj_ext,
                                      NULL);
        char *prereq = chaz_Util_join("", dir, dir_sep, "%.c", NULL);
        chaz_MakeRule *rule = S_chaz_MakeRule_new(target, prereq);

        chaz_MakeRule_add_command(rule, command);
        S_chaz_MakeRule_write(rule, out);

        S_chaz_MakeRule_destroy(rule);
        free(prereq);
        free(target);
    }

    free(command);
    chaz_CFlags_destroy(output_cflags);
}

void
chaz_MakeVar_append(chaz_MakeVar *self, const char *element) {
    char *value;

    if (element[0] == '\0') { return; }

    if (self->num_elements == 0) {
        value = chaz_Util_strdup(element);
    }
    else {
        value = (char*)malloc(strlen(self->value) + strlen(element) + 20);

        if (self->num_elements == 1) {
            sprintf(value, "\\\n    %s \\\n    %s", self->value, element);
        }
        else {
            sprintf(value, "%s \\\n    %s", self->value, element);
        }
    }

    free(self->value);
    self->value = value;
    self->num_elements++;
}

static chaz_MakeRule*
S_chaz_MakeRule_new(const char *target, const char *prereq) {
    chaz_MakeRule *rule = (chaz_MakeRule*)malloc(sizeof(chaz_MakeRule));

    rule->targets  = NULL;
    rule->prereqs  = NULL;
    rule->commands = NULL;

    if (target) { chaz_MakeRule_add_target(rule, target); }
    if (prereq) { chaz_MakeRule_add_prereq(rule, prereq); }

    return rule;
}

static void
S_chaz_MakeRule_destroy(chaz_MakeRule *self) {
    if (self->targets)  { free(self->targets); }
    if (self->prereqs)  { free(self->prereqs); }
    if (self->commands) { free(self->commands); }
    free(self);
}

static void
S_chaz_MakeRule_write(chaz_MakeRule *self, FILE *out) {
    fprintf(out, "%s :", self->targets);
    if (self->prereqs) {
        fprintf(out, " %s", self->prereqs);
    }
    fprintf(out, "\n");
    if (self->commands) {
        fprintf(out, "%s", self->commands);
    }
    fprintf(out, "\n");
}

void
chaz_MakeRule_add_target(chaz_MakeRule *self, const char *target) {
    char *targets;

    if (!self->targets) {
        targets = chaz_Util_strdup(target);
    }
    else {
        targets = chaz_Util_join(" ", self->targets, target, NULL);
        free(self->targets);
    }

    self->targets = targets;
}

void
chaz_MakeRule_add_prereq(chaz_MakeRule *self, const char *prereq) {
    char *prereqs;

    if (!self->prereqs) {
        prereqs = chaz_Util_strdup(prereq);
    }
    else {
        prereqs = chaz_Util_join(" ", self->prereqs, prereq, NULL);
        free(self->prereqs);
    }

    self->prereqs = prereqs;
}

void
chaz_MakeRule_add_command(chaz_MakeRule *self, const char *command) {
    char *commands;

    if (!self->commands) {
        commands = (char*)malloc(strlen(command) + 20);
        sprintf(commands, "\t%s\n", command);
    }
    else {
        commands = (char*)malloc(strlen(self->commands) + strlen(command) + 20);
        sprintf(commands, "%s\t%s\n", self->commands, command);
        free(self->commands);
    }

    self->commands = commands;
}

void
chaz_MakeRule_add_mkdir_command(chaz_MakeRule *self, const char *dir) {
    char *command;

    if (chaz_Make.shell_type == CHAZ_OS_POSIX) {
        command = chaz_Util_join("", "mkdir -p \"", dir, "\"", NULL);
    }
    else if (chaz_Make.shell_type == CHAZ_OS_CMD_EXE) {
        command = chaz_Util_join("", "if not exist \"", dir, "\" mkdir \"",
                                 dir, "\"", NULL);
    }
    else {
        chaz_Util_die("Unsupported shell type: %d", chaz_Make.shell_type);
    }

    chaz_MakeRule_add_command(self, command);
    free(command);
}

void
chaz_MakeRule_add_rm_command(chaz_MakeRule *self, const char *files) {
    char *command;

    if (chaz_Make.shell_type == CHAZ_OS_POSIX) {
        command = chaz_Util_join(" ", "rm -f", files, NULL);
    }
    else if (chaz_Make.shell_type == CHAZ_OS_CMD_EXE) {
        command = chaz_Util_join("", "for %%i in (", files,
                                 ") do @if exist %%i del /f %%i", NULL);
    }
    else {
        chaz_Util_die("Unsupported shell type: %d", chaz_Make.shell_type);
    }

    chaz_MakeRule_add_command(self, command);
    free(command);
}

void
chaz_MakeRule_add_recursive_rm_command(chaz_MakeRule *self, const char *dirs) {
    char *command;

    if (chaz_Make.shell_type == CHAZ_OS_POSIX) {
        command = chaz_Util_join(" ", "rm -rf", dirs, NULL);
    }
    else if (chaz_Make.shell_type == CHAZ_OS_CMD_EXE) {
        command = chaz_Util_join("", "for %%i in (", dirs,
                                 ") do @if exist %%i rmdir /s /q %%i", NULL);
    }
    else {
        chaz_Util_die("Unsupported shell type: %d", chaz_Make.shell_type);
    }

    chaz_MakeRule_add_command(self, command);
    free(command);
}

void
chaz_MakeRule_add_make_command(chaz_MakeRule *self, const char *dir,
                               const char *target) {
    char *command;

    if (chaz_Make.shell_type == CHAZ_OS_POSIX) {
        if (!target) {
            command = chaz_Util_join("", "(cd ", dir, " && $(MAKE))", NULL);
        }
        else {
            command = chaz_Util_join("", "(cd ", dir, " && $(MAKE) ", target,
                                     ")", NULL);
        }
    }
    else if (chaz_Make.shell_type == CHAZ_OS_CMD_EXE) {
        if (!target) {
            command = chaz_Util_join(" ", "pushd", dir, "&& $(MAKE) && popd",
                                     NULL);
        }
        else {
            command = chaz_Util_join(" ", "pushd", dir, "&& $(MAKE)", target,
                                     "&& popd", NULL);
        }
    }
    else {
        chaz_Util_die("Unsupported shell type: %d", chaz_Make.shell_type);
    }

    chaz_MakeRule_add_command(self, command);
    free(command);
}

static void
S_chaz_MakeBinary_destroy(chaz_MakeBinary *self) {
    size_t i;

    free(self->obj_dollar_var);
    S_chaz_MakeRule_destroy(self->rule);

    for (i = 0; i < self->num_sources; i++) {
        free(self->sources[i]);
    }
    free(self->sources);
    for (i = 0; i < self->num_single_sources; i++) {
        free(self->single_sources[i]);
    }
    free(self->single_sources);
    for (i = 0; i < self->num_dirs; i++) {
        free(self->dirs[i]);
    }
    free(self->dirs);

    chaz_CFlags_destroy(self->cflags);
    chaz_CFlags_destroy(self->ldflags);

    free(self);
}

void
chaz_MakeBinary_add_src_file(chaz_MakeBinary *self, const char *dir,
                             const char *filename) {
    size_t num_sources = self->num_single_sources;
    size_t alloc_size  = (num_sources + 2) * sizeof(char*);
    char **sources = (char**)realloc(self->single_sources, alloc_size);
    char *path;

    if (dir == NULL || strcmp(dir, ".") == 0) {
        path = chaz_Util_strdup(filename);
    }
    else {
        const char *dir_sep = chaz_OS_dir_sep();
        path = chaz_Util_join(dir_sep, dir, filename, NULL);
    }

    /* Add to single_sources. */
    sources[num_sources]     = path;
    sources[num_sources+1]   = NULL;
    self->single_sources     = sources;
    self->num_single_sources = num_sources + 1;

    S_chaz_MakeBinary_do_add_src_file(self, path);
}

void
chaz_MakeBinary_add_src_dir(chaz_MakeBinary *self, const char *path) {
    chaz_MakeBinary_add_filtered_src_dir(self, path, NULL, NULL);
}

void
chaz_MakeBinary_add_filtered_src_dir(chaz_MakeBinary *self, const char *path,
                                     chaz_Make_file_filter_t filter,
                                     void *filter_ctx) {
    chaz_MakeBinaryContext context;
    size_t num_dirs = self->num_dirs;
    char **dirs = (char**)realloc(self->dirs, (num_dirs + 2) * sizeof(char*));

    dirs[num_dirs]   = chaz_Util_strdup(path);
    dirs[num_dirs+1] = NULL;
    self->dirs     = dirs;
    self->num_dirs = num_dirs + 1;

    context.binary     = self;
    context.filter     = filter;
    context.filter_ctx = filter_ctx;

    chaz_Make_list_files(path, "c", S_chaz_MakeBinary_list_files_callback,
                         &context);
}

static void
S_chaz_MakeBinary_list_files_callback(const char *dir, char *file,
                                      void *vcontext) {
    chaz_MakeBinaryContext *context = (chaz_MakeBinaryContext*)vcontext;
    const char *dir_sep = chaz_OS_dir_sep();

    if (context->filter == NULL
        || context->filter(dir, file, context->filter_ctx) != 0
       ) {
        char *path = chaz_Util_join(dir_sep, dir, file, NULL);
        S_chaz_MakeBinary_do_add_src_file(context->binary, path);
        free(path);
    }
}

static void
S_chaz_MakeBinary_do_add_src_file(chaz_MakeBinary *self, const char *path) {
    size_t num_sources = self->num_sources;
    size_t alloc_size  = (num_sources + 2) * sizeof(char*);
    char **sources = (char**)realloc(self->sources, alloc_size);
    char *obj_path;

    sources[num_sources]   = chaz_Util_strdup(path);
    sources[num_sources+1] = NULL;
    self->sources     = sources;
    self->num_sources = num_sources + 1;

    obj_path = S_chaz_MakeBinary_obj_path(path);
    if (obj_path == NULL) {
        chaz_Util_warn("Invalid source filename: %s", path);
    }
    else {
        chaz_MakeVar_append(self->obj_var, obj_path);
        free(obj_path);
    }
}

static char*
S_chaz_MakeBinary_obj_path(const char *src_path) {
    const char *dir_sep = chaz_OS_dir_sep();
    const char *obj_ext = chaz_CC_obj_ext();
    size_t obj_ext_len = strlen(obj_ext);
    size_t i = strlen(src_path);
    char *retval;

    while (i > 0) {
        i -= 1;
        if (src_path[i] == dir_sep[0]) { return NULL; }
        if (src_path[i] == '.')        { break; }
    }

    if (src_path[i] != '.') { return NULL; }

    retval = (char*)malloc(i + obj_ext_len + 1);
    memcpy(retval, src_path, i);
    memcpy(retval + i, obj_ext, obj_ext_len + 1);

    return retval;
}

void
chaz_MakeBinary_add_prereq(chaz_MakeBinary *self, const char *prereq) {
    chaz_MakeRule_add_prereq(self->rule, prereq);
}

char*
chaz_MakeBinary_obj_string(chaz_MakeBinary *self) {
    char *retval = chaz_Util_strdup("");
    size_t i;

    for (i = 0; i < self->num_sources; i++) {
        const char *sep = retval[0] == '\0' ? "" : " ";
        char *obj_path = S_chaz_MakeBinary_obj_path(self->sources[i]);
        char *tmp;

        if (obj_path == NULL) { continue; }

        tmp = chaz_Util_join("", retval, sep, obj_path, NULL);
        free(retval);
        retval = tmp;

        free(obj_path);
    }

    return retval;
}

const char*
chaz_MakeBinary_get_target(chaz_MakeBinary *self) {
    return self->rule->targets;
}

chaz_CFlags*
chaz_MakeBinary_get_compile_flags(chaz_MakeBinary *self) {
    return self->cflags;
}

chaz_CFlags*
chaz_MakeBinary_get_link_flags(chaz_MakeBinary *self) {
    return self->ldflags;
}

void
chaz_Make_list_files(const char *dir, const char *ext,
                     chaz_Make_file_callback_t callback, void *context) {
    int         shell_type = chaz_OS_shell_type();
    const char *pattern;
    char       *command;
    char       *list;
    char       *prefix;
    char       *file;
    size_t      command_size;
    size_t      list_len;
    size_t      prefix_len;

    /* List files using shell. */

    if (shell_type == CHAZ_OS_POSIX) {
        pattern = "find %s -name '*.%s' -type f";
    }
    else if (shell_type == CHAZ_OS_CMD_EXE) {
        pattern = "dir %s\\*.%s /s /b /a-d";
    }
    else {
        chaz_Util_die("Unknown shell type %d", shell_type);
    }

    command_size = strlen(pattern) + strlen(dir) + strlen(ext) + 10;
    command = (char*)malloc(command_size);
    sprintf(command, pattern, dir, ext);
    list = chaz_OS_run_and_capture(command, &list_len);
    free(command);
    if (!list) {
        chaz_Util_die("Failed to list files in '%s'", dir);
    }
    list[list_len-1] = 0;

    /* Find directory prefix to strip from files */

    if (shell_type == CHAZ_OS_POSIX) {
        prefix_len = strlen(dir);
        prefix = (char*)malloc(prefix_len + 2);
        memcpy(prefix, dir, prefix_len);
        prefix[prefix_len++] = '/';
        prefix[prefix_len]   = '\0';
    }
    else {
        char   *output;
        size_t  output_len;

        /* 'dir /s' returns absolute paths, so we have to find the absolute
         * path of the directory. This is done by using the variable
         * substitution feature of the 'for' command.
         */
        pattern = "for %%I in (%s) do @echo %%~fI";
        command_size = strlen(pattern) + strlen(dir) + 10;
        command = (char*)malloc(command_size);
        sprintf(command, pattern, dir);
        output = chaz_OS_run_and_capture(command, &output_len);
        free(command);
        if (!output) { chaz_Util_die("Failed to find absolute path"); }

        /* Strip whitespace from end of output. */
        for (prefix_len = output_len; prefix_len > 0; --prefix_len) {
            if (!isspace((unsigned char)output[prefix_len-1])) { break; }
        }
        prefix = (char*)malloc(prefix_len + 2);
        memcpy(prefix, output, prefix_len);
        prefix[prefix_len++] = '\\';
        prefix[prefix_len]   = '\0';
        free(output);
    }

    /* Iterate file list and invoke callback. */

    for (file = strtok(list, "\r\n"); file; file = strtok(NULL, "\r\n")) {
        if (strlen(file) <= prefix_len
            || memcmp(file, prefix, prefix_len) != 0
           ) {
            chaz_Util_die("Expected prefix '%s' for file name '%s'", prefix,
                          file);
        }

        callback(dir, file + prefix_len, context);
    }

    free(prefix);
    free(list);
}

