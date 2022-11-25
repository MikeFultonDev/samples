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
  pid_t pid;
  char* line;
  int printed;
};

static int add_entry(char* line, struct ProcessEntry** entryp)
{
  int i;

  size_t len = strlen(line);
  char* procline = malloc(len+1);
  struct ProcessEntry* entry = malloc(sizeof(struct ProcessEntry));
  if (!procline || !entry) {
    return 16;
  }

  memcpy(procline, line, len+1);
  entry->line = procline;
  entry->next_entry = *entryp;
  entry->proc_parent = NULL; 
  entry->pid = 0;
  entry->printed = 0;

  *entryp = entry;

  return 0;
}

#define PID_START_COL 9
#define PPID_START_COL 18
static pid_t getentrypid(struct ProcessEntry* entry, int startcol) 
{
  int val = 0;
  int innum=0;
  int col;

  if (entry->pid) {
    return entry->pid;
  }

  for (col=startcol; ; ++col) {
    int c = entry->line[col];
    switch (c) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9': {
        innum = 1;
        val = val*10 + (c-'0');
        break;
      }
      case ' ': {
        if (innum) {
          entry->pid = (pid_t) val;
          return entry->pid;
        }
        break;
      }
      default: {
        fprintf(stderr, "Unexpected error processing ps line %s\n", entry->line);
        return -1;
      }
    }
  }
  return -1; /* not reachable */
}

static struct ProcessEntry* parent(struct ProcessEntry* entry, pid_t pid)
{
  while (entry) {
    pid_t mypid = getentrypid(entry, PID_START_COL);
    if (mypid == pid) {
      return entry;
    }
    entry = entry->next_entry;
  }
  return NULL;
}

static int update_parents(struct ProcessEntry* head) 
{
  struct ProcessEntry* entry = head;
  while (entry) {
    pid_t ppid = getentrypid(entry, PPID_START_COL);
    struct ProcessEntry* proc_parent = parent(head, ppid);
    if (proc_parent) {
      entry->proc_parent = proc_parent;
    }
    entry = entry->next_entry;
  }

  return 0;
}

int main() 
{
  char line[MAX_CMD_LEN+1];
	int childout[2]; 
  int rc, i;
  int c;
  struct ProcessEntry* head = NULL;
  struct ProcessEntry* entry;
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
         if (add_entry(line, &head)) {
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

    update_parents(head);
    entry = head;
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

