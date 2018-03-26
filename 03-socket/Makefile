all: master worker

master: master.c
	gcc -Wall -g master.c -o master

worker: worker.c
	gcc -Wall -g worker.c -o worker

clean:
	@rm master worker
