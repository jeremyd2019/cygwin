#include "test.h"
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int handle_child (char *arg)
{
  pid_t pgid = atoi (arg);
  if (!pgid)
    pgid = getpid ();
  testAssertMsg (getpgid (0) == pgid,
		 "Process group %d != expected %d", getpgid (0), pgid);
  return 0;
}

int main (int argc, char **argv)
{
  posix_spawnattr_t sa;
  pid_t pid;
  int status;
  char buf[12];
  char *childargv[] = {"pgroup", "--child", buf, NULL};

  /* unbuffer stdout */
  setvbuf(stdout, NULL, _IONBF, 0);

  if (argc == 3 && !strcmp (argv[1], "--child"))
    return handle_child (argv[2]);

  /* ensure pgroup inherited by default */
  sprintf (buf, "%d", getpgid (0));
  errCode (posix_spawn (&pid, MYSELF, NULL, NULL, childargv, environ));
  negError (waitpid (pid, &status, 0));
  exitStatus (status, 0);

  /* ensure setpgroup 0 sets pgroup to pid */
  errCode (posix_spawnattr_init (&sa));
  errCode (posix_spawnattr_setpgroup (&sa, 0));
  errCode (posix_spawnattr_setflags (&sa, POSIX_SPAWN_SETPGROUP));
  strcpy (buf, "0");
  errCode (posix_spawn (&pid, MYSELF, NULL, &sa, childargv, environ));
  negError (waitpid (pid, &status, 0));
  exitStatus (status, 0);
  errCode (posix_spawnattr_destroy (&sa));

  /* ensure setpgroup to ppid works (assume ppid is already a pgroup) */
  errCode (posix_spawnattr_init (&sa));
  errCode (posix_spawnattr_setpgroup (&sa, getppid ()));
  errCode (posix_spawnattr_setflags (&sa, POSIX_SPAWN_SETPGROUP));
  sprintf (buf, "%d", getppid ());
  errCode (posix_spawn (&pid, MYSELF, NULL, &sa, childargv, environ));
  negError (waitpid (pid, &status, 0));
  exitStatus (status, 0);
  errCode (posix_spawnattr_destroy (&sa));

  /* ensure setpgroup to arbitrary value fails with EPERM */
#ifndef __CYGWIN__
  /* The value of the pgid argument is valid but does not match the process ID
     of the process indicated by the pid argument and there is no process with
     a process group ID that matches the value of the pgid argument in the same
     session as the calling process. */
  errCode (posix_spawnattr_init (&sa));
  errCode (posix_spawnattr_setpgroup (&sa, 42));
  errCode (posix_spawnattr_setflags (&sa, POSIX_SPAWN_SETPGROUP));
  strcpy (buf, "42");
  errCodeExpected (EPERM,
		   posix_spawn (&pid, MYSELF, NULL, &sa, childargv, environ));
  errCode (posix_spawnattr_destroy (&sa));
#endif

  /* ensure setpgroup to negative number fails with EINVAL */
  errCode (posix_spawnattr_init (&sa));
  errCode (posix_spawnattr_setpgroup (&sa, -2));
  errCode (posix_spawnattr_setflags (&sa, POSIX_SPAWN_SETPGROUP));
  strcpy (buf, "-2");
  errCodeExpected (EINVAL,
		   posix_spawn (&pid, MYSELF, NULL, &sa, childargv, environ));
  errCode (posix_spawnattr_destroy (&sa));

  return 0;
}
