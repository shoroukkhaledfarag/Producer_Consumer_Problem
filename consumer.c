#include <stdio.h>      //if you don't use scanf/printf change this include
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
key_t up,down,size;

//----------------------STRUCT
struct msgbuff
{
	long mtype;
	char mtext[256];
};

struct buff
{
	int value[100];
	int counter;
};
//----------------------Semaphore
union Semun
{
    int val;               		/* value for SETVAL */
    struct semid_ds *buf;  	/* buffer for IPC_STAT & IPC_SET */
    ushort *array;          	/* array for GETALL & SETALL */
    struct seminfo *__buf;  	/* buffer for IPC_INFO */
    void *__pad;
};


void down_sem(int sem)
{
    struct sembuf p_op;

    p_op.sem_num = 0;
    p_op.sem_op = -1;
    p_op.sem_flg = !IPC_NOWAIT;

    if(semop(sem, &p_op, 1) == -1)
    {
        perror("Error in down()");
        exit(-1);
    }
}


void up_sem(int sem)
{
    struct sembuf v_op;

    v_op.sem_num = 0;
    v_op.sem_op = 1;
    v_op.sem_flg = !IPC_NOWAIT;

    if(semop(sem, &v_op, 1) == -1)
    {
        perror("Error in up()");
        exit(-1);
    }
}
int main()
{

	struct msgbuff message,rec_msg;
	int counter=0;
	int buffer_size=0;
	int buffer_key=111,size_key=233,up_key=332,down_key=255,counter_key=553,SEM1=234;
	
	//-------------------------------------------------------initializing semaphore
	union Semun semun;

        int sem1 = semget(SEM1, 1, 0666|IPC_CREAT);

        if(sem1 == -1 )
        {
		perror("Error in create semaphore");
		exit(-1);
        }

	//-------------------------------------------------------taking buffer size from user

	size = msgget(size_key,IPC_CREAT|0666); // or msgget(12613, IPC_CREATE | 0644)
        if(size == -1)
        {
          perror("Error in create");
          exit(-1);
        }
        int rv = msgrcv(size, &buffer_size, sizeof(buffer_size), 0, !IPC_NOWAIT);

        if(rv == -1)
            perror("Error in receive");
        else
            printf("\nMessage received: %d\n", buffer_size);

	//-------------------------------------------------------Creating buffer shared memory
	
	int shmid = shmget(buffer_key, buffer_size * 4, IPC_CREAT | 0644);
    	while ((int)shmid == -1)
    	{
		//Make sure that the clock exists
		printf("Wait! The buffer is  not initialized yet!\n");
		sleep(1);
		shmid = shmget(buffer_key, 4, 0444);
	}
	struct buff * buffer =shmat(shmid, (void *)0, 0);	
	//-------------------------------------------------------initializing the up and down queue 
    	
    	up = msgget(up_key, IPC_CREAT|0666); // created the up queue
   	if(up == -1)
   	{
        	perror("Error in creating up queue at producer");
        	exit(-1);
    	}
    	printf("up = %d\n", up);

    	down = msgget(down_key, IPC_CREAT|0666); // created the down queue
    	if(down == -1)
    	{
        	perror("Error in creating down queue at producer");
        	exit(-1);
    	}
    	printf("down = %d\n", down);
	//-------------------------------------------------------Main loop
	int index=0;
			int rec_val = msgrcv(up, &rec_msg, sizeof(rec_msg.mtext), 7, !IPC_NOWAIT);
	    		if(rec_val == -1)
				perror("Error in receive at consumer");
	    		else
	    		{
				printf("\nMessage received at consumer : %s\n", rec_msg.mtext);
			}
	while(1)
	{
		sleep(1);
		down_sem(sem1);
		if (buffer->counter > buffer_size)
			exit(0);
		if(buffer->counter==0)
		{	
			up_sem(sem1);
			int rec_val = msgrcv(up, &rec_msg, sizeof(rec_msg.mtext), 7, !IPC_NOWAIT);
	    		if(rec_val == -1)
				perror("Error in receive at consumer");
	    		else
	    		{
				printf("\nMessage received at consumer : %s\n", rec_msg.mtext);
			}
		}
		else if( buffer->counter == buffer_size)
		{
			printf("\nbuffer[%d]= %d at consumer\n", index,buffer->value[index]);
			buffer->value[index]=-1;
			buffer->counter=buffer->counter-1;
			index=(index+1)%buffer_size;

			strncpy(message.mtext, "buffer has an empty slot", 256);
			message.mtype=4;
			int send_val = msgsnd(down, &message, sizeof(message.mtext), !IPC_NOWAIT);
			if(send_val == -1)
				perror("Error in sending from consumer");
			up_sem(sem1);
		}else 
		{	
			printf("\nbuffer[%d]= %d at consumer\n", index,buffer->value[index]);
			buffer->value[index]=-1;
			buffer->counter=buffer->counter-1;
			index=(index+1)%buffer_size;
			up_sem(sem1);
		}
		
		
	}
return 0;

}
