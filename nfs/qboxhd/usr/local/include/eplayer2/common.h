#ifndef COMMON_H_
#define COMMON_H_

#include "container.h"
#include "output.h"
#include "manager.h"
#include "playback.h"
#include <pthread.h>

///ML --- start
#include <stdlib.h>
#include <math.h>
#include <string.h>
///ML --- end

typedef struct Context_s {
	PlaybackHandler_t	* playback;
	ContainerHandler_t	* container;
	OutputHandler_t		* output;
	ManagerHandler_t	* manager;
} Context_t;


#endif
