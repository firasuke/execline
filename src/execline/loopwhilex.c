/* ISC license. */

#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

#include <skalibs/gol.h>
#include <skalibs/uint64.h>
#include <skalibs/types.h>
#include <skalibs/strerr.h>
#include <skalibs/djbunix.h>

#include <execline/execline.h>

#define USAGE "loopwhilex [ -o code,code,... | -x code,code,... ] prog..."
#define dieusage() strerr_dieusage(100, USAGE)

enum lw_gola_e
{
  LW_GOLA_OK,
  LW_GOLA_EXCLUDE,
  LW_GOLA_N
} ;

static int lw_uint8_cmp (void const *a, void const *b)
{
  uint8_t aa = *(uint8_t const *)a ;
  uint8_t bb = *(uint8_t const *)b ;
  return aa < bb ? -1 : aa > bb ;
}

static size_t lw_scanlist (char const *s, uint8_t *tab)
{
  size_t n = 0 ;
  while (*s)
  {
    uint64_t u ;
    size_t l = uint64_scan(s, &u) ;
    if (!l) break ;
    if (u > 256) strerr_dief(100, "exit codes must be 0 to 255") ;
    if (n >= 255) strerr_dief(100, "too many exit codes") ;
    tab[n++] = u ;
    s += l ;
    while (*s == ',') s++ ;
  }
  if (*s) dieusage() ;
  qsort(tab, n, 1, &lw_uint8_cmp) ;
  return n ;
}

int main (int argc, char const *const *argv, char const *const *envp)
{
  static gol_arg const rgola[] =
  {
    { .so = 'o', .lo = "ok-codes", .i = LW_GOLA_OK },
    { .so = 'x', .lo = "exclude-codes", .i = LW_GOLA_EXCLUDE },
  } ;
  int rev = 0 ;
  char const *wgola[LW_GOLA_N] = { 0 } ;
  size_t n = 0 ;
  uint8_t codes[256] = { 0 } ;
  PROG = "loopwhilex" ;

  {
    unsigned int golc = gol_main(argc, argv, 0, 0, rgola, sizeof(rgola)/sizeof(gol_arg), 0, wgola) ;
    argc -= golc ; argv += golc ;
  }
  if (!argc) dieusage() ;
  if (wgola[LW_GOLA_OK]) n = lw_scanlist(wgola[LW_GOLA_OK], codes) ;
  else if (wgola[LW_GOLA_EXCLUDE])
  {
    n = lw_scanlist(wgola[LW_GOLA_EXCLUDE], codes) ;
    rev = 1 ;
  }
  else n = 1 ;

  for (;;)
  {
    int wstat ;
    pid_t pid = el_spawn0(argv[0], argv, envp) ;
    if (!pid)
    {
      if (errno == ENOENT && argv[0][0] == ' ')
        strerr_dief(111, "spawn ", argv[0], ": name begins with a space, are you trying to spawn a block as your loop body?") ;
      else strerr_diefusys(111, "spawn ", argv[0]) ;
    }
    if (wait_pid(pid, &wstat) < 0) strerr_diefusys(111, "wait_pid") ;
    codes[255] = wait_estatus(wstat) ;
    if (rev != !bsearch(codes + 255, codes, n, 1, &lw_uint8_cmp)) break ;
  }
  return codes[255] ;
}
