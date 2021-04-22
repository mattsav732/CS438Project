
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ncurses.h>

//Main functions
int fq_cd(char **args);
int fq_help(char **args);
int fq_exit(char **args);

//Bullitin commands
char *builtin_str[] = {
  "cd - change directory, 'cd name' or 'cd ..' to move up one level",
  "help",
  "exit - close the shell",
  "ls - list contents of current directory",
  "touch/mkdir - make a new file/directory, 'touch name.txt' or 'mkdir name'",
  "top - list all current processes",
  "rm/rmdir - remove a file/directory, 'rm name'"
};

int (*builtin_func[]) (char **) = {
  &fq_cd,
  &fq_help,
  &fq_exit
};

int fq_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

int fq_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "fq: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("fq");
    }
  }
  return 1;
}

int fq_help(char **args)
{
  int i;
  
  printf("           `-:+osyhhhhyyoo/:-.``    -::- \n" 
 "`  `  ``:+syyhdddmmddddddddhhhyo/.` `` \n"  
"`  `  ./ssyhddmmmmmmmmmmmmmdddddhhy/` \n"  
 "    :ssyhdddmmmmdhhhhdddmmmmddddhy.    \n"
 "`  `syhhdddhso+++ooossssyyhdddhhs.`  ` \n"
"`  ` :yhhhyo+shy/..----:/+ooohhyo.  `  \n" 
 "     ./syoymMN:.:+ss/::-::/oyo-       \n" 
 "`  `  ``.shmmy.-/ossy//+syso:  `  `  ` \n"
"`  `  `  +dyyd:.--://-.:oysoys`  `  `  \n" 
 "       /ddhdh---:/+++-:oooosd`       \n"  
 "`  ` .ymNNNdo:-:++/:/+ssso+sm` `  `  ` \n"
"`  ``omNNNNNm+//+o::/+ossoo+mm-  `  `  \n "
 "   ommmmmmmdo++++/:/+ssoosoNNy`    \n"    
 "`  oNNNNNNNNhooo++++oosyyyyNNNo`  `  ` \n"
"`  `.hNMMMMMMdyysssosyhyssyhdNNm``  `   \n"
 "     `:yNNMmssyysysyysossyyymm/     \n"   
 "`  ` `.-oo++/:::/+ossosyyyyhy` ````  ` \n"
" _______                                     _ \n"
"(_______)                                   | | \n"
" _____ ____  ____ ____ _   _  ____  ____  _ | |\n"
"|  ___) _  |/ ___) _  | | | |/ _  |/ _  |/ || |\n"
"| |  ( ( | | |  | | | | |_| ( ( | ( ( | ( (_| |\n"
"|_|  \\_||_|_|    \\_|| |\\____|\\_||_|\\_||_|\\____|\n"
 "                    |_|                        \n "                     
 );

  printf("The following commands can be called:\n");

  for (i = 0; i < fq_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Exit the shell by typing 'exit' or ctrl+c\n");

  return 1;
}

int fq_exit(char **args)
{
  return 0;
}


int fq_launch(char **args)
{
  
  pid_t pid;
  int status;

  pid = fork();
  if (pid == 0) {
    // Child process
    if (execvp(args[0], args) == -1) {
      perror("fq");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // Error forking
    perror("fq");
  } else {
    // Parent process
    do {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

int fq_execute(char **args)
{
  int i;

  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < fq_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return fq_launch(args);
}

char *fq_read_line(void)
{
#ifdef FQ_USE_STD_GETLINE
  char *line = NULL;
  ssize_t bufsize = 0; // have getline allocate a buffer for us
  if (getline(&line, &bufsize, stdin) == -1) {
    if (feof(stdin)) {
      exit(EXIT_SUCCESS);  // We received an EOF
    } else  {
      perror("fq: getline\n");
      exit(EXIT_FAILURE);
    }
  }
  return line;
#else
#define FQ_RL_BUFSIZE 1024
  int bufsize = FQ_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "fq: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    // Read a character
    c = getchar();

    if (c == EOF) {
      exit(EXIT_SUCCESS);
    } else if (c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;

    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize) {
      bufsize += FQ_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "fq: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
#endif
}

#define FQ_TOK_BUFSIZE 64
#define FQ_TOK_DELIM " \t\r\n\a"

char **fq_split_line(char *line)
{
  int bufsize = FQ_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token, **tokens_backup;

  if (!tokens) {
    fprintf(stderr, "fq: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, FQ_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += FQ_TOK_BUFSIZE;
      tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
		free(tokens_backup);
        fprintf(stderr, "fq: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, FQ_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

void fq_loop(void)
{
  char *line;
  char **args;
  int status;
 
   
  do {
    printf("$ ");
    line = fq_read_line();
    args = fq_split_line(line);
    status = fq_execute(args);
    

    free(line);
    free(args);
  } while (status);
}

int main(int argc, char **argv)
{
  // Load config files, if any.
     
  // Run command loop.
  fq_loop();

  // Perform any shutdown/cleanup.

  return EXIT_SUCCESS;
}
