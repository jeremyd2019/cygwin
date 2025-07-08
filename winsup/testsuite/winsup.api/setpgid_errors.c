#define _GNU_SOURCE
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main (int argc, char **argv)
{
  sigset_t set, oldset;
  int ret = 0, status;
  pid_t pid, ppid, pgid, childpid;

  if (argc == 3 && !strcmp (argv[1], "--child"))
    {
      if (kill (atoi (argv[2]), SIGUSR1) == -1)
	{
	  perror ("kill");
	  return 1;
	}

      usleep (100000);
      return 0;
    }

  if ((childpid = fork ()) == -1)
    {
      perror ("fork");
      return 1;
    }
  else if (childpid != 0)
    {
      if (waitpid (childpid, &status, 0) == -1)
	{
	  perror ("waitpid");
	  return 1;
	}
      else if (WIFEXITED (status))
	return WEXITSTATUS (status);
      else if (WIFSIGNALED (status))
	return 128 + WTERMSIG (status);
      else
	return 127;
    }

  pid = getpid ();
  ppid = getppid ();
  pgid = getpgid (0);

  /* tests of error codes from
     https://pubs.opengroup.org/onlinepubs/9799919799/functions/setpgid.html */

  errno = 0;
  /* pgid is less than 0, fail with EINVAL */
  /* this one works on Cygwin */
  if (setpgid (pid, -4) != -1 || errno != EINVAL)
  {
    perror ("setpgid (pid, -4)");
    ret |= 1;
  }

  errno = 0;
  /* attempt to set pgid of process other than self or child, fail with ESRCH */
  if (setpgid (ppid, pid) != -1 || errno != ESRCH)
  {
    perror ("setpgid (ppid, pid)");
    ret |= 1;
  }

  errno = 0;
  /* attempt to set pgid of process to non-existing pgid other than pid, EPERM */
  if (setpgid (pid, 42) != -1 || errno != EPERM)
  {
    perror ("setpgid (pid, 42)");
    ret |= 1;
  }

  sigemptyset (&set);
  sigaddset (&set, SIGUSR1);
  errno = 0;
  /* attempt to set pgid of child which is not in the same session, EPERM */
  sigprocmask (SIG_BLOCK, &set, &oldset);

  switch ((childpid = fork ()))
  {
  case -1:
    perror ("fork");
    ret |= 1;
    break;
  case 0:
    /* child */
    if (setsid () == -1)
      {
	perror ("setsid");
	return 1;
      }
    if (kill (pid, SIGUSR1) == -1)
      {
	perror ("kill");
	return 1;
      }
    usleep (100000);
    return 0;
    /* parent */
  default:
    while (1)
      {
	int sig;
	if (sigwait (&set, &sig))
	  {
	    perror ("sigwait");
	    return 2;
	  }
	if (sig == SIGUSR1)
	  break;
      }
    errno = 0;
    if (setpgid (childpid, pgid) != -1 || errno != EPERM)
      {
	perror ("different session setpgid (childpid, pgid)");
	ret |= 1;
      }
    if (waitpid (childpid, &status, 0) == -1)
      {
	perror ("waitpid");
	ret |= 1;
      }
    else if (WIFEXITED (status))
      ret |= WEXITSTATUS (status);
    else if (WIFSIGNALED (status))
      ret |= WTERMSIG (status);
  }

  /* attempt to set pgid of child which has execed, EACCES */
  switch ((childpid = fork ()))
  {
  case -1:
    perror ("fork");
    ret |= 1;
    break;
  case 0:
    /* child */
    {
      char buf[64];
      char *childargs[] = {"/proc/self/exe", "--child", buf, NULL};
      sprintf (buf, "%d", pid);
      execv (childargs[0], childargs);
      perror ("execv");
    }
    return 1;

    /* parent */
  default:
    while (1)
      {
	int sig;
	if (sigwait (&set, &sig))
	  {
	    perror ("sigwait");
	    return 2;
	  }
	if (sig == SIGUSR1)
	  break;
      }
    errno = 0;
    if (setpgid (childpid, pgid) != -1 || errno != EACCES)
      {
	perror ("post-exec setpgid (childpid, pgid)");
	ret |= 1;
      }
    if (waitpid (childpid, &status, 0) == -1)
      {
	perror ("waitpid");
	ret |= 1;
      }
    else if (WIFEXITED (status))
      ret |= WEXITSTATUS (status);
    else if (WIFSIGNALED (status))
      ret |= WTERMSIG (status);
  }
  sigprocmask (SIG_SETMASK, &oldset, NULL);

  /* attempt to set pgid of session leader, EPERM */
  errno = 0;
  switch ((childpid = fork ()))
  {
  case -1:
    perror ("fork");
    ret |= 1;
    break;
  case 0:
    /* child */
    if (setsid () == -1)
      {
	perror ("setsid");
	return 1;
      }
    errno = 0;
    if (setpgid (0, pgid) != -1 || errno != EPERM)
      {
	perror ("sessionleader setpgid (0, pgid)");
	return 1;
      }
    return 0;
    /* parent */
  default:
    if (waitpid (childpid, &status, 0) == -1)
      {
	perror ("waitpid");
	ret |= 1;
      }
    else if (WIFEXITED (status))
      ret |= WEXITSTATUS (status);
    else if (WIFSIGNALED (status))
      ret |= 1;
  }

  return ret;
}
