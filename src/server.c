#include "util.h"

#include <pthread.h>
#include <signal.h>

int sock_desc;

static void *
answer (void *data)
{
    char buffer[BUFFER_SIZE + 1];
    buffer[BUFFER_SIZE] = '\0';
    const int sock = *((int *)data);

    FILE *file = fopen ("/tmp/__foobar__", "w");
    if (!file)
    {
        fprintf (stderr, "couldn't open file for write\n");
        goto out;
    }

    while ((read (sock, buffer, BUFFER_SIZE) > 0)
            && (strcmp(END_OF_FILE, buffer) != 0))
        fprintf (file, "%s", buffer);

    file = freopen ("/tmp/__foobar__", "r", file);
    if (!file)
    {
        fprintf (stderr, "couldn't reopen file for read\n");
        goto out;
    }

    while (fgets (buffer, BUFFER_SIZE, file))
    {
        if ((write (sock, buffer, BUFFER_SIZE)) < 0)
             error ("error: couldn't write to client");
    }
    printf ("\n");
    fclose (file);

out:
    close (sock);
    return NULL;
}

static void
close_socket (int signal)
{
    printf ("Signal %d received\n", signal);
    close (sock_desc);
    exit (0);
}

int
main()
{
    char hostname[256];
    gethostname(hostname, 255);
    const hostent const *host;
    if ((host = gethostbyname (hostname)) == NULL)
        error ("error: couldn't resolve hostname: %s", hostname);

    sockaddr_in local_addr;
    memcpy (host->h_addr, (char*)&local_addr.sin_addr.s_addr, host->h_length);
    local_addr.sin_family = host->h_addrtype;
    local_addr.sin_addr.s_addr = INADDR_ANY;

    const servent const *serv;
    if ((serv = getservbyname ("irc", "tcp")) == NULL)
        error ("error: couldn't resolve service irc");

    local_addr.sin_port = htons (serv->s_port);
    printf ("Listening on port: %d\n", ntohs (local_addr.sin_port));

    if ((sock_desc = socket (AF_INET, SOCK_STREAM, 0)) < 0)
        error ("error: couldn't create socket");

    signal (SIGINT, close_socket);
    signal (SIGKILL, close_socket);
    signal (SIGTERM, close_socket);

    if ((bind(sock_desc, (sockaddr*)(&local_addr), sizeof (local_addr))) < 0)
        error ("error: coudln't bind to socket");

    listen(sock_desc, 10);

    for (;;)
    {
        int new_sock_desc;
        sockaddr_in current_client_addr;
        unsigned current_addr_length = sizeof (current_client_addr);
        if ((new_sock_desc = accept (sock_desc, (sockaddr*)(&current_client_addr), &current_addr_length)) < 0)
            error ("error: couldn't connect to client");

        pthread_t thread;
        pthread_create (&thread, NULL, answer, &new_sock_desc);
    }

    close (sock_desc);
    return 0;
}

