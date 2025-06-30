#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int handle_child (char *arg)
{
  pid_t pgid = getpgid (0);
  pid_t expectedpgid = atoi (arg);
  if (!expectedpgid)
    expectedpgid = getpid ();
  if (pgid != expectedpgid)
    {
      fprintf (stderr, "Process group %d != expected %d\n", pgid, expectedpgid);
      return 1;
    }
  return 0;
}

int main (int argc, char **argv)
{
  int status;
  char buf[12];
  const char * const childargv[] = {"pgroup", "--child", buf, NULL};

  /* unbuffer stdout */
  setvbuf(stdout, NULL, _IONBF, 0);

  if (argc == 3 && !strcmp (argv[1], "--child"))
    return handle_child (argv[2]);

  /* ensure pgroup inherited by default */
  sprintf (buf, "%d", getpgid (0));
  status = spawnv (_P_WAIT, "/proc/self/exe", childargv);
  if (status < 0)
    {
      perror ("spawnv");
      return 1;
    }
  else if (WIFSIGNALED (status))
    {
      fprintf (stderr, "child termintated with signal %d\n", WTERMSIG (status));
      return 1;
    }
  else if (WIFEXITED (status) && WEXITSTATUS (status) != 0)
    {
      fprintf (stderr, "child exited with code %d\n", WEXITSTATUS (status));
      return 1;
    }
  else if (!WIFEXITED (status))
    {
      fprintf (stderr, "child terminated with status %x\n", status);
      return 1;
    }

  return 0;
}
