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

#include "Charmonizer/Core/HeaderChecker.h"
#include "Charmonizer/Core/Compiler.h"
#include "Charmonizer/Core/ConfWriter.h"
#include "Charmonizer/Core/Util.h"
#include "Charmonizer/Probe/Integers.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Determine endian-ness of this machine.
 */
static int
chaz_Integers_machine_is_big_endian(void);

static const char chaz_Integers_sizes_code[] =
    CHAZ_QUOTE(  #include <stdio.h>                        )
    CHAZ_QUOTE(  int main () {                             )
    CHAZ_QUOTE(      printf("%d ", (int)sizeof(char));     )
    CHAZ_QUOTE(      printf("%d ", (int)sizeof(short));    )
    CHAZ_QUOTE(      printf("%d ", (int)sizeof(int));      )
    CHAZ_QUOTE(      printf("%d ", (int)sizeof(long));     )
    CHAZ_QUOTE(      printf("%d ", (int)sizeof(void*));    )
    CHAZ_QUOTE(      printf("%d ", (int)sizeof(size_t));   )
    CHAZ_QUOTE(      return 0;                             )
    CHAZ_QUOTE(  }                                         );

static const char chaz_Integers_type64_code[] =
    CHAZ_QUOTE(  #include <stdio.h>                        )
    CHAZ_QUOTE(  int main()                                )
    CHAZ_QUOTE(  {                                         )
    CHAZ_QUOTE(      printf("%%d", (int)sizeof(%s));       )
    CHAZ_QUOTE(      return 0;                             )
    CHAZ_QUOTE(  }                                         );

static const char chaz_Integers_literal64_code[] =
    CHAZ_QUOTE(  #include <stdio.h>                        )
    CHAZ_QUOTE(  #define big 9000000000000000000%s         )
    CHAZ_QUOTE(  int main()                                )
    CHAZ_QUOTE(  {                                         )
    CHAZ_QUOTE(      int truncated = (int)big;             )
    CHAZ_QUOTE(      printf("%%d\n", truncated);           )
    CHAZ_QUOTE(      return 0;                             )
    CHAZ_QUOTE(  }                                         );

static const char chaz_Integers_u64_to_double_code[] =
    CHAZ_QUOTE(  #include <stdio.h>                        )
    CHAZ_QUOTE(  int main()                                )
    CHAZ_QUOTE(  {                                         )
    CHAZ_QUOTE(      unsigned %s int_num = 0;              )
    CHAZ_QUOTE(      double float_num;                     )
    CHAZ_QUOTE(      float_num = (double)int_num;          )
    CHAZ_QUOTE(      printf("%%f\n", float_num);           )
    CHAZ_QUOTE(      return 0;                             )
    CHAZ_QUOTE(  }                                         );

void
chaz_Integers_run(void) {
    char *output;
    size_t output_len;
    int sizeof_char       = -1;
    int sizeof_short      = -1;
    int sizeof_int        = -1;
    int sizeof_ptr        = -1;
    int sizeof_long       = -1;
    int sizeof_long_long  = -1;
    int sizeof___int64    = -1;
    int sizeof_size_t     = -1;
    int has_8             = false;
    int has_16            = false;
    int has_32            = false;
    int has_64            = false;
    int has_long_long     = false;
    int has___int64       = false;
    int has_inttypes      = chaz_HeadCheck_check_header("inttypes.h");
    int has_stdint        = chaz_HeadCheck_check_header("stdint.h");
    int can_convert_u64_to_double = true;
    char i32_t_type[10];
    char i32_t_postfix[10];
    char u32_t_postfix[10];
    char i64_t_type[10];
    char i64_t_postfix[10];
    char u64_t_postfix[10];
    char code_buf[1000];
    char scratch[50];

    chaz_ConfWriter_start_module("Integers");

    /* Document endian-ness. */
    if (chaz_Integers_machine_is_big_endian()) {
        chaz_ConfWriter_add_def("BIG_END", NULL);
    }
    else {
        chaz_ConfWriter_add_def("LITTLE_END", NULL);
    }

    /* Record sizeof() for several common integer types. */
    output = chaz_CC_capture_output(chaz_Integers_sizes_code, &output_len);
    if (output != NULL) {
        char *ptr     = output;
        char *end_ptr = output;

        sizeof_char   = strtol(ptr, &end_ptr, 10);
        ptr           = end_ptr;
        sizeof_short  = strtol(ptr, &end_ptr, 10);
        ptr           = end_ptr;
        sizeof_int    = strtol(ptr, &end_ptr, 10);
        ptr           = end_ptr;
        sizeof_long   = strtol(ptr, &end_ptr, 10);
        ptr           = end_ptr;
        sizeof_ptr    = strtol(ptr, &end_ptr, 10);
        ptr           = end_ptr;
        sizeof_size_t = strtol(ptr, &end_ptr, 10);

        free(output);
    }

    /* Determine whether long longs are available. */
    sprintf(code_buf, chaz_Integers_type64_code, "long long");
    output = chaz_CC_capture_output(code_buf, &output_len);
    if (output != NULL) {
        has_long_long    = true;
        sizeof_long_long = strtol(output, NULL, 10);
        free(output);
    }

    /* Determine whether the __int64 type is available. */
    sprintf(code_buf, chaz_Integers_type64_code, "__int64");
    output = chaz_CC_capture_output(code_buf, &output_len);
    if (output != NULL) {
        has___int64 = true;
        sizeof___int64 = strtol(output, NULL, 10);
        free(output);
    }

    /* Figure out which integer types are available. */
    if (sizeof_char == 1) {
        has_8 = true;
    }
    if (sizeof_short == 2) {
        has_16 = true;
    }
    if (sizeof_int == 4) {
        has_32 = true;
        strcpy(i32_t_type, "int");
        strcpy(i32_t_postfix, "");
        strcpy(u32_t_postfix, "U");
    }
    else if (sizeof_long == 4) {
        has_32 = true;
        strcpy(i32_t_type, "long");
        strcpy(i32_t_postfix, "L");
        strcpy(u32_t_postfix, "UL");
    }
    if (sizeof_long == 8) {
        has_64 = true;
        strcpy(i64_t_type, "long");
    }
    else if (sizeof_long_long == 8) {
        has_64 = true;
        strcpy(i64_t_type, "long long");
    }
    else if (sizeof___int64 == 8) {
        has_64 = true;
        strcpy(i64_t_type, "__int64");
    }

    /* Probe for 64-bit literal syntax. */
    if (has_64 && sizeof_long == 8) {
        strcpy(i64_t_postfix, "L");
        strcpy(u64_t_postfix, "UL");
    }
    else if (has_64) {
        sprintf(code_buf, chaz_Integers_literal64_code, "LL");
        output = chaz_CC_capture_output(code_buf, &output_len);
        if (output != NULL) {
            strcpy(i64_t_postfix, "LL");
            free(output);
        }
        else {
            sprintf(code_buf, chaz_Integers_literal64_code, "i64");
            output = chaz_CC_capture_output(code_buf, &output_len);
            if (output != NULL) {
                strcpy(i64_t_postfix, "i64");
                free(output);
            }
            else {
                chaz_Util_die("64-bit types, but no literal syntax found");
            }
        }
        sprintf(code_buf, chaz_Integers_literal64_code, "ULL");
        output = chaz_CC_capture_output(code_buf, &output_len);
        if (output != NULL) {
            strcpy(u64_t_postfix, "ULL");
            free(output);
        }
        else {
            sprintf(code_buf, chaz_Integers_literal64_code, "Ui64");
            output = chaz_CC_capture_output(code_buf, &output_len);
            if (output != NULL) {
                strcpy(u64_t_postfix, "Ui64");
                free(output);
            }
            else {
                chaz_Util_die("64-bit types, but no literal syntax found");
            }
        }
    }

    /* Write out some conditional defines. */
    if (has_inttypes) {
        chaz_ConfWriter_add_def("HAS_INTTYPES_H", NULL);
    }
    if (has_stdint) {
        chaz_ConfWriter_add_def("HAS_STDINT_H", NULL);
    }
    if (has_long_long) {
        chaz_ConfWriter_add_def("HAS_LONG_LONG", NULL);
    }
    if (has___int64) {
        chaz_ConfWriter_add_def("HAS___INT64", NULL);
    }

    /* Write out sizes. */
    sprintf(scratch, "%d", sizeof_char);
    chaz_ConfWriter_add_def("SIZEOF_CHAR", scratch);
    sprintf(scratch, "%d", sizeof_short);
    chaz_ConfWriter_add_def("SIZEOF_SHORT", scratch);
    sprintf(scratch, "%d", sizeof_int);
    chaz_ConfWriter_add_def("SIZEOF_INT", scratch);
    sprintf(scratch, "%d", sizeof_long);
    chaz_ConfWriter_add_def("SIZEOF_LONG", scratch);
    sprintf(scratch, "%d", sizeof_ptr);
    chaz_ConfWriter_add_def("SIZEOF_PTR", scratch);
    sprintf(scratch, "%d", sizeof_size_t);
    chaz_ConfWriter_add_def("SIZEOF_SIZE_T", scratch);
    if (has_long_long) {
        sprintf(scratch, "%d", sizeof_long_long);
        chaz_ConfWriter_add_def("SIZEOF_LONG_LONG", scratch);
    }
    if (has___int64) {
        sprintf(scratch, "%d", sizeof___int64);
        chaz_ConfWriter_add_def("SIZEOF___INT64", scratch);
    }

    /* Write affirmations. */
    if (has_8) {
        chaz_ConfWriter_add_def("HAS_INT8_T", NULL);
    }
    if (has_16) {
        chaz_ConfWriter_add_def("HAS_INT16_T", NULL);
    }
    if (has_32) {
        chaz_ConfWriter_add_def("HAS_INT32_T", NULL);
    }
    if (has_64) {
        chaz_ConfWriter_add_def("HAS_INT64_T", NULL);
    }

    /* Create macro for promoting pointers to integers. */
    if (has_64) {
        if (sizeof_ptr == 8) {
            chaz_ConfWriter_add_def("PTR_TO_I64(ptr)",
                                    "((int64_t)(uint64_t)(ptr))");
        }
        else {
            chaz_ConfWriter_add_def("PTR_TO_I64(ptr)",
                                    "((int64_t)(uint32_t)(ptr))");
        }
    }

    /* Create macro for converting uint64_t to double. */
    if (has_64) {
        /*
         * Determine whether unsigned 64-bit integers can be converted to
         * double. Older MSVC versions don't support this conversion.
         */
        sprintf(code_buf, chaz_Integers_u64_to_double_code, i64_t_type);
        output = chaz_CC_capture_output(code_buf, &output_len);
        if (output != NULL) {
            chaz_ConfWriter_add_def("U64_TO_DOUBLE(num)", "((double)(num))");
            free(output);
        }
        else {
            chaz_ConfWriter_add_def(
                "U64_TO_DOUBLE(num)",
                "((num) & UINT64_C(0x8000000000000000) ? "
                "(double)(int64_t)((num) & UINT64_C(0x7FFFFFFFFFFFFFFF)) + "
                "9223372036854775808.0 : "
                "(double)(int64_t)(num))");
        }
    }

    chaz_ConfWriter_end_module();

    /* Integer typedefs. */

    chaz_ConfWriter_start_module("IntegerTypes");

    if (has_stdint) {
        chaz_ConfWriter_add_sys_include("stdint.h");
    }
    else {
        /* We support only the following subset of stdint.h
         *   int8_t
         *   int16_t
         *   int32_t
         *   int64_t
         *   uint8_t
         *   uint16_t
         *   uint32_t
         *   uint64_t
         */
        if (has_8) {
            chaz_ConfWriter_add_global_typedef("signed char", "int8_t");
            chaz_ConfWriter_add_global_typedef("unsigned char", "uint8_t");
        }
        if (has_16) {
            chaz_ConfWriter_add_global_typedef("signed short", "int16_t");
            chaz_ConfWriter_add_global_typedef("unsigned short", "uint16_t");
        }
        if (has_32) {
            chaz_ConfWriter_add_global_typedef(i32_t_type, "int32_t");
            sprintf(scratch, "unsigned %s", i32_t_type);
            chaz_ConfWriter_add_global_typedef(scratch, "uint32_t");
        }
        if (has_64) {
            chaz_ConfWriter_add_global_typedef(i64_t_type, "int64_t");
            sprintf(scratch, "unsigned %s", i64_t_type);
            chaz_ConfWriter_add_global_typedef(scratch, "uint64_t");
        }
    }

    chaz_ConfWriter_end_module();

    /* Integer limits. */

    chaz_ConfWriter_start_module("IntegerLimits");

    if (has_stdint) {
        chaz_ConfWriter_add_sys_include("stdint.h");
    }
    else {
        /* We support only the following subset of stdint.h
         *   INT8_MAX
         *   INT16_MAX
         *   INT32_MAX
         *   INT64_MAX
         *   INT8_MIN
         *   INT16_MIN
         *   INT32_MIN
         *   INT64_MIN
         *   UINT8_MAX
         *   UINT16_MAX
         *   UINT32_MAX
         *   UINT64_MAX
         *   SIZE_MAX
         */
        if (has_8) {
            chaz_ConfWriter_add_global_def("INT8_MAX", "127");
            chaz_ConfWriter_add_global_def("INT8_MIN", "-128");
            chaz_ConfWriter_add_global_def("UINT8_MAX", "255");
        }
        if (has_16) {
            chaz_ConfWriter_add_global_def("INT16_MAX", "32767");
            chaz_ConfWriter_add_global_def("INT16_MIN", "-32768");
            chaz_ConfWriter_add_global_def("UINT16_MAX", "65535");
        }
        if (has_32) {
            chaz_ConfWriter_add_global_def("INT32_MAX", "2147483647");
            chaz_ConfWriter_add_global_def("INT32_MIN", "(-INT32_MAX-1)");
            chaz_ConfWriter_add_global_def("UINT32_MAX", "4294967295U");
        }
        if (has_64) {
            sprintf(scratch, "9223372036854775807%s", i64_t_postfix);
            chaz_ConfWriter_add_global_def("INT64_MAX", scratch);
            chaz_ConfWriter_add_global_def("INT64_MIN", "(-INT64_MAX-1)");
            sprintf(scratch, "18446744073709551615%s", u64_t_postfix);
            chaz_ConfWriter_add_global_def("UINT64_MAX", scratch);
        }
        chaz_ConfWriter_add_global_def("SIZE_MAX", "((size_t)-1)");
    }

    chaz_ConfWriter_end_module();

    /* Integer literals. */

    chaz_ConfWriter_start_module("IntegerLiterals");

    if (has_stdint) {
        chaz_ConfWriter_add_sys_include("stdint.h");
    }
    else {
        /* We support only the following subset of stdint.h
         *   INT32_C
         *   INT64_C
         *   UINT32_C
         *   UINT64_C
         */
        if (has_32) {
            if (strcmp(i32_t_postfix, "") == 0) {
                chaz_ConfWriter_add_global_def("INT32_C(n)", "n");
            }
            else {
                sprintf(scratch, "n##%s", i32_t_postfix);
                chaz_ConfWriter_add_global_def("INT32_C(n)", scratch);
            }
            sprintf(scratch, "n##%s", u32_t_postfix);
            chaz_ConfWriter_add_global_def("UINT32_C(n)", scratch);
        }
        if (has_64) {
            sprintf(scratch, "n##%s", i64_t_postfix);
            chaz_ConfWriter_add_global_def("INT64_C(n)", scratch);
            sprintf(scratch, "n##%s", u64_t_postfix);
            chaz_ConfWriter_add_global_def("UINT64_C(n)", scratch);
        }
    }

    chaz_ConfWriter_end_module();

    /* Integer format strings. */

    chaz_ConfWriter_start_module("IntegerFormatStrings");

    if (has_inttypes) {
        chaz_ConfWriter_add_sys_include("inttypes.h");
    }
    else {
        /* We support only the following subset of inttypes.h
         *   PRId64
         *   PRIu64
         */
        if (has_64) {
            int i;
            const char *options[] = {
                "ll",
                "l",
                "L",
                "q",  /* Some *BSDs */
                "I64", /* Microsoft */
                NULL,
            };

            /* Buffer to hold the code, and its start and end. */
            static const char format_64_code[] =
                CHAZ_QUOTE(  #include <stdio.h>                            )
                CHAZ_QUOTE(  int main() {                                  )
                CHAZ_QUOTE(      printf("%%%su", 18446744073709551615%s);  )
                CHAZ_QUOTE(      return 0;                                 )
                CHAZ_QUOTE( }                                              );

            for (i = 0; options[i] != NULL; i++) {
                /* Try to print 2**64-1, and see if we get it back intact. */
                sprintf(code_buf, format_64_code, options[i], u64_t_postfix);
                output = chaz_CC_capture_output(code_buf, &output_len);

                if (output != NULL
                    && strcmp(output, "18446744073709551615") == 0
                   ) {
                    sprintf(scratch, "\"%sd\"", options[i]);
                    chaz_ConfWriter_add_global_def("PRId64", scratch);
                    sprintf(scratch, "\"%su\"", options[i]);
                    chaz_ConfWriter_add_global_def("PRIu64", scratch);
                    free(output);
                    break;
                }
            }
        }
    }

    chaz_ConfWriter_end_module();
}

static int
chaz_Integers_machine_is_big_endian(void) {
    long one = 1;
    return !(*((char*)(&one)));
}


