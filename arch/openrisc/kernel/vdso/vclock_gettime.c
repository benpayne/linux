/*
 * Copyright (C) 2015 Ben Payne <bpayne@jetheaddev.com>
 *
 * Based on x86 versions of this code. 
 */
#include <asm/linkage.h>
#include <linux/time.h>
#include <asm/vgtod.h>
#include <asm/vvar.h>

#define notrace __attribute__((no_instrument_function))

#define gtod (&VVAR(vsyscall_gtod_data))

notrace static int __always_inline do_realtime(struct timespec *ts)
{
	unsigned long seq;
	u64 ns;

	do {
		seq = gtod_read_begin(gtod);
		ts->tv_sec = gtod->wall_time_sec;
		ns = gtod->wall_time_snsec;
		ns >>= gtod->shift;
	} while (unlikely(gtod_read_retry(gtod, seq)));

	ts->tv_sec += __iter_div_u64_rem(ns, NSEC_PER_SEC, &ns);
	ts->tv_nsec = ns;

	return 0;
}

notrace static int __always_inline do_monotonic(struct timespec *ts)
{
	unsigned long seq;
	u64 ns;

	do {
		seq = gtod_read_begin(gtod);
		ts->tv_sec = gtod->monotonic_time_sec;
		ns = gtod->monotonic_time_snsec;
		ns >>= gtod->shift;
	} while (unlikely(gtod_read_retry(gtod, seq)));

	ts->tv_sec += __iter_div_u64_rem(ns, NSEC_PER_SEC, &ns);
	ts->tv_nsec = ns;

	return 0;
}

notrace static void do_realtime_coarse(struct timespec *ts)
{
	unsigned long seq;
	do {
		seq = gtod_read_begin(gtod);
		ts->tv_sec = gtod->wall_time_coarse_sec;
		ts->tv_nsec = gtod->wall_time_coarse_nsec;
	} while (unlikely(gtod_read_retry(gtod, seq)));
}

notrace static void do_monotonic_coarse(struct timespec *ts)
{
	unsigned long seq;
	do {
		seq = gtod_read_begin(gtod);
		ts->tv_sec = gtod->monotonic_time_coarse_sec;
		ts->tv_nsec = gtod->monotonic_time_coarse_nsec;
	} while (unlikely(gtod_read_retry(gtod, seq)));
}

notrace int __vdso_gettimeofday(struct timeval *tv, struct timezone *tz)
{
	if (likely(tv != NULL)) {
		do_realtime((struct timespec *)tv);
		tv->tv_usec /= 1000;
	}
	if (unlikely(tz != NULL)) {
		tz->tz_minuteswest = gtod->tz_minuteswest;
		tz->tz_dsttime = gtod->tz_dsttime;
	}

	return 0;
}

int gettimeofday(struct timeval *tv, struct timezone *tz)
    __attribute__((weak, alias("__vdso_gettimeofday")));

notrace int __vdso_clock_gettime(clockid_t clk_id, struct timespec *ts)
{
	switch (clk_id) {
	case CLOCK_REALTIME:
		do_realtime(ts);
		break;
	case CLOCK_MONOTONIC:
		do_monotonic(ts);
		break;
	case CLOCK_REALTIME_COARSE:
		do_realtime_coarse(ts);
		break;
	case CLOCK_MONOTONIC_COARSE:
		do_monotonic_coarse(ts);
		break;
	default:
		goto fallback;
	}

	return 0;
fallback:
	return -1;
}

int clock_gettime(clockid_t clk_id, struct timespec *ts)
    __attribute__((weak, alias("__vdso_clock_gettime")));

