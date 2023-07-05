#ifndef __ZOSSYS__
  #define __ZOSSYS__ 1

  /*
   * runcmd: run a program via fork/exec and return a pointer to malloc'ed storage
   *         for the stdout and stderr stream. 
   *         The output is NULL terminated, but the length of both streams is also returned
   *         in case there are embedded NULLs
   * Returns:
   *         the return code of the program, or -1 if a failure occurred.
   * Parameters:
   *         cmd: the command to run, using an absolute path, e.g. /bin/tsocmd. 
   *         argv: standard argv list with a NULL pointer as the last parameter
   *         out:  this parameter will be set on return to malloc'ed storage to the stdout of the program
   *               It is your responsibility to free the storage after use.
   *               The output is NULL terminated.
   *         outsize: The length of the output. 
   *         err:  this parameter will be set on return to malloc'ed storage to the stderr of the program
   *               It is your responsibility to free the storage after use.
   *               The output is NULL terminated.
   *         errsize: The length of the output. 
   */
  int runcmd(const char* cmd, char* argv[], char** out, size_t* outsize, char** err, size_t* errsize);

#endif
