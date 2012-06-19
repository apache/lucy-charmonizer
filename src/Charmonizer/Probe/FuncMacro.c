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

#include "Charmonizer/Core/Compiler.h"
#include "Charmonizer/Core/ConfWriter.h"
#include "Charmonizer/Core/Util.h"
#include "Charmonizer/Probe/FuncMacro.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Code for verifying ISO func macro. */
static const char iso_func_code[] =
    QUOTE(  #include "_charm.h"               )
    QUOTE(  int main() {                      )
    QUOTE(      Charm_Setup;                  )
    QUOTE(      printf("%s", __func__);       )
    QUOTE(      return 0;                     )
    QUOTE(  }                                 );

/* Code for verifying GNU func macro. */
static const char gnuc_func_code[] =
    QUOTE(  #include "_charm.h"               )
    QUOTE(  int main() {                      )
    QUOTE(      Charm_Setup;                  )
    QUOTE(      printf("%s", __FUNCTION__);   )
    QUOTE(      return 0;                     )
    QUOTE(  }                                 );

/* Code for verifying inline keyword. */
static const char inline_code[] =
    QUOTE(  #include "_charm.h"               )
    QUOTE(  static %s int foo() { return 1; } )
    QUOTE(  int main() {                      )
    QUOTE(      Charm_Setup;                  )
    QUOTE(      printf("%%d", foo());         )
    QUOTE(      return 0;                     )
    QUOTE(  }                                 );

static char*
S_try_inline(const char *keyword, size_t *output_len) {
    char code[sizeof(inline_code) + 30];
    sprintf(code, inline_code, keyword);
    return CC_capture_output(code, output_len);
}

static const char* inline_options[] = {
    "__inline",
    "__inline__",
    "inline"
};
static int num_inline_options = sizeof(inline_options) / sizeof(void*);

void
FuncMacro_run(void) {
    int i;
    char *output;
    size_t output_len;
    int has_funcmac      = false;
    int has_iso_funcmac  = false;
    int has_gnuc_funcmac = false;
    int has_inline       = false;

    ConfWriter_start_module("FuncMacro");

    /* Check for ISO func macro. */
    output = CC_capture_output(iso_func_code, &output_len);
    if (output != NULL && strncmp(output, "main", 4) == 0) {
        has_funcmac     = true;
        has_iso_funcmac = true;
    }
    free(output);

    /* Check for GNUC func macro. */
    output = CC_capture_output(gnuc_func_code, &output_len);
    if (output != NULL && strncmp(output, "main", 4) == 0) {
        has_funcmac      = true;
        has_gnuc_funcmac = true;
    }
    free(output);

    /* Write out common defines. */
    if (has_funcmac) {
        const char *macro_text = has_iso_funcmac
                                 ? "__func__"
                                 : "__FUNCTION__";
        ConfWriter_add_def("HAS_FUNC_MACRO", NULL);
        ConfWriter_add_def("FUNC_MACRO", macro_text);
    }

    /* Write out specific defines. */
    if (has_iso_funcmac) {
        ConfWriter_add_def("HAS_ISO_FUNC_MACRO", NULL);
    }
    if (has_gnuc_funcmac) {
        ConfWriter_add_def("HAS_GNUC_FUNC_MACRO", NULL);
    }

    /* Check for inline keyword. */

    for (i = 0; i < num_inline_options; i++) {
        const char *inline_option = inline_options[i];
        output = S_try_inline(inline_option, &output_len);
        if (output != NULL) {
            has_inline = true;
            ConfWriter_add_def("INLINE", inline_option);
            free(output);
            break;
        }
    }
    if (!has_inline) {
        ConfWriter_add_def("INLINE", NULL);
    }

    ConfWriter_end_module();
}



