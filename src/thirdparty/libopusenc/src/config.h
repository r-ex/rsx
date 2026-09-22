/* Build configuration for the libopusenc sources compiled into rsx.
   libopusenc is normally configured by autotools/meson; this provides the
   equivalent defines for the MSVC build and is included via HAVE_CONFIG_H. */
#pragma once

#define PACKAGE_NAME "libopusenc"
#define PACKAGE_VERSION "0.3"

/* The bundled Speex resampler is built standalone, without the speexdsp headers */
#define OUTSIDE_SPEEX 1
#define RANDOM_PREFIX libopusenc
