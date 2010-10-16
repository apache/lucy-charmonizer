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

#include "Charmonizer/Test.h"
#include "Charmonizer/Test/AllTests.h"
#include <stdio.h>
#include <string.h>
#include "charmony.h"
#ifdef HAS_DIRENT_H
  #include <dirent.h>
#endif
#ifdef HAS_SYS_STAT_H
  #include <sys/stat.h>
#endif
#ifdef HAS_SYS_TYPES_H
  #include <sys/stat.h>
#endif
#ifdef HAS_UNISTD_H
  #include <unistd.h>
#endif
#ifdef HAS_DIRECT_H
  #include <direct.h>
#endif

TestBatch*
TestDirManip_prepare()
{
    return Test_new_batch("Integers", 6, TestDirManip_run);
}

void
TestDirManip_run(TestBatch *batch)
{
    TEST_INT_EQ(batch, 0, makedir("_chaz_test_dir", 0777), "makedir");
    TEST_INT_EQ(batch, 0, makedir("_chaz_test_dir" DIR_SEP "deep", 0777),
        "makedir with DIR_SEP");
    TEST_INT_EQ(batch, 0, rmdir("_chaz_test_dir" DIR_SEP "deep"),
        "rmdir with DIR_SEP");
    TEST_INT_EQ(batch, 0, rmdir("_chaz_test_dir"), "rmdir");
#ifdef CHY_HAS_DIRENT_D_NAMLEN
    {
        struct dirent entry;
        entry.d_namlen = 5;
        TEST_INT_EQ(batch, 5, entry.d_namlen, "d_namlen");
    }
#else
    SKIP(batch, "no d_namlen member on this platform");
#endif
#ifdef CHY_HAS_DIRENT_D_TYPE
    {
        struct dirent entry;
        entry.d_type = 5;
        TEST_INT_EQ(batch, 5, entry.d_type, "d_type");
    }
#else
    SKIP(batch, "no d_type member on this platform");
#endif
}


