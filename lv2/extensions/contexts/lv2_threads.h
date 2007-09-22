#define LV2_THREADS_SET_OUTPUT_WRITTEN(flags, index) \
	(flags)[(index) / 8] |= 1 << ((index) % 8)

#define LV2_THREADS_UNSET_OUTPUT_WRITTEN(flags, index) \
	(flags)[(index) / 8] &= ~(1 << ((index) % 8))

