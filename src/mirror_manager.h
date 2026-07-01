#ifndef MIRROR_MANAGER_H
#define MIRROR_MANAGER_H

#include <glib.h>

typedef struct {
    const char *name;
    const char *url;
    const char *region;
    const char *location;
    int tier;
} MirrorInfo;

GList *get_all_mirrors(void);

#endif
