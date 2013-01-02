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

/* Charmonize.c -- Create Charmony.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Charmonizer/Probe.h"
#include "Charmonizer/Probe/AtomicOps.h"
#include "Charmonizer/Probe/DirManip.h"
#include "Charmonizer/Probe/Floats.h"
#include "Charmonizer/Probe/FuncMacro.h"
#include "Charmonizer/Probe/Headers.h"
#include "Charmonizer/Probe/Integers.h"
#include "Charmonizer/Probe/LargeFiles.h"
#include "Charmonizer/Probe/Memory.h"
#include "Charmonizer/Probe/SymbolVisibility.h"
#include "Charmonizer/Probe/UnusedVars.h"
#include "Charmonizer/Probe/VariadicMacros.h"
#include "Charmonizer/Core/HeaderChecker.h"
#include "Charmonizer/Core/ConfWriter.h"
#include "Charmonizer/Core/ConfWriterC.h"
#include "Charmonizer/Core/ConfWriterPerl.h"
#include "Charmonizer/Core/ConfWriterRuby.h"

#define MAX_CC_LEN 128
#define MAX_FLAGS_LEN 2048

struct CLIArgs {
    char cc_command[MAX_CC_LEN + 1];
    char cc_flags[MAX_FLAGS_LEN + 1];
    int  enable_c;
    int  enable_perl;
    int  enable_ruby;
};

/* Parse command line arguments. */
static void
S_parse_arguments(int argc, char **argv, struct CLIArgs *args) {
    int i;
    int output_enabled = 0;

    /* Parse most args. */
    for (i = 1; i < argc; i++) {
        char *arg = argv[i];
        if (strcmp(arg, "--") == 0) {
            /* From here on out, everything will be a compiler flag. */
            i++;
            break;
        }
        if (strcmp(arg, "--enable-c") == 0) {
            args->enable_c = 1;
            output_enabled = 1;
        }
        else if (strcmp(arg, "--enable-perl") == 0) {
            args->enable_perl = 1;
            output_enabled = 1;
        }
        else if (strcmp(arg, "--enable-ruby") == 0) {
            args->enable_ruby = 1;
            output_enabled = 1;
        }
        else if (memcmp(arg, "--cc=", 5) == 0) {
            if (strlen(arg) > MAX_CC_LEN - 5) {
                fprintf(stderr, "Exceeded max length for compiler command");
                exit(1);
            }
            strcpy(args->cc_command, arg + 5);
        }
    }

    /* Accumulate compiler flags. */
    for (; i < argc; i++) {
        char *arg = argv[i];
        if (strlen(arg) + strlen(args->cc_flags) + 2 >= MAX_FLAGS_LEN) {
            fprintf(stderr, "Exceeded max length for compiler flags");
            exit(1);
        }
        strcat(args->cc_flags, " ");
        strcat(args->cc_flags, arg);

    }

    /* Validate. */
    if (!args->cc_command
        || !strlen(args->cc_command)
        || !output_enabled
       ) {
        fprintf(stderr,
                "Usage: ./charmonize --cc=CC_COMMAND [--enable-c] "
                "[--enable-perl] [--enable-ruby] -- CC_FLAGS\n");
        exit(1);
    }

}

int main(int argc, char **argv) {
    /* Initialize. */
    {
        struct chaz_CLIArgs args;
        int result = chaz_Probe_parse_cli_args(argc, argv, &args);
        if (!result) {
            chaz_Probe_die_usage();
        }
        chaz_Probe_init(&args);
    }

    /* Run probe modules. */
    chaz_DirManip_run();
    chaz_Headers_run();
    chaz_AtomicOps_run();
    chaz_FuncMacro_run();
    chaz_Integers_run();
    chaz_Floats_run();
    chaz_LargeFiles_run();
    chaz_Memory_run();
    chaz_SymbolVisibility_run();
    chaz_UnusedVars_run();
    chaz_VariadicMacros_run();

    /* Write custom postamble. */
    chaz_ConfWriter_append_conf(
        "#ifdef CHY_HAS_SYS_TYPES_H\n"
        "  #include <sys/types.h>\n"
        "#endif\n\n"
    );
    chaz_ConfWriter_append_conf(
        "#ifdef CHY_HAS_STDARG_H\n"
        "  #include <stdarg.h>\n"
        "#endif\n\n"
    );
    chaz_ConfWriter_append_conf(
        "#ifdef CHY_HAS_ALLOCA_H\n"
        "  #include <alloca.h>\n"
        "#elif defined(CHY_HAS_MALLOC_H)\n"
        "  #include <malloc.h>\n"
        "#elif defined(CHY_ALLOCA_IN_STDLIB_H)\n"
        "  #include <stdlib.h>\n"
        "#endif\n\n"
    );
    chaz_ConfWriter_append_conf(
        "#ifdef CHY_HAS_WINDOWS_H\n"
        "  /* Target Windows XP. */\n"
        "  #ifndef WINVER\n"
        "    #define WINVER 0x0500\n"
        "  #endif\n"
        "  #ifndef _WIN32_WINNT\n"
        "    #define _WIN32_WINNT 0x0500\n"
        "  #endif\n"
        "#endif\n\n"
    );

    /* Clean up. */
    chaz_Probe_clean_up();

    return 0;
}


