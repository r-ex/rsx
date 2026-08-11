#pragma once

// RSX's versioning system is genuinely horrible...
#define VERSION_MAJOR 2
#define VERSION_MINOR 2
#define VERSION_PATCH 1
#define VERSION_REVIS "" // If there is no revision letter, leave this as ""

#define STR2(x) #x
#define STR(x) STR2(x)
#define STRtoCHAR(x) (*x)

#define VERSION_STRING STR(VERSION_MAJOR) "." STR(VERSION_MINOR) "." STR(VERSION_PATCH) VERSION_REVIS
