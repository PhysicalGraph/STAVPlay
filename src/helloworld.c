#include <stdio.h>
#include <string.h>

#include "ppapi/c/pp_errors.h"
#include "ppapi/c/ppp.h"
#include "ppapi/c/ppp_instance.h"
#include "ppapi/c/ppp_messaging.h"
#include "ppapi/c/pp_bool.h"
#include "ppapi/c/ppb_var.h"
#include "ppapi/c/ppb_messaging.h"
#include "ppapi/c/pp_var.h"

/* Global pointers to selected browser interfaces. */
const PPB_Messaging* g_messaging_interface;
const PPB_Var* g_var_interface;

static PP_Bool Instance_DidCreate(PP_Instance instance, uint32_t argc,
                                  const char* argn[], const char* argv[]) {return PP_TRUE;}

static void Instance_DidDestroy(PP_Instance instance) {}

static void Instance_DidChangeView(PP_Instance pp_instance, PP_Resource view) {}

static void Instance_DidChangeFocus(PP_Instance pp_instance, PP_Bool has_focus) {}

static PP_Bool Instance_HandleDocumentLoad(PP_Instance pp_instance,
        PP_Resource pp_url_loader) {return PP_FALSE;}

PP_EXPORT int32_t PPP_InitializeModule(PP_Module module_id,
                                       PPB_GetInterface get_browser_interface)
{
	// Initializing pointers to used browser interfaces.
	g_messaging_interface = (const PPB_Messaging*) get_browser_interface(
	                            PPB_MESSAGING_INTERFACE);
	g_var_interface = (const PPB_Var*) get_browser_interface(PPB_VAR_INTERFACE);
	return PP_OK;
}

PP_EXPORT void PPP_ShutdownModule() {}

void Messaging_HandleMessage(PP_Instance instance, struct PP_Var message)
{
	char str_buff[] = "Hello World!\n";

	// Create <code>PP_Var</code> containing the message body.
	struct PP_Var var_response = g_var_interface->VarFromUtf8(str_buff, strlen(str_buff));

	// Post message to the Javascript layer.
	g_messaging_interface->PostMessage(instance, var_response);
}

PP_EXPORT const void* PPP_GetInterface(const char* interface_name)
{

	if (strcmp(interface_name, PPP_INSTANCE_INTERFACE) == 0) {
		static PPP_Instance instance_interface = {
			&Instance_DidCreate,
			&Instance_DidDestroy,
			&Instance_DidChangeView,
			&Instance_DidChangeFocus,
			&Instance_HandleDocumentLoad
		};
		return &instance_interface;
	}
	if (strcmp(interface_name, PPP_MESSAGING_INTERFACE) == 0) {
		static PPP_Messaging messaging_interface = {
			&Messaging_HandleMessage
		};
		return &messaging_interface;
	}
	return NULL;
}

