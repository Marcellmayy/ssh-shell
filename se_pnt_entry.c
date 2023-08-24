#include "shell.h"

/**
 * main_shell - main control for the shell program
 * @pnt: pointer to an pnt_s struct containing shell pntrmation
 * @sv: array of strings containing arguments to the shell
 *
 * Return: the status of the last executed builtin command
 */
int shell_main(pnt_s *pnt, char **sv)
{
	ssize_t read_result = 0;
	int builtin_return_value = 0;

	while (read_result != -1 && builtin_return_value != -2)
	{
		clear_pnt(pnt);

		/* Display the shell prompt if in interactive mode */
		if (from_terminal(pnt))
			_puts("$ ");

		putchar_err(NEG_ONE);
		read_result = get_input(pnt);

		/* Handle input if it was successfully read */
		if (read_result != -1)
		{
			set_pnt(pnt, sv);
			builtin_return_value = handle_builtin(pnt);

			/* Check for command execution if not a builtin command */
			if (builtin_return_value == -1)
				check_command(pnt);
		}

		/* Handle end of input if in from_terminal mode */
		else if (from_terminal(pnt))
			_putchar('\n');

		free_pnt(pnt, 0);
	}

	/* Create and store the history list */
	create_history(pnt);

	/* Free memory and exit */
	free_pnt(pnt, 1);
	if (!from_terminal(pnt) && pnt->status)
		exit(pnt->status);

	/* Handle exit with error */
	if (builtin_return_value == -2)
	{
		if (pnt->error_code == -1)
			exit(pnt->status);
		exit(pnt->error_code);
	}

	/* Return the last executed builtin command's status */
	return (builtin_return_value);
}

/**
 * handle_builtin - finds a builtin command
 * @pnt: the parameter & return pnt struct
 *
 * Return: -1 if builtin not found,
 * 0 if builtin executed successfully,
 * 1 if builtin found but not successful,
 * 2 if builtin signals exit()
 */
int handle_builtin(pnt_s *pnt)
{
	int i, builtin_return_value = -1;

	builtin_commands builtints[] = {
		{"cd", handle_cd},
		{"env", _printenv},
		{"exit", handle_exit},
		{"help", handle_help},
		{"alias", handle_alias},
		{"setenv", check_setenv},
		{"history", handle_history},
		{"unsetenv", check_unsetenv},
		{NULL, NULL}};

	for (i = 0; builtints[i].type; i++)
		if (_strcmp(pnt->argv[0], builtints[i].type) == 0)
		{
			pnt->lines++;
			builtin_return_value = builtints[i].func(pnt);
			break;
		}
	return (builtin_return_value);
}

/**
 * check_command - searches for a command in PATH or the current directory
 * @pnt: a pointer to the parameter and return pnt struct
 *
 * Return: void
 */
void check_command(pnt_s *pnt)
{
	char *path = NULL;
	int i, count_str;

	pnt->path = pnt->argv[0];
	if (pnt->lc_flag == 1)
	{
		pnt->lines++;
		pnt->lc_flag = 0;
	}

	/* Count the number of non-delimiter words in the argument */
	for (i = 0, count_str = 0; pnt->arg[i]; i++)
		if (!is_delimiter(pnt->arg[i], " \t\n"))
			count_str++;

	/* If there are no words, exit without doing anything */
	if (!count_str)
		return;

	/* Check if the command is in the PATH variable */
	path = check_file_in_path(pnt, _getenv(pnt, "PATH="), pnt->argv[0]);
	if (path)
	{
		pnt->path = path;
		create_process(pnt);
	}
	else
	{
		/* Check if the command is in the current directory */
		if ((from_terminal(pnt) || _getenv(pnt, "PATH=") || pnt->argv[0][0] == '/') && is_executable(pnt, pnt->argv[0]))
			create_process(pnt);
		/* If the command is not found, print an error message */
		else if (*(pnt->arg) != '\n')
		{
			pnt->status = 127;
			print_error(pnt, "not found\n");
		}
	}
}

/**
 * create_process - forks a new process to run the command
 * @pnt: pointer to the parameter & return pnt struct
 *
 * This function forks a new process and runs the command specified by the
 * @pnt->argv array. The new process runs in a separate memory space and
 * shares environment variables with the parent process.
 *
 * Return: void
 */
void create_process(pnt_s *pnt)
{
	pid_t cpid;

	/* Fork a new process */
	cpid = fork();
	if (cpid == -1)
	{
		/* TODO: PUT ERROR FUNCTION */
		perror("Error:");
		return;
	}

	/* Child process: execute the command */
	if (cpid == 0)
	{
		/* Execute the command */
		if (execve(pnt->path, pnt->argv, get_environ(pnt)) == -1)
		{
			/* Handle execve errors */
			free_pnt(pnt, 1);
			if (errno == EACCES)
				exit(126);
			exit(1);
		}
		/* TODO: PUT ERROR FUNCTION */
	}
	/* Parent process: wait for child process to finish */
	else
	{
		wait(&(pnt->status));
		if (WIFEXITED(pnt->status))
		{
			/* Set return status to child's exit status */
			pnt->status = WEXITSTATUS(pnt->status);

			/* Print error message for permission denied errors */
			if (pnt->status == 126)
				print_error(pnt, "Permission denied\n");
		}
	}
}
