#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_CMD_LEN 1023
#define MAX_CMD_LEN_STR "1023"

struct ProcessEntry;
struct ProcessEntry {
  struct ProcessEntry* proc_parent;
  struct ProcessEntry* next_entry;
  char* line;
};

static int add_entries(char* line, struct ProcessEntry** entryp)
{
  int i;
  printf("process line %s\n", line);

  size_t len = strlen(line);
  char* procline = malloc(len+1);
  struct ProcessEntry* entry = malloc(sizeof(struct ProcessEntry));
  if (!procline || !entry) {
    return 16;
  }

  memcpy(procline, line, len+1);
  entry->line = procline;
  entry->next_entry = *entryp;

  *entryp = entry;

  return 0;
}

int main() 
{
  char line[MAX_CMD_LEN+1];
	int childout[2]; 
  int rc, i;
  int c;
  struct ProcessEntry* entry = NULL;
  int count=0;

	pid_t pid;

	if (pipe(childout) == -1) {
		fprintf(stderr, "Unable to create pipe\n");
		return 8;
	}

	pid = fork();

	if (pid < 0) {
		fprintf(stderr, "Unable to create fork\n");
		return 8;
	}

	if (pid > 0) {
    /*
     * Parent process
     */
		close(childout[1]); /* Close writing end of pipe */

    dup2(childout[0], STDIN_FILENO);

    while ((c = getchar()) > 0) {
      if (c == '\n') {
         line[i] = '\0';
         if (add_entries(line, &entry)) {
           fprintf(stderr, "out of memory\n");
           return 16;
         } else {
           i = 0;
         }
      } else {
        line[i++] = c;
      }
    }
    close(childout[0]);

    /*
     * wait for child to complete
     */
		wait(NULL);

    while (entry) {
      printf("%d: %s\n", count++, entry->line);
      entry = entry->next_entry;
    }
	} else {

    /*
     * Child process
     */
    char* const psargv[] = { 
      "/bin/sh", "-c", "COLUMNS=" MAX_CMD_LEN_STR " /bin/ps -ef | /bin/tail +2", NULL
    };
		close(childout[0]); /* Close reading end of pipe */
    dup2(childout[1], STDOUT_FILENO);

    rc = execv(psargv[0], psargv);

		close(childout[1]); /* Close writing end of pipe */

		exit(rc);
	}
}

