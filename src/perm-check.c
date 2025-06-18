/*
 * Copyright (C) 2015-2025 IoT.bzh Company
 * Author: José Bollo <jose.bollo@iot.bzh>
 *
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

#ifndef DEFAULT_API_NAME
#define DEFAULT_API_NAME  "perm"
#endif

#ifndef DEFAULT_VERB_NAME
#define DEFAULT_VERB_NAME "check"
#endif

static void process(void *closure, struct afb_req_common *req);

static const char *api = DEFAULT_API_NAME;
static const char *verb = DEFAULT_VERB_NAME;
static const char *scope = NULL;
static struct afb_api_itf itf = {
	.process = process
};

/*************************************************************/
/*************************************************************/
/** AFB-EXTENSION interface                                 **/
/*************************************************************/
/*************************************************************/

AFB_EXTENSION("PERM-CHECK")

const struct argp_option AfbExtensionOptionsV1[] = {
	{ .name="perm-check-api",   .key='a', .arg="NAME",
		.doc="Set the API name (default "DEFAULT_API_NAME")" },

	{ .name="perm-check-verb",  .key='v', .arg="NAME",
		.doc="Set the VERB name (default "DEFAULT_VERB_NAME")" },

	{ .name="perm-check-scope", .key='s', .arg="NAME",
		.doc="Set scope of the declared API" },

	{ .name=0, .key=0, .doc=0 }
};

static void set(struct json_object *config, const char *key, const char **target)
{
	struct json_object *value;
	if (json_object_object_get_ex(config, key, &value))
		*target = json_object_get_string(json_object_get(value));
}

int AfbExtensionConfigV1(void **data, struct json_object *config, const char *uid)
{
	LIBAFB_NOTICE("Extension %s got config %s", AfbExtensionManifest.name, json_object_get_string(config));
	set(config, "perm-check-api", &api);
	set(config, "perm-check-verb", &verb);
	set(config, "perm-check-scope", &scope);
	return 0;
}

int AfbExtensionDeclareV1(void *data, struct afb_apiset *declare_set, struct afb_apiset *call_set)
{
	int rc;
	struct afb_api_item aai = { .closure = NULL, .group = NULL, .itf = &itf };
	struct afb_apiset *ds = declare_set;

	LIBAFB_NOTICE("Extension %s doing registration", AfbExtensionManifest.name);

	/* check if possible */
	rc = afb_perm_check_perm_check_api(api);
	if (rc < 0)
		LIBAFB_ERROR("Can't use API %s in extension %s",
						api, AfbExtensionManifest.name);
	else {
		/* change scope if required */
		if (scope != NULL)
			ds = afb_apiset_subset_find(ds, scope) ?: ds;

		/* record */
		ds = afb_apiset_addref(ds);
		rc = afb_apiset_add(ds, api, aai);
		if (rc < 0)
			LIBAFB_ERROR("Extension %s failed to register",
						AfbExtensionManifest.name);
	}

	return rc;
}

static void callback(void *closure, int status)
{
	struct afb_req_common *req = closure;
	if (status == AFB_ERRNO_UNKNOWN_API)
		status = AFB_ERRNO_NOT_AVAILABLE;
	afb_req_common_reply_hookable(req, status, 0, 0);
	afb_req_common_unref(req);
}

static int get(struct json_object *config, const char *key, const char **target)
{
	struct json_object *value;
	if (!json_object_object_get_ex(config, key, &value))
		return -1;
	*target = json_object_get_string(value);
	return 0;
}

static int extract(const char *stringz[4], struct afb_req_common *req)
{
	struct json_object *jval;

	if (req->params.ndata == 4
	 && afb_data_type(req->params.data[0]) == &afb_type_predefined_stringz 
	 && afb_data_type(req->params.data[1]) == &afb_type_predefined_stringz 
	 && afb_data_type(req->params.data[2]) == &afb_type_predefined_stringz 
	 && afb_data_type(req->params.data[3]) == &afb_type_predefined_stringz) {
		stringz[0] = afb_data_ro_pointer(req->params.data[0]);
		stringz[1] = afb_data_ro_pointer(req->params.data[1]);
		stringz[2] = afb_data_ro_pointer(req->params.data[2]);
		stringz[3] = afb_data_ro_pointer(req->params.data[3]);
		return 0;
	}

	if (req->params.ndata != 1
	 || 0 > afb_req_common_param_convert(req, 0, &afb_type_predefined_json_c, NULL))
		return -1;

	jval = afb_data_ro_pointer(req->params.data[0]);
	return get(jval, "client", &stringz[0])
	    ?: get(jval, "user", &stringz[1])
	    ?: get(jval, "session", &stringz[2])
	    ?: get(jval, "permission", &stringz[3]);
}

static void process(void *closure, struct afb_req_common *req)
{
	const char *stringz[4];

	/* check verb */
	if (strcmp(req->verbname, verb) != 0)
		afb_req_common_reply_verb_unknown_error_hookable(req);

	/* check parameters */
	else if (extract(stringz, req) < 0)
		afb_req_common_reply_hookable(req, AFB_ERRNO_INVALID_REQUEST, 0, 0);

	/* ok then process */
	else
		afb_perm_check_async(
			stringz[0],
			stringz[1],
			stringz[2],
			stringz[3],
			callback,
			afb_req_common_addref(req));
}

