#ifndef _ASM_X86_VGTOD_H
#define _ASM_X86_VGTOD_H

#include <linux/compiler.h>
#include <linux/clocksource.h>

typedef unsigned long gtod_long_t;

struct vsyscall_gtod_data {
	unsigned seq;

	cycle_t	cycle_last;
	cycle_t	mask;
	u32	mult;
	u32	shift;

	/* open coded 'struct timespec' */
	u64		wall_time_snsec;
	gtod_long_t	wall_time_sec;
	gtod_long_t	monotonic_time_sec;
	u64		monotonic_time_snsec;
	gtod_long_t	wall_time_coarse_sec;
	gtod_long_t	wall_time_coarse_nsec;
	gtod_long_t	monotonic_time_coarse_sec;
	gtod_long_t	monotonic_time_coarse_nsec;

	int		tz_minuteswest;
	int		tz_dsttime;
};
extern struct vsyscall_gtod_data vsyscall_gtod_data;

static inline unsigned gtod_read_begin(const struct vsyscall_gtod_data *s)
{
	unsigned ret;

repeat:
	ret = ACCESS_ONCE(s->seq);
	if (unlikely(ret & 1)) {
		cpu_relax();
		goto repeat;
	}
	return ret;
}

static inline int gtod_read_retry(const struct vsyscall_gtod_data *s,
					unsigned start)
{
	return unlikely(s->seq != start);
}

static inline void gtod_write_begin(struct vsyscall_gtod_data *s)
{
	++s->seq;
}

static inline void gtod_write_end(struct vsyscall_gtod_data *s)
{
	++s->seq;
}

#endif /* _ASM_X86_VGTOD_H */
