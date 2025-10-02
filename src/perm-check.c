/*
 * Copyright (C) 2015-2025 IoT.bzh Company
 * Author: José Bollo <jose.bollo@iot.bzh>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <argp.h>

#include <json-c/json.h>

#include <libafb/afb-extension.h>
#include <libafb/afb-core.h>
#include <libafb/afb-misc.h>
#include <libafb/afb-sys.h>

/*************************************************************/
/*************************************************************/
/** AFB-EXTENSION interface - DECLARATION                   **/
/*************************************************************/
/*************************************************************/

AFB_EXTENSION("PERM-CHECK")

/*************************************************************/
/*************************************************************/
/** AFB-EXTENSION interface - CONFIGURATION                 **/
/*************************************************************/
/*************************************************************/

/* default values for the api */
#ifndef DEFAULT_API_NAME
#define DEFAULT_API_NAME  "perm"
#endif

/* default values for the verb */
#ifndef DEFAULT_VERB_NAME
#define DEFAULT_VERB_NAME "check"
#endif

static const char *api_name = DEFAULT_API_NAME;
static const char *verb_name = DEFAULT_VERB_NAME;
static const char *scope_name = NULL;
static int disabled = 0;

/* description of configuration options */
const struct argp_option AfbExtensionOptionsV1[] = {
	{ .name="perm-check-api",   .key='a', .arg="NAME",
		.doc="Set the API name (default "DEFAULT_API_NAME")" },

	{ .name="perm-check-verb",  .key='v', .arg="NAME",
		.doc="Set the VERB name (default "DEFAULT_VERB_NAME")" },

	{ .name="perm-check-scope", .key='s', .arg="NAME",
		.doc="Set scope of the declared API" },

	{ .name="perm-check-disabled", .key='d', .arg=NULL,
		.doc="Disable the extension" },

	{ .name=0, .key=0, .doc=0 }
};

/* returns true if key exists and is a boolean true */
static int is_set(struct json_object *config, const char *key)
{
	struct json_object *value;
	return json_object_object_get_ex(config, key, &value)
		&& json_object_get_boolean(value);
}

/* set the target with the string value if key exists
 * returns 1 if exist or 0 if not existing  */
static int set_if_exist(struct json_object *config, const char *key, const char **target, int addref)
{
	struct json_object *value;
	int gotit = !!json_object_object_get_ex(config, key, &value);
	if (gotit)
		*target = json_object_get_string(addref ? json_object_get(value) : value);
	return gotit;
}

/* handle setting of configuration */
int AfbExtensionConfigV1(void **data, struct json_object *config, const char *uid)
{
	LIBAFB_NOTICE("Extension %s got config %s", AfbExtensionManifest.name, json_object_get_string(config));

	set_if_exist(config, "perm-check-api", &api_name, 1);
	set_if_exist(config, "perm-check-verb", &verb_name, 1);
	set_if_exist(config, "perm-check-scope", &scope_name, 1);
	disabled = is_set(config, "perm-check-disabled");
	return 0;
}

/*************************************************************/
/*************************************************************/
/** AFB-EXTENSION interface - DECLARATION OF API            **/
/*************************************************************/
/*************************************************************/

static void process_permission_check_request(void *closure, struct afb_req_common *req);

static struct afb_api_itf api_itf = {
	.process = process_permission_check_request
};

/* declare the permission check API */
int AfbExtensionDeclareV1(void *data, struct afb_apiset *declare_set, struct afb_apiset *call_set)
{
	int rc;
	struct afb_api_item aai;
	struct afb_apiset *ds;

	if (disabled) {
		LIBAFB_NOTICE("Extension %s is disabled", AfbExtensionManifest.name);
		return 0;
	}

	LIBAFB_NOTICE("Registering extension %s for API %s VERB %s SCOPE %s",
			AfbExtensionManifest.name, api_name, verb_name,
			scope_name ? scope_name : "(default)");

	/* check if possible */
	rc = afb_perm_check_perm_check_api(api_name);
	if (rc < 0)
		LIBAFB_ERROR("Can't use API %s in extension %s",
						api_name, AfbExtensionManifest.name);
	else {
		/* get the declare set, changing scope if required */
		ds = declare_set;
		if (scope_name != NULL) {
			ds = afb_apiset_subset_find(ds, scope_name);
			if (ds == NULL) {
				LIBAFB_ERROR("Invalid scope %s in extension %s",
						scope_name, AfbExtensionManifest.name);
				return X_EINVAL;
			}
		}

		/* record api */
		aai.closure = NULL;
		aai.group = NULL;
		aai.itf = &api_itf;
		rc = afb_apiset_add(ds, api_name, aai);
		if (rc >= 0)
			ds = afb_apiset_addref(ds);
		else
			LIBAFB_ERROR("Extension %s failed to register",
						AfbExtensionManifest.name);
	}

	return rc;
}

/*************************************************************/
/*************************************************************/
/** BUSINESS LOGIC                                          **/
/*************************************************************/
/*************************************************************/

/* structure for recording extracted parameter values */
typedef struct {
	const char *client;
	const char *user;
	const char *session;
	const char *permission;
}
	values_t;

enum {
	idx_client,
	idx_user,
	idx_session,
	idx_permission
};

/* extract parameter values
 * returns 0 on success or -1 in case of error */
static int extract_values(values_t *values, struct afb_req_common *req)
{
	struct json_object *jval;

	/* positional STRINGZ parameters: CLIENT USER SESSION PERMISSION */
	if (req->params.ndata == 4
	 && afb_data_type(req->params.data[0]) == &afb_type_predefined_stringz 
	 && afb_data_type(req->params.data[1]) == &afb_type_predefined_stringz 
	 && afb_data_type(req->params.data[2]) == &afb_type_predefined_stringz 
	 && afb_data_type(req->params.data[3]) == &afb_type_predefined_stringz) {
		values->client = afb_data_ro_pointer(req->params.data[idx_client]);
		values->user = afb_data_ro_pointer(req->params.data[idx_user]);
		values->session = afb_data_ro_pointer(req->params.data[idx_session]);
		values->permission = afb_data_ro_pointer(req->params.data[idx_permission]);
		return 0;
	}

	/* JSON structured query? */
	if (req->params.ndata != 1
	 || afb_req_common_param_convert(req, 0, &afb_type_predefined_json_c, NULL) < 0)
		return -1;

	/* yes, try extraction */
	jval = afb_data_ro_pointer(req->params.data[0]);
	return (set_if_exist(jval, "client", &values->client, 0)
	     && set_if_exist(jval, "user", &values->user, 0)
	     && set_if_exist(jval, "session", &values->session, 0)
	     && set_if_exist(jval, "permission", &values->permission, 0)) - 1;
}

/* callback of the asynchronous permission check */
static void check_async_callback(void *closure, int status)
{
	struct afb_req_common *req = closure;

	LIBAFB_DEBUG("perm-check result %d", status);

	/* normalize status */
	if (status == AFB_ERRNO_UNKNOWN_API)
		status = AFB_ERRNO_NOT_AVAILABLE;

	/* send reply */
	afb_req_common_reply_hookable(req, status, 0, 0);
	afb_req_common_unref(req);
}

/* process request of permission check */
static void process_permission_check_request(void *closure, struct afb_req_common *req)
{
	values_t values;

	/* check that the verb is the expected verb name */
	if (strcmp(req->verbname, verb_name) != 0)
		afb_req_common_reply_verb_unknown_error_hookable(req);

	/* check parameters */
	else if (extract_values(&values, req) < 0)
		afb_req_common_reply_hookable(req, AFB_ERRNO_INVALID_REQUEST, 0, 0);

	/* ok then cwasynchronous check of the permission */
	else {
		LIBAFB_DEBUG("perm-check for c=%s u=%s s=%s p=%s",
			values.client,
			values.user,
			values.session,
			values.permission);
		afb_perm_check_async(
			values.client,
			values.user,
			values.session,
			values.permission,
			check_async_callback,
			afb_req_common_addref(req));
	}
}

