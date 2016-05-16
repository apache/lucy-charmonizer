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

/* Charmonizer/Core/Library.h
 */

#ifndef H_CHAZ_LIB
#define H_CHAZ_LIB

#ifdef __cplusplus
extern "C" {
#endif

typedef struct chaz_Lib chaz_Lib;

chaz_Lib*
chaz_Lib_new_shared(const char *name, const char *version,
                    const char *major_version);

chaz_Lib*
chaz_Lib_new_static(const char *name);

void
chaz_Lib_destroy(chaz_Lib *flags);

const char*
chaz_Lib_get_name(chaz_Lib *lib);

const char*
chaz_Lib_get_version(chaz_Lib *lib);

const char*
chaz_Lib_get_major_version(chaz_Lib *lib);

int
chaz_Lib_is_shared(chaz_Lib *lib);

int
chaz_Lib_is_static(chaz_Lib *lib);

char*
chaz_Lib_filename(chaz_Lib *lib);

char*
chaz_Lib_major_version_filename(chaz_Lib *lib);

char*
chaz_Lib_no_version_filename(chaz_Lib *lib);

char*
chaz_Lib_implib_filename(chaz_Lib *lib);

char*
chaz_Lib_export_filename(chaz_Lib *lib);

#ifdef __cplusplus
}
#endif

#endif /* H_CHAZ_LIB */


