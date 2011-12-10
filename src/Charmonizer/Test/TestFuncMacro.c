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

#include "charmony.h"
#include <string.h>
#include "Charmonizer/Test.h"

#ifdef INLINE
static INLINE const char* S_inline_function() {
    return "inline works";
}
#endif

static void
S_run_tests(TestBatch *batch) {

#ifdef HAS_FUNC_MACRO
    STR_EQ(FUNC_MACRO, "S_run_tests", "FUNC_MACRO");
#else
    SKIP("no FUNC_MACRO");
#endif

#ifdef HAS_ISO_FUNC_MACRO
    STR_EQ(__func__, "S_run_tests", "HAS_ISO_FUNC_MACRO");
#else
    SKIP("no ISO_FUNC_MACRO");
#endif

#ifdef HAS_GNUC_FUNC_MACRO
    STR_EQ(__FUNCTION__, "S_run_tests", "HAS_GNUC_FUNC_MACRO");
#else
    SKIP("no GNUC_FUNC_MACRO");
#endif

#ifdef INLINE
    PASS(S_inline_function());
#else
    SKIP("no INLINE functions");
#endif
}


int main(int argc, char **argv) {
    TestBatch *batch = Test_start(4);
    S_run_tests(batch);
    return !Test_finish();
}

