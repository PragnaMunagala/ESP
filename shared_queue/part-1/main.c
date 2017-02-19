#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<sched.h>
#include<string.h>
#include"sharedQueue.h"
#include<time.h>
#include<unistd.h>
#include<sys/time.h>
#include<math.h>

#define BASE_PERIOD 1000

const int period_multiplier[] = {22, 24, 26, 28, 12, 15, 17, 19};
const int thread_priority[] = {94, 95, 96, 97, 90, 91, 92, 93};

sharedQueue *bus_in_q,*bus_out_q1,*bus_out_q2,*bus_out_q3;
pthread_mutex_t lock;
pthread_t senderThread[4],daemonThread,receiverThread[3];
pthread_attr_t sattr[4], dattr, rattr[3];
struct sched_param sparam[4], dparam, rparam[3];
int msgCount = 0;
int p = 0;
int dropMsgCount = 0;
float avgElapsedTime = 0;
float avgstd = 0;

struct messageThread{
	int sourceId;
	int destinationId;
};

float timedifference_msec(struct timeval t0, struct timeval t1)
{
    return (double)(t1.tv_sec - t0.tv_sec) * 1000.0f + (t1.tv_usec - t0.tv_usec) / 1000.0f ;
}

/* calling function when sender threads are created */
void* sender_function(void *ptr){
	while(msgCount < 2000)
	{	
		float millisec = 0;
		struct timeval start,end;		
		pthread_mutex_lock(&lock);
		struct messageThread *mt;
		mt  = (struct messageThread*)ptr;
		
		gettimeofday(&start, NULL);
		p = sq_write(bus_in_q, msgCount, mt->sourceId, mt->destinationId, "MessageInput");
		gettimeofday(&end, NULL);

		millisec = timedifference_msec(start, end);
				
		if(p == -1){
			dropMsgCount++;	
			msgCount++;
			asm volatile("nop");
		}
		else{
			msgCount++;
			bus_in_q->elements[p].elapseTime = millisec;			
		}
		
		pthread_mutex_unlock(&lock);
		switch(mt->sourceId)
		{
			case 11:usleep(period_multiplier[0]*BASE_PERIOD);break;
			case 12:usleep(period_multiplier[1]*BASE_PERIOD);break;
			case 13:usleep(period_multiplier[2]*BASE_PERIOD);break;
			case 14:usleep(period_multiplier[3]*BASE_PERIOD);break;
			default:asm volatile("nop");
		}
	}
}

/* calling function when daemon thread is created */
void* daemon_function(void *ptr){
	while(msgCount < 2000)
	{
		pthread_mutex_lock(&lock);
		int p=0;
		int q=0;
		struct messages dq;
		float millisec = 0;
		struct timeval start,end;
		
		gettimeofday(&start, NULL);
		p = sq_read(bus_in_q);
		gettimeofday(&end, NULL);
		
		millisec = timedifference_msec(start, end);
		
		if(p == -1)	
			asm volatile("nop");
		else{
			dq = bus_in_q->elements[p];
			dq.elapseTime = bus_in_q->elements[p].elapseTime + millisec;
		}

		/* to create output queues and write the dequeued msgs into them */
		switch(dq.destinationId){
			case 21:gettimeofday(&start, NULL);
				q = sq_write(bus_out_q1, dq.messageId, dq.sourceId, dq.destinationId, dq.message);
				gettimeofday(&end, NULL);
				millisec = timedifference_msec(start, end);
				if(q != -1)
					bus_out_q1->elements[q].elapseTime = bus_out_q1->elements[q].elapseTime + millisec;
				break;
			case 22:gettimeofday(&start, NULL);
				q = sq_write(bus_out_q2, dq.messageId, dq.sourceId, dq.destinationId, dq.message);
				gettimeofday(&end, NULL);
				millisec = timedifference_msec(start, end);
				if(q != -1)
					bus_out_q2->elements[q].elapseTime = bus_out_q2->elements[q].elapseTime + millisec;
				break;
			case 23:gettimeofday(&start, NULL);
				q = sq_write(bus_out_q3, dq.messageId, dq.sourceId, dq.destinationId, dq.message);
				gettimeofday(&end, NULL);
				millisec = timedifference_msec(start, end);
				if(q != -1)
					bus_out_q3->elements[q].elapseTime = bus_out_q3->elements[q].elapseTime + millisec;
				break;
			default:asm volatile("nop");
		}
		
		if(q == -1){	
			dropMsgCount++;
			msgCount++;
		}

		pthread_mutex_unlock(&lock);
		usleep(period_multiplier[4]*BASE_PERIOD);
	}
}

/* calling function when receiver threads are created */
void *receiver_function(void *ptr){
	while(msgCount < 2000){
		pthread_mutex_lock(&lock);
		struct messageThread *mt;
		mt  = (struct messageThread*)ptr;
		int p;
		struct messages rq;
		float millisec = 0;
		struct timeval start,end;
		switch(mt->destinationId){
				case 21:gettimeofday(&start, NULL);
					sq_read(bus_out_q1);
					gettimeofday(&end, NULL);
					millisec = timedifference_msec(start, end);
					if(p == -1)	
						asm volatile("nop");
					else{
						rq = bus_out_q1->elements[p];
						rq.elapseTime = bus_out_q1->elements[p].elapseTime + millisec;
					}
					break;
				case 22:gettimeofday(&start, NULL);
					sq_read(bus_out_q2);
					gettimeofday(&end, NULL);
					millisec = timedifference_msec(start, end);
					if(p == -1)	
						asm volatile("nop");
					else{
						rq = bus_out_q2->elements[p];
						rq.elapseTime = bus_out_q2->elements[p].elapseTime + millisec;
					}
					break;
				case 23:gettimeofday(&start, NULL);
					sq_read(bus_out_q3);
					gettimeofday(&end, NULL);
					millisec = timedifference_msec(start, end);
					if(p == -1)	
						asm volatile("nop");
					else{
						rq = bus_out_q3->elements[p];
						rq.elapseTime = bus_out_q3->elements[p].elapseTime + millisec;
					}
					break;
				default:asm volatile("nop");
		}
		avgElapsedTime = avgElapsedTime + millisec;
		avgstd = avgstd + millisec * millisec;
		pthread_mutex_unlock(&lock);
		switch(mt->destinationId)
		{
			case 21:usleep(period_multiplier[5]*BASE_PERIOD);break;
			case 22:usleep(period_multiplier[6]*BASE_PERIOD);break;
			case 23:usleep(period_multiplier[7]*BASE_PERIOD);break;
			default:asm volatile("nop");
		}
	}
}

int main()
{
	float mean,tempmean;
	double std;
	struct messageThread *msgS1, *msgS2, *msgS3, *msgS4;
	msgS1 = malloc(sizeof(struct messageThread *));
	msgS2 = malloc(sizeof(struct messageThread *));
	msgS3 = malloc(sizeof(struct messageThread *));
	msgS4 = malloc(sizeof(struct messageThread *));

	bus_in_q = sq_create();	
	bus_out_q1 = sq_create();
	bus_out_q2 = sq_create();
	bus_out_q3 = sq_create();

	/* Initialized with default attributes */
	pthread_attr_init (&sattr[0]);
	pthread_attr_init (&sattr[1]);
	pthread_attr_init (&sattr[2]);
	pthread_attr_init (&sattr[3]);
	pthread_attr_init (&dattr);
	pthread_attr_init (&rattr[0]);
	pthread_attr_init (&rattr[1]);
	pthread_attr_init (&rattr[2]);

	/* setting the scheduling policy */
	pthread_attr_setschedpolicy(&sattr[0], SCHED_FIFO);
	pthread_attr_setschedpolicy(&sattr[1], SCHED_FIFO);
	pthread_attr_setschedpolicy(&sattr[2], SCHED_FIFO);
	pthread_attr_setschedpolicy(&sattr[3], SCHED_FIFO);
	pthread_attr_setschedpolicy(&dattr, SCHED_FIFO);
	pthread_attr_setschedpolicy(&rattr[0], SCHED_FIFO);
	pthread_attr_setschedpolicy(&rattr[1], SCHED_FIFO);
	pthread_attr_setschedpolicy(&rattr[2], SCHED_FIFO);

	/* safe to get existing scheduling param */
	pthread_attr_getschedparam (&sattr[0], &sparam[0]);
	pthread_attr_getschedparam (&sattr[1], &sparam[1]);
	pthread_attr_getschedparam (&sattr[2], &sparam[2]);
	pthread_attr_getschedparam (&sattr[3], &sparam[3]);
	pthread_attr_getschedparam (&dattr, &dparam);
	pthread_attr_getschedparam (&rattr[0], &rparam[0]);
	pthread_attr_getschedparam (&rattr[1], &rparam[1]);
	pthread_attr_getschedparam (&rattr[2], &rparam[2]);

	/* setting the priority */
	sparam[0].sched_priority = thread_priority[0];
	sparam[1].sched_priority = thread_priority[1];
	sparam[2].sched_priority = thread_priority[2];
	sparam[3].sched_priority = thread_priority[3];
	dparam.sched_priority = thread_priority[4];
	rparam[0].sched_priority = thread_priority[5];
	rparam[1].sched_priority = thread_priority[6];
	rparam[2].sched_priority = thread_priority[7];

	/* setting the new scheduling param */
	pthread_attr_setschedparam (&sattr[0], &sparam[0]);
	pthread_attr_setschedparam (&sattr[1], &sparam[1]);
	pthread_attr_setschedparam (&sattr[2], &sparam[2]);
	pthread_attr_setschedparam (&sattr[3], &sparam[3]);
	pthread_attr_setschedparam (&dattr, &dparam);
	pthread_attr_setschedparam (&rattr[0], &rparam[0]);
	pthread_attr_setschedparam (&rattr[1], &rparam[1]);
	pthread_attr_setschedparam (&rattr[2], &rparam[2]);

	/* initializing the mutex lock */
	pthread_mutex_init(&lock,NULL);


	/* assigning id and message values for the threads */
	msgS1->sourceId = 11;
	msgS1->destinationId = 21;
	msgS2->sourceId = 12;
	msgS2->destinationId = 22;
	msgS3->sourceId = 13;
	msgS3->destinationId = 23;
	msgS4->sourceId = 14;
	msgS4->destinationId = 21;
		
	/* creating the pthreads */
	pthread_create(&senderThread[0], &sattr[0], sender_function, (void *)msgS1);
	pthread_create(&senderThread[1], &sattr[1], sender_function, (void *)msgS2);
	pthread_create(&senderThread[2], &sattr[2], sender_function, (void *)msgS3);
	pthread_create(&senderThread[3], &sattr[3], sender_function, (void *)msgS4);
	pthread_create(&daemonThread, &dattr, daemon_function, NULL);
	pthread_create(&receiverThread[0], &rattr[0], receiver_function, (void *)msgS1);
	pthread_create(&receiverThread[1], &rattr[1], receiver_function, (void *)msgS2);
	pthread_create(&receiverThread[2], &rattr[2], receiver_function, (void *)msgS3);
	
	pthread_join(senderThread[0], NULL);
	pthread_join(senderThread[1], NULL);
	pthread_join(senderThread[2], NULL);
	pthread_join(senderThread[3], NULL);	
	pthread_join(daemonThread, NULL);	
	pthread_join(receiverThread[0], NULL);
	pthread_join(receiverThread[1], NULL);
	pthread_join(receiverThread[2], NULL);

	pthread_mutex_destroy(&lock);
	
	/* calculation of mean and standard deviation of elapsed times */
	mean = avgElapsedTime / 2000;
	tempmean = 2000 * mean * mean;
	std = (avgstd + tempmean - 2 * mean * avgElapsedTime);
	if (std < 0.0)
		std = std * -1;	
	std = sqrt(std);
	std = std / sqrt(2000);
	
	/* final output */
	printf("No of dropped messages = %d\n",dropMsgCount);	
	printf("Average Elapsed Time = %f milliseconds\n",mean);
	printf("Standard Deviation of Elapsed Time = %.10lf milliseconds\n",std);
	sq_delete(bus_in_q);
	sq_delete(bus_out_q1);
	sq_delete(bus_out_q2);
	sq_delete(bus_out_q3);
	return 0;		
}
