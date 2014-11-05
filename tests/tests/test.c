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

#include "Charmonizer/Test.h"

int
main() {
    int success =
        chaz_Test_test_dir_manip() &&
        chaz_Test_test_func_macro() &&
        chaz_Test_test_headers() &&
        chaz_Test_test_integers() &&
        chaz_Test_test_large_files() &&
        chaz_Test_test_unused_vars() &&
        chaz_Test_test_variadic_macros();

    return success ? 0 : 1;
}

