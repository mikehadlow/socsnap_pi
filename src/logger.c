#include <stdio.h>
#include <stdarg.h>

#define LOGFILE "/tmp/socsnap.log"

void writeline(const char *format, ...)
{
    FILE *log;

    va_list(args);
    va_start(args, format);

#ifdef DEBUG
    vprintf(format, args);
#endif
    
    log = fopen(LOGFILE, "a");
    if(log) {
        vfprintf(log, format, args);
        fclose(log);
    } else {
        printf("Can't open log file.\n");
    }

    va_end(args);
}
