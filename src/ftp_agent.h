/**
 * =====================================================================================
 *
 *   @file ftp_agent.h
 *
 *   
 *        Version:  1.0
 *        Created:  04/16/2013 08:24:08 PM
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


#ifndef _HERMES_FTP_AGENT_H_
#define _HERMES_FTP_AGENT_H_ 

#ifdef __cplusplus
extern "C" {
#endif

#define FTP_AGENT_SEND_HASHMAP "FTP_SEND_HASHMAP"

void ftp_agent (void *user_args, zctx_t *ctx, void *pipe);


#ifdef __cplusplus
}
#endif

#endif // _HERMES_FTP_AGENT_H_
