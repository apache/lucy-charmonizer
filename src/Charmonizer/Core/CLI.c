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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "Charmonizer/Core/CLI.h"
#include "Charmonizer/Core/Util.h"

typedef struct chaz_CLIOption {
    char *name;
    char *help;
    char *value;
    int   defined;
    int   flags;
} chaz_CLIOption;

struct chaz_CLI {
    char *name;
    char *desc;
    char *usage;
    char *help;
    chaz_CLIOption *opts;
    int   num_opts;
};

static void
S_chaz_CLI_error(chaz_CLI *self, const char *pattern, ...) {
    va_list ap;
    if (chaz_Util_verbosity > 0) {
        va_start(ap, pattern);
        vfprintf(stderr, pattern, ap);
        va_end(ap);
        fprintf(stderr, "\n");
    }
}

static void
S_chaz_CLI_rebuild_help(chaz_CLI *self) {
    int i;
    size_t amount = 200; /* Length of section headers. */

    /* Allocate space. */
    if (self->usage) {
        amount += strlen(self->usage);
    }
    else {
        amount += strlen(self->name);
    }
    if (self->desc) {
        amount += strlen(self->desc);
    }
    for (i = 0; i < self->num_opts; i++) {
        chaz_CLIOption *opt = &self->opts[i];
        amount += 24 + 2 * strlen(opt->name);
        if (opt->flags) {
            amount += strlen(opt->name);
        }
        if (opt->help) {
            amount += strlen(opt->help);
        }
    }
    free(self->help);
    self->help = (char*)malloc(amount);
    self->help[0] = '\0';

    /* Accumulate "help" string. */
    if (self->usage) {
        strcat(self->help, self->usage);
    }
    else {
        strcat(self->help, "Usage: ");
        strcat(self->help, self->name);
        if (self->num_opts) {
            strcat(self->help, " [OPTIONS]");
        }
    }
    if (self->desc) {
        strcat(self->help, "\n\n");
        strcat(self->help, self->desc);
    }
    strcat(self->help, "\n");
    if (self->num_opts) {
        strcat(self->help, "\nArguments:\n");
        for (i = 0; i < self->num_opts; i++) {
            chaz_CLIOption *opt = &self->opts[i];
            size_t line_start = strlen(self->help);
            size_t current_len;

            strcat(self->help, "  --");
            strcat(self->help, opt->name);
            current_len = strlen(self->help);
            if (opt->flags) {
                int j;
                if (opt->flags & CHAZ_CLI_ARG_OPTIONAL) {
                    self->help[current_len++] = '[';
                }
                self->help[current_len++] = '=';
                for (j = 0; opt->name[j]; j++) {
                    self->help[current_len++] = toupper(opt->name[j]);
                }
                if (opt->flags & CHAZ_CLI_ARG_OPTIONAL) {
                    self->help[current_len++] = ']';
                }
                self->help[current_len] = '\0';
            }
            if (opt->help) {
                self->help[current_len++] = ' ';
                while (current_len - line_start < 25) {
                    self->help[current_len++] = ' ';
                }
                self->help[current_len] = '\0';
                strcpy(self->help + current_len, opt->help);
            }
            strcat(self->help, "\n");
        }
    }
    strcat(self->help, "\n");
}

chaz_CLI*
chaz_CLI_new(const char *name, const char *description) {
    chaz_CLI *self  = calloc(1, sizeof(chaz_CLI));
    self->name      = chaz_Util_strdup(name ? name : "PROGRAM");
    self->desc      = description ? chaz_Util_strdup(description) : NULL;
    self->help      = NULL;
    self->opts      = NULL;
    self->num_opts  = 0;
    S_chaz_CLI_rebuild_help(self);
    return self;
}

void
chaz_CLI_destroy(chaz_CLI *self) {
    int i;
    for (i = 0; i < self->num_opts; i++) {
        chaz_CLIOption *opt = &self->opts[i];
        free(opt->name);
        free(opt->help);
        free(opt->value);
    }
    free(self->name);
    free(self->desc);
    free(self->opts);
    free(self->usage);
    free(self->help);
    free(self);
}

void
chaz_CLI_set_usage(chaz_CLI *self, const char *usage) {
    free(self->usage);
    self->usage = chaz_Util_strdup(usage);
}

const char*
chaz_CLI_help(chaz_CLI *self) {
    return self->help;
}

int
chaz_CLI_register(chaz_CLI *self, const char *name, const char *help,
                  int flags) {
    int rank;
    int i;
    int arg_required = !!(flags & CHAZ_CLI_ARG_REQUIRED);
    int arg_optional = !!(flags & CHAZ_CLI_ARG_OPTIONAL);

    /* Validate flags */
    if (arg_required && arg_optional) {
        S_chaz_CLI_error(self, "Conflicting flags: value both optional "
                         "and required");
        return 0;
    }

    /* Insert new option.  Keep options sorted by name. */
    for (rank = self->num_opts; rank > 0; rank--) {
        int comparison = strcmp(name, self->opts[rank - 1].name);
        if (comparison == 0) {
            S_chaz_CLI_error(self, "Option '%s' already registered", name);
            return 0;
        }
        else if (comparison > 0) {
            break;
        }
    }
    self->num_opts += 1;
    self->opts = realloc(self->opts, self->num_opts * sizeof(chaz_CLIOption));
    for (i = self->num_opts - 1; i > rank; i--) {
        self->opts[i] = self->opts[i - 1];
    }
    self->opts[rank].name    = chaz_Util_strdup(name);
    self->opts[rank].help    = help ? chaz_Util_strdup(help) : NULL;
    self->opts[rank].flags   = flags;
    self->opts[rank].defined = 0;
    self->opts[rank].value   = NULL;

    /* Update `help` with new option. */
    S_chaz_CLI_rebuild_help(self);

    return 1;
}

int
chaz_CLI_set(chaz_CLI *self, const char *name, const char *value) {
    int i;
    for (i = 0; i < self->num_opts; i++) {
        chaz_CLIOption *opt = &self->opts[i];
        if (strcmp(opt->name, name) == 0) {
            if (opt->defined) {
                S_chaz_CLI_error(self, "'%s' specified multiple times", name);
                return 0;
            }
            opt->defined = 1;
            if (value != NULL) {
                opt->value = chaz_Util_strdup(value);
            }
            return 1;
        }
    }
    S_chaz_CLI_error(self, "Attempt to set unknown option: '%s'", name);
    return 0;
}

int
chaz_CLI_unset(chaz_CLI *self, const char *name) {
    int i;
    for (i = 0; i < self->num_opts; i++) {
        chaz_CLIOption *opt = &self->opts[i];
        if (strcmp(opt->name, name) == 0) {
            free(opt->value);
            opt->value = NULL;
            opt->defined = 0;
            return 1;
        }
    }
    S_chaz_CLI_error(self, "Attempt to unset unknown option: '%s'", name);
    return 0;
}

int
chaz_CLI_defined(chaz_CLI *self, const char *name) {
    int i;
    for (i = 0; i < self->num_opts; i++) {
        chaz_CLIOption *opt = &self->opts[i];
        if (strcmp(opt->name, name) == 0) {
            return opt->defined;
        }
    }
    S_chaz_CLI_error(self, "Inquiry for unknown option: '%s'", name);
    return 0;
}

long
chaz_CLI_longval(chaz_CLI *self, const char *name) {
    int i;
    for (i = 0; i < self->num_opts; i++) {
        chaz_CLIOption *opt = &self->opts[i];
        if (strcmp(opt->name, name) == 0) {
            if (!opt->defined || !opt->value) {
                return 0;
            }
            return strtol(opt->value, NULL, 10);
        }
    }
    S_chaz_CLI_error(self, "Longval request for unknown option: '%s'", name);
    return 0;
}

const char*
chaz_CLI_strval(chaz_CLI *self, const char *name) {
    int i;
    for (i = 0; i < self->num_opts; i++) {
        chaz_CLIOption *opt = &self->opts[i];
        if (strcmp(opt->name, name) == 0) {
            return opt->value;
        }
    }
    S_chaz_CLI_error(self, "Strval request for unknown option: '%s'", name);
    return 0;
}

int
chaz_CLI_parse(chaz_CLI *self, int argc, const char *argv[]) {
    int i;
    char *name = NULL;
    size_t name_cap = 0;

    /* Parse most args. */
    for (i = 1; i < argc; i++) {
        const char *arg = argv[i];
        size_t name_len = 0;
        int has_equals = 0;
        const char *value = NULL;

        /* Stop processing if we see `-` or `--`. */
        if (strcmp(arg, "--") == 0 || strcmp(arg, "-") == 0) {
            break;
        }

        if (strncmp(arg, "--", 2) != 0) {
            S_chaz_CLI_error(self, "Unexpected argument: '%s'", arg);
            free(name);
            return 0;
        }

        /* Extract the name of the argument, look for a potential value. */
        while (1) {
            char c = arg[name_len + 2];
            if (isalnum(c) || c == '-' || c == '_') {
                name_len++;
            }
            else if (c == '\0') {
                break;
            }
            else if (c == '=') {
                /* The rest of the arg is the value. */
                value = arg + 2 + name_len + 1;
                break;
            }
            else {
                free(name);
                S_chaz_CLI_error(self, "Malformed argument: '%s'", arg);
                return 0;
            }
        }
        if (name_len + 1 > name_cap) {
            name_cap = name_len + 1;
            name = (char*)realloc(name, name_cap);
        }
        memcpy(name, arg + 2, name_len);
        name[name_len] = '\0';

        /* Attempt to set the option. */
        if (!chaz_CLI_set(self, name, value)) {
            free(name);
            return 0;
        }
    }

    free(name);
    return 1;
}

