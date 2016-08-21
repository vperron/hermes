/**
 * =====================================================================================
 *
 *   @file sops.h
 *
 *   
 *        Version:  1.0
 *        Created:  01/08/2013 05:27:02 PM
 *
 *
 *   @section DESCRIPTION
 *
 *       
 *       
 *   @section LICENSE
 *
 *       
 *
 * =====================================================================================
 */

#ifndef _HERMES_SOPS_H_
#define _HERMES_SOPS_H_

typedef struct sops_handle_t_st sops_handle_t;

sops_handle_t* sops_new_token(const char* uri, const char* token, const char* version, int timeout);
sops_handle_t* sops_new_basic(const char* uri, const char* user, const char* secret, int timeout);
sops_handle_t* sops_new_user_password(const char* uri, const char* token_endp, 
                              const char* user, const char* secret, int timeout);
void sops_destroy(sops_handle_t* sops);

int sops_post_datastream(sops_handle_t* sops, const char* data_endp, const char* data, size_t len);

void sops_compress_requests(sops_handle_t *sops, bool enable);

#endif
