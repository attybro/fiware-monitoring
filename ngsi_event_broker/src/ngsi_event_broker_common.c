/*
 * Copyright 2013 Telefónica I+D
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */


#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "config.h"
#include "nebstructs.h"
#include "nebmodules.h"
#include "nebcallbacks.h"
#include "macros.h"
#include "broker.h"
#include "curl/curl.h"
#include "argument_parser.h"
#include "ngsi_event_broker_common.h"


/* define common global variables */
char* adapter_url = NULL;
char* region_id   = NULL;
char* host_addr   = NULL;


/************************************************************************/


/* deinitialization */
int nebmodule_deinit(int flags, int reason)
{
	int result = 0;

	curl_global_cleanup();
	free_module_variables();

	if (reason != NEBMODULE_ERROR_BAD_INIT) {
		logging("info", "%s - Finished", module_name);
	}

	return result;
}


/* initialization */
int nebmodule_init(int flags, char* args, void* handle)
{
	int result = 0;

	init_module_handle_info(handle);
	if (check_nagios_object_version()) {
		result = -1;
	} else if (init_module_variables(args)) {
		result = -1;
	} else if (curl_global_init(CURL_GLOBAL_ALL)) {
		logging("error", "%s - Could not initialize libcurl", module_name);
		result = -1;
	} else {
		result = neb_register_callback(NEBCALLBACK_SERVICE_CHECK_DATA,
		                               module_handle, 0, callback_service_check);
	}

	/* check for errors in initialization */
	if (result) {
		nebmodule_deinit(0, NEBMODULE_ERROR_BAD_INIT);
	}

	return result;
}


/************************************************************************/


/* checks to make sure Nagios object version matches what we know about */
int check_nagios_object_version(void)
{
	int result = 0;

	if (__nagios_object_structure_version != CURRENT_OBJECT_STRUCTURE_VERSION) {
		logging("error", "%s - Nagios object version mismatch: %d,%d", module_name,
		        __nagios_object_structure_version, CURRENT_OBJECT_STRUCTURE_VERSION);
		result = 1;
	}

	return result;
}


/* initializes module variables */
int init_module_variables(char* args)
{
	char		name[HOST_NAME_MAX];
	char		addr[INET_ADDRSTRLEN];
	struct hostent* host	= NULL;
	option_list_t	opts	= NULL;
	int		result	= 0;

	/* process arguments passed to module in Nagios configuration file */
	if ((opts = parse_args(args, ":u:r:")) != NULL) {
		size_t	i;
		for (i = 0; opts[i].opt != -1; i++) {
			switch(opts[i].opt) {
				case 'u': { /* adapter URL without trailing slash */
					size_t len = strlen(adapter_url = strdup(opts[i].val));
					if ((len > 0) && (adapter_url[len-1] == '/')) adapter_url[len-1] = '\0';
					break;
				}
				case 'r': { /* region id */
					region_id = strdup(opts[i].val);
					break;
				}
				default: {
					logging("warning", "%s - Unrecognized option %c", module_name, opts[i].opt);
				}
			}
		}
	}
	if (!adapter_url || !region_id) {
		logging("error", "%s - Missing required broker module options", module_name);
		result = -1;
	} else if (gethostname(name, HOST_NAME_MAX)) {
		logging("error", "%s - Cannot get localhost name", module_name);
		result = -1;
	} else if (resolve_address(name, addr, INET_ADDRSTRLEN)) {
		logging("error", "%s - Cannot get localhost address", module_name);
		result = -1;
	} else {
		host_addr = strdup(addr);
		logging("info", "%s - Adapter URL = %s", module_name, adapter_url);
		logging("info", "%s - Region Id = %s", module_name, region_id);
		logging("info", "%s - Host addr = %s", module_name, host_addr);
	}

	free(opts);
	return result;
}


/* deinitializes module variables */
int free_module_variables(void)
{
	free(adapter_url);
	free(region_id);
	free(host_addr);
	return 0;
}


/* writes a formatted string to Nagios logs */
void logging(const char* level, const char* format, ...)
{
	char	buffer[MAXBUFLEN];
	va_list	ap;

	va_start(ap, format);
	vsnprintf(buffer, sizeof(buffer)-1, format, ap);
	buffer[sizeof(buffer)-1] = '\0';
	va_end(ap);

	write_to_all_logs(buffer, NSLOG_INFO_MESSAGE);
}


/* resolves the IP address of a hostname */
int resolve_address(const char* hostname, char* addr, size_t addrmaxlen)
{
	int result = EXIT_SUCCESS;

	struct hostent* hostent = gethostbyname(hostname);
	if (!hostent || (inet_ntop(AF_INET, hostent->h_addr_list[0], addr, addrmaxlen) == NULL)) {
		result = EXIT_FAILURE;
	}

	return result;
}


/* gets the name and arguments of the executed plugin */
char* find_plugin_name(nebstruct_service_check_data* data, char** args)
{
	host*    check_host	= NULL;
	service* check_service	= NULL;
	command* check_command	= NULL;
	char*    check_plugin	= NULL;

	if (((check_host = find_host(data->host_name)) != NULL)
	    && ((check_service = find_service(data->host_name, data->service_description)) != NULL)) {
		char* service_check_command = strdup(check_service->service_check_command);
		char* command_name;
		char* command_args;
		command_name = strtok_r(service_check_command, "!", &command_args);
		if ((check_command = find_command(command_name)) != NULL) {
			nagios_macros	mac;
			char*		raw = NULL;
			char*		cmd = NULL;
			memset(&mac, 0, sizeof(mac));
			grab_host_macros_r(&mac, check_host);
			grab_service_macros_r(&mac, check_service);
			get_raw_command_line_r(&mac, check_command, check_service->service_check_command, &raw, 0);
			if (raw != NULL) {
				char* ptr;
				char* last;
				process_macros_r(&mac, raw, &cmd, 0);
				strtok_r(cmd, " ", &last);
				ptr = strrchr(cmd, '/');
				check_plugin = strdup((ptr) ? ++ptr : cmd);
				if (args) *args = strdup(last);
			}
			my_free(raw);
			my_free(cmd);
		}
		free(service_check_command);
	}

	return check_plugin;
}


/* Nagios service check callback */
int callback_service_check(int callback_type, void* data)
{
	int result = 0;

	nebstruct_service_check_data*	check_data	= NULL;
	char*				request_url	= NULL;
	struct curl_slist*		curl_headers	= NULL;
	CURL*				curl_handle	= NULL;
	CURLcode			curl_result;

	assert(callback_type == NEBCALLBACK_SERVICE_CHECK_DATA);
	check_data = (nebstruct_service_check_data*) data;

	/* Process output only AFTER plugin is executed */
	if (check_data->type != NEBTYPE_SERVICECHECK_PROCESSED) {
		return result;
	}

	/* Async POST request to NGSI Adapter */
	if ((request_url = get_adapter_request(check_data)) == ADAPTER_REQUEST_INVALID) {
		logging("error", "%s - Cannot set adapter request URL", module_name);
	} else if (!strcmp(request_url, ADAPTER_REQUEST_IGNORE)) {
		/* nothing to do: plugin is ignored */
	} else if ((curl_handle = curl_easy_init()) == NULL) {
		logging("error", "%s - Cannot open HTTP session", module_name);
	} else {
		char request_txt[MAXBUFLEN];
		snprintf(request_txt, sizeof(request_txt)-1, "%s|%s",
		         check_data->output, check_data->perf_data);
		request_txt[sizeof(request_txt)-1] = '\0';

		curl_headers = curl_slist_append(curl_headers, "Content-Type: text/plain");
		curl_easy_setopt(curl_handle, CURLOPT_URL, request_url);
		curl_easy_setopt(curl_handle, CURLOPT_POST, 1);
		curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, request_txt);
		curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, strlen(request_txt));
		curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, curl_headers);
		if ((curl_result = curl_easy_perform(curl_handle)) == CURLE_OK) {
			logging("info", "%s - Request sent to %s", module_name,
			        request_url);
		} else {
			logging("error", "%s - Request to %s failed: %s", module_name,
			        request_url, curl_easy_strerror(curl_result));
		}
		curl_slist_free_all(curl_headers);
		curl_easy_cleanup(curl_handle);
	}

	free(request_url);
	return result;
}
