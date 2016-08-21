/* =====================================================================================
 *
 *   @file box-status.c
 *
 *
 *   Version:  1.0
 *   Created:  03/29/2014 07:49:49 PM
 *
 *   @section DESCRIPTION
 *
 *       Status updates
 *
 *   @section LICENSE
 *
 *
 *
 * =====================================================================================
 */

#include <uci.h>

#include "main.h"
#include "config.h"
#include "sops.h"


char* s_get_timestamp() {
  struct timeval tv;
  struct tm *tm;
  char formatted_date0[MAX_STRING_LEN];
  char formatted_date1[MAX_STRING_LEN];
  char my_timezone[7];

  gettimeofday(&tv, NULL);
  tm = localtime(&tv.tv_sec);
  assert(tm);

  strftime(formatted_date0, MAX_STRING_LEN, "%Y-%m-%dT%H:%M:%S.000%%s", tm);
  if (strftime(my_timezone, 7, "%z", tm) != 0) {
    my_timezone[6] = '\0';
    my_timezone[5] = my_timezone[4];
    my_timezone[4] = my_timezone[3];
    my_timezone[3] = ':';
    snprintf(formatted_date1,MAX_STRING_LEN, formatted_date0, my_timezone);
  } else {
    snprintf(formatted_date1,MAX_STRING_LEN, formatted_date0, "");
  }

  return strdup(formatted_date1);
}

int main(int argc, char *argv[])
{
  int res = STATUS_ERROR;
  sops_handle_t* sops = NULL;
  const char* status_msg = "{\"ts\": \"%s\", \"level\": 2, \"status\": 1}";
  char buffer[MAX_STRING_LEN];
  char* server = NULL;
  char* token = NULL;
  char* version = NULL;
  char* status_endpoint = NULL;

  /*  Read config */
  config_context *cfg_ctx = config_new();
  token =  config_get_str(cfg_ctx, "hermes.http.token");

  if ((token == NULL) && (cfg_ctx->err != UCI_OK)) {
    errorLog("token is currently not provisioned");
    config_destroy(cfg_ctx);
    exit(1);
  }

  server = config_get_str(cfg_ctx, "hermes.http.server");
  version =  config_get_str(cfg_ctx, "hermes.http.version");
  status_endpoint = config_get_str(cfg_ctx, "hermes.http.status_endpoint");
  int timeout = config_get_int(cfg_ctx, "hermes.http.timeout");
  config_destroy(cfg_ctx);

  if (server == NULL || token == NULL || version == NULL || status_endpoint == NULL) {
    return 1;
  }

  sops = sops_new_token(server, token, version, timeout);

  if (sops != NULL) {
    char *ts = s_get_timestamp();
    snprintf(buffer, MAX_STRING_LEN, status_msg, ts);
    res = sops_post_datastream(sops, status_endpoint, buffer, strlen(buffer));
    sops_destroy(sops);
    mem_free(ts);
  } 

  if(res == STATUS_OK) {
    debugLog("Status send succeeded.");
    res = 0;
  } else {
    debugLog("Status send failed.");
    res = 1;
  }

  return res;
}
