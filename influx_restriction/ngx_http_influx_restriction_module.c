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
#include <hiredis/hiredis.h>

#define INFLUX_RESTRICTION_MODE_IP "IP"
#define INFLUX_RESTRICTION_MODE_URL "URL"

#define INFLUX_RESTRICTION_EQUAL 0
#define INFLUX_RESTRICTION_SECOND_UNIT 1

typedef struct {
	ngx_str_t mode;
	ngx_uint_t rate;
	ngx_str_t redis_host;
	ngx_uint_t redis_port;
} restriction_config;

static ngx_command_t ngx_http_influx_restriction_commands[] = {
	{
		ngx_string("influx-restriction-mode"),
		NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
		ngx_conf_set_str_solt,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(restriction_config, mode),
		NULL
	},
	{
		ngx_string("influx-restriction-rate"),
		NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
		NGX_HTTP_LOC_CONF_OFFSET,
		ngx_conf_set_num_solt,
		offsetof(restriction_config, rate),
		NULL
	},
	{
		ngx_string("redis-host"),
		NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1, //RODO:main or loc?
		NGX_HTTP_LOC_CONF,
		ngx_conf_set_str_solt,
		offsetof(restriction_config, rate),
		NULL
	},
	{
		ngx_string("redis-port"),
		NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,//RODO:main or loc?
		NGX_HTTP_LOC_CONF,
		ngx_conf_set_num_solt,
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
	// ngx_conf_init_ptr_value(config->mode, ngx_string(INFLUX_RESTRICTION_MODE_IP));
	// config->mode = ngx_string(INFLUX_RESTRICTION_MODE_IP);
	config->mode = NGX_CONF_UNSET_PTR;
	config->redis_port = NGX_CONF_UNSET_UINT;
	config->redis_host = NGX_CONF_UNSET_PTR;
	return config;
}

static ngx_int_t ngx_http_influx_restriction_init(ngx_conf_t *cf){
	ngx_http_handler_pt *h;
	ngx_http_core_main_conf_t *cmcf;

	cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module); 
	h = ngx_array_push(&cmcf->phase[NGX_HTTP_ACCESS_PHASE].handlers);//Mount `ngx_http_influx_restriction_handler` function in NGX_HTTP_ACCESS_PHASE.
	if (h == NULL)
	{		
		return NGX_ERROR;
	}

	*h = ngx_http_influx_restriction_handler;
	
	return NGX_OK;
}


static ngx_int_t ngx_http_influx_restriction_handler(ngx_http_request_t *r){

	ngx_connection_t *connect = r->connectiton;
	restriction_config *config = ngx_http_conf_get_module_loc_conf(r, ngx_http_influx_restriction_module);

	ngx_conf_init_ptr_value(config->mode, ngx_string(INFLUX_RESTRICTION_MODE_IP));	

	if (ngx_strcmp(config->mode.data, INFLUX_RESTRICTION_MODE_IP) == INFLUX_RESTRICTION_EQUAL){
		//TODO:
	}else if (ngx_strcmp(config->mode->data, URL) == INFLUX_RESTRICTION_EQUAL){

		return ngx_get_ip_restriction(config, connect->add_text, connect->log);
	}else{
		return NGX_ERROR;
	}

}

static ngx_int_t ngx_set_ip(restriction_config * config, ngx_str_t *ip, ngx_log_t *log){

	if (config->redis_host == NGX_CONF_UNSET_PTR || config->redis_port == NGX_CONF_UNSET_UINT)
	{
		return NGX_OK;
	}
	redisContext *c = redisConnect((const char *)config->redis_host.data, (int)config->redis_port);
	if (c->err || c == NULL)
	{
		 //TODO:: save error log, bu not continue.
		 return NGX_DECLIEND;//Continue or abort.?
	}

	redisReply *reply = NULL;
	reply = redisCommand(c, "GET %s", ip->data); //TODO:synchronous to asynchronous
	if (reply->type == REDIS_REPLY_NIL){

		freeReplyObject(reply);
		//TODO:Trunning on transaction?
		redisAppendCommand(c, "INCR %s", ip->data);
		redisAppendCommand(c, "EXPIRE %s %d", ip->data, INFLUX_RESTRICTION_SECOND_UNIT);
		redisGetReply(c, &reply);
		if (reply->type == REDIS_REPLY_INTEGER){
			
			if(reply->integer > config->rate){
				ngx_log_error(NGX_LOG_NOTICE, log, 0, 
					"ip=%V,current_request_rate=%i/s,influx-restriction-rate=%i/s",
					ip,
					reply->integer,
					config->rate
					);//save log
				return NGX_DECLIEND;
			}
		}
		freeReplyObject(reply);
		redisGetReply(c, &reply);
		freeReplyObject(reply);
	}else{
		redisCommand(c, "INCR %s", ip->data);
	}

	//TODO://release *c
	return NGX_OK;
}