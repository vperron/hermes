/**
 * =====================================================================================
 *
 *   @file utils.h
 *
 *   
 *        Version:  1.0
 *        Created:  04/23/2013 11:27:59 AM
 *
 *
 *   @section DESCRIPTION
 *
 *       Utility I/O functions
 *       
 *   @section LICENSE
 *
 *       
 *
 * =====================================================================================
 */


#ifndef _HERMES_UTILS_H_
#define _HERMES_UTILS_H_


int utils_copy_file(const char *src, const char *dst);
char *utils_get_vfat_mountpoint();
void utils_send_success(const char* msg_id);
void utils_send_failure(const char* msg_id);
void utils_recursive_mkdir(const char *path);
int utils_gzip_write(const char* file_name, char* data, size_t len);
int utils_gzip_read(const char* file_name, char* buffer, size_t maxlen);
int utils_delete_file(const char *file_name);
char *utils_generate_json(const char *timestamp, const char* mac, const char* prefix,
                          const char* dbm, const char* bssid, const char* ssid);


#endif // _HERMES_UTILS_H_
