#ifndef _PARAMS_H_
#define _PARAMS_H_

#define FUSE_USE_VERSION 26

#include <limits.h>
#include <stdio.h>
#include <libssh/libssh.h>
struct my_state
{
    FILE *logfile;
    char *rootdir;
    ssh_session *my_ssh_session;
};
#define MY_DATA ((struct my_state *)fuse_get_context()->private_data)

#endif
