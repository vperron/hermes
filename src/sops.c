/**
 * =====================================================================================
 *
 *   @file sops.c
 *
 *   
 *        Version:  1.0
 *        Created:  01/08/2013 05:26:52 PM
 *
 *
 *   @section DESCRIPTION
 *
 *       SOPS
 *       
 *   @section LICENSE
 *
 *       
 *
 * =====================================================================================
 */

#include <string.h>
#include <curl/curl.h>
#include <zlib.h>

#include "main.h"
#include "sops.h"
#include "http_status.h"

#define SOPS_DEFAULT_TIMEOUT 30 // secs

struct sops_handle_t_st {
  char* base_uri;
  char* user;
  char* secret;
  char* token;
  char* version;
  bool use_token;
  bool compress_requests;
  int timeout;
}; 

size_t s_http_write_callback_phony(void *ptr, size_t size, size_t nmemb, void *userdata)
{
//  printf("RECEIVED :\n%s\n", (char*) ptr);
  return size * nmemb;
}

int s_http_post_request(sops_handle_t* sops, const char* uri, const char* data, size_t len) 
{
  int ret = -1;
  CURL *curl_handle;
  CURLcode res;
  char header_buf[MAX_STRING_LEN];
  char *out_buf = NULL;

  // debugLog("HTTP POST URI : '%s', data = %s", uri, data);
  if (sops->use_token && sops->token == NULL)
    return -1;

  curl_handle = curl_easy_init();
  if (curl_handle) {

    struct curl_slist *headers=NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    if (sops->version) {
      bzero(header_buf, sizeof(header_buf));
      snprintf(header_buf, MAX_STRING_LEN, "Accept: %s", sops->version);
      curl_slist_append(headers, header_buf);
    }

    if (sops->token) {
      bzero(header_buf, sizeof(header_buf));
      snprintf(header_buf, MAX_STRING_LEN, "Authorization: Token %s",sops->token);
      curl_slist_append(headers, header_buf);
    } else if (sops->user && sops->secret) {
      bzero(header_buf, sizeof(header_buf));
      snprintf(header_buf,MAX_STRING_LEN,"%s:%s",sops->user, sops->secret);
      curl_easy_setopt(curl_handle, CURLOPT_USERPWD, header_buf); 
    }
    if (sops->compress_requests) {
      uLongf dest_len = compressBound(len);
      out_buf = malloc(dest_len);
      if (out_buf) {
        int ret = compress2((Bytef*) out_buf, &dest_len, (Bytef*)data, (uLongf) len, Z_BEST_COMPRESSION);
        if (ret == Z_OK) {
          curl_slist_append(headers, "Content-Encoding: gzip");
          bzero(header_buf, sizeof(header_buf));
          snprintf(header_buf, MAX_STRING_LEN, "X-Decoded-Length: %d", (int)len);
          curl_slist_append(headers, header_buf);
          curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, out_buf);
          curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, dest_len);
        } else {
          curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, data);
          curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, len);
        }
      }
    } else {
      curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, data);
      curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, len);
    }
    curl_easy_setopt(curl_handle, CURLOPT_URL, uri);
    curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, s_http_write_callback_phony);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "curl/7.28.1");
    curl_easy_setopt(curl_handle, CURLOPT_POST, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, sops->timeout);

    res = curl_easy_perform(curl_handle);
    switch(res) {
      case CURLE_OPERATION_TIMEDOUT:
        debugLog("libcurl timeout");
        ret = HTTP_STATUS_REQUEST_TIME_OUT;
        break;
      case CURLE_OK:
        {
          long http_code = 0;
          curl_easy_getinfo (curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
          ret = (int)http_code;
        } break;
      default:
        debugLog("Error: %s", strerror(res));
        break;
    }
  
    if (out_buf) mem_free(out_buf);
    curl_easy_cleanup(curl_handle);
    curl_slist_free_all(headers);
  }

  return ret;
}

size_t s_http_write_token(void *ptr, size_t size, size_t nmemb, void *userdata)
{
  char buffer[MAX_STRING_LEN];
  char **token = (char**)userdata;

  bzero(buffer, MAX_STRING_LEN);
  if(ptr != NULL) {
    sscanf(ptr,"{\"token\": \"%s\"}",buffer);
    int len = strlen(buffer);
    buffer[len-2] = '\0';
    if(token != NULL)
      *token = strdup(buffer);
  }

  return size * nmemb;
}

sops_handle_t* sops_new_user_password(const char* uri, const char* token_endp, 
    const char* user, const char* secret, int timeout)
{
  assert (uri);
  assert (token_endp);
  assert (user);
  assert (secret);

  char credentials[MAX_STRING_LEN];
  char get_token_uri[MAX_STRING_LEN];
  char *token = NULL;
  CURL *curl_handle;
  CURLcode res = STATUS_ERROR;
  // GET THE TOKEN
  curl_handle = curl_easy_init();
  if (curl_handle) {

    snprintf(credentials, MAX_STRING_LEN, 
        "{\"username\":\"%s\",\"password\":\"%s\"}", user, secret);

    snprintf(get_token_uri, MAX_STRING_LEN, "%s%s", uri, token_endp);

    struct curl_slist *headers=NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl_handle, CURLOPT_URL, get_token_uri);
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, s_http_write_token);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &token);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "curl/7.28.1");
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, credentials);
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, strlen(credentials));
    curl_easy_setopt(curl_handle, CURLOPT_POST, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, timeout);
    res = curl_easy_perform(curl_handle);
    switch(res) {
      case CURLE_OPERATION_TIMEDOUT:
        debugLog("libcurl timeout");
        res = STATUS_ERROR;
        break;
      case CURLE_OK:
        res = STATUS_OK;
        break;
      default:
        debugLog("Error: %s", strerror(res));
        res = STATUS_ERROR;
        break;
    }
    curl_easy_cleanup(curl_handle);
    curl_slist_free_all(headers);
  }


  if (res == STATUS_OK) {
    sops_handle_t* sops = malloc(sizeof(sops_handle_t));
    memset(sops,0,sizeof(sops_handle_t));

    sops->base_uri = strdup(uri);
    sops->use_token = true;
    sops->token = token;
    sops->compress_requests = false;

    if (timeout == -1)
      sops->timeout = SOPS_DEFAULT_TIMEOUT;
    else
      sops->timeout = timeout;
    return sops;
  }

  return NULL;
  
}

sops_handle_t* sops_new_token(const char* uri, const char* token, const char* version, int timeout) 
{
  assert (uri);

  sops_handle_t* sops = malloc(sizeof(sops_handle_t));
  memset(sops,0,sizeof(sops_handle_t));
  
  sops->base_uri = strdup(uri);
  if (token) {
    sops->token = strdup(token);
    sops->version = strdup(version);
    sops->use_token = true;
  }

  if (timeout == -1)
    sops->timeout = SOPS_DEFAULT_TIMEOUT;
  else
    sops->timeout = timeout;
  
  sops->compress_requests = false;
  
  return sops;
}

sops_handle_t* sops_new_basic(const char* uri, const char* user, const char* secret, int timeout) 
{
  assert (uri);
  assert (user);
  assert (secret);

  sops_handle_t* sops = malloc(sizeof(sops_handle_t));
  memset(sops,0,sizeof(sops_handle_t));
  
  sops->base_uri = strdup(uri);
  if (user) sops->user = strdup(user);
  if (secret) sops->secret = strdup(secret);
  if (timeout == -1)
    sops->timeout = SOPS_DEFAULT_TIMEOUT;
  else
    sops->timeout = timeout;
  
  sops->compress_requests = false;
  
  return sops;
}

void sops_compress_requests(sops_handle_t *sops, bool enable) 
{
  assert(sops);
  sops->compress_requests = enable;
}

void sops_destroy(sops_handle_t* sops) 
{
  if (sops) {
    free(sops->base_uri);
    if (sops->user) free(sops->user);
    if (sops->secret) free(sops->secret);
    if (sops->token) free(sops->token);
    free(sops);
  }
}

static int s_post_datastream(sops_handle_t* sops, const char* uri, const char* data, size_t len)
{
  int ret = STATUS_ERROR;

  ret = s_http_post_request(sops, uri, data, len);
  switch(ret) {
    case HTTP_STATUS_OK:
    case HTTP_STATUS_CREATED:
    case HTTP_STATUS_ACCEPTED:
    case HTTP_STATUS_NON_AUTHORITATIVE_INFORMATION:
    case HTTP_STATUS_NO_CONTENT:
    case HTTP_STATUS_RESET_CONTENT:
    case HTTP_STATUS_PARTIAL_CONTENT:
      debugLog("SOPS upload OK.");
      return STATUS_OK;
      break;
    case HTTP_STATUS_REQUEST_TIME_OUT:
      debugLog("SOPS request timeout");
      return STATUS_ERROR;
    case HTTP_STATUS_FORBIDDEN:
    case HTTP_STATUS_UNAUTHORIZED:
      // TODO: Authenticate and retry
      debugLog("SOPS authentication failed.");
      ret = STATUS_ERROR;
      break;
    default:
      debugLog("SOPS unknown error : %d", ret);
      return STATUS_ERROR; // Return an error and retry later
  }
  return ret;
}

int sops_post_datastream(sops_handle_t* sops, const char* data_endp, const char* data, size_t len)
{
  assert (sops);

  char full_uri[MAX_STRING_LEN];
  snprintf(full_uri, MAX_STRING_LEN, "%s%s", sops->base_uri, data_endp);
  return s_post_datastream(sops, full_uri, data, len);
}

#ifdef TESTING

/* Test */
int sops_selftest(bool verbose)
{
    printf (" * sops: ");

    printf ("OK\n");
    return 0;
}

#endif
