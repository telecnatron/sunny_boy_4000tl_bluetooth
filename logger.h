#ifndef LOGGER_H
#define LOGGER_H
// -----------------------------------------------------------------------------
// Copyright telecnatron.com. 2014.
// $Id: logger.h 125 2014-09-09 02:50:47Z steves $
// -----------------------------------------------------------------------------

// macros to allow unformatted logging
#define LOGGER_ERROR(msg) logger_log(LOGGER_LEVEL_ERROR, msg)
#define LOGGER_WARN(msg) logger_log(LOGGER_LEVEL_WARN, msg)
#define LOGGER_INFO(msg) logger_log(LOGGER_LEVEL_INFO, msg)
#define LOGGER_DEBUG(msg) logger_log(LOGGER_LEVEL_DEBUG, msg)

// macros to allow formatted logging
#define LOGGER_FMT_ERROR(fmt, msg...) logger_fmt(LOGGER_LEVEL_ERROR, fmt, msg)
#define LOGGER_FMT_WARN(fmt, msg...)  logger_fmt(LOGGER_LEVEL_WARN, fmt, msg)
#define LOGGER_FMT_INFO(fmt, msg...)  logger_fmt(LOGGER_LEVEL_INFO, fmt, msg)
#define LOGGER_FMT_DEBUG(fmt, msg...) logger_fmt(LOGGER_LEVEL_DEBUG, fmt, msg)

// set the logger level, level should be one of the LOGGER_LEVEL_xxx defines
#define LOGGER_SET_LEVEL(level) logger_level = level

// log level definitions
#define LOGGER_LEVEL_DEBUG  3
#define LOGGER_LEVEL_INFO   2
#define LOGGER_LEVEL_WARN   1
#define LOGGER_LEVEL_ERROR  0

// level names, use LOGGER_LEVEL_xxx to index into these
static const char *logger_level_str[]={
    "ERROR",
    "WARN",
    "INFO",
    "DEBUG"
};

// return string corresponding to passed log level
#define LOGGER_LEVEL_STR(level) logger_level_str[level]

// the current logger level
extern unsigned char logger_level;

/** 
 * Log the passed message with passed level, message will only be logged
 * if the level is equal to or below logger_level
 * @param level The log-level int
 * @param msg  The message to be logged
 */
void logger_log(int level, char *msg);

/** 
 * Log at passed log-level  the formatted messaged,
 * fmt and arguments are similiar to printf()
 * message will only be logged if the level is equal to or below logger_level
 * @param level The log-level int
 * @param fmt The format string, followed by the arguments
 */
void logger_fmt(int level, const char *fmt, ...);


//! Log the starting part of a log message
void logger_start(int level);
//! Log the ending part of a log message
void logger_end();

//! output the passed string. Should be called after logger_start() and before logger_end()
void logger_output(char *msg);

//! output the passed string using passed format. Should be called after logger_start() and before logger_end()
void logger_fmt_output(const char *fmt,...);


#endif
