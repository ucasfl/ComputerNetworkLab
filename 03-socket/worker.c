/* worker application */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
 
int main(int argc, char *argv[])
{
    int sock, cs;
	struct sockaddr_in worker, master;
	char name[25];
	void *msg = malloc (100);
	void *send_msg = malloc (104);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("Could not create socket\n");
    }
    printf("Socket created\n");

    // Prepare the sockaddr_in structure
    worker.sin_family = AF_INET;
    worker.sin_addr.s_addr = INADDR_ANY;
    worker.sin_port = htons(8888);

	if ( bind(sock, (struct sockaddr *)&worker, sizeof(worker)) <0 ){
		perror( "bind failed. Error\n" );
		return -1;
	}
	
	listen(sock, 3);
	printf ( "Waiting for incoming connectionos...\n" );

	int c = sizeof(struct sockaddr_in);
	if ((cs = accept(sock, (struct sockaddr *)&master, (socklen_t *)&c)) < 0){
		perror("accept failed!\n");
		return 1;
	}
	printf ( "Connection accepted.\n" );

	int msg_len = 0;
	msg_len = recv(cs, msg, 100 ,0);
	if (msg_len <= 0){
		printf ("recv message error.\n");
		return 1;
	}
	printf ("Recv successful.\n");

	/*printf ("%d %d %d\n", *(int *)(msg), *(int *)((int *)msg + 1), *(int *)((int *)msg + 2));*/
	/*printf ("msg_len = %d\n", msg_len);*/
	/*int len = *(int *)msg;*/
	int begin = *(int *)((int *)msg + 1);
	int end = *(int *)((int *)msg + 2);
	/*printf ("\nmsg+12 : %s\n", (char *)(msg) + 12);*/
	/*printf ("before memcpy\n");*/
	strncpy(name,((char *)(msg) + 12), 18);
	/*name[18] = '\0';*/
	/*printf ("after memcpy\n");*/
	/*printf ("name == %s\n", name);*/
	/* count  the number of a~z
	 */
	FILE *fp;
	if ( !(fp = fopen((char *)name, "r")) ){
		printf ( "Open file failed.\n" );
		return 1;
	}

	fseek (fp, begin, SEEK_SET);

	for ( int i = 0; i < 26; ++i ){
		((int *)send_msg)[i] = 0;
	}

	char ac;
	for ( int i = 0; i < end - begin && (ac = fgetc(fp)) != EOF; i++ ){
		if ( ac >= 'a' && ac <= 'z' ){
			++((int *)send_msg)[ac - 'a'];
		}
		if ( ac >= 'A' && ac <= 'Z' ){
			++((int *)send_msg)[ac - 'A'];
		}
	}

	write (cs, send_msg, 104);

    close(sock);
	fclose(fp);
	if (msg){
		free(msg);
	}
	if (send_msg){
		free(send_msg);
	}
    return 0;
}
