
#ifndef __HW_CTRL__
#define __HW_CTRL__


#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#include <stddef.h>
#include <linux/input.h>


#include "common.h"
 
#ifndef __CLOCK_ARCH_H__
#define __CLOCK_ARCH_H__

typedef int clock_time_t;
#define CLOCK_CONF_SECOND 1000

#endif /* __CLOCK_ARCH_H__ */


#ifndef __CLOCK_H__
#define __CLOCK_H__

#include "clock-arch.h"

/**
 * Initialize the clock library.
 *
 * This function initializes the clock library and should be called
 * from the main() function of the system.
 *
 */
void clock_init(void);

/**
 * Get the current clock time.
 *
 * This function returns the current system clock time.
 *
 * \return The current clock time, measured in system ticks.
 */
clock_time_t clock_time(void);

/**
 * A second, measured in system clock time.
 *
 * \hideinitializer
 */
#ifdef CLOCK_CONF_SECOND
#define CLOCK_SECOND CLOCK_CONF_SECOND
#else
#define CLOCK_SECOND (clock_time_t)32
#endif

#endif /* __CLOCK_H__ */


#ifndef __TIMER_H__
#define __TIMER_H__

/**
 * A timer.
 *
 * This structure is used for declaring a timer. The timer must be set
 * with timer_set() before it can be used.
 *
 * \hideinitializer
 */
struct timer {
  clock_time_t start;
  clock_time_t interval;
};

void timer_set(struct timer *t, clock_time_t interval);
void timer_reset(struct timer *t);
void timer_restart(struct timer *t);
int timer_expired(struct timer *t);

#endif /* __TIMER_H__ */

int  KeyInput(void) ;
void Buzzer( char onoff ) ;
void BuzzerBeep( void ) ;

#endif
