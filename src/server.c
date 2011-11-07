#include "util.h"

#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>

/*
 * TODO: free memory on error, support files with extension other than mp3 (strlen (extension) != 3)
 */

int sock_desc;

static void *
convert_thread (void *data)
{
    char **cmd = (char **) data;
    execv ("/usr/bin/ffmpeg", cmd);
    return NULL;
}

static void
exec_bg (char **cmd)
{
    //pthread_t thread;
    //pthread_create (&thread, NULL, convert_thread, cmd);
    //pthread_join (thread, NULL);
    pid_t pid;
    if (!(pid = fork ()))
        convert_thread (cmd);
    else waitpid (pid, NULL, 0);
}

static void *
answer (void *data)
{
    char buffer[BUFFER_SIZE + 1];
    buffer[BUFFER_SIZE] = '\0';
    const int sock = *((int *)data);

    /* Read the input file name */
    if (read (sock, buffer, BUFFER_SIZE) <= 0)
        error ("Failed to recieve filename");
    char *filename = strdup (buffer);
    char *input_file = (char *) malloc ((strlen (filename) + 6) * sizeof (char));
    sprintf (input_file, "/tmp/%s", filename);
    FILE *file = fopen (input_file, "w");
    if (!file)
    {
        fprintf (stderr, "couldn't open file for write\n");
        goto out;
    }

    ssize_t s;
    while (((s = read (sock, buffer, BUFFER_SIZE)) > 0))
    {
        if (write (fileno(file), buffer, s) <= 0)
            error ("failed to write to client");
        if (s != BUFFER_SIZE)
            break;
    }

    char *output_file = strdup (input_file);
    strcpy (output_file + strlen (output_file) - 3, "ogg");
    char **cmd = (char **) malloc (5 * sizeof (char *));
    cmd[0] = "ffmpeg";
    cmd[1] = "-i";
    cmd[2] = input_file;
    cmd[3] = output_file;
    cmd[4] = NULL;
    exec_bg (cmd);
    free (cmd);

    file = freopen (output_file, "r", file);
    if (!file)
    {
        fprintf (stderr, "couldn't reopen file for read\n");
        goto err;
    }

    strcpy (filename + strlen (filename) - 3, "ogg");
    if ((write (sock, filename, BUFFER_SIZE)) <= 0)
         error ("error: couldn't write to client");
    while ((s = read (fileno (file), buffer, BUFFER_SIZE)) > 0)
    {
        if ((write (sock, buffer, s)) <= 0)
             error ("error: couldn't write to client");
    }
    fclose (file);

err:
    free (output_file);
out:
    free (input_file);
    free (filename);
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

    signal (SIGINT,  close_socket);
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

