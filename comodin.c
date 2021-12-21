/*-
 * main.c
 * Minishell C source
 * Shows how to use "obtain_order" input interface function.
 *
 * Copyright (c) 1993-2002-2019, Francisco Rosales <frosal@fi.upm.es>
 * Todos los derechos reservados.
 *
 * Publicado bajo Licencia de Proyecto Educativo Práctico
 * <http://laurel.datsi.fi.upm.es/~ssoo/LICENCIA/LPEP>
 *
 * Queda prohibida la difusión total o parcial por cualquier
 * medio del material entregado al alumno para la realización
 * de este proyecto o de cualquier material derivado de este,
 * incluyendo la solución particular que desarrolle el alumno.
 *
 * DO NOT MODIFY ANYTHING OVER THIS LINE
 * THIS FILE IS TO BE MODIFIED
 */

#include <stddef.h> /* NULL */
#include <stdio.h>	/* setbuf, printf */
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <dirent.h>
#include <glob.h>

extern int obtain_order(); /* See parser.y for description */

void ignorarSenales()
{
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
}

void defaultSenales()
{
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
}

char *cambiarString(char *str, char *orig, char *rep)
{
	static char buffer[4096];
	char *p;

	if (!(p = strstr(str, orig)))
		return str;

	strncpy(buffer, str, p - str);
	buffer[p - str] = '\0';

	sprintf(buffer + (p - str), "%s%s", rep, p + strlen(orig));

	return buffer;
}

void buscarMetacaracteres(char ***argvv)
{
	char **argv;
	char *virgulilla = "~";
	char *dolar = "$";
	char *comodin = "?";

	for (int argvc = 0; (argv = argvv[argvc]); argvc++)
	{
		int numComodines = 0;
		for (int argc = 0; argv[argc]; argc++)
		{
			if (!strncmp(argv[argc], virgulilla, 1))
			{
				int n = -1;
				char *usuario = strtok(argv[argc], virgulilla);
				if (usuario != NULL)
				{
					n = sscanf(usuario, "%[_a-zA-Z0-9]");
				}
				if (n == 1)
				{
					struct passwd *pw;
					if ((pw = getpwnam(usuario)) == NULL)
					{
						strcpy(argv[argc], getenv("HOME"));
					}
					else
					{
						strcpy(argv[argc], pw->pw_dir);
					}
				}
				else
				{
					strcpy(argv[argc], getenv("HOME"));
				}
			}
			else
			{
				char *caracter;
				char *varDolar[1000];
				for (caracter = argv[argc]; *caracter != '\0'; caracter++)
				{
					if (!strncmp(caracter, dolar, 1))
					{
						sprintf(varDolar, caracter);
						int n = -1;
						char *variable = strtok(varDolar, dolar);
						if (variable != NULL)
						{
							n = sscanf(variable, "%[_a-zA-Z0-9]");
						}
						if (n == 1)
						{
							if (getenv(variable) == NULL)
							{
								fprintf(stderr, "La variable de entorno no existe\n");
							}
							else
							{
								strcpy(argv[argc], cambiarString(argv[argc], varDolar, getenv(variable)));
							}
						}
						else
						{
							fprintf(stderr, "Utilización del $ incorrecta");
						}
						break;
					}
					else if (!strncmp(caracter, comodin, 1))
					{
						numComodines = argc;
						break;
					}
				}
			}
		}
		glob_t glob_buff = {0};
		glob(argv[numComodines], GLOB_DOOFFS, NULL, &glob_buff);
		modificarParametros(glob_buff, numComodines, argv);
	}
}

void modificarParametros(glob_t glob_buff, int argc, char **argv)
{
	int sizeSecuencia = 0;
	while (argv[sizeSecuencia] != NULL)
	{
		argv[sizeSecuencia] = argv[sizeSecuencia];
		sizeSecuencia++;
	}
	for (int i = sizeSecuencia - 1; i > argc; i--)
	{
		argv[i + glob_buff.gl_pathc - 1] = argv[i];
	}
	for (size_t i = 0; i < glob_buff.gl_pathc; i++)
	{
		argv[argc] = glob_buff.gl_pathv[i];
		argc++;
	}
}

int comprobarRedireccionIN(char *filev[3])
{
	int fd;
	if (filev[0])
	{
		if ((fd = open(filev[0], O_RDONLY)) < 0)
		{
			fprintf(stderr, "Error al abrir el fichero %s\n", filev[0]);
			return fd;
		}
		else
		{
			dup2(fd, 0);
			close(fd);
		}
	}
}

int comprobarRedireccionOUT(char *filev[3])
{
	int fd;
	if (filev[1])
	{
		if ((fd = open(filev[1], O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0)
		{
			fprintf(stderr, "Error al abrir el fichero %s\n", filev[1]);
			return fd;
		}
		else
		{
			dup2(fd, 1);
			close(fd);
		}
	}
}

int comprobarRedireccion(char *filev[3])
{
	int fd[3];
	if (filev[0])
	{
		if ((fd[0] = open(filev[0], O_RDONLY)) < 0)
		{
			fprintf(stderr, "Error al abrir el fichero %s\n", filev[0]);
			return fd[0];
		}
		else
		{
			dup2(fd[0], 0);
			close(fd[0]);
		}
	}
	if (filev[1])
	{
		if ((fd[1] = open(filev[1], O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0)
		{
			fprintf(stderr, "Error al abrir el fichero %s\n", filev[1]);
			return fd[1];
		}
		else
		{
			dup2(fd[1], 1);
			close(fd[1]);
		}
	}
	if (filev[2])
	{
		if ((fd[2] = open(filev[2], O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0)
		{
			fprintf(stderr, "Error al abrir el fichero %s\n", filev[2]);
			return fd[2];
		}
		else
		{
			dup2(fd[2], 2);
			close(fd[2]);
		}
	}
	return 0;
}

int manCD(char **argv)
{
	if (argv[1] != NULL && argv[2] == NULL)
	{
		if (chdir(argv[1]) == 0)
		{
			printf("%s\n", getcwd(argv[1], 1000));
		}
		else
		{
			fprintf(stderr, "cd: %s: No such file or directory\n", argv[1]);
			return -1;
		}
	}
	else if (argv[1] == NULL)
	{
		chdir(getenv("HOME"));
		printf("%s\n", getcwd(getenv("HOME"), 1000));
	}
	else
	{
		fprintf(stderr, "cd: too many arguments\n");
		return -1;
	}
	return 0;
}

int manUmask(char **argv)
{
	int errno = 0;
	if (argv[1] == NULL)
	{
		int mask = (int)umask(0000);
		umask(mask);
		printf("%o\n", mask);
	}
	else if (argv[1] != NULL && argv[2] == NULL)
	{
		int newMask = strtol(argv[1], NULL, 8);
		if (errno == newMask)
		{
			fprintf(stderr, "Error en strtol, fuera de rango\n");
			return -1;
		}
		else
		{
			int mask = (int)umask(newMask);
			printf("%o\n", mask);
		}
	}
	else
	{
		fprintf(stderr, "umask: too many arguments\n");
		return -1;
	}
	return 0;
}

int manLimit(char **argv)
{
	struct rlimit rlim;

	if (argv[1] == NULL)
	{
		getrlimit(RLIMIT_CPU, &rlim);
		printf("%s\t%d\n", "cpu", (int)rlim.rlim_cur);

		getrlimit(RLIMIT_FSIZE, &rlim);
		printf("%s\t%d\n", "fsize", (int)rlim.rlim_cur);

		getrlimit(RLIMIT_DATA, &rlim);
		printf("%s\t%d\n", "data", (int)rlim.rlim_cur);

		getrlimit(RLIMIT_STACK, &rlim);
		printf("%s\t%d\n", "stack", (int)rlim.rlim_cur);

		getrlimit(RLIMIT_CORE, &rlim);
		printf("%s\t%d\n", "core", (int)rlim.rlim_cur);

		getrlimit(RLIMIT_NOFILE, &rlim);
		printf("%s\t%d\n", "nofile", (int)rlim.rlim_cur);
	}
	else if (argv[1] != NULL && (argv[2] == NULL || (argv[2] != NULL && argv[3] == NULL)))
	{
		char *limites[6] = {"cpu", "fsize", "data", "stack", "core", "nofile"};
		if (!strcmp(limites[0], argv[1]))
		{
			getrlimit(RLIMIT_CPU, &rlim);
			if (argv[2] == NULL)
			{
				printf("%s\t%d\n", limites[0], (int)rlim.rlim_cur);
			}
			else
			{
				rlim.rlim_cur = strtol(argv[2], NULL, 10);
				setrlimit(RLIMIT_CPU, &rlim);
			}
		}
		else if (!strcmp(limites[1], argv[1]))
		{
			getrlimit(RLIMIT_FSIZE, &rlim);
			if (argv[2] == NULL)
			{
				printf("%s\t%d\n", limites[1], (int)rlim.rlim_cur);
			}
			else
			{
				rlim.rlim_cur = strtol(argv[2], NULL, 10);
				setrlimit(RLIMIT_FSIZE, &rlim);
			}
		}
		else if (!strcmp(limites[2], argv[1]))
		{
			getrlimit(RLIMIT_DATA, &rlim);
			if (argv[2] == NULL)
			{
				printf("%s\t%d\n", limites[2], (int)rlim.rlim_cur);
			}
			else
			{
				rlim.rlim_cur = strtol(argv[2], NULL, 10);
				setrlimit(RLIMIT_DATA, &rlim);
			}
		}
		else if (!strcmp(limites[3], argv[1]))
		{
			getrlimit(RLIMIT_STACK, &rlim);
			if (argv[2] == NULL)
			{
				printf("%s\t%d\n", limites[3], (int)rlim.rlim_cur);
			}
			else
			{
				rlim.rlim_cur = strtol(argv[2], NULL, 10);
				setrlimit(RLIMIT_STACK, &rlim);
			}
		}
		else if (!strcmp(limites[4], argv[1]))
		{
			getrlimit(RLIMIT_CORE, &rlim);
			if (argv[2] == NULL)
			{
				printf("%s\t%d\n", limites[4], (int)rlim.rlim_cur);
			}
			else
			{
				rlim.rlim_cur = strtol(argv[2], NULL, 10);
				setrlimit(RLIMIT_CORE, &rlim);
			}
		}
		else if (!strcmp(limites[5], argv[1]))
		{
			getrlimit(RLIMIT_NOFILE, &rlim);
			if (argv[2] == NULL)
			{
				printf("%s\t%d\n", limites[5], (int)rlim.rlim_cur);
			}
			else
			{
				rlim.rlim_cur = strtol(argv[2], NULL, 10);
				setrlimit(RLIMIT_NOFILE, &rlim);
			}
		}
	}
	else
	{
		fprintf(stderr, "limit: too many arguments\n");
		return -1;
	}
	return 0;
}

extern char **environ;
int manSet(char **argv)
{
	if (argv[1] == NULL)
	{
		int i = 0;
		while (environ[i] != NULL)
		{
			printf("%s\n", environ[i]);
			i++;
		}
	}
	else if (argv[1] != NULL && argv[2] == NULL)
	{
		char *variable;
		if ((variable = getenv(argv[1])) == NULL)
		{
			fprintf(stderr, "La variable de entorno no existe\n");
			return -1;
		}
		else
		{
			printf("%s=%s\n", argv[1], variable);
		}
	}
	else
	{
		char lista[1000];
		sprintf(lista, "%s=%s", argv[1], argv[2]);
		int i = 3;
		while (argv[i] != NULL)
		{
			sprintf(lista, "%s %s", lista, argv[i]);
			i++;
		}
		putenv(lista);
	}
	return 0;
}

void sinPipes(char ***argvv, int *bg)
{
	char **argv = argvv[0];
	char *mandatos[4] = {"cd", "umask", "limit", "set"};

	int pid, status;
	pid = fork();
	switch (pid)
	{
	case -1:
		fprintf(stderr, "Error en fork");
		exit(1);
	case 0:
		if (!*bg)
		{
			defaultSenales();
		}
		if (!strcmp(mandatos[0], argv[0]))
		{
			int n = manCD(argv);
			if (n == -1)
			{
				exit(-1);
			}
		}
		else if (!strcmp(mandatos[1], argv[0]))
		{
			int n = manUmask(argv);
			if (n == -1)
			{
				exit(-1);
			}
		}
		else if (!strcmp(mandatos[2], argv[0]))
		{
			int n = manLimit(argv);
			if (n == -1)
			{
				exit(-1);
			}
		}
		else if (!strcmp(mandatos[3], argv[0]))
		{
			int n = manSet(argv);
			if (n == -1)
			{
				exit(-1);
			}
		}
		else
		{
			for (int argc2 = 0; argv[argc2]; argc2++)
				printf("%s ", argv[argc2]);
			execvp(argv[0], argv);
			perror("execvp");
			exit(1);
		}
	default:
		if (*bg)
		{
			char var[1000];
			sprintf(var, "bgpid=%d", pid);
			putenv(var);
			printf("[%d]\n", pid);
		}
		else
		{
			waitpid(pid, &status, NULL);
			char var[1000];
			sprintf(var, "status=%d", status);
			putenv(var);
		}
	}
}

void conPipes(char ***argvv, int *bg, char *filev[3])
{
	char **argv = argvv[0];
	char *mandatos[4] = {"cd", "umask", "limit", "set"};

	int numPipes = 0;
	while (argvv[numPipes] != NULL)
	{
		numPipes++;
	}

	int pipes[numPipes - 1][2];
	pipe(pipes[0]);

	int pid, status;
	// Primer elemento:
	pid = fork();
	switch (pid)
	{
	case -1:
		fprintf(stderr, "Error en fork");
		exit(1);
	case 0:
		if (!*bg)
		{
			defaultSenales();
		}
		dup2(pipes[0][1], 1);
		close(pipes[0][0]);
		comprobarRedireccionIN(filev);
		if (!strcmp(mandatos[0], argv[0]))
		{
			int n = manCD(argv);
			if (n == -1)
			{
				exit(-1);
			}
			else
			{
				exit(1);
			}
		}
		else if (!strcmp(mandatos[1], argv[0]))
		{
			int n = manUmask(argv);
			if (n == -1)
			{
				exit(-1);
			}
			else
			{
				exit(1);
			}
		}
		else if (!strcmp(mandatos[2], argv[0]))
		{
			int n = manLimit(argv);
			if (n == -1)
			{
				exit(-1);
			}
			else
			{
				exit(1);
			}
		}
		else if (!strcmp(mandatos[3], argv[0]))
		{
			int n = manSet(argv);
			if (n == -1)
			{
				exit(-1);
			}
			else
			{
				exit(1);
			}
		}
		else
		{
			execvp(argv[0], argv);
			perror("execvp");
			exit(1);
		}
	default:
		// Elementos intermedios:
		if (numPipes > 2)
		{
			for (int i = 1; i < numPipes - 1; i++)
			{
				pipe(pipes[i]);
				pid = fork();
				switch (pid)
				{
				case -1:
					fprintf(stderr, "Error en fork");
					exit(1);
				case 0:
					for (int j = 0; j <= i; j++)
					{
						if (j == i - 1)
						{
							dup2(pipes[j][0], 0);
							close(pipes[j][1]);
						}
						else if (j == i)
						{
							dup2(pipes[j][1], 1);
							close(pipes[j][0]);
						}
						else
						{
							close(pipes[j][0]);
							close(pipes[j][1]);
						}
					}
					if (!strcmp(mandatos[0], argvv[i][0]))
					{
						int n = manCD(argvv[i]);
						if (n == -1)
						{
							exit(-1);
						}
						else
						{
							exit(1);
						}
					}
					else if (!strcmp(mandatos[1], argvv[i][0]))
					{
						int n = manUmask(argvv[i]);
						if (n == -1)
						{
							exit(-1);
						}
						else
						{
							exit(1);
						}
					}
					else if (!strcmp(mandatos[2], argvv[i][0]))
					{
						int n = manLimit(argvv[i]);
						if (n == -1)
						{
							exit(-1);
						}
						else
						{
							exit(1);
						}
					}
					else if (!strcmp(mandatos[3], argvv[i][0]))
					{
						int n = manSet(argvv[i]);
						if (n == -1)
						{
							exit(-1);
						}
						else
						{
							exit(1);
						}
					}
					else
					{
						execvp(argvv[i][0], argvv[i]);
						perror("execvp");
						exit(1);
					}
				}
			}
		}
		// Ultimo elemento:
		pid = fork();
		switch (pid)
		{
		case -1:
			fprintf(stderr, "Error en fork");
			exit(1);
		case 0:
			dup2(pipes[numPipes - 2][0], 0);
			close(pipes[numPipes - 2][1]);
			for (int i = 0; i < numPipes - 2; i++)
			{
				close(pipes[i][0]);
				close(pipes[i][1]);
			}
			comprobarRedireccionOUT(filev);
			if (!strcmp(mandatos[0], argvv[numPipes - 1][0]))
			{
				int n = manCD(argvv[numPipes - 1]);
				if (n == -1)
				{
					exit(-1);
				}
				else
				{
					exit(1);
				}
			}
			else if (!strcmp(mandatos[1], argvv[numPipes - 1][0]))
			{
				int n = manUmask(argvv[numPipes - 1]);
				if (n == -1)
				{
					exit(-1);
				}
				else
				{
					exit(1);
				}
			}
			else if (!strcmp(mandatos[2], argvv[numPipes - 1][0]))
			{
				int n = manLimit(argvv[numPipes - 1]);
				if (n == -1)
				{
					exit(-1);
				}
				else
				{
					exit(1);
				}
			}
			else if (!strcmp(mandatos[3], argvv[numPipes - 1][0]))
			{
				int n = manSet(argvv[numPipes - 1]);
				if (n == -1)
				{
					exit(-1);
				}
				else
				{
					exit(1);
				}
			}
			else
			{
				execvp(argvv[numPipes - 1][0], argvv[numPipes - 1]);
				perror("execvp");
				exit(1);
			}
		default:
			for (int i = 0; i < numPipes - 1; i++)
			{
				close(pipes[i][0]);
				close(pipes[i][1]);
			}
			if (*bg)
			{
				char var[1000];
				sprintf(var, "bgpid=%d", pid);
				putenv(var);
				printf("[%d]\n", pid);
			}
			else
			{
				waitpid(pid, &status, NULL);
				char var[1000];
				sprintf(var, "status=%d", status);
				putenv(var);
			}
		}
	}
}

int main(void)
{
	ignorarSenales();

	char ***argvv = NULL;
	int argvc;
	char **argv = NULL;
	int argc;
	char *filev[3] = {NULL, NULL, NULL};
	int bg;
	int ret;

	setbuf(stdout, NULL); /* Unbuffered */
	setbuf(stdin, NULL);

	char var[100];
	sprintf(var, "mypid=%d", getpid());
	putenv(var);

	while (1)
	{
		fprintf(stderr, "%s", "msh> "); /* Prompt */
		ret = obtain_order(&argvv, filev, &bg);
		if (ret == 0)
			break; /* EOF */
		if (ret == -1)
			continue;	 /* Syntax error */
		argvc = ret - 1; /* Line */
		if (argvc == 0)
			continue; /* Empty line */

#if 1

		int redirecc[3] = {dup(0), dup(1), dup(2)};

		buscarMetacaracteres(argvv);

		if (argvv[0] != NULL && argvv[1] == NULL)
		{
			if (comprobarRedireccion(filev) == 0)
			{
				sinPipes(argvv, &bg);
			}
		}
		else
		{
			conPipes(argvv, &bg, filev);
		}

		for (int i = 0; i < 3; i++)
		{
			if (filev[i])
			{
				dup2(redirecc[i], i);
			}
			close(redirecc[i]);
		}

#endif
	}
	exit(0);
	return 0;
}
