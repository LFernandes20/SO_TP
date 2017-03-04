#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

typedef void (*sighandler_t)(int);
char* file;

void handler1(){
	/* sucesso no backup */
	printf("%s: copiado\n", file);
} 

void handler2(){
	/* erro no backup */
	printf("%s não existe na diretoria\n", file);
}

void handler3(){
	/* encerrar servidor */
	printf("Server OFF: ... Shutting Down ...\n");
} 

void handler4(){
	/* erro no restore ou delete */
	printf("%s não possui cópia de segurança\n", file);
} 

void handler5(){
	/* sucesso no restore */
	printf("%s: recuperado\n", file);
}

void handler6(){
	/*sucesso no delete */
	printf("%s: apagado\n", file);
}

int main(int argc, char** argv){
	signal(SIGUSR1, handler1);
	signal(SIGUSR2, handler2);
	signal(SIGURG, handler3);
	signal(SIGINT, handler4);
	signal(SIGCONT, handler5);
	signal(SIGQUIT, handler6);
	char* home = getenv("HOME");
	char* path = strcat(home, "/.Backup/fifo");
	int i = 1;
	int fd = open(path, O_WRONLY | O_NONBLOCK);
	if (fd == -1)
		return 0;
	int pid = getpid();
	char* buf = malloc(256);
	sprintf(buf, "%d ", pid);
	if (argc > 2)
		file = strdup(argv[2]);
	while(argv[i+1]){
		strcat(buf, argv[i]);
		strcat(buf, " ");
		i++;
	}
	strcat(buf, argv[i]);
	strcat(buf, "\n");
	write(fd, buf, 128);
	if (argc > 2)
		pause(); 
	free(buf);
	close(fd);
	return 0;
}