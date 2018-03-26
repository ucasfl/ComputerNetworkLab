/*master application */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#define bufferSize 10


int main(int argc, const char *argv[])
{
	int s1, s2;//, cs;
	char ip1[bufferSize], ip2[bufferSize];

	void *recv_msg1 = malloc (104);
	void *recv_msg2 = malloc (104);

	// Create socket
	if ((s1 = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Could not create socket\n");
		return -1;
	}
	if ((s2 = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Could not create socket\n");
		return -1;
	}
	printf("Socket created\n");

	FILE *fp;
	if ( !(fp = fopen("./workers.conf", "r")) ){
		printf ( "Open config file failed!\n" );
		return 1;
	}
	if (!(fgets(ip1, bufferSize, fp) && fgets(ip2, bufferSize, fp))){
		printf ( "Read config file failed!\n" );
		return 0;
	}
	fclose(fp);
	struct sockaddr_in worker1, worker2;

	worker1.sin_addr.s_addr = inet_addr(ip1);
	worker1.sin_family = AF_INET;
	worker1.sin_port = htons(8888);
	worker2.sin_addr.s_addr = inet_addr(ip2);
	worker2.sin_family = AF_INET;
	worker2.sin_port = htons(8888);

	if (connect(s1, (struct sockaddr *)&worker1, sizeof(worker1))<0){
		perror( "Connect worker1 failed!\n" );
		return 1;
	}
	if (connect(s2, (struct sockaddr *)&worker2, sizeof(worker2))<0){
		perror( "Connect worker2 failed!\n" );
		return 1;
	}
	printf ("Connected\n");

	printf ("Connected all workers!\n");

	int numOfChar = 0;
	FILE *fpc;
	if ( !(fpc = fopen("./war_and_peace.txt", "r")) ){
		printf ( "Open war_and_peace.txt failed!\n" );
		return 1;
	}
	while( (fgetc(fpc)) != EOF ){
		++numOfChar;
	}
	fclose(fpc);
	int middle = numOfChar / 2;
	int total = 30;
	int zero = 0;

	char loc[] = {'w', 'a','r','_','a','n','d','_','p','e','a','c','e','.','t','x','t','\0'};
	void *message1 = malloc (30);
	void *message2 = malloc (30);
	memcpy((void *)message1, &total, 4);
	memcpy((void *)((int *)(message1) + 1), &zero, 4);
	memcpy((void *)((int *)(message1) + 2), &middle, 4);
	strncpy(((char *)message1 + 12), loc, 18);
	memcpy((void *)message2, &total, 4);
	memcpy((void *)((int *)(message2) + 1), &middle, 4);
	memcpy((void *)((int *)(message2) + 2), &numOfChar, 4);
	strncpy(((char *)message2 + 12), loc, 18);
	/*printf ("%d %d %d\n", *(int *)(message1), *(int *)((int *)message1 + 1), *(int *)((int *)message1 + 2));*/
	/*printf ("%d %d %d\n", *(int *)(message2), *(int *)((int *)message2 + 1), *(int *)((int *)message2 + 2));*/

	/*printf ("%s\n", (char *)((char *)message1 + 12));*/
	if ( send(s1, message1, 30, 0) < 0 ){
		printf ( "Send to worker1 failed!\n" );
		return 1;
	}
	if ( send(s2, message2, 30, 0) < 0 ){
		printf ( "Send to worker2 failed!\n" );
		return 1;
	}
	/* TODO */
	/* recv message from worker about count result
	 * and merge count result then output the result
	 */
	if (recv(s1, recv_msg1, 104, 0) < 0){
		printf ("recv failed!\n");
		return 1;
	}
	if (recv(s2, recv_msg2, 104, 0) < 0){
		printf ("recv failed!\n");
		return 1;
	}

	 /* Print the count result*/
	for (int i = 0; i < 26; ++i){
		printf ("%c %d\n", 'a' + i, ((int *)recv_msg1)[i] + ((int *)recv_msg2)[i]);
	}

	close (s1);
	close (s2);
	if (message1){
		free(message1);
	}
	if (message2){
		free(message2);
	}
	if (recv_msg1){
		free(recv_msg1);
	}
	if (recv_msg2){
		free(recv_msg2);
	}
	return 0;
}
