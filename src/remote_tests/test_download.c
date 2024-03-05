#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>

int download_remote(ssh_session session, const char *remote_path, const char *local_path);

int main()
{
    ssh_session my_ssh_session = ssh_new();
    const char *servername = "aggravation.cs.utexas.edu";
    const char *username = "";
    const char *password = "";
    const char *remote_path = "test.txt";
    const char *local_path = "/tmp/test.txt";

    if (my_ssh_session == NULL)
        exit(-1);

    ssh_options_set(my_ssh_session, SSH_OPTIONS_HOST, servername);
    ssh_options_set(my_ssh_session, SSH_OPTIONS_USER, username);

    int rc = ssh_connect(my_ssh_session);
    if (rc != SSH_OK)
    {
        fprintf(stderr, "Error connecting to localhost: %s\n", ssh_get_error(my_ssh_session));
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

    rc = download_remote(my_ssh_session, remote_path, local_path);
    if (rc == 0)
    {
        printf("File downloaded successfully.\n");
    }
    else
    {
        fprintf(stderr, "Error downloading file.\n");
    }

    ssh_disconnect(my_ssh_session);
    ssh_free(my_ssh_session);

    return rc;
}

int download_remote(ssh_session session, const char *remote_path, const char *local_path)
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

    sftp_file file = sftp_open(sftp, remote_path, O_RDONLY, 0);
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

    return 0;
}
