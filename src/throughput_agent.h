/**
 * =====================================================================================
 *
 *   @file throughput_agent.h
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


#ifndef _HERMES_throughput_agent_H_
#define _HERMES_throughput_agent_H_ 

#ifdef __cplusplus
extern "C" {
#endif


#ifndef E2E_TESTING
void throughput_agent (void *user_args, zctx_t *ctx, void *pipe);
#else
void throughput_agent (zmsg_t* msg, zhash_t* throughput, int maxsize, uint64_t maxtime);
extern uint64_t creation_time;
#endif



#ifdef __cplusplus
}
#endif

#endif // _HERMES_throughput_agent_H_
