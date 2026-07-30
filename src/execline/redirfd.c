/* ISC license. */

#include <fcntl.h>
#include <errno.h>

#include <skalibs/uint64.h>
#include <skalibs/types.h>
#include <skalibs/djbunix.h>
#include <skalibs/envexec.h>

#include <execline/execline.h>

#define USAGE "redirfd -[ r | w | u | a | x ] [ -N | -n ] [ -b ] [ -f ] [ -E | -e ] fd|var file prog..."
#define dieusage() strerr_dieusage(100, USAGE)

enum redirfd_golb_e
{
  REDIRFD_GOLB_READ = 0x0001,
  REDIRFD_GOLB_WRITE = 0x0002,
  REDIRFD_GOLB_CREAT = 0x0004,
  REDIRFD_GOLB_APPEND = 0x0008,
  REDIRFD_GOLB_TRUNC = 0x0010,
  REDIRFD_GOLB_EXCL = 0x0020,
  REDIRFD_GOLB_NONBLOCK = 0x0040,
  REDIRFD_GOLB_CHANGEMODE = 0x0100,
  REDIRFD_GOLB_OPEN = 0x0200,
  REDIRFD_GOLB_AUTOIMPORT = 0x0400,
} ;

int main (int argc, char const *const *argv)
{
  static gol_bool const rgolb[] =
  {
    { .so = 'r', .lo = 0, .clear = REDIRFD_GOLB_WRITE | REDIRFD_GOLB_APPEND | REDIRFD_GOLB_CREAT | REDIRFD_GOLB_TRUNC | REDIRFD_GOLB_EXCL, .set = REDIRFD_GOLB_READ },
    { .so = 'w', .lo = 0, .clear = REDIRFD_GOLB_READ | REDIRFD_GOLB_APPEND | REDIRFD_GOLB_EXCL, .set = REDIRFD_GOLB_WRITE | REDIRFD_GOLB_CREAT | REDIRFD_GOLB_TRUNC },
    { .so = 'u', .lo = 0, .clear = REDIRFD_GOLB_APPEND | REDIRFD_GOLB_CREAT | REDIRFD_GOLB_TRUNC | REDIRFD_GOLB_EXCL, .set = REDIRFD_GOLB_READ | REDIRFD_GOLB_WRITE },
    { .so = 'a', .lo = 0, .clear = REDIRFD_GOLB_READ | REDIRFD_GOLB_TRUNC | REDIRFD_GOLB_EXCL, .set = REDIRFD_GOLB_WRITE | REDIRFD_GOLB_CREAT | REDIRFD_GOLB_APPEND },
    { .so = 'x', .lo = 0, .clear = REDIRFD_GOLB_READ | REDIRFD_GOLB_APPEND | REDIRFD_GOLB_TRUNC, .set = REDIRFD_GOLB_WRITE | REDIRFD_GOLB_CREAT | REDIRFD_GOLB_EXCL },
    { .so = 'N', .lo = "block", .clear = REDIRFD_GOLB_NONBLOCK, .set = 0 },
    { .so = 'n', .lo = "nonblock", .clear = 0, .set = REDIRFD_GOLB_NONBLOCK },
    { .so = 'b', .lo = "switch-block", .clear = 0, .set = REDIRFD_GOLB_CHANGEMODE },
    { .so = 'f', .lo = "open", .clear = 0, .set = REDIRFD_GOLB_OPEN },
    { .so = 'e', .lo = "no-autoimport", .clear = REDIRFD_GOLB_AUTOIMPORT, .set = 0 },
    { .so = 'E', .lo = "autoimport", .clear = 0, .set = REDIRFD_GOLB_AUTOIMPORT },
    { .so = 0, .lo = "no-read", .clear = REDIRFD_GOLB_READ, .set = 0 },
    { .so = 0, .lo = "read", .clear = 0, .set = REDIRFD_GOLB_READ },
    { .so = 0, .lo = "no-write", .clear = REDIRFD_GOLB_WRITE, .set = 0 },
    { .so = 0, .lo = "write", .clear = 0, .set = REDIRFD_GOLB_WRITE },
    { .so = 0, .lo = "no-create", .clear = REDIRFD_GOLB_CREAT, .set = 0 },
    { .so = 0, .lo = "create", .clear = 0, .set = REDIRFD_GOLB_CREAT },
    { .so = 0, .lo = "no-append", .clear = REDIRFD_GOLB_APPEND, .set = 0 },
    { .so = 0, .lo = "append", .clear = 0, .set = REDIRFD_GOLB_APPEND },
    { .so = 0, .lo = "no-trunc", .clear = REDIRFD_GOLB_TRUNC, .set = 0 },
    { .so = 0, .lo = "trunc", .clear = 0, .set = REDIRFD_GOLB_TRUNC },
    { .so = 0, .lo = "no-excl", .clear = REDIRFD_GOLB_EXCL, .set = 0 },
    { .so = 0, .lo = "excl", .clear = 0, .set = REDIRFD_GOLB_EXCL },
  } ;
  uint64_t wgolb = 0 ;
  unsigned int golc ;
  int fd ;
  unsigned int fdto ;
  unsigned int flags ;
  PROG = "redirfd" ;

  golc = gol_main(argc, argv, rgolb, sizeof(rgolb)/sizeof(gol_bool), 0, 0, &wgolb, 0) ;
  argc -= golc ; argv += golc ;
  if (argc < 3) dieusage() ;
  if (!(wgolb & (REDIRFD_GOLB_READ | REDIRFD_GOLB_WRITE))) dieusage() ;
  if (!(wgolb & REDIRFD_GOLB_OPEN))
    if (!uint0_scan(argv[0], &fdto)) dieusage() ;

  flags =
    (wgolb & REDIRFD_GOLB_WRITE ? wgolb & REDIRFD_GOLB_READ ? O_RDWR : O_WRONLY : O_RDONLY) |
    (wgolb & REDIRFD_GOLB_CREAT ? O_CREAT : 0) |
    (wgolb & REDIRFD_GOLB_APPEND ? O_APPEND : 0) |
    (wgolb & REDIRFD_GOLB_TRUNC ? O_TRUNC : 0) |
    (wgolb & REDIRFD_GOLB_EXCL ? O_EXCL : 0) |
    (wgolb & REDIRFD_GOLB_NONBLOCK ? O_NONBLOCK : 0) ;

  fd = open3(argv[1], flags, 0666) ;
  if (fd == -1 && wgolb & REDIRFD_GOLB_WRITE && errno == ENXIO)
  {
    int fdr = open_read(argv[1]) ;
    if (fdr == -1) strerr_diefu2sys(111, "open_read ", argv[1]) ;
    fd = open3(argv[1], flags, 0666) ;
    fd_close(fdr) ;
  }
  if (fd == -1) strerr_diefu2sys(111, "open ", argv[1]) ;

  if (wgolb & REDIRFD_GOLB_CHANGEMODE)
    if ((wgolb & REDIRFD_GOLB_NONBLOCK ? ndelay_off(fd) : ndelay_on(fd)) == -1)
      strerr_diefu1sys(111, "change blocking mode") ;
  if (wgolb & REDIRFD_GOLB_OPEN)
  {
    char fmt[UINT_FMT] ;
    fmt[uint_fmt(fmt, fd)] = 0 ;
    el_modif_and_exec(argv+2, argv[0], fmt, !!(wgolb & REDIRFD_GOLB_AUTOIMPORT)) ;
  }
  else
  {
    if (fd_move(fdto, fd) == -1) strerr_diefu1sys(111, "fd_move") ;
    xexec(argv+2) ;
  }
}
