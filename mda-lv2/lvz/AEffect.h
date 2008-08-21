#ifndef __LVZ_AEFFECT_H
#define __LVZ_AEFFECT_H

class AudioEffect;

enum AEffectFlags {
	effFlagsHasEditor
};

struct AEffect {
	int flags;
};

#endif // __LVZ_AFFECT_H
