/**
 * @file debug.h
 *
 * @brief debug.h contanis debugging functionality.
 *
 * This file contains DEBUG() macro for debugging and optional DO_DEBUG
 * definition to enable debugging.
 */
#ifndef _DEBUG_H
#define _DEBUG_H

/*****************************************************************************
 * Includes                                                                  *
 *****************************************************************************/

/* Configuration */
#include "config.h"

/* System */
#include <stdio.h>

/*****************************************************************************
 * Definitions                                                               *
 *****************************************************************************/

/**
 * @brief Whether to enable or not debugging
 *
 * Define DO_DEBUG as 1 to enable debugging and as 0 to disbale
 * debugging.
 */
#ifndef DO_DEBUG /* Allow modulespecific debugging on/off */
#define DO_DEBUG	1
#endif

/**
 * @brief A definition that will print debug no matter whether debugging
 * is enabled or not
 *
 * This is intended for internal use of debug.h
 */
#define _DEBUG_PRINT_MESSAGE(fmt, ...) \
	fprintf(stderr, fmt "\n", ##__VA_ARGS__)

#if(DO_DEBUG)

#ifdef DEBUG
#undef DEBUG
#endif

/**
 * \def DEBUG
 * @brief Macro for printing debugging messages.
 *
 * If DO_DEBUG is defined to be non-zero, this macro will print a debugging
 * message to stderr. The message is in user-defined format (printf style),
 * with a new-line automatically added to the end.
 */
#define DEBUG(fmt, ...) \
	_DEBUG_PRINT_MESSAGE(fmt, ##__VA_ARGS__)
#else /* DO_DEBUG */
#define DEBUG(fmt, ...)
#endif /* DO_DEBUG */

/**
 * \def DEBUG_LONG
 * @brief Macro for printing debugging messages.
 *
 * If DO_DEBUG is defined to be non-zero, this macro will print a debugging
 * message to stderr. The message is in format
 * <pre>&lt;FILE&gt;: &lt;FUNCTION&gt;() (line &lt;LINE&gt;) &lt;USER_MESSAGE&gt;.</pre>
 * The user message and parameters can be given in normal printf style.
 *
 * Example use (file name example.c):<br>
 *<pre>void example_function(int parameter)
 *{
 *	DEBUG_LONG("Entering example function with parameter value %d", parameter);
 *}</pre>
 * would print the following style error message:<br>
 * <pre>example.c: 3: example_function(): Entering example function with parameter value 3</pre>
 * A newline is added to the end of each message.
 *
 * If DO_DEBUG is not defined, the this macro will be disabled and DEBUG_LONG()
 * does nothing.
 */

#define DEBUG_LONG(fmt, ...) \
        DEBUG("%s: %u: %s(): " fmt, \
                         __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

/**
 * \def DEBUG_BEGIN
 *
 * @brief Print a function entering debug message
 *
 * If DO_DEBUG is defined to be non-zero, this function prints a message
 * in format:<br>
 * <pre>example.c: 3: example_function(): BEGIN</pre>
 *
 * If DO_DEBUG is defined to be zero, this macro does nothing.
 */
#define DEBUG_BEGIN() DEBUG_LONG("BEGIN")

/**
 * \def DEBUG_END
 *
 * @brief Print a function leaving debug message
 *
 * If DO_DEBUG is defined to be non-zero, this function prints a message
 * in format:<br>
 * <pre>example.c: 3: example_function(): END</pre>
 *
 * If DO_DEBUG is defined to be zero, this macro does nothing.
 */
#define DEBUG_END() DEBUG_LONG("END")

/**
 * \def DEBUG_NOT_IMPLEMENTED
 *
 * @brief Print a function not implemented warning
 */
#define DEBUG_NOT_IMPLEMENTED() \
	_DEBUG_PRINT_MESSAGE("*** Not implemented: %s: %u: %s() ***", \
                         __FILE__, __LINE__, __FUNCTION__)

#endif // _DEBUG_H
