#define LV2_CONTEXTS_SET_OUTPUT_WRITTEN(flags, index) \
	(flags)[(index) / 8] |= 1 << ((index) % 8)

#define LV2_CONTEXTS_UNSET_OUTPUT_WRITTEN(flags, index) \
	(flags)[(index) / 8] &= ~(1 << ((index) % 8))

