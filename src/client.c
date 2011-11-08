#include "util.h"

int
main (int argc, char *argv[])
{
    int retval = -1;
    sockaddr_in local_addr;

    if (argc != 3)
        error (true, "usage: %s <serv_addr> <file>", argv[0]);

    const hostent const *host;
    if ((host = gethostbyname (argv[1])) == NULL)
        error (true, "couldn't resolve hostname: %s", argv[1]);

    memcpy (host->h_addr, (char*)&local_addr.sin_addr.s_addr, host->h_length);
    local_addr.sin_family = AF_INET;

    const servent const *serv;
    if ((serv = getservbyname ("irc", "tcp")) == NULL)
        error (true, "error: couldn't resolve service irc");
    
    local_addr.sin_port = htons (serv->s_port);

    int sock_desc;
    if ((sock_desc = socket (AF_INET, SOCK_STREAM, 0)) < 0)
        error (true, "error: couldn't create socket to server");

    if ((connect (sock_desc, (sockaddr*)(&local_addr), sizeof (local_addr))) < 0)
        error (true, "error: couldn't connect to server");

    FILE *file = fopen(argv[2], "r");
    if (!file)
    {
        error (false, "error: couldn't open file: %s", argv[2]);
        goto err;
    }

    char buffer[BUFFER_SIZE + 1];
    buffer[BUFFER_SIZE] = '\0';

    printf ("Sending.\n");

    if ((write (sock_desc, argv[2], BUFFER_SIZE)) <= 0)
    {
         error (false, "couldn't write filename to server");
         goto close;
    }

    ssize_t s;
    while ((s = read (fileno (file), buffer, BUFFER_SIZE)))
    {
        if ((write (sock_desc, buffer, s)) <= 0)
        {
             error (false, "couldn't write to server");
             goto close;
        }
    }

    printf ("Recieving.\n");
    if (read (sock_desc, buffer, BUFFER_SIZE) <= 0)
    {
        error (false, "Failed to receive filename");
        goto close;
    }
    file = freopen (buffer, "w", file);
    if (!file)
    {
        error (false, "couldn't open file: %s to write", buffer);
        goto err;
    }

    while ((s = read (sock_desc, buffer, BUFFER_SIZE)) > 0)
    {
        if (write (fileno (file), buffer, s) <= 0)
        {
            error (false, "Could not write to file");
            goto close;
        }
    }
    retval = 0;

close:
    fclose (file);
err:
    close (sock_desc);
    return retval;
}
