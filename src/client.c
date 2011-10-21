#include "util.h"

int
main (int argc, char *argv[])
{
    sockaddr_in local_addr;

    if (argc != 3)
        error ("usage: %s <serv_addr> <file>", argv[0]);

    const hostent const *host;
    if ((host = gethostbyname (argv[1])) == NULL)
        error ("error: couldn't resolve hostname: %s", argv[1]);

    memcpy (host->h_addr, (char*)&local_addr.sin_addr.s_addr, host->h_length);
    local_addr.sin_family = AF_INET;

    const servent const *serv;
    if ((serv = getservbyname ("irc", "tcp")) == NULL)
        error ("error: couldn't resolve service irc");
    
    local_addr.sin_port = htons (serv->s_port);

    int sock_desc;
    if ((sock_desc = socket (AF_INET, SOCK_STREAM, 0)) < 0)
        error ("error: couldn't create socket to server");

    if ((connect (sock_desc, (sockaddr*)(&local_addr), sizeof (local_addr))) < 0)
        error ("error: couldn't connect to server");

    FILE *file = fopen(argv[2], "r");
    if (!file)
        error ("error: couldn't open file: %s", argv[2]);

    char buffer[BUFFER_SIZE + 1];
    buffer[BUFFER_SIZE] = '\0';

    printf ("Sending.\n");

    if ((write (sock_desc, argv[2], BUFFER_SIZE)) < 0)
         error ("error: couldn't write to server");
    while (fgets (buffer, BUFFER_SIZE, file))
    {
        if ((write (sock_desc, buffer, BUFFER_SIZE)) < 0)
             error ("error: couldn't write to server");
    }
    if ((write (sock_desc, END_OF_FILE, sizeof (END_OF_FILE) + 1)) < 0)
         error ("error: couldn't write to server");

    printf ("Recieving.\n");
    if (read (sock_desc, buffer, BUFFER_SIZE) <= 0)
        error ("Failed to recieve filename");
    file = freopen (buffer, "w", file);
    if (!file)
        error ("error: couldn't open file: %s to write", buffer);

    while (read (sock_desc, buffer, BUFFER_SIZE) > 0)
        fprintf (file, "%s", buffer);

    close (sock_desc);
    fclose (file);
    return 0;
}
