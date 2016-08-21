/**
 * =====================================================================================
 *
 *   @file hashtable_agent.h
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


#ifndef _HERMES_hashtable_agent_H_
#define _HERMES_hashtable_agent_H_ 

#ifdef __cplusplus
extern "C" {
#endif


#ifndef E2E_TESTING
void hashtable_agent (void *user_args, zctx_t *ctx, void *pipe);
#else
void hashtable_agent (zmsg_t* msg, zlist_t* beacon_list, int maxsize, uint64_t maxtime, int first_maxsize);
extern uint64_t creation_time;
#endif



#ifdef __cplusplus
}
#endif

#endif // _HERMES_hashtable_agent_H_
