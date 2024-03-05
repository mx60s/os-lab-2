#ifndef _REMOTE_ACCESS_H_
#define _REMOTE_ACCESS_H_

#include <libssh/libssh.h>

int start_session(ssh_session *session, const char *servername, const char *username, const char *password);
int end_session(ssh_session *session);
int download_remote(const ssh_session *session, const char *remote_path, const char *local_path, const unsigned long permissions);
int upload_remote(const ssh_session *session, const char *remote_path, const char *local_path);
unsigned long remote_stat(const ssh_session *session, const char *path);
#endif
