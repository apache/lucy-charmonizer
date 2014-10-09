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

#ifndef H_CHAZ_CLI
#define H_CHAZ_CLI 1

#ifdef __cplusplus
extern "C" {
#endif

#define CHAZ_CLI_NO_ARG       0
#define CHAZ_CLI_ARG_REQUIRED (1 << 0)
#define CHAZ_CLI_ARG_OPTIONAL (1 << 1)

/* The CLI module provides argument parsing for a command line interface.
 */

typedef struct chaz_CLI chaz_CLI;

/* Constructor.
 *
 * @param name The name of the application.
 * @param description A description of the application.
 */
chaz_CLI*
chaz_CLI_new(const char *name, const char *description);

/* Destructor.
 */
void
chaz_CLI_destroy(chaz_CLI *self);

/* Return a string combining usage header with documentation of options.
 */
const char*
chaz_CLI_help(chaz_CLI *self);

/* Override the generated usage header.
 */
void
chaz_CLI_set_usage(chaz_CLI *self, const char *usage);

/* Register an option.  Updates the "help" string, invalidating previous
 * values.  Returns true on success, or reports an error and returns false if
 * the option was already registered.
 */
int
chaz_CLI_register(chaz_CLI *self, const char *name, const char *help,
                  int flags);

/* Set an option.  The specified option must have been registered previously.
 * The supplied `value` is optional and will be copied.
 *
 * Returns true on success.  Reports an error and returns false on failure.
 */
int
chaz_CLI_set(chaz_CLI *self, const char *name, const char *value);

/* Returns true if the option has been set, false otherwise.
 */
int
chaz_CLI_defined(chaz_CLI *self, const char *name);

/* Return the value of a given option converted to a long int.  Defaults to 0.
 * Reports an error if the named option has not been registered.
 */
long
chaz_CLI_longval(chaz_CLI *self, const char *name);

/* Return the value of an option as a C string.  Defaults to NULL.  Reports an
 * error if the named option has not been registered.
 */
const char*
chaz_CLI_strval(chaz_CLI *self, const char *name);

/* Unset an option, making subsequent calls to `get` return false and making
 * it possible to call `set` again.
 *
 * Returns true if the option exists and was able to be unset.
 */
int
chaz_CLI_unset(chaz_CLI *self, const char *name);

/* Parse `argc` and `argv`, setting options as appropriate.  Returns true on
 * success.  Reports an error and returns false if either an unexpected option
 * was encountered or an option which requires an argument was supplied
 * without one.
 */
int
chaz_CLI_parse(chaz_CLI *self, int argc, const char *argv[]);

#ifdef __cplusplus
}
#endif

#endif /* H_CHAZ_CLI */


