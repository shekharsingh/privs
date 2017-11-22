#include "privs.h"

#define EXIT_SUCCESS	1
#define EXIT_FAILURE	-1

#define RL_BUFSIZE 1024
#define TOK_BUFSIZE 64
#define TOK_DELIM " \t\r\n\a"


char *privs_rdline(void)
{
	char *line = NULL;
	ssize_t buffsz = 0;
	getline(&line, &buffsz, stdin);
	return line;
}

char **privs_split_line(char *line)
{
	int bufsize = TOK_BUFSIZE, position = 0;
	char **tokens = malloc(bufsize * sizeof(char*));
	char *token;
	
	if (!tokens) 
	{
		fprintf(stderr, "privs: allocation error\n");
		exit(EXIT_FAILURE);
	}
	
	token = strtok(line, TOK_DELIM);
	
	while (token != NULL) 
	{
		tokens[position] = token;
		position++;

		if (position >= bufsize) 
		{
			bufsize += TOK_BUFSIZE;
			tokens = realloc(tokens, bufsize * sizeof(char*));
			if (!tokens) 
			{
				fprintf(stderr, "privs: allocation error\n");
				exit(EXIT_FAILURE);
			}
		}

		token = strtok(NULL, TOK_DELIM);
	}
	tokens[position] = NULL;
	return tokens;
}

int privs_execute(char **args)
{
	int i;

	if (args[0] == NULL)
		return EXIT_SUCCESS;

	for (i = 0; i < privs_num_builtins(); i++) {
		if (strcmp(args[0], builtin_str[i]) == 0) {
			return (*builtin_func[i])(args);
		}
	}
	return privs_launch(args);
}

void privs_loop(void)
{
	char *line;
	char **args;
	int status;

	do {
		printf("@privs#> ");
		line = privs_rdline();
		args = privs_spltline(line);
		status = privs_exec(args);

		free(line);
		free(args);
	} while (status);
}

int privs_launch(char **args)
{
	pid_t pid, wpid;
	int status;

	pid = fork();
	if (pid == 0) 
	{
		// Child process
		if (execvp(args[0], args) == -1) 
			perror("privs");
		exit(EXIT_FAILURE);
	} 
	else if (pid < 0) 
		perror("privs");	// Error forking
	else 
	{
		// Parent process
		do 
		{
			wpid = waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}

	return EXIT_SUCCESS;
}


/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
	"cd",
	"help",
	"exit"
};

int (*builtin_func[]) (char **) = {
	&privs_cd,
	&privs_help,
	&privs_exit
};

int privs_num_builtins() 
{
	return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin function implementations.
*/
int privs_cd(char **args)
{
	if (args[1] == NULL) 
		fprintf(stderr, "privs: expected argument to \"cd\"\n");
	else 
	{
		if (chdir(args[1]) != 0)
			perror("privs");
	}
	return EXIT_SUCCESS;
}

int privs_help(char **args)
{
	int i;
	printf("Stephen Brennan's privs\n");
	printf("Type program names and arguments, and hit enter.\n");
	printf("The following are built in:\n");

	for (i = 0; i < privs_num_builtins(); i++)
		printf("  %s\n", builtin_str[i]);

	printf("Use the man command for information on other programs.\n");
	return EXIT_SUCCESS;
}

int privs_exit(char **args)
{
	return 0;
}

int main(int rgc, char **argv)
{
	privs_loop();

	return EXIT_SUCCESS;
}