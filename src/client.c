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

    FILE *in = fopen(argv[2], "r");
    if (!in)
        error ("error: couldn't open file: %s", argv[2]);

    char buffer[BUFFER_SIZE + 1];
    buffer[BUFFER_SIZE] = '\0';
    printf ("Sending:\n");
    while (fgets (buffer, BUFFER_SIZE, in))
    {
        printf ("%s", buffer);
        if ((write (sock_desc, buffer, BUFFER_SIZE)) < 0)
             error ("error: couldn't write to server");
    }
    if ((write (sock_desc, END_OF_FILE, sizeof (END_OF_FILE) + 1)) < 0)
         error ("error: couldn't write to server");
    printf ("\nRecieving:\n");
    while (read (sock_desc, buffer, BUFFER_SIZE) > 0)
        printf ("%s", buffer);
    printf ("\n");

    close (sock_desc);
    fclose (in);
    return 0;
}
