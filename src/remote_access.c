#include <string.h>
#include <libssh/sftp.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "log.h"

int start_session(ssh_session *session, const char *servername, const char *username, const char *password)
{
	*session = ssh_new();
	if (session == NULL)
		return -1;

	ssh_options_set(*session, SSH_OPTIONS_HOST, servername);
	ssh_options_set(*session, SSH_OPTIONS_USER, username);

	int rc = ssh_connect(*session);
	if (rc != SSH_OK)
	{
		ssh_free(*session);
		return -1;
	}
	printf("started session happily");

	rc = ssh_userauth_password(*session, NULL, password);
	if (rc != SSH_AUTH_SUCCESS)
	{
		ssh_disconnect(*session);
		ssh_free(*session);
		return -1;
	}

	return 0;
}

int end_session(ssh_session *session)
{
	if (session != NULL)
	{
		ssh_disconnect(*session);
		ssh_free(*session);
		*session = NULL;
	}

	return 0;
}

int download_remote(const ssh_session *session, const char *remote_path, const char *local_path, const unsigned long permissions)
{
	sftp_session sftp = sftp_new(*session);
	if (sftp == NULL)
		return -1;

	int rc = sftp_init(sftp);
	if (rc != SSH_OK)
	{
		sftp_free(sftp);
		return -1;
	}

	sftp_file file = sftp_open(sftp, remote_path, O_RDONLY, 0);
	log_msg("file download sftp opened %s\n", remote_path);
	if (file == NULL)
	{
		sftp_free(sftp);
		return -1;
	}

	FILE *local_file = fopen(local_path, "wb");
	if (local_file == NULL)
	{
		sftp_close(file);
		sftp_free(sftp);
		return -1;
	}

	char buffer[1024];
	int nbytes;
	while ((nbytes = sftp_read(file, buffer, sizeof(buffer))) > 0)
	{
		if (fwrite(buffer, 1, nbytes, local_file) != nbytes)
		{
			sftp_close(file);
			sftp_free(sftp);
			fclose(local_file);
			return -1;
		}
	}

	if (nbytes < 0)
	{
		sftp_close(file);
		sftp_free(sftp);
		fclose(local_file);
		return -1;
	}

	sftp_close(file);
	sftp_free(sftp);
	fclose(local_file);

	if (chmod(local_path, (mode_t)permissions) < 0)
	{
		perror("chmod failed");
		return -1;
	}

	return 0;
}

int upload_remote(const ssh_session *session, const char *remote_path, const char *local_path)
{
	sftp_session sftp = sftp_new(*session);
	if (sftp == NULL)
	{
		log_msg("sftp new failed\n");
		return -1;
	}

	int rc = sftp_init(sftp);
	if (rc != SSH_OK)
	{
		sftp_free(sftp);
		log_msg("ssh not ok\n");
		return -1;
	}

	sftp_file file = sftp_open(sftp, remote_path, O_WRONLY | O_CREAT, S_IRWXU);
	if (file == NULL)
	{
		log_msg("file null path: %s\n", remote_path);
		sftp_free(sftp);
		return -1;
	}

	FILE *local_file = fopen(local_path, "r");
	if (local_file == NULL)
	{
		log_msg("local file null\n");
		sftp_close(file);
		sftp_free(sftp);
		return -1;
	}

	char buffer[1024];
	int nbytes;
	int total_bytes = 0;
	while ((nbytes = fread(buffer, 1, sizeof(buffer), local_file)) > 0)
	{
		if (sftp_write(file, buffer, nbytes) != nbytes)
		{
			log_msg("sftp write got bad bytes\n");
			sftp_close(file);
			sftp_free(sftp);
			fclose(local_file);
			return -1;
		}
		total_bytes += nbytes;
	}

	sftp_close(file);
	sftp_free(sftp);
	fclose(local_file);

	if (ferror(local_file))
	{
		log_msg("local file ferror\n");
		return -1;
	}

	return total_bytes;
}

unsigned long remote_stat(const ssh_session *session, const char *path)
{
	sftp_session sftp = sftp_new(*session);
	if (sftp == NULL)
		return -1;

	int rc = sftp_init(sftp);
	if (rc != SSH_OK)
	{
		sftp_free(sftp);
		return -1;
	}

	sftp_attributes attributes = sftp_stat(sftp, path);

	if (attributes)
	{
		unsigned long permissions = attributes->permissions;
		sftp_attributes_free(attributes);
		sftp_free(sftp);
		return permissions;
	}
	else
	{
		sftp_attributes_free(attributes);
		sftp_free(sftp);
		return -1;
	}
}
