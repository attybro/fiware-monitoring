/*
 * Copyright 2013 Telefónica I+D
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */


/**
 * @file   argument_parser.h
 * @brief  Argument parsing macros and declarations
 *
 * This file defines several macros and declares functions to parse the arguments
 * (initially, from Nagios configuration file) passed to [Event Broker](@NagiosModule_ref)
 * module.
 */


#ifndef ARGUMENT_PARSER_H
#define ARGUMENT_PARSER_H


#ifdef __cplusplus
extern "C" {
#endif


/** Macro meaning no option char (therefore, the end of options list) */
#ifndef NO_CHAR
#define NO_CHAR -1
#endif /*NO_CHAR*/


/** Macro meaning unknown option */
#ifndef UNKNOWN_OPTION
#define UNKNOWN_OPTION '?'
#endif /*UNKNOWN_OPTION*/


/** Macro meaning missing option value */
#ifndef MISSING_VALUE
#define MISSING_VALUE ':'
#endif /*MISSING_VALUE*/


/** Option-value pair */
typedef struct option_value {
	int	opt;		/**< option ('?' unknown, ':' missing value)    */
	int	err;		/**< option that caused error (unknown/missing) */
	char*	val;		/**< option value, or NULL if an error is found */
} *option_list_t;		/**< Options list as result of argument parsing */


/**
 * Parses module arguments given in configuration file
 *
 * @param[in] args		The module arguments as a space-separated string (may be null).
 * @param[in] optstr		The option string as defined for [getopt()](@getopt_ref).
 *
 * @return			The module arguments as options list.
 */
option_list_t parse_args(const char* args, const char* optstr);


/**
 * Releases resources for given options list
 *
 * @param[in] optlist		The options list.
 */
void free_option_list(option_list_t optlist);


#ifdef __cplusplus
}
#endif


#endif /*ARGUMENT_PARSER_H*/
