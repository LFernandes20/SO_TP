#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>


int findFile (char* file, char* path) {
	int auxpd[2];
	int tam;
	char *auxbuff = malloc (128);
		if (pipe(auxpd) != -1) {
			if (!fork()) {
				dup2(auxpd[1],1);
				close(auxpd[0]);
				close(auxpd[1]);
				execlp("find", "find", path, "-name", file, NULL);
				close(1);
				_exit(0);
			}
			
			wait(0);
			close(auxpd[1]);
			read(auxpd[0], auxbuff, 128);
			
			if (auxbuff == NULL) tam = 0;
			else tam = strlen(auxbuff);
		}
		free(auxbuff);
		return tam;
}


int existLink (char* cod, char* caminhoMetadata) {
	int fstpd[2], sndpd[2];
	char* buffer = malloc(128);

	pipe (fstpd);
	if (!fork()) {
		dup2 (fstpd[1], 1);
		close (fstpd[0]);
		close (fstpd[1]);
		execlp ("ls", "ls", "-l", caminhoMetadata, NULL);
		_exit(0);
	}
	wait(0);

	dup2 (fstpd[0], 0);
	close (fstpd[0]);
	close (fstpd[1]);
	pipe (sndpd);
	
	if (!fork()) {
		dup2 (sndpd[1], 1);
		close (sndpd[0]);
		close (sndpd[1]);
		execlp ("grep", "grep", cod, NULL);
		_exit(0);
	}
	wait(0);

	close(sndpd[1]);
	int tam = read (sndpd[0], buffer, 128);

	if (tam <= 0)
		return 0;
	else
		return 1;
}


void backup (char** files, int pid, char* name, char* caminhoData, char* caminhoMetadata) {
	int pd[2];
	char* zipped = malloc(128), *cod = malloc(128);
	if (pipe(pd) != -1) {
		if (!fork()) {
			dup2(pd[1],1);
			close(pd[0]);
			close(pd[1]);
			execlp("sha1sum", "sha1sum", *(files + 2), NULL);
			close(1);
			_exit(0);
		}
		
		close(pd[1]);
		read(pd[0], cod, 128);
		char* tok = strtok(cod, " ");
		wait(0);

		if (!fork()) {
			execlp("gzip", "gzip", "-k", *(files + 2), NULL);
			_exit(0);
		}
		wait(0);

		if (findFile (name, caminhoMetadata) != 0) {
			if (!fork()) {
				strcat (caminhoMetadata, name);
				execlp ("rm", "rm", caminhoMetadata, NULL);
				_exit(0);
			}
			wait (0);
				
			if (!fork()) {
				strcat (caminhoData, cod);
				execlp ("rm", "rm", caminhoData, NULL);
				_exit(0);
			}
			wait (0);
		}

		if (!fork()) {
			strcpy (zipped, *(files + 2));
			strcat(zipped, ".gz");
			strcat (caminhoData, tok);
			execlp("mv", "mv", zipped, caminhoData, NULL);
			_exit(0);
		}
		wait(0);

		if (!fork()) {
			strcat (caminhoData, tok);
			strcat (caminhoMetadata, name);
			execlp ("ln", "ln", "-s", caminhoData, caminhoMetadata, NULL);
			_exit(0);
		}
		wait(0);								
	}
	free(cod);
	free(zipped);
	kill(pid, SIGUSR1);
}


void restore (char** files, int pid, char* name, char* caminhoData, char* caminhoMetadata) {
	char *diretoria = malloc (128), *cod = malloc(128), *restoredFile = malloc(128), *novoNome = malloc(128), *caminhoMetadataAux = malloc(128);
	char buffer[PATH_MAX + 1];
	diretoria = getcwd (buffer, PATH_MAX + 1);

	int pd[2];
	if (pipe(pd) != -1) {
		if (!fork()) {
			dup2(pd[1],1);
			close(pd[0]);
			close(pd[1]);
			strcat (caminhoMetadata, name);
			execlp("readlink", "readlink", caminhoMetadata, NULL);
			close(1);
			_exit(0);
		}
		
		close(pd[1]);
		read(pd[0], cod, 128);
		cod = strtok(cod, "\n");
		char *p = strrchr(cod, '/'); 
    		if (p && *(p + 1)) 
    			strcpy(cod, (p + 1));
		wait(0);

		if (!fork()) {
			strcpy (novoNome, name);
			strcat (novoNome, ".gz");
			strcpy (caminhoMetadataAux, caminhoMetadata);
			strcat (caminhoMetadata, name);
			strcat (caminhoMetadataAux, novoNome);
			execlp ("mv", "mv", caminhoMetadata, caminhoMetadataAux, NULL);
			_exit(0);
		}
		wait(0);

		if (!fork()) {
			strcpy (novoNome, name);
			strcat (novoNome, ".gz");
			strcpy (caminhoMetadataAux, caminhoMetadata);
			strcat (caminhoMetadata, name);
			strcat (caminhoMetadataAux, novoNome);
			execlp ("gzip", "gzip", "-fd", caminhoMetadataAux, NULL);
			_exit(0);
		}
		wait(0);

		if (strchr(*(files + 1), '/') != NULL) {
			if (!fork()) {
				strcpy (restoredFile, caminhoMetadata);
				strcat (restoredFile, name);
				execlp ("mv", "mv", restoredFile, *(files + 2), NULL);
				_exit(0);
			}
			wait(0);
		}
		else {
			if (!fork()) {
				strcpy (restoredFile, caminhoMetadata);
				strcat (restoredFile, name);
				strcat (diretoria, "/");
				strcat (diretoria, name);
				execlp ("mv", "mv", restoredFile, diretoria, NULL);
				_exit(0);
			}
			wait(0);
		}

		if (existLink (cod, caminhoMetadata) == 0) {
			if (!fork()) {
				strcat (caminhoData, cod);
				execlp ("rm", "rm", caminhoData, NULL);
				_exit(0);
			}
			wait(0);
		}
	}
	free(caminhoMetadataAux);
	free(novoNome);
	free(restoredFile);
	free(cod);
	kill(pid, SIGCONT);
}


void delete (char** files, char* name, char* caminhoData, char* caminhoMetadata) {
	int pd[2], fstpd[2], sndpd[2];
	char *cod = malloc(128);

	pipe(pd);
	if (!fork()) {
		dup2(pd[1],1);
		close(pd[0]);
		close(pd[1]);
		strcat (caminhoMetadata, name);
		execlp("readlink", "readlink", caminhoMetadata, NULL);
		close(1);
		_exit(0);
	}

	close(pd[1]);
	read(pd[0], cod, 128);
	cod = strtok(cod, "\n");
	char *p = strrchr(cod, '/'); 
    	if (p && *(p + 1)) 
    		strcpy(cod, (p + 1));
	wait(0);

	if (!fork()) {
		strcat (caminhoMetadata, name);
		execlp ("rm", "rm", caminhoMetadata, NULL);
		_exit(0);
	}
	wait(0);

	if (existLink (cod, caminhoMetadata) == 0) {
		if (!fork()) {
			strcat (caminhoData, cod);
			execlp ("rm", "rm", caminhoData, NULL);
			_exit(0);
		}
		wait(0);
	}
	free(cod);
	kill(atoi(*files), SIGQUIT);
}


void sair () {
	while (wait(NULL))
   	if (errno == ECHILD) break;
		_exit(kill(getppid(), 9));
}


int main() {
	printf("Server ON: ... waiting for connections ...\n");
	printf("\n---  se pretender encerrar o servidor, execute ./sobucli exit\n");
	char* home = malloc(128);
	strcpy(home, getenv("HOME"));
	char* path = strcat(home, "/.Backup/fifo");
	char* caminhoData = malloc(256);
	strcpy (caminhoData, getenv ("HOME")); 
	caminhoData = strcat (caminhoData, "/.Backup/data/");
	char* caminhoMetadata = malloc(256);
	strcpy (caminhoMetadata, getenv ("HOME")); 
	caminhoMetadata = strcat (caminhoMetadata, "/.Backup/metadata/");
	char* diretoria = malloc (128);
	char buffer[PATH_MAX + 1];
	diretoria = getcwd (buffer, PATH_MAX + 1);

	int fst = fork();
	if (fst == -1) perror("fork error");
	else
		if(!fst) {
			int fd = open(path, O_RDONLY);
			char *token = malloc(128), *buf = malloc(128), *cod = malloc(128), *meta = malloc(128), *name = malloc(128);
			while(1) {
				while (read(fd, buf, 128)) {

					int i = 3;
					char** files = malloc(sizeof(char*) * 3);
					//PID do cliente
					*files = malloc(128);
					strcpy(*files, strtok (strtok (buf, "\n"), " "));
					//operação (backup/restore/exit)
					*(files+1) = malloc(128);
					strcpy(*(files+1), strtok (NULL, " "));
					//ficheiro
					*(files+2) = malloc(128);
					strcpy(*(files+2), strtok (NULL, " "));
					//para cobrir os casos em que é passado mais do qe um ficheiro
					char* token = strtok(NULL, " ");
					for(i; token != NULL; i++){
						if (*(files+i) == NULL)
							files = realloc(files, sizeof(char*) * (i+2));
						*(files+i) = malloc(128);
						strcpy(*(files+i),token);
						token = strtok (NULL, " ");
					}
					
					if (!strcmp(*(files+1), "exit")) {
						kill(atoi(*files), SIGURG);
						sair();
					}

					if (!strcmp(*(files+1), "backup")) {
						if (strchr(*(files+2), '/') != NULL) {
							
							strcpy(name, *(files+2));
    						char *p = strrchr(name, '/'); 
    						if (p && *(p + 1)) 
    							strcpy(name, (p + 1));
    						if (findFile (name, *(files + 2)) != 0)
								backup (files, atoi(*files), name, caminhoData, caminhoMetadata);
							else
								kill(atoi(*files), SIGUSR2);
						}
						else {
							strcpy (name, *(files+2));
							if (findFile (name, diretoria) != 0)
								backup (files, atoi(*files), name, caminhoData, caminhoMetadata);
							else
								kill(atoi(*files), SIGUSR2);
						}
					}

					if (!strcmp(*(files+1), "restore")) {
						if (strchr(*(files+2), '/') != NULL) {
							
							strcpy(name, *(files+2));
    						char *p = strrchr(name, '/'); 
    						if (p && *(p + 1)) 
    							strcpy(name, (p + 1));
    						if (findFile (name, caminhoMetadata) != 0)
								restore (files, atoi(*files), name, caminhoData, caminhoMetadata);
							else
								kill(atoi(*files), SIGINT);
						}
						else {
							strcpy (name, *(files+2));
							if (findFile (name, caminhoMetadata) != 0)
								restore (files, atoi(*files), name, caminhoData, caminhoMetadata);
							else
								kill(atoi(*files), SIGINT);
						}
					}
					
					if (!strcmp(*(files+1), "delete")) {
						strcpy (name, *(files + 2));
						if (findFile (name, caminhoMetadata) != 0) {
							delete (files, name, caminhoData, caminhoMetadata);
						}
						else
							kill(atoi(*files), SIGQUIT);
					}
				} //fecha o while read
			}
			free(buf);
			free(token);
			free(cod);
			free(name);
			free(meta);
			close(fd);
			return 0;	
		}		
}