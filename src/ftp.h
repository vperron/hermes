/**
 * =====================================================================================
 *
 *   @file ftp.h
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

#ifndef _HERMES_FTP_H_
#define _HERMES_FTP_H_

typedef struct ftp_handle_t_st ftp_handle_t;

ftp_handle_t* ftp_new(const char* uri, const char *datastream_id, int timeout);
void ftp_destroy(ftp_handle_t* ftp);
int ftp_file_upload(ftp_handle_t* ftp, const char* filename);

int ftp_selftest(bool verbose);

#endif // _HERMES_FTP_H_
