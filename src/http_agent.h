/**
 * =====================================================================================
 *
 *   @file http_agent.h
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


#ifndef _HERMES_http_agent_H_
#define _HERMES_http_agent_H_ 

#ifdef __cplusplus
extern "C" {
#endif


#ifndef E2E_TESTING
void http_agent (void *user_args, zctx_t *ctx, void *pipe);
#else
void http_agent (zmsg_t* message);
#endif


int http_agent_selftest(bool verbose);


#ifdef __cplusplus
}
#endif

#endif // _HERMES_http_agent_H_
