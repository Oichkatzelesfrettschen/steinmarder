#include "gbuffer.h"

static SM_GBuffer g_gb = {0};

void sm_gbuffer_set_targets(SM_GBuffer gb) {
    g_gb = gb;
}

// render.c accesses this via extern
SM_GBuffer sm_gbuffer_get_targets(void);
SM_GBuffer sm_gbuffer_get_targets(void) { return g_gb; }
