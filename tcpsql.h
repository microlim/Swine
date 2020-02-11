#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <string.h>
#include <stdlib.h>

#include "tcpman.h"
#include "Packet.h"
#include "util.h"
#include "TTA12_packet.h"
extern "C" {
#include "iniparser.h"
#include "rs232.h"
#include "hwctrl.h"
}

#include <time.h>

#define USE_MYSQL 1

#include "cf_base.h"
#include "cf_db_mysql.h"




/* Die with fatal error. */
#define FATAL(msg)                                        		\
	do {                                                    	\
		fprintf(stderr,                                       	\
			"Fatal error in %s on line %d: %s\n",         		\
			__FILE__,                                     		\
			__LINE__,                                     		\
			msg);                                         		\
		fflush(stderr);                                       	\
		abort();                                              	\
	} while (0)

/* No error Assertion Test */
#define ASSERT_OK(expr,msg)											\
	do {                                                     	\
		cf_ret_t __stat;  \
		if (CF_OK != (__stat = expr)) {                                          \
			fprintf(stderr,                                     \
				"Assertion failed in %s on line %d: %s\n\t%s\n",   	\
				__FILE__,                                     	\
				__LINE__,                                     	\
				#expr, msg);                                       	\
			fflush(stderr);                                       	\
			abort();                                            \
		}                                                       \
	} while (0)

/* Assertion Test */
#define ASSERT(expr,msg)											\
	do {                                                     	\
		if (!(expr)) {                                          \
			fprintf(stderr,                                     \
				"Assertion failed in %s on line %d: %s\n\t%s\n",    	\
				__FILE__,                                     	\
				__LINE__,                                     	\
				#expr, msg);                                       	\
			fflush(stderr);                                       	\
			abort();                                            \
		}                                                       \
	} while (0)

/* Test Run */
#define TEST(expr)											\
	do {                                                     	\
		if (CF_OK == (expr)) {                                          \
			fprintf(stderr,                                     \
				"%s test pass in %s on line %d\n",    	\
				#expr,                                       	\
				__FILE__,                                     	\
				__LINE__);                                     	\
			fflush(stderr);                                       	\
		}                                                       \
	} while (0)
