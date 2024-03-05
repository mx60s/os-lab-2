#include <libssh/libssh.h>
#include <stdlib.h>
#include <stdio.h>

int start_session(ssh_session *session, const char *servername, const char *username, const char *password);

int main()
{
    const char *servername = "aggravation.cs.utexas.edu";
    const char *username = "";
    const char *password = "";
    ssh_session my_ssh_session;
    int rc;

    rc = start_session(&my_ssh_session, servername, username, password);

    if (rc == 0)
    {
        printf("SSH session started successfully.\n");
    }
    else
    {
        fprintf(stderr, "Error starting SSH session: %d\n", rc);
    }

    if (my_ssh_session)
    {
        ssh_disconnect(my_ssh_session);
        ssh_free(my_ssh_session);
    }

    return rc;
}

int start_session(ssh_session *session, const char *servername, const char *username, const char *password)
{
    *session = ssh_new();
    if (*session == NULL)
        return -1;

    ssh_options_set(*session, SSH_OPTIONS_HOST, servername);
    ssh_options_set(*session, SSH_OPTIONS_USER, username);

    int rc = ssh_connect(*session);
    if (rc != SSH_OK)
    {
        ssh_free(*session);
        return -1;
    }
    printf("Started session happily.\n");

    rc = ssh_userauth_password(*session, NULL, password);
    if (rc != SSH_AUTH_SUCCESS)
    {
        ssh_disconnect(*session);
        ssh_free(*session);
        return -1;
    }

    return 0;
}
