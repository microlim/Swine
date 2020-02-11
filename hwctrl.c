
#include "hwctrl.h"
#include <sys/time.h>

/*---------------------------------------------------------------------------*/
clock_time_t
clock_time(void)
{
  struct timeval tv;
  struct timezone tz;

  gettimeofday(&tv, &tz);

  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}
/*---------------------------------------------------------------------------*/

void
timer_set(struct timer *t, clock_time_t interval)
{
  t->interval = interval;
  t->start = clock_time();
}
/*---------------------------------------------------------------------------*/
/**
 * Reset the timer with the same interval.
 *
 * This function resets the timer with the same interval that was
 * given to the timer_set() function. The start point of the interval
 * is the exact time that the timer last expired. Therefore, this
 * function will cause the timer to be stable over time, unlike the
 * timer_rester() function.
 *
 * \param t A pointer to the timer.
 *
 * \sa timer_restart()
 */
void
timer_reset(struct timer *t)
{
  t->start += t->interval;
}
/*---------------------------------------------------------------------------*/
/**
 * Restart the timer from the current point in time
 *
 * This function restarts a timer with the same interval that was
 * given to the timer_set() function. The timer will start at the
 * current time.
 *
 * \note A periodic timer will drift if this function is used to reset
 * it. For preioric timers, use the timer_reset() function instead.
 *
 * \param t A pointer to the timer.
 *
 * \sa timer_reset()
 */
void
timer_restart(struct timer *t)
{
  t->start = clock_time();
}
/*---------------------------------------------------------------------------*/
/**
 * Check if a timer has expired.
 *
 * This function tests if a timer has expired and returns true or
 * false depending on its status.
 *
 * \param t A pointer to the timer
 *
 * \return Non-zero if the timer has expired, zero otherwise.
 *
 */
int
timer_expired(struct timer *t)
{
  return (clock_time_t)(clock_time() - t->start) >= (clock_time_t)t->interval;
}

int KeyInput(void)
{
		int fd;	
		int ret;
		char dev_name[20]= "/dev/input/event3"; // 키입력 디바이스 드라이버 
		struct input_event *key_event; 



		fd = open(dev_name, O_RDONLY);
		if(fd < 0)
		{
			printf("error to open /dev/input/event3\n");
		} 
		key_event = malloc(sizeof(struct input_event));
		memset(key_event,0,sizeof(key_event));
		
		
		ret = read(fd,key_event,sizeof(struct input_event)); 
		if(ret < 0)
		{
			printf("key event read error\n");
		}

		close(fd);		
		
		if(key_event->type == EV_KEY && key_event->value==1)
		{
		
			return key_event->code ;
		}
		

		return 0 ;
}

void Buzzer( char onoff )
{
	int fd;
	int ret = 0;
	int buzzer_on= 8;
	int buzzer_off= 7;

	unsigned int i, num;
	char read_pin_buf;

	fd = open("/dev/ctl_io",O_RDWR);
	if(fd < 0)
	{
		perror("error to open /dev/ctl_io\n");
		return ;
	}
	if( onoff == 1 )
			ret = write(fd, &buzzer_on,4);
	 else 
			ret = write(fd, &buzzer_off,4);
	
	if(fd < 0)
	{
		printf("error to open /dev/ctl_io\n");
		return ;
	}
		
	close(fd);
}

void BuzzerBeep( void )
{
		Buzzer(1) ; 
		usleep(200*1000) ;
		Buzzer(0) ;
		usleep(100*1000) ;
}