#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <stdio.h>
#include <stdlib.h>

int remote_file_exists(ssh_session session, const char *path);

int main()
{
    ssh_session my_ssh_session = ssh_new();
    const char *servername = "aggravation.cs.utexas.edu";
    const char *username = "";
    const char *password = "";
    const char *remote_path = "howdy.txt";

    if (my_ssh_session == NULL)
    {
        fprintf(stderr, "Failed to allocate SSH session.\n");
        exit(-1);
    }

    ssh_options_set(my_ssh_session, SSH_OPTIONS_HOST, servername);
    ssh_options_set(my_ssh_session, SSH_OPTIONS_USER, username);

    int rc = ssh_connect(my_ssh_session);
    if (rc != SSH_OK)
    {
        fprintf(stderr, "Error connecting to %s: %s\n", servername, ssh_get_error(my_ssh_session));
        ssh_free(my_ssh_session);
        exit(-1);
    }

    rc = ssh_userauth_password(my_ssh_session, NULL, password);
    if (rc != SSH_AUTH_SUCCESS)
    {
        fprintf(stderr, "Error authenticating with password: %s\n", ssh_get_error(my_ssh_session));
        ssh_disconnect(my_ssh_session);
        ssh_free(my_ssh_session);
        exit(-1);
    }

    rc = remote_file_exists(my_ssh_session, remote_path);
    if (rc == 0)
    {
        printf("The file '%s' exists on the remote server.\n", remote_path);
    }
    else
    {
        printf("The file '%s' does not exist on the remote server.\n", remote_path);
    }

    ssh_disconnect(my_ssh_session);
    ssh_free(my_ssh_session);

    return rc;
}

int remote_file_exists(ssh_session session, const char *path)
{
    sftp_session sftp = sftp_new(session);
    if (sftp == NULL)
        return -1;

    int rc = sftp_init(sftp);
    if (rc != SSH_OK)
    {
        sftp_free(sftp);
        return -1;
    }

    sftp_attributes attributes = sftp_stat(sftp, path);
    int file_exists = (attributes != NULL);

    if (attributes)
    {
        sftp_attributes_free(attributes);
    }

    sftp_free(sftp);

    return file_exists ? 0 : -1;
}
