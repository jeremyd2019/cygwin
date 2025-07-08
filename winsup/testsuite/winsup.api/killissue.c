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
  pid_t pid, childpid;

  if (argc == 3 && !strcmp (argv[1], "--child"))
    {
      if (kill (atoi (argv[2]), SIGUSR1) == -1)
	{
	  perror ("kill after exec");
	  return 1;
	}

      usleep (100000);
      return 0;
    }

  pid = getpid ();

  sigemptyset (&set);
  sigaddset (&set, SIGUSR1);
  errno = 0;
  sigprocmask (SIG_BLOCK, &set, &oldset);

#if 0
  switch ((childpid = fork ()))
  {
  case -1:
    perror ("fork");
    ret |= 1;
    break;
  case 0:
    /* child */
    if (kill (pid, SIGUSR1) == -1)
      {
	perror ("kill without exec");
	return 1;
      }
    return 0;
  default:
    /* parent */
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
#endif

  switch ((childpid = fork ()))
  {
  case -1:
    perror ("fork");
    ret |= 1;
    break;
  case 0:
    /* child */
    {
      char buf[16];
      char *childargs[] = {"/proc/self/exe", "--child", buf, NULL};
      sprintf (buf, "%d", pid);
      execv (childargs[0], childargs);
      perror ("execv");
    }
    return 1;
  default:
    /* parent */
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

  return ret;
}
