#include "util.h"

#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>

int sock_desc;

struct _thread {
    pthread_t tid;
    struct _thread *next;
};

struct _thread *tlist = NULL;

static void
thread_list_prepend (pthread_t tid)
{
    struct _thread *t = (struct _thread *) malloc (sizeof (struct _thread));
    t->tid = tid;
    t->next = tlist;
    tlist = t;
}

static void
exec_bg (char **cmd)
{
    pid_t pid;
    if (!(pid = fork ()))
        execv ("/usr/bin/ffmpeg", cmd);
    else
        waitpid (pid, NULL, 0);
}

static void *
answer (void *data)
{
    char buffer[BUFFER_SIZE + 1];
    buffer[BUFFER_SIZE] = '\0';
    const int sock = *((int *)data);

    /* Read the input file name */
    if (read (sock, buffer, BUFFER_SIZE) <= 0)
    {
        error (false, "Failed to recieve filename");
        goto close;
    }
    char *filename = strdup (buffer);
    char *input_file = (char *) malloc ((strlen (filename) + 6) * sizeof (char));
    sprintf (input_file, "/tmp/%s", filename);
    FILE *file = fopen (input_file, "w");
    if (!file)
    {
        error (false, "couldn't open file for write");
        goto out;
    }

    ssize_t s;
    while (((s = read (sock, buffer, BUFFER_SIZE)) > 0))
    {
        if (write (fileno(file), buffer, s) <= 0)
        {
            error (false, "failed to write to client");
            goto out;
        }
        if (s != BUFFER_SIZE)
            break;
    }

    char *output_file = strdup (input_file);
    strcpy (output_file + strlen (output_file) - 3, "ogg");
    char **cmd = (char **) malloc (6 * sizeof (char *));
    cmd[0] = "ffmpeg";
    cmd[1] = "-i";
    cmd[2] = input_file;
    cmd[3] = output_file;
    cmd[4] = "-vn";
    cmd[5] = NULL;
    exec_bg (cmd);
    free (cmd);

    file = freopen (output_file, "r", file);
    if (!file)
    {
        error (false, "couldn't reopen file for read");
        goto file_err;
    }

    strcpy (filename + strlen (filename) - 3, "ogg");
    if ((write (sock, filename, BUFFER_SIZE)) <= 0)
    {
         error (false, "couldn't write to client");
         goto err;
    }
    while ((s = read (fileno (file), buffer, BUFFER_SIZE)) > 0)
    {
        if ((write (sock, buffer, s)) <= 0)
        {
             error (false, "couldn't write to client");
             break;
        }
    }

err:
    fclose (file);
file_err:
    free (output_file);
out:
    free (input_file);
    free (filename);
close:
    close (sock);
    return NULL;
}

static void
close_socket (int signal)
{
    printf ("Signal %d received\n", signal);
    while (tlist)
    {
        pthread_join (tlist->tid, NULL);
        struct _thread *next = tlist->next;
        free (tlist);
        tlist = next;
    }
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
        error (true, "couldn't resolve hostname: %s", hostname);

    sockaddr_in local_addr;
    memcpy (host->h_addr, (char*)&local_addr.sin_addr.s_addr, host->h_length);
    local_addr.sin_family = host->h_addrtype;
    local_addr.sin_addr.s_addr = INADDR_ANY;

    const servent const *serv;
    if ((serv = getservbyname ("irc", "tcp")) == NULL)
        error (true, "couldn't resolve service irc");

    local_addr.sin_port = htons (serv->s_port);
    printf ("Listening on port: %d\n", ntohs (local_addr.sin_port));

    if ((sock_desc = socket (AF_INET, SOCK_STREAM, 0)) < 0)
        error (true, "couldn't create socket");

    signal (SIGINT,  close_socket);
    signal (SIGTERM, close_socket);

    if ((bind(sock_desc, (sockaddr*)(&local_addr), sizeof (local_addr))) < 0)
        error (true, "coudln't bind to socket");

    listen(sock_desc, 10);

    for (;;)
    {
        int new_sock_desc;
        sockaddr_in current_client_addr;
        unsigned current_addr_length = sizeof (current_client_addr);
        if ((new_sock_desc = accept (sock_desc, (sockaddr*)(&current_client_addr), &current_addr_length)) < 0)
        {
            error (false, "couldn't connect to client");
            continue;
        }

        pthread_t thread;
        pthread_create (&thread, NULL, answer, &new_sock_desc);
        thread_list_prepend (thread);
    }

    /* We never get here, though */
    close (sock_desc);
    return 0;
}

