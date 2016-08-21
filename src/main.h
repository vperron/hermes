/**
 * =====================================================================================
 *
 *   @file main.h
 *
 *   
 *        Version:  1.0
 *        Created:  09/20/2012 08:26:35 PM
 *
 *
 *   @section DESCRIPTION
 *
 *       General headers
 *       
 *   @section LICENSE
 *
 *       
 *
 * =====================================================================================
 */

// FIXME: has to add something in configure.in to make this not break the asserts
// #include "platform.h"

#include <czmq.h> // Includes everything we would need
#include <stdbool.h>

#ifndef _HERMES_MAIN_H_
#define _HERMES_MAIN_H_ 

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Handy macros 
 * */
#define now() zclock_time()
#define streq(s1,s2)    (!strcmp ((s1), (s2)))
#define errorLog(fmt, ...) \
	do { \
		syslog(LOG_INFO,"%s [%s:%d] " fmt "\n", __func__,__FILE__,__LINE__, ##__VA_ARGS__); \
		fprintf(stderr,"%s [%s:%d] " fmt "\n", __func__,__FILE__,__LINE__, ##__VA_ARGS__); \
		fflush(stderr); \
	} while(0)

#if defined(DEBUG) && !defined(TESTING) && !defined(E2E_TESTING)
#define debugLog(fmt, ...) \
	do { \
		syslog(LOG_DEBUG,"%s [%s:%d] " fmt "\n", __func__,__FILE__,__LINE__, ##__VA_ARGS__); \
		printf("%s [%s:%d] " fmt "\n", __func__,__FILE__,__LINE__, ##__VA_ARGS__); \
		fflush(stdout); \
	} while(0)

#else
#define debugLog(fmt, ...) 
#endif


#if !defined(E2E_TESTING)
#define agent_msg_send zmsg_send
#define agent_msg_destroy zmsg_destroy
#define mem_free free
#else
void agent_msg_send(zmsg_t**, void*);
void agent_msg_destroy(zmsg_t**);
void mem_free(void*);
#endif



/*
 * Program-wide useful variables  
 * */
#define STATUS_OK 0
#define STATUS_ERROR -1
#define MAX_STRING_LEN 512

#define HERMES_THREAD_SLEEP 1 // 1 ms between pollings


/* 
 * Define messages between agents 
 * */
#define MSG_POST "POSTMSG"
#define MSG_POST_ID "POSTIDMSG"
#define MSG_POST_THROUGHPUT "POSTTHROUGHPUTMSG"
#define MSG_POST_HASH "POSTHASHMSG"
#define MSG_POST_FILE "POSTFILEMSG"
#define MSG_SUCCESS "SUCCESSMSG"
#define MSG_ERROR "ERRORMSG"


/*  Define all the agents as globals  */
extern void *ftp_agent_pipe;
extern void *http_agent_pipe;
extern void *failure_agent_pipe;
extern void *hashtable_agent_pipe;
extern void *persist_agent_pipe;


#ifdef __cplusplus
}
#endif

#endif // _HERMES_MAIN_H_
