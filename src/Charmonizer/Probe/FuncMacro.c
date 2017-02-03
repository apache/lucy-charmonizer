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

#include "Charmonizer/Core/Compiler.h"
#include "Charmonizer/Core/ConfWriter.h"
#include "Charmonizer/Core/Util.h"
#include "Charmonizer/Probe/FuncMacro.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Attempt to verify inline keyword. */
static int
chaz_FuncMacro_try_inline(const char *keyword) {
    static const char inline_code[] = "static %s int f() { return 1; }";
    char code[sizeof(inline_code) + 30];
    sprintf(code, inline_code, keyword);
    return chaz_CC_test_compile(code);
}

static void
chaz_FuncMacro_probe_inline(void) {
    static const char* inline_options[] = {
        "__inline",
        "__inline__",
        "inline"
    };
    const int num_inline_options = sizeof(inline_options) / sizeof(void*);
    int has_inline = false;
    int i;

    for (i = 0; i < num_inline_options; i++) {
        const char *inline_option = inline_options[i];
        if (chaz_FuncMacro_try_inline(inline_option)) {
            has_inline = true;
            chaz_ConfWriter_add_def("INLINE", inline_option);
            break;
        }
    }
    if (!has_inline) {
        chaz_ConfWriter_add_def("INLINE", NULL);
    }
}

void
chaz_FuncMacro_run(void) {
    int has_funcmac      = false;
    int has_iso_funcmac  = false;
    int has_gnuc_funcmac = false;

    chaz_ConfWriter_start_module("FuncMacro");

    /* Check for func macros. */
    if (chaz_CC_test_compile("const char *f() { return __func__; }")) {
        has_funcmac     = true;
        has_iso_funcmac = true;
    }
    if (chaz_CC_test_compile("const char *f() { return __FUNCTION__; }")) {
        has_funcmac      = true;
        has_gnuc_funcmac = true;
    }

    /* Write out common defines. */
    if (has_funcmac) {
        const char *macro_text = has_iso_funcmac
                                 ? "__func__"
                                 : "__FUNCTION__";
        chaz_ConfWriter_add_def("HAS_FUNC_MACRO", NULL);
        chaz_ConfWriter_add_def("FUNC_MACRO", macro_text);
    }

    /* Write out specific defines. */
    if (has_iso_funcmac) {
        chaz_ConfWriter_add_def("HAS_ISO_FUNC_MACRO", NULL);
    }
    if (has_gnuc_funcmac) {
        chaz_ConfWriter_add_def("HAS_GNUC_FUNC_MACRO", NULL);
    }

    /* Check for inline keyword. */
    chaz_FuncMacro_probe_inline();

    chaz_ConfWriter_end_module();
}



