/*
* Copyright (C) 2009 Mamadou Diop.
*
* Contact: Mamadou Diop <diopmamadou(at)doubango.org>
*	
* This file is part of Open Source Doubango Framework.
*
* DOUBANGO is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*	
* DOUBANGO is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*	
* You should have received a copy of the GNU General Public License
* along with DOUBANGO.
*
*/

/**@file thttp_action.h
 * @brief HTTP action.
 *
 * @author Mamadou Diop <diopmamadou(at)doubango.org>
 *
 * @date Created: Sat Nov 8 16:54:58 2009 mdiop
 */
#include "tinyHTTP/thttp_action.h"

#include "thttp.h"

#include "tsk_debug.h"

/**@defgroup thttp_action_group Sending Requests
*/

/**@ingroup thttp_action_group
* Sends a custom HTTP/HTTPS request.
* @param session The @a session (or connection) to use.
* @param urlstring The Request-URI. If the url scheme is 'https', then the default port will be 443, otherwise the port value will be 80.
* @param method The method to use for the HTTP request (e.g. GET, PUT, DELETE, POST ...).
* @retval Zero if succeed and non-zero error code otherwise.
*
* @code
thttp_action_perform(session, "http://www.google.com", "GET"
		// request-level parameters
		THTTP_ACTION_SET_PARAM("timeout", "6000"),

		// request-level headers
		THTTP_ACTION_SET_HEADER("Pragma", "No-Cache"),
		THTTP_ACTION_SET_HEADER("Connection", "Keep-Alive"),
		
		// close parameters
		THTTP_ACTION_SET_NULL());
* @endcode
* @sa @ref thttp_action_CONNECT<br>@ref thttp_action_DELETE<br>@ref thttp_action_GET<br>@ref thttp_action_HEAD<br>@ref thttp_action_OPTIONS<br>
* @ref thttp_action_PATCH<br>@ref thttp_action_POST<br>@ref thttp_action_PUT<br>@ref thttp_action_TRACE
*/
int thttp_action_perform(thttp_session_handle_t *session, const char* urlstring, const char* method, ...)
{
	thttp_session_t* sess = session;
	va_list ap;
	thttp_action_t* action;
	thttp_dialog_t* dialog;
	int ret = -1;	

	if(!sess || !sess->stack || !urlstring || !method){
		return ret;
	}
	
	va_start(ap, method);
	if((action = THTTP_ACTION_CREATE(atype_o_request, urlstring, method, &ap))){		
		if((dialog = thttp_dialog_new(sess))){
			ret = thttp_dialog_fsm_act(dialog, action->type, tsk_null, action);
			
			tsk_object_unref(dialog);
		}
		else{
			TSK_DEBUG_ERROR("Failed to create new HTTP/HTTPS dialog.");
			ret = -2;
		}
		TSK_OBJECT_SAFE_FREE(action);
	}
	va_end(ap);

	return ret;
}




//=================================================================================================
//	HTTP action object definition
//
static tsk_object_t* thttp_action_create(tsk_object_t * self, va_list * app)
{
	thttp_action_t *action = self;
	if(action){
		va_list* app_2;
		thttp_action_param_type_t curr;

		action->type = va_arg(*app, thttp_action_type_t);
		action->url = tsk_strdup(va_arg(*app, const char*));
		action->method = tsk_strdup(va_arg(*app, const char*));
		app_2 = va_arg(*app, va_list*);	

		action->options = TSK_LIST_CREATE();
		action->headers = TSK_LIST_CREATE();

		if(!app_2){ /* XCAP stack will pass null va_list */
			goto bail;
		}

		while((curr = va_arg(*app_2, thttp_action_param_type_t)) != aptype_null){
			switch(curr){
				case aptype_option:
					{
						thhtp_action_option_t id = va_arg(*app_2, thhtp_action_option_t);
						const char* value = va_arg(*app_2, const char *);
						tsk_options_add_option(&action->options, id, value);
						break;
					}

				case aptype_header:
					{
						const char* name = va_arg(*app_2, const char *);
						const char* value = va_arg(*app_2, const char *);
						tsk_params_add_param(&action->headers, name, value);
						break;
					}

				case aptype_payload:
					{
						const void* payload = va_arg(*app_2, const void *);
						size_t size = va_arg(*app_2, size_t);
						if(payload && size){
							TSK_OBJECT_SAFE_FREE(action->payload);
							action->payload = TSK_BUFFER_CREATE(payload, size);
						}
						break;
					}

				default:
					{
						TSK_DEBUG_ERROR("NOT SUPPORTED.");
						goto bail;
					}
			} /* switch */
		} /* while */
	}
bail:
	return self;
}

static tsk_object_t* thttp_action_destroy(tsk_object_t * self)
{ 
	thttp_action_t *action = self;
	if(action){
		TSK_FREE(action->url);
		TSK_FREE(action->method);

		TSK_OBJECT_SAFE_FREE(action->options);
		TSK_OBJECT_SAFE_FREE(action->headers);
		TSK_OBJECT_SAFE_FREE(action->payload);
	}

	return self;
}

static const tsk_object_def_t thttp_action_def_s = 
{
	sizeof(thttp_action_t),
	thttp_action_create, 
	thttp_action_destroy,
	tsk_null, 
};
const tsk_object_def_t *thttp_action_def_t = &thttp_action_def_s;

