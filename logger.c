// -----------------------------------------------------------------------------
// Copyright telecnatron.com. 2014.
// $Id: $
// -----------------------------------------------------------------------------
#include <stdarg.h>
#include <stdio.h>

#include "logger.h"

unsigned char logger_level=LOGGER_LEVEL_INFO;

void logger_start(int level)
{
   fprintf(stderr,"%s: ", LOGGER_LEVEL_STR(level));
}

void logger_output(char *msg)
{
    fprintf(stderr,"%s",msg);
}

void logger_fmt_output(const char *fmt,...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void logger_end()
{
    fprintf(stderr,"\n");
}

/** 
 * Loge the passed message with passed level
 * @param level The log-level int
 * @param msg  The message to be logged
 */
void logger_log(int level, char *msg)
{
    if(level > logger_level )
	return;
    logger_start(level);
    fprintf(stderr,"%s", msg);
    logger_end();
}

/** 
 * Log a formatted message, with arguments similiar to printf
 * @param level The log level string
 * @param fmt The format, followed by arguments
 */
void logger_fmt(int level, const char *fmt, ...)
{
    if(level > logger_level)
	return;
    va_list args;
    logger_start(level);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    logger_end();
}





