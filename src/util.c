#include "util.h"

void
error (bool fatal, const char *format, ...)
{
    fprintf (stderr, "error: ");
    va_list args;
    va_start (args, format);
    vfprintf (stderr, format, args);
    va_end (args);
    fprintf (stderr, "\n");
    if (fatal)
        exit (1);
}
