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

#include "Charmonizer/Probe/Memory.h"
#include "Charmonizer/Core/Compiler.h"
#include "Charmonizer/Core/HeaderChecker.h"
#include "Charmonizer/Core/ConfWriter.h"
#include "Charmonizer/Core/Util.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Probe for alloca() or equivalent. */
static void
chaz_Memory_probe_alloca(void);

void
chaz_Memory_run(void) {
    chaz_ConfWriter_start_module("Memory");

    chaz_Memory_probe_alloca();

    chaz_ConfWriter_end_module();
}

static void
chaz_Memory_probe_alloca(void) {
    static const char alloca_code[] =
        "#include <%s>\n"
        CHAZ_QUOTE(  int main() {                   )
        CHAZ_QUOTE(      void *foo = %s(1);         )
        CHAZ_QUOTE(      return 0;                  )
        CHAZ_QUOTE(  }                              );
    chaz_CFlags *temp_cflags = chaz_CC_get_temp_cflags();
    int has_alloca = false;
    char code_buf[sizeof(alloca_code) + 100];

    {
        /* OpenBSD needs sys/types.h for sys/mman.h to work and mmap() to be
         * available. Everybody else that has sys/mman.h should have
         * sys/types.h as well. */
        const char *mman_headers[] = {
            "sys/types.h",
            "sys/mman.h",
            NULL
        };
        if (chaz_HeadCheck_check_many_headers((const char**)mman_headers)) {
            chaz_ConfWriter_add_def("HAS_SYS_MMAN_H", NULL);
        }
    }

    /* Under GCC, alloca is a builtin that works without including the
     * correct header, generating only a warning. To avoid misdetection,
     * disable the alloca builtin temporarily. */
    if (chaz_CC_is_gcc()) {
        chaz_CFlags_append(temp_cflags, "-fno-builtin-alloca");
    }

    /* Unixen. */
    sprintf(code_buf, alloca_code, "alloca.h", "alloca");
    if (chaz_CC_test_link(code_buf)) {
        has_alloca = true;
        chaz_ConfWriter_add_def("HAS_ALLOCA_H", NULL);
        chaz_ConfWriter_add_def("alloca", "alloca");
    }
    if (!has_alloca) {
        sprintf(code_buf, alloca_code, "stdlib.h", "alloca");
        if (chaz_CC_test_link(code_buf)) {
            has_alloca = true;
            chaz_ConfWriter_add_def("ALLOCA_IN_STDLIB_H", NULL);
            chaz_ConfWriter_add_def("alloca", "alloca");
        }
    }

    /* Windows. */
    if (!has_alloca) {
        sprintf(code_buf, alloca_code, "malloc.h", "alloca");
        if (chaz_CC_test_link(code_buf)) {
            has_alloca = true;
            chaz_ConfWriter_add_def("HAS_MALLOC_H", NULL);
            chaz_ConfWriter_add_def("alloca", "alloca");
        }
    }
    if (!has_alloca) {
        sprintf(code_buf, alloca_code, "malloc.h", "_alloca");
        if (chaz_CC_test_link(code_buf)) {
            chaz_ConfWriter_add_def("HAS_MALLOC_H", NULL);
            chaz_ConfWriter_add_def("alloca", "_alloca");
        }
    }

    chaz_CFlags_clear(temp_cflags);
}


