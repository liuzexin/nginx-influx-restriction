/*
** Copy Right CoderLiu(github)
** First of all,we should make sure the aim of Influx Restriction.
   Utility list:
   1.According to the "location" config to set strategy which how to restrict influx.
   Rate req/s -> Maximum Request Rate.If exceed the maximum-request-rate 1000/s will return the 403 http status code.
   Dimension:
   		(1)IP:The unique sign of client in general circumstance.(default value)
		(2)URL:URL is the sign of the request whitch combine the request parameter.
		//TODO::
   Configuration:

   location {
		influx-restriction-mode IP;
		influx-restriction-rate 1000;
   }			
*/

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#define INFLUX_RESTRICTION_MODE_IP "IP"
#define INFLUX_RESTRICTION_MODE_URL "URL"

typedef struct {
	ngx_str_t mode;
	ngx_uint_t rate;
} restriction_config;

static ngx_command_t ngx_http_influx_restriction_commands[] = {
	{
		ngx_string("influx-restriction-mode"),
		NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(restriction_config, mode),
		NULL
	},
	{
		ngx_string("influx-restriction-rate"),
		NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(restriction_config, rate),
		NULL
	},
	ngx_null_omcmand
}

static ngx_http_module_t ngx_http_influx_restriction_module_ctx = { //Create loc configuration

	NULL,
	ngx_http_influx_restriction_init,//After parse configuration then call this function to mount the handler.

	NULL,
	NULL,

	ngx_http_influx_restriction_create_loc,
	NULL
}

ngx_module_t ngx_http_influx_restriction_module = {
	NGX_MODULE_V1,
	&ngx_http_influx_restriction_module_ctx,
	ngx_http_influx_restriction_commands,
	NGX_HTTP_MODULE,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
	NGX_MODULE_V1_PADDING
}

static void * ngx_http_influx_restriction_create_loc(ngx_conf_t *cf){
	ngx_pool_t *pool = cf->pool;
	restriction_config *config = ngx_palloc(pool, sizeof(restriction_config));
	if (config == NULL)
	{
		return NGX_ERROR;
	}

	config->rate = NGX_CONF_UNSET_UINT;
	//TODO::!!
	//ngx_conf_init config->mode
}

static ngx_int_t ngx_http_influx_restriction_init(ngx_conf_t *cf){
	ngx_http_handler_pt *h;
	ngx_http_core_main_conf_t *cmcf;

	cmcf = ngx_http_conf_get_module_main_conf_t(cf, ngx_http_core_module); 
	h = ngx_array_push(&cmcf->phase[NGX_HTTP_ACCESS_PHASE].handlers);//Mount `ngx_http_influx_restriction_handler` function in NGX_HTTP_ACCESS_PHASE.
	if (h == NULL)
	{		
		return NGX_ERROR;
	}

	*h = ngx_http_influx_restriction_handler;
	return NGX_OK;
}

