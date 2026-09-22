/* Copyright (c) 2026 Mark Harris

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#if defined HAVE_ARC4RANDOM
# include <stdlib.h>
#elif defined _WIN32
# define _CRT_RAND_S    /* for rand_s */
# include <stdlib.h>
# include <windows.h>
# if !defined HAVE_RAND_S
#  if defined __MINGW64_VERSION_MAJOR
#   define HAVE_RAND_S 1
#  elif defined _MSC_VER
#   if _MSC_VER >= 1400
#    define HAVE_RAND_S 1
#   endif
#  endif
# endif
# if !defined HAVE_RAND_S
#  include <wincrypt.h>
# endif
# include <process.h>
# define getpid _getpid
#else
# include <errno.h>
# include <fcntl.h>
# include <time.h>
# include <unistd.h>
# if defined HAVE_SYS_RANDOM_H
#  if defined HAVE_GETRANDOM || defined HAVE_GETENTROPY
#   include <sys/random.h>
#  endif
# endif
# if !(defined HAVE_CLOCK_GETTIME && defined CLOCK_REALTIME)
#  include <sys/time.h>
# endif
#endif

#if !defined HAVE_ARC4RANDOM
/* Multipliers to spread low quality entropy to the higher bits. */
# define MUL_TIME_HIGH  3234390213U
# define MUL_TIME       3472989573U
# define MUL_CLOCK_HIGH 3275524069U
# define MUL_CLOCK      3630618349U
# define MUL_PID        2920575093U
# define MUL_SP32       3417857517U
# define MUL_SP64       12620523164723586485ULL
# define MUL_ITER       3120012813U

# if defined CLOCK_THREAD_CPUTIME_ID
#  define ALT_CLOCK_ID  CLOCK_THREAD_CPUTIME_ID
# elif defined CLOCK_PROCESS_CPUTIME_ID
#  define ALT_CLOCK_ID  CLOCK_PROCESS_CPUTIME_ID
# elif defined CLOCK_MONOTONIC
#  define ALT_CLOCK_ID  CLOCK_MONOTONIC
# endif

static unsigned int
entropy_from_time(void)
{
  /* Obtain entropy from the system time. */
# if defined _WIN32
  FILETIME now;
  GetSystemTimeAsFileTime(&now);
  return now.dwHighDateTime * MUL_TIME_HIGH + now.dwLowDateTime;
# elif defined HAVE_CLOCK_GETTIME && defined CLOCK_REALTIME
  struct timespec now;
  if (clock_gettime(CLOCK_REALTIME, &now) != 0) return 0;
  return (unsigned int)now.tv_sec * MUL_TIME_HIGH + (unsigned int)now.tv_nsec;
# else
  struct timeval now;
  if (gettimeofday(&now, NULL) != 0) return 0;
  return (unsigned int)now.tv_sec * MUL_TIME_HIGH + (unsigned int)now.tv_usec;
# endif
}

static unsigned int
entropy_from_alt_clock(void)
{
  /* Obtain entropy from an alternate clock source that may have better
     resolution and provide additional entropy. */
# if defined _WIN32
  LARGE_INTEGER ticks;
  if (!QueryPerformanceCounter(&ticks)) return 0;
  return (unsigned int)ticks.u.HighPart * MUL_CLOCK_HIGH + ticks.u.LowPart;
# elif defined HAVE_CLOCK_GETTIME && defined ALT_CLOCK_ID
  struct timespec ts;
  if (clock_gettime(ALT_CLOCK_ID, &ts) != 0) return (unsigned int)clock();
  return (unsigned int)ts.tv_sec * MUL_CLOCK_HIGH + (unsigned int)ts.tv_nsec;
# else
  return (unsigned int)clock();
# endif
}
#endif  /* !defined HAVE_ARC4RANDOM */

unsigned int
get_random_uint32(void)
{
#if defined HAVE_ARC4RANDOM
  /* glibc 2.36+, BSD, macOS, iOS, Android, Solaris, illumos, ... */
  return arc4random();

#else
  /* Use whatever entropy is available from the operating system.  It is
     expected that this function will be called infrequently within a
     particular process, so we avoid persistent state (such as a PRNG) and
     the need to access it in a thread-safe manner.  If this function is
     called frequently then performance could be improved by using the
     available entropy to seed a PRNG on the first call and then using
     that to generate the returned values. */
  unsigned int val;

# if defined _WIN32
#  if defined HAVE_RAND_S
  /* Microsoft Visual Studio 2005 or later, MinGW-w64 */
  if (rand_s(&val) == 0) return val;
#  else
  /* old Windows, MinGW32 */
  HCRYPTPROV hprovider = 0;
  if (CryptAcquireContext(&hprovider, NULL, NULL, PROV_RSA_FULL,
        CRYPT_VERIFYCONTEXT | CRYPT_SILENT)) {
    BOOL ok = CryptGenRandom(hprovider, sizeof(val), (BYTE *)&val);
    CryptReleaseContext(hprovider, 0);
    if (ok) return val;
  }
#  endif
# else  /* !defined _WIN32 */
  size_t fill = 0;

  /* use getentropy() or getrandom() if available */
#  if defined HAVE_GETENTROPY
  /* POSIX.1-2024, glibc 2.25+, musl 1.1.20+, BSD, macOS, emscripten, ... */
  for (;;) {
    int ret = getentropy(&val, sizeof(val));
    if (ret == 0) return val;
    if (ret != -1 || errno != EINTR) break;
  }
#  elif defined HAVE_GETRANDOM
  /* glibc 2.25+, musl 1.1.20+, Redox */
  for (;;) {
    ssize_t inc = getrandom((char *)&val + fill, sizeof(val) - fill, 0);
    if (inc > 0) {
      fill += inc;
      if (fill >= sizeof(val)) return val;
    } else if (inc != -1 || errno != EINTR) break;
  }
#  endif

  /* try reading from /dev/urandom */
  {
    int fd = open("/dev/urandom", O_RDONLY
#  if defined O_CLOEXEC
      | O_CLOEXEC
#  endif
      );
    if (fd >= 0) {
      for (;;) {
        ssize_t inc = read(fd, (char *)&val + fill, sizeof(val) - fill);
        if (inc > 0) {
          fill += inc;
          if (fill >= sizeof(val)) break;
        } else if (inc != -1 || errno != EINTR) break;
      }
      close(fd);
      if (fill >= sizeof(val)) return val;
    }
  }
# endif  /* !defined _WIN32 */

  /* High quality entropy is not available (we are not running on a serious
     platform); do what we can.  Fall back to a mix of low quality entropy
     from clocks, the process id, and the stack pointer, to try and at least
     avoid the same values from being returned in common situations,
     including when multiple threads are started simultaneously.  Spread the
     available entropy around so that it isn't concentrated in specific bits.
     Don't use this for cryptographic purposes. */
  {
    unsigned int etime = entropy_from_time();
    unsigned int eclock = entropy_from_alt_clock();
    unsigned int mix = etime * MUL_TIME;
    mix = ((mix << 12) ^ (mix >> 20)) * MUL_TIME;
    mix ^= eclock * MUL_CLOCK;
    mix ^= (unsigned int)getpid() * MUL_PID;
    mix ^= sizeof(size_t) > sizeof(unsigned int)
      ? (unsigned int)(((size_t)&val * MUL_SP64) >> 32)
      : ((unsigned int)(size_t)&val >> 2) * MUL_SP32;

    /* Ensure that the clock-based entropy changes, to avoid returning the
       same value on the next call.  If the resolution is so coarse that
       the same entropy is produced multiple times, the number of iterations
       required before the entropy changes will act as additional bits of
       resolution and another source of entropy. */
    while (entropy_from_time() == etime) {
      mix += MUL_ITER;
      if (entropy_from_alt_clock() != eclock) break;
      mix += MUL_ITER;
    }
    return mix;
  }
#endif  /* !defined HAVE_ARC4RANDOM */
}
