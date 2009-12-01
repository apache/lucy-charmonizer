/* Charmonizer/CompilerSpec.h
 */

#ifndef H_CHAZ_COMPILER_SPEC
#define H_CHAZ_COMPILER_SPEC

#include <stddef.h>
#include "Charmonizer/Core/Defines.h"

typedef struct chaz_CompilerSpec chaz_CompilerSpec;

struct chaz_CompilerSpec {
    char *nickname;
    char *include_flag;
    char *object_flag;
    char *exe_flag;
};

/* Detect a supported compiler and return its profile.
 */
chaz_CompilerSpec*
chaz_CCSpec_find_spec();

#ifdef CHAZ_USE_SHORT_NAMES
  #define CompilerSpec                chaz_CompilerSpec
  #define CCSpec_find_spec            chaz_CCSpec_find_spec
#endif

#endif /* H_CHAZ_COMPILER_SPEC */

/**
 * Copyright 2006 The Apache Software Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
