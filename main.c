#include "FreeRTOS.h"
//#include "atomic.h"
//#include "croutine.h"
//#include "deprecated_definitions.h"
//#include "event_groups.h"
//#include "list.h"
//#include "message_buffer.h"
//#include "portable.h"
//#include "portmacro.h"
#include "queue.h"
#include "semphr.h"
//#include "stack_macros.h"
//#include "StackMacros.h"
//#include "stream_buffer.h"
#include "task.h"
#include "timers.h"
#include <stdio.h>
//made global because they need to be accessed by the init function
int senderCounter=0;	//counter for sent messages by task 1
int blockedCounter=0;  //counter for blocked messages
int receiverCounter=0; //counter for received messages
//handles for used APIS
TaskHandle_t senderTask=NULL;
TaskHandle_t recieverTask=NULL;
QueueHandle_t queue;
SemaphoreHandle_t senderSem;
SemaphoreHandle_t recieverSem;
TimerHandle_t senderTimer;
TimerHandle_t recieverTimer;
/*
 * Task name: task1
 * description: sending message to queue (if there's available space)
 * 			 when senderTimer expires until then it's blocked over a semaphore
 * 			 if there's no available space in the queue it increments a counter for blocked messages
 * arguments: void
 * returns: void
 */
void task1(void *pvParameters)
{
	/*printf("we entered task 1\n");
	fflush(stdout);
	xSemaphoreTake(senderSem, portMAX_DELAY );*/

	int tickCounter =0; 	//counter for current time of the system since the schedular started

	char message_sent [15]; //string for message

	while (1)
	{	printf("this is task 1\n");
		fflush(stdout);
		xSemaphoreTake(senderSem, portMAX_DELAY ); /* we take the semaphore and block the task
													indefinitely til the semaphore is released */
		printf("semaphore1 taken\n");
		fflush(stdout);
		tickCounter=xTaskGetTickCount(); //get the time in ticks since the start of the schedular

		/*the following line is used to append a string with a variable */
		snprintf( message_sent, sizeof(message_sent),"Time is %c", tickCounter);

		/*if the queue has avaliable space then send a message to it
		 * and increment the number of sent tasks if not increment the number of
		 * blocked messages
		 */
		if ( xQueueSend(queue, (void *) &message_sent, (TickType_t)0 ))
		{
			senderCounter++;
			printf("sent messages= %d\n", senderCounter);
			fflush(stdout);
		}
		else
		{
			blockedCounter++;
			printf("blocked messages= %d\n", blockedCounter);
			fflush(stdout);
			printf("the number of messages in queue %d\n", uxQueueMessagesWaiting(queue));
			fflush(stdout);
		}
		message_sent[0] = '\0'; //resetting the message string for the next message

	}
}
/*
 * Task name: task2
 * description: receiving message into queue (if there's available space)
 * 			 when receiverTimer expires until then it's blocked over a semaphore
 *
 * arguments: void
 * returns: void
 */
void task2(void *pvParameters)
{

	/*printf("222222\n");
	fflush(stdout);
	xSemaphoreTake(recieverSem, portMAX_DELAY );*/
	char message_recieved [15];
	while (1)
	{
		xSemaphoreTake(recieverSem, portMAX_DELAY );
		printf("semaphore2 taken\n");
		fflush(stdout);
		/*if there are available messages in the queue read them */
		if (xQueueReceive(queue, &message_recieved,(TickType_t)0 ) == pdTRUE)
		{
			receiverCounter++;
			printf("received messages= %d\n", receiverCounter);
			fflush(stdout);

		}

	}
}
/*
 * function name: init
 * description: function used to update the period of senderTimer
 * 				when the available periods are done, the system is resetted by deleting timers and
 * 				stopping the schedular
 * 				if there's still available periods, it resets the reciever counter and prints the number of sent
 * 				and blocked messages and resets the queue
 * arguments: void
 * returns: int
 */
int init (void)
{
	static int i =-1;
	i++;
	if (i == 6)
	{ 	//delete timers
		xTimerDelete( recieverTimer,(TickType_t)0  );
		xTimerDelete( senderTimer,(TickType_t)0  );
		printf("Game Over");
		vTaskEndScheduler ();
		return 0;
	}
	receiverCounter =0;
	int Tsender[6] = {100, 140, 180, 220, 260, 300};
	printf("the number of sent messages %d\n", senderCounter);
	fflush(stdout);
	printf("the number of blocked messages %d\n", blockedCounter);
	fflush(stdout);
	xQueueReset( queue );
	return Tsender[i];
}
/*
* function name: senderTimerCallBackFun
 * description: this function is called when the senderTimer expires
 * 				then it releases the semaphore for task1
 * arguments: handle of senderTimer
 * returns: void
 */
void senderTimerCallBackFun(TimerHandle_t senderTimer)
{
	printf("timer 1 cbf\n");
	fflush(stdout);
	xSemaphoreGive(senderSem);
}
/*
* function name: recieverTimerCallBackFun
 * description: this function is called when the recieverTimer expires
 * 				then it releases the semaphore for task2
 * 				when the number of recieved messages is 500 it calls init function
 * 				to change the period of the senderTimer
 * arguments: handle of recieverTimer
 * returns: void
 */
void recieverTimerCallBackFun(TimerHandle_t recieverTimer)
{
	xSemaphoreGive(recieverSem);
	if (receiverCounter == 500)
	{
		int time = init();
		xTimerChangePeriod(senderTimer, time,(TickType_t)0  );



	}
}

int main (void)
{

	//create a queue for messages

	queue= xQueueCreate( 2, sizeof(int));
	//call init function at the beginning of the program to get the first sleeping period
	int time = init();
	//create blocking semaphores
	senderSem= xSemaphoreCreateBinary();
	recieverSem= xSemaphoreCreateBinary();
	//creating and starting timers
	senderTimer=xTimerCreate  ( "sender timer",	time, pdTRUE, NULL, senderTimerCallBackFun );
	xTimerStart( senderTimer, 0 );
	recieverTimer=xTimerCreate  ( "recievertimer",200, pdTRUE, NULL, recieverTimerCallBackFun );
	xTimerStart( recieverTimer, 0 );
	//creation of tasks
	 xTaskCreate(task1, "Task1",1000, NULL,2, &senderTask);
	 xTaskCreate(task2, "Task2", 1000, NULL,2, &recieverTask);
/*	printf("wtf\n");
	fflush(stdout);
	if(xReturned == pdPASS)
	{
		printf("here\n");
		fflush(stdout);
	}
	 if(xReturn == pdPASS)
		{
			printf("here\n");
			fflush(stdout);
		}

	printf("i am here\n");
	fflush(stdout);*/
	vTaskStartScheduler();
	while (1)
	{

	}


}
