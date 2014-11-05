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
#include <string.h>
#include <ctype.h>
#include "Charmonizer/Probe.h"
#include "Charmonizer/Core/HeaderChecker.h"
#include "Charmonizer/Core/ConfWriter.h"
#include "Charmonizer/Core/ConfWriterC.h"
#include "Charmonizer/Core/ConfWriterPerl.h"
#include "Charmonizer/Core/ConfWriterPython.h"
#include "Charmonizer/Core/ConfWriterRuby.h"
#include "Charmonizer/Core/Util.h"
#include "Charmonizer/Core/CLI.h"
#include "Charmonizer/Core/Compiler.h"
#include "Charmonizer/Core/Make.h"
#include "Charmonizer/Core/OperatingSystem.h"

int
chaz_Probe_parse_cli_args(int argc, const char *argv[], chaz_CLI *cli) {
    int i;
    int output_enabled = 0;

    /* Register Charmonizer-specific options. */
    chaz_CLI_register(cli, "enable-c", "generate charmony.h", CHAZ_CLI_NO_ARG);
    chaz_CLI_register(cli, "enable-perl", "generate Charmony.pm", CHAZ_CLI_NO_ARG);
    chaz_CLI_register(cli, "enable-python", "generate charmony.py", CHAZ_CLI_NO_ARG);
    chaz_CLI_register(cli, "enable-ruby", "generate charmony.rb", CHAZ_CLI_NO_ARG);
    chaz_CLI_register(cli, "enable-makefile", NULL, CHAZ_CLI_NO_ARG);
    chaz_CLI_register(cli, "enable-coverage", NULL, CHAZ_CLI_NO_ARG);
    chaz_CLI_register(cli, "cc", "compiler command", CHAZ_CLI_ARG_REQUIRED);
    chaz_CLI_register(cli, "cflags", NULL, CHAZ_CLI_ARG_REQUIRED);
    chaz_CLI_register(cli, "make", "make command", 0);

    /* Parse options, exiting on failure. */
    if (!chaz_CLI_parse(cli, argc, argv)) {
        fprintf(stderr, "%s", chaz_CLI_help(cli));
        exit(1);
    }

    /* Accumulate compiler flags. */
    {
        char *cflags = chaz_Util_strdup("");
        size_t cflags_len = 0;
        for (i = 0; i < argc; i++) {
            if (strcmp(argv[i], "--") == 0) {
                i++;
                break;
            }
        }
        for (; i < argc; i++) {
            const char *arg = argv[i];
            cflags_len += strlen(arg) + 2;
            cflags = (char*)realloc(cflags, cflags_len);
            strcat(cflags, " ");
            strcat(cflags, arg);
        }
        chaz_CLI_set(cli, "cflags", cflags);
        free(cflags);
    }

    /* Some Perl setups have a 'cc' config value with leading whitespace. */
    if (chaz_CLI_defined(cli, "cc")) {
        const char *arg = chaz_CLI_strval(cli, "cc");
        char  *cc;
        size_t len = strlen(arg);
        size_t l   = 0;
        size_t r   = len;
        size_t trimmed_len;

        while (isspace(arg[l])) {
            ++l;
        }
        while (r > l && isspace(arg[r-1])) {
            --r;
        }

        trimmed_len = r - l;
        cc = (char*)malloc(trimmed_len + 1);
        memcpy(cc, arg + l, trimmed_len);
        cc[trimmed_len] = '\0';
        chaz_CLI_unset(cli, "cc");
        chaz_CLI_set(cli, "cc", cc);
        free(cc);
    }

    /* Validate. */
    if (!chaz_CLI_defined(cli, "cc")
        || !strlen(chaz_CLI_strval(cli, "cc"))
       ) {
        return false;
    }

    return true;
}

void
chaz_Probe_die_usage(void) {
    fprintf(stderr,
            "Usage: ./charmonize --cc=CC_COMMAND [--enable-c] "
            "[--enable-perl] [--enable-python] [--enable-ruby] -- CFLAGS\n");
    exit(1);
}

void
chaz_Probe_init(struct chaz_CLI *cli) {
    int output_enabled = 0;

    {
        /* Process CHARM_VERBOSITY environment variable. */
        const char *verbosity_env = getenv("CHARM_VERBOSITY");
        if (verbosity_env && strlen(verbosity_env)) {
            chaz_Util_verbosity = strtol(verbosity_env, NULL, 10);
        }
    }

    /* Dispatch other initializers. */
    chaz_OS_init();
    chaz_CC_init(chaz_CLI_strval(cli, "cc"), chaz_CLI_strval(cli, "cflags"));
    chaz_ConfWriter_init();
    chaz_HeadCheck_init();
    chaz_Make_init(chaz_CLI_strval(cli, "make"));

    /* Enable output. */
    if (chaz_CLI_defined(cli, "enable-c")) {
        chaz_ConfWriterC_enable();
        output_enabled = true;
    }
    if (chaz_CLI_defined(cli, "enable-perl")) {
        chaz_ConfWriterPerl_enable();
        output_enabled = true;
    }
    if (chaz_CLI_defined(cli, "enable-python")) {
        chaz_ConfWriterPython_enable();
        output_enabled = true;
    }
    if (chaz_CLI_defined(cli, "enable-ruby")) {
        chaz_ConfWriterRuby_enable();
        output_enabled = true;
    }
    if (!output_enabled) {
        fprintf(stderr, "No output formats enabled\n");
        exit(1);
    }

    if (chaz_Util_verbosity) { printf("Initialization complete.\n"); }
}

void
chaz_Probe_clean_up(void) {
    if (chaz_Util_verbosity) { printf("Cleaning up...\n"); }

    /* Dispatch various clean up routines. */
    chaz_ConfWriter_clean_up();
    chaz_CC_clean_up();
    chaz_Make_clean_up();

    if (chaz_Util_verbosity) { printf("Cleanup complete.\n"); }
}

int
chaz_Probe_gcc_version_num(void) {
    return chaz_CC_gcc_version_num();
}

const char*
chaz_Probe_gcc_version(void) {
    return chaz_CC_gcc_version_num() ? chaz_CC_gcc_version() : NULL;
}

int
chaz_Probe_msvc_version_num(void) {
    return chaz_CC_msvc_version_num();
}
