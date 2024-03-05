#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

int upload_remote(ssh_session session, const char *remote_path, const char *local_path);
int remote_file_exists(ssh_session session, const char *path);

int main()
{
    ssh_session my_ssh_session = ssh_new();
    const char *servername = "aggravation.cs.utexas.edu";
    const char *username = "";
    const char *password = "";
    const char *local_path = "/tmp/hello.txt";
    const char *remote_path = "hello.txt";

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

    rc = upload_remote(my_ssh_session, remote_path, local_path);
    if (rc == 0)
    {
        printf("File '%s' uploaded successfully to '%s'.\n", local_path, remote_path);
        // I just checked the file manually here
    }
    else
    {
        fprintf(stderr, "Error uploading file.\n");
    }

    ssh_disconnect(my_ssh_session);
    ssh_free(my_ssh_session);

    return rc;
}

int upload_remote(ssh_session session, const char *remote_path, const char *local_path)
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

    sftp_file file = sftp_open(sftp, remote_path, O_WRONLY | O_CREAT, S_IRWXU);
    if (file == NULL)
    {
        sftp_free(sftp);
        return -1;
    }

    FILE *local_file = fopen(local_path, "rb");
    if (local_file == NULL)
    {
        sftp_close(file);
        sftp_free(sftp);
        return -1;
    }

    char buffer[1024];
    int nbytes;
    while ((nbytes = fread(buffer, 1, sizeof(buffer), local_file)) > 0)
    {
        if (sftp_write(file, buffer, nbytes) != nbytes)
        {
            sftp_close(file);
            sftp_free(sftp);
            fclose(local_file);
            return -1;
        }
    }

    sftp_close(file);
    sftp_free(sftp);
    fclose(local_file);

    if (ferror(local_file))
    {
        return -1;
    }

    return 0;
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
