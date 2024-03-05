#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "remote_access.h"
#include "params.h"
#include "log.h"

#define PATH_MAX 4096

static void my_fullpath(char fpath[PATH_MAX], const char *path)
{
	strcpy(fpath, MY_DATA->rootdir);
	strncat(fpath, path, PATH_MAX);

	log_msg("    my_fullpath:  rootdir = \"%s\", path = \"%s\", fpath = \"%s\"\n",
			MY_DATA->rootdir, path, fpath);
}

static int my_getattr(const char *path, struct stat *stbuf)
{
	int retstat = 0;
	char fpath[PATH_MAX];

	log_msg("\nmy_getattr(path=\"%s\", statbuf=0x%08x)\n",
			path, stbuf);

	my_fullpath(fpath, path);

	memset(stbuf, 0, sizeof(struct stat));

	if (strcmp(path, "/") == 0)
	{
		retstat = -ENOENT; // for now
						   // I won't be implementing an opendir or anything like that so I'm not sure what to do with this function
	}
	else
	{
		if (access(fpath, F_OK) != -1)
		{
			retstat = log_syscall("stat", stat(fpath, stbuf), 0);
			log_stat(stbuf);
		}
		else
		{
			char *remote_path = (char *)malloc(strlen(path) * sizeof(char));
			strcpy(remote_path, path + 1);
			unsigned long permissions = remote_stat(MY_DATA->my_ssh_session, remote_path);
			if (permissions != -1)
			{
				int dl_retstat = download_remote(MY_DATA->my_ssh_session, remote_path, fpath, permissions);
				if (dl_retstat < 0)
				{
					retstat = dl_retstat;
				}
				else
				{
					retstat = log_syscall("stat", stat(fpath, stbuf), 0);
					log_stat(stbuf);
				}
			}
			else
			{
				retstat = -ENOENT;
			}

			free(remote_path);
		}
	}

	if (retstat < 0)
		retstat = -errno;

	return retstat;
}

static int my_open(const char *path, struct fuse_file_info *fi)
{
	int retstat = 0;
	int fd;
	char fpath[PATH_MAX];

	my_fullpath(fpath, path);

	fd = log_syscall("open", open(fpath, fi->flags), 0);

	if (fd < 0)
		retstat = log_error("open");

	fi->fh = fd;

	log_fi(fi);

	return retstat;
}

static int my_read(const char *path, char *buf, size_t size, off_t offset,
				   struct fuse_file_info *fi)
{
	int retstat = 0;

	retstat = pread(fi->fh, buf, size, offset);

	if (retstat < 0)
		retstat = -errno;

	return retstat;
}

static int my_write(const char *path, const char *buf, size_t size, off_t offset,
					struct fuse_file_info *fi)
{
	log_fi(fi);

	int retstat = log_syscall("pwrite", pwrite(fi->fh, buf, size, offset), 0);

	char fpath[PATH_MAX];
	my_fullpath(fpath, path);

	char *remote_path = (char *)malloc(strlen(path) * sizeof(char));
	strcpy(remote_path, path + 1);
	retstat = upload_remote(MY_DATA->my_ssh_session, remote_path, fpath);

	return retstat;
}

static int my_release(const char *path, struct fuse_file_info *fi)
{

	log_fi(fi);

	int retstat = 0;
	retstat = log_syscall("close", close(fi->fh), 0);

	return retstat;
}

static int my_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{
	int retstat = 0;
	char fpath[PATH_MAX];
	my_fullpath(fpath, path);

	retstat = upload_remote(MY_DATA->my_ssh_session, path, fpath);

	if (retstat < 0)
	{
		log_msg("upload failed\n");
		return retstat;
	}

	retstat = fsync(fi->fh);

	return retstat;
}

void *my_init(struct fuse_conn_info *conn)
{
	return MY_DATA;
}

void my_destroy(void *userdata)
{
	struct my_state *data = (struct my_state *)userdata;

	if (data->my_ssh_session != NULL)
	{
		ssh_disconnect(*data->my_ssh_session);
		ssh_free(*data->my_ssh_session);
	}
	if (data->logfile != NULL)
	{
		fclose(data->logfile);
		data->logfile = NULL;
	}

	free(data);
}

void my_usage()
{
	fprintf(stderr, "usage:  myfs [FUSE and mount options] rootdir mountPoint\n");
	abort();
}

static struct fuse_operations my_oper = {
	.getattr = my_getattr,
	.open = my_open,
	.read = my_read,
	.write = my_write,
	.release = my_release,
	.fsync = my_fsync,
	.init = my_init,
	.destroy = my_destroy};

int main(int argc, char *argv[])
{
	int fuse_stat;
	struct my_state *my_data;

	if ((argc < 3) || (argv[argc - 2][0] == '-') || (argv[argc - 1][0] == '-'))
		my_usage();

	my_data = malloc(sizeof(struct my_state));
	if (my_data == NULL)
	{
		perror("main calloc");
		abort();
	}

	const char *pass = "";
	const char *user = "";
	const char *server = "aggravation.cs.utexas.edu";
	ssh_session session;

	int res = start_session(&session, server, user, pass);

	if (res < 0)
	{
		perror("main ssh session");
		abort();
	}

	my_data->my_ssh_session = &session;
	my_data->rootdir = argv[argc - 2];
	argv[argc - 2] = argv[argc - 1];
	argv[argc - 1] = NULL;
	argc--;

	my_data->logfile = log_open();

	fprintf(stderr, "about to call fuse_main\n");
	fuse_stat = fuse_main(argc, argv, &my_oper, my_data);

	return fuse_stat;
}
