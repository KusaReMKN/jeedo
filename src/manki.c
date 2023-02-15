/*-
 * Copyright (c) 2023, KusaReMKN.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <math.h>      /* for NaN */
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>
#include <jansson.h>

#include "manki.h"

struct membuf {
	char *buffer;   /* pointer to buffer; terminated by NUL ('\0') */
	size_t length;  /* length of buffer */
};

struct response {
	bool succeeded;
	char *reason;
	char *response;
	int sequence;
	struct latlng destination;
};

static const char *const cmdTab[] = {
	"hello",
	"ping",
	"next",
	"halt",
	NULL,
};

static size_t
recvResponse(char *data, size_t size, size_t nmemb, void *userp)
{
	size_t realsize;
	struct membuf *mem;
	char *ptr;

	realsize = size * nmemb;
	mem = userp;
	ptr = realloc(mem->buffer, mem->length + realsize + 1);
	if (ptr == NULL)
		return 0;
	mem->buffer = ptr;
	memcpy(mem->buffer + mem->length, data, realsize);
	mem->length += realsize;
	mem->buffer[mem->length] = '\0';

	return realsize;
}

static CURLcode
postJson(const char *restrict url, const char *restrict json,
		const char *restrict *resptr)
{
	struct curl_slist *slist, *tmp;
	CURL *hnd;
	struct membuf response;
	CURLcode ret;

	slist = NULL;
	tmp = curl_slist_append(slist,
			"Accept: application/json; charset=utf-8");
	if (tmp == NULL)
		return (CURLcode)-2;
	slist = tmp;
	tmp = curl_slist_append(slist,
			"Content-Type: application/json; charset=utf-8");
	if (tmp == NULL) {
		curl_slist_free_all(slist);
		return (CURLcode)-2;
	}
	slist = tmp;

	hnd = curl_easy_init();
	if (hnd == NULL) {
		curl_slist_free_all(slist);
		return (CURLcode)-1;
	}
	(void)curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, recvResponse);
	response.buffer = NULL;
	response.length = 0;
	(void)curl_easy_setopt(hnd, CURLOPT_WRITEDATA,     &response   );
	(void)curl_easy_setopt(hnd, CURLOPT_FAILONERROR,   1L          );
	(void)curl_easy_setopt(hnd, CURLOPT_URL,           (char *)url );
	(void)curl_easy_setopt(hnd, CURLOPT_POST,          1L          );
	(void)curl_easy_setopt(hnd, CURLOPT_POSTFIELDS,    (char *)json);
	(void)curl_easy_setopt(hnd, CURLOPT_HTTPHEADER,    slist       );

	ret = curl_easy_perform(hnd);
	if (ret == CURLE_OK) {
		if (resptr != NULL)
			*resptr = response.buffer;
	} else {
		free(response.buffer);
	}

	return ret;
}

static json_t *
json_latlng(const struct latlng *pos)
{
	json_t *root, *tmp;
	int result;

	root = json_object();
	if (root == NULL)
		return NULL;

	tmp = json_real(pos->lat);
	if (tmp == NULL) {
fatal:         json_decref(root);
	       return NULL;
	}
	result = json_object_set(root, "lat", tmp);
	json_decref(tmp);
	if (result != 0)
		goto fatal;

	tmp = json_real(pos->lng);
	if (tmp == NULL)
		goto fatal;
	result = json_object_set(root, "lng", tmp);
	json_decref(tmp);
	if (result != 0)
		goto fatal;

	return root;
}

static char *
generateJson(const struct carContext *restrict ctx, int cmd,
		const struct latlng *restrict pos, int btr)
{
	json_t *json, *tmp;
	int result;
	char *str;

	if (cmd < 0 || MR_MAX <= cmd)
		return NULL;

	json = json_object();
	if (json == NULL)
		return NULL;

	tmp = json_string(cmdTab[cmd]);
	if (tmp == NULL) {
fatal:         json_decref(json);
	       return NULL;
	}
	result = json_object_set(json, "request", tmp);
	json_decref(tmp);
	if (result != 0)
		goto fatal;

	tmp = json_latlng(pos);
	if (tmp == NULL)
		goto fatal;
	result = json_object_set(json, "location", tmp);
	json_decref(tmp);
	if (result != 0)
		goto fatal;

	tmp = json_integer((json_int_t)btr);
	if (tmp == NULL)
		goto fatal;
	result = json_object_set(json, "battery", tmp);
	json_decref(tmp);
	if (result != 0)
		goto fatal;

	if (cmd != MR_HELLO) {
		tmp = json_string(ctx->carId);
		if (tmp == NULL)
			goto fatal;
		result = json_object_set(json, "carId", tmp);
		json_decref(tmp);
		if (result != 0)
			goto fatal;

		tmp = json_integer((json_int_t)ctx->sequence);
		if (tmp == NULL)
			goto fatal;
		result = json_object_set(json, "sequence", tmp);
		json_decref(tmp);
		if (result != 0)
			goto fatal;
	}

	str = json_dumps(json, JSON_INDENT(4));
	json_decref(json);
	return str;
}

static int
parseResponse(struct response *restrict res, const char *restrict resJson)
{
	json_t *json, *tmp;
	const char *str;

	res->succeeded = false;
	res->reason = res->response = NULL;
	res->sequence = 0;
	res->destination.lat = res->destination.lng = NAN;

	json = json_loads(resJson, 0, NULL);
	if (json == NULL)
		return -1;

	tmp = json_object_get(json, "succeeded");
	if (tmp == NULL) {
fatal:         json_decref(json);
	       return -1;
	}
	if (!json_is_boolean(tmp)) {
fatal2:                json_decref(tmp);
		       goto fatal;
	}
	res->succeeded = json_boolean_value(tmp);
	json_decref(tmp);

	if (!res->succeeded) {
		tmp = json_object_get(json, "reason");
		if (tmp == NULL)
			goto fatal;
		if (!json_is_string(tmp))
			goto fatal2;
		str = json_string_value(tmp);
		res->reason = malloc(strlen(str) + 1);
		if (res->reason == NULL)
			goto fatal2;
		strcpy(res->reason, str);
		json_decref(tmp);
		json_decref(json);
		return 0;
	}

	tmp = json_object_get(json, "response");
	if (tmp == NULL)
		goto fatal;
	if (!json_is_string(tmp))
		goto fatal2;
	str = json_string_value(tmp);
	res->response = malloc(strlen(str) + 1);
	if (res->response == NULL)
		goto fatal2;
	strcpy(res->response, str);
	json_decref(tmp);

	tmp = json_object_get(json, "sequence");
	if (tmp == NULL)
		goto fatal;
	if (!json_is_integer(tmp))
		goto fatal2;
	res->sequence = (int)json_integer_value(tmp);
	json_decref(tmp);

	tmp = json_object_get(json, "destination");
	if (tmp != NULL) {
		json_t *tmp2;

		tmp2 = json_object_get(tmp, "lat");
		if (tmp2 == NULL)
			goto nodest;
		if (!json_is_number(tmp2)) {
			json_decref(tmp2);
			goto nodest;
		}
		res->destination.lat = json_number_value(tmp2);
		json_decref(tmp2);

		tmp2 = json_object_get(tmp, "lng");
		if (tmp2 == NULL)
			goto nodest;
		if (!json_is_number(tmp2)) {
			json_decref(tmp2);
			goto nodest;
		}
		res->destination.lng = json_number_value(tmp2);
		json_decref(tmp2);

nodest:                json_decref(tmp);
	}

	json_decref(json);

	return 0;
}

CURLcode
manki(struct carContext *restrict ctx, int cmd,
		const struct latlng *restrict pos, int btr)
{
#define POSTURL        "http://sazasub.kohga.local/sendCarInfo"
	const char *reqJson, *resJson;
	CURLcode result;
	struct response res;

	reqJson = generateJson(ctx, cmd, pos, btr);
	if (reqJson == NULL)
		return (CURLcode)-1;

	resJson = NULL;
	result = postJson(POSTURL, reqJson, &resJson);
	if (result != CURLE_OK) {
		free((void *)reqJson);
		free((void *)resJson);
		return result;
	}

	if (parseResponse(&res, resJson) != 0) {
		free((void *)resJson);
		return (CURLcode)-2;
	}

	if (!res.succeeded) {
		ctx->response = res.reason;
		return (CURLcode)-3;
	}

	ctx->sequence    = res.sequence;
	ctx->response    = res.response;
	ctx->destination = res.destination;
	if (cmd == MR_HELLO)
		ctx->carId = ctx->response;

	return (CURLcode)0;
}
