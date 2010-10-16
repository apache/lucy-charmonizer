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

#define CHAZ_USE_SHORT_NAMES

#include "Charmonizer/Probe/Memory.h"
#include "Charmonizer/Core/Compiler.h"
#include "Charmonizer/Core/HeaderChecker.h"
#include "Charmonizer/Core/ConfWriter.h"
#include "Charmonizer/Core/Util.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static char alloca_code[] = 
           "#include <%s>\n"
    QUOTE(  int main() {                   )
    QUOTE(      void *foo = %s(1);         )
    QUOTE(      return 0;                  )
    QUOTE(  }                              );

void
Memory_run(void) 
{
    chaz_bool_t has_alloca_h  = false;
    chaz_bool_t has_malloc_h  = false;
    chaz_bool_t need_stdlib_h = false;
    chaz_bool_t has_alloca    = false;
    chaz_bool_t has_underscore_alloca = false;
    char code_buf[sizeof(alloca_code) + 100];

    ConfWriter_start_module("Memory");

    /* Unixen. */
    sprintf(code_buf, alloca_code, "alloca.h", "alloca");
    if (CC_test_compile(code_buf, strlen(code_buf))) {
        has_alloca_h = true;
        has_alloca   = true;
        ConfWriter_append_conf("#define CHY_HAS_ALLOCA_H\n");
        ConfWriter_append_conf("#define chy_alloca alloca\n");
    }
    else {
        sprintf(code_buf, alloca_code, "stdlib.h", "alloca");
        if (CC_test_compile(code_buf, strlen(code_buf))) {
            has_alloca    = true;
            need_stdlib_h = true;
            ConfWriter_append_conf("#define CHY_ALLOCA_IN_STDLIB_H\n");
            ConfWriter_append_conf("#define chy_alloca alloca\n");
        }
    }

    /* Windows. */
    if (!has_alloca) {
        sprintf(code_buf, alloca_code, "malloc.h", "alloca");
        if (CC_test_compile(code_buf, strlen(code_buf))) {
            has_malloc_h = true;
            has_alloca   = true;
            ConfWriter_append_conf("#define CHY_HAS_MALLOC_H\n");
            ConfWriter_append_conf("#define chy_alloca alloca\n");
        }
    }
    if (!has_alloca) {
        sprintf(code_buf, alloca_code, "malloc.h", "_alloca");
        if (CC_test_compile(code_buf, strlen(code_buf))) {
            has_malloc_h = true;
            has_underscore_alloca = true;
            ConfWriter_append_conf("#define CHY_HAS_MALLOC_H\n");
            ConfWriter_append_conf("#define chy_alloca _alloca\n");
        }
    }

    /* Shorten */
    ConfWriter_start_short_names();
    if (has_alloca_h) {
        ConfWriter_shorten_macro("HAS_ALLOCA_H");
    }
    if (has_malloc_h) {
        ConfWriter_shorten_macro("HAS_MALLOC_H");
        if (!has_alloca && has_underscore_alloca) {
            ConfWriter_shorten_function("alloca");
        }
    }
    if (need_stdlib_h) {
        ConfWriter_shorten_macro("ALLOCA_IN_STDLIB_H");
    }
    ConfWriter_end_short_names();

    ConfWriter_end_module();
}


