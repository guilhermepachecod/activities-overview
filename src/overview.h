#pragma once

#include <glib.h>

typedef struct _AoOverview AoOverview;

AoOverview *ao_overview_new  (void);
void        ao_overview_free (AoOverview *overview);

void ao_overview_show   (AoOverview *overview);
void ao_overview_hide   (AoOverview *overview);
void ao_overview_toggle (AoOverview *overview);

gboolean ao_overview_is_visible (AoOverview *overview);
