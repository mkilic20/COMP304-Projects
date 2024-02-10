#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>

#define overloadLimit 10
#define systemLimit 100

//They are integers we used it to debug
int createdACounter = 0;
int totalACounter = 0;
int createdBCounter = 0;
int totalBCounter = 0;
int createdECounter = 0;
int totalECounter = 0;
int createdFCounter = 0;
int totalFCounter = 0;
int totalSec = 0;
//It is a train structure.
typedef struct {
    int length;
    int speed;
    int pos;
    int ID;
} Train;

//It is a metro line, for example fromA to C fromA
typedef struct {
    Train trains[systemLimit];
    int totalcount;
    pthread_mutex_t mutex;
} TrainQueue;

//I created it to use when initializing thread. I pass it as argumeter to thread function.
typedef struct {
    void* queueFrom;
    void* queueTo;
} Direction;

//I created it to use when initializing thread. I pass it as argumeter to thread function.
typedef struct {
    void* queueFrom;
    void* queue1;
    void* queue2;
    void* queue3;
    void* queue4;
    void* queue5;
    void* queue6;
    void* queue7;
    void* queue8;
} FromDirection;

//I created it to use when initializing thread. I pass it as argumeter to thread function.
typedef struct {
    void* queue1;
    void* queue2;
    void* queue3;
    void* queue4;
} WaitingQueues;

//It initializes queue
void initializeQueue(TrainQueue* queue) {
    queue->totalcount = 0;
    pthread_mutex_init(&queue->mutex, NULL);
}

//Enqueue function
void enqueue(TrainQueue* queue, Train train) {
    pthread_mutex_lock(&queue->mutex);
    if (queue->totalcount < systemLimit) {
        queue->trains[queue->totalcount++] = train;
    } else {
        printf("Error, queue is overfull\n");
    }
    pthread_mutex_unlock(&queue->mutex);
}

//Dequeue function
Train dequeue(TrainQueue* queue) {
    pthread_mutex_lock(&queue->mutex);
    Train train = queue->trains[0];
    for (int i = 1; i < queue->totalcount; ++i) {
        queue->trains[i - 1] = queue->trains[i];
    }
    queue->totalcount--;
    pthread_mutex_unlock(&queue->mutex);
    return train;
}

//train passing refers to train which passes to tunnel now.
//train pass_A, B, E, F refers to train which departed from A, B, E, F lines.
Train trainPassing;
Train trainPass_A;
Train trainPass_B;
Train trainPass_E;
Train trainPass_F;
sem_t sem_log;
sem_t bd_log;
sem_t ol_log;
sem_t tc_log;
sem_t tunnel_log;
sem_t tunnel_sem;
sem_t controllog;
sem_t pass_recorded;
sem_t dep_recordedA;
sem_t dep_recordedB;
sem_t dep_recordedE;
sem_t dep_recordedF;
sem_t pass_recordedA;
sem_t pass_recordedB;
sem_t pass_recordedE;
sem_t pass_recordedF;

//It checks(look for a message from A line if there is departure and if there is departure it write it to train log 
void departureTimeA()
{
    while (1) {
    sem_wait(&dep_recordedA);
    //printf("Dep Recorded A online");
    sem_wait(&tunnel_log);
    //printf("Dep Recorded A aktif");
    int id = trainPass_A.ID; 
    char departure_time[80]; 
    char destpoint = 'A';
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    char inputsec[400];
    int k = snprintf(inputsec, 70,
                 "    \t    %c, \t \t ", destpoint);
    
    strftime(departure_time, sizeof(departure_time), "%H:%M:%S ", timeinfo);
    strcat(inputsec, departure_time);
    strcat(inputsec, " ]");
    //printf("INPUTSEC: %s \n", inputsec);
    FILE* ptr;
    FILE* ptr2;
    char str[4096];
    char strtop[4096];
    char buffer[4096];
    char input[4096];
    int j = snprintf(input, 40,
                 " %d,", id);
    
   
    char* pointer;
    char total[4096] = "";
    strcat(input, " "); //e.g. l 'space'
    char *aliases[100]; 
    char *repair[100];
        ptr = fopen("trainlog.txt", "r"); 
    if (NULL == ptr) {
        perror("Cannot open the file!!! \n");
    }
    while (fgets(str, 4096, ptr) != NULL) { 
        strcat(strtop,str);
    }
    fclose(ptr);

    char *token = strtok(strtop, "\n");
    int counter = 0;
    while (token != NULL) {
        aliases[counter] = strdup(token); 
        token = strtok(NULL, "\n"); 
        if (aliases[counter] == NULL) {
            perror("Error occurred while allocating memory!!!");
            exit(1);
        }
        counter++;
    }
    for (int i = 0; i < counter; i++) {
        pointer = strstr(aliases[i], input);
        //printf("anaaaaaaaa bulduu");
        if(pointer != NULL)
        {
	    char *token2 = strtok(aliases[i], "W");
	    int counter2 = 0;
        while (token2 != NULL) { 
        repair[counter2] = strdup(token2); 
        token2 = strtok(NULL, "W"); 
        if (repair[counter2] == NULL) {
            perror("Error occurred while allocating memory!!!");
            exit(1);
        }
        counter2++;
        }
        //printf("Repair 0 : %s \n", repair[0]);
        //printf("Repair 1 : %s \n", repair[1]);
        if(strcmp(repair[1], " ] ") == 0)
        {
            repair[1] = inputsec;
        }
        //repair[1] = inputsec;
        
        strcat(buffer, repair[0]); //e.g. input = l mkdir (change l with mkdir)
        strcat(buffer, repair[1]);
        aliases[i] = buffer; //changes l ls to l mkdir
}
        strcat(total, aliases[i]); //total is all the strings except the changed line l ls (is now l mkdir)
        strcat(total, "\n");
    }
        ptr2= fopen("trainlog.txt", "w");//open recorder.txt as write not append 
        fprintf(ptr2, "%s", total); //write total into ptr2
        fclose(ptr2);
        memset(departure_time, '\0', sizeof(departure_time));
        memset(inputsec, '\0', sizeof(inputsec));
        memset(str, '\0', sizeof(str));
        memset(strtop, '\0', sizeof(strtop));
        memset(buffer, '\0', sizeof(buffer));
        memset(total, '\0', sizeof(total));
        sem_post(&pass_recordedA);
        sem_post(&tunnel_log);
    }
}
//It checks(look for a message from B line if there is departure and if there is departure it write it to train log 
void departureTimeB()
{
    while (1) {
    sem_wait(&dep_recordedB);
    //printf("Dep Recorded B online");
    sem_wait(&tunnel_log);
    //printf("Dep Recorded B aktif");
    int id = trainPass_B.ID; 
    char departure_time[80]; 
    char destpoint = 'B';
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    char inputsec[400];
    int k = snprintf(inputsec, 70,
                 "    \t    %c, \t \t ", destpoint);
    
    strftime(departure_time, sizeof(departure_time), "%H:%M:%S ", timeinfo);
    strcat(inputsec, departure_time);
    strcat(inputsec, " ]");
    //printf("INPUTSEC: %s \n", inputsec);
    FILE* ptr; //read
    FILE* ptr2; //write
    char str[4096];
    char strtop[4096];
    char buffer[4096];
    char input[4096];
    int j = snprintf(input, 40,
                 " %d,", id);
    
   
    char* pointer;
    char total[4096] = "";
    strcat(input, " "); //e.g. l 'space'
    char *aliases[100]; 
    char *repair[100];
        ptr = fopen("trainlog.txt", "r"); 
    if (NULL == ptr) {
        perror("Cannot open the file!!! \n");
    }
    while (fgets(str, 4096, ptr) != NULL) { 
        strcat(strtop,str); //strtop contains all string in file
    }
    fclose(ptr);

    char *token = strtok(strtop, "\n"); //divide from line get the tokens
    int counter = 0;
    while (token != NULL) { //traverse all tokens
        aliases[counter] = strdup(token); 
        token = strtok(NULL, "\n"); 
        if (aliases[counter] == NULL) {
            perror("Error occurred while allocating memory!!!");
            exit(1);
        }
        counter++;
    }
    for (int i = 0; i < counter; i++) {
        //printf("Aliases %d, : %s \n", i, aliases[i]);
        pointer = strstr(aliases[i], input); //look for 'l ' in the line  (l 'space')
        //printf("anaaaaaaaa bulduu");
        if(pointer != NULL)
        {
	    char *token2 = strtok(aliases[i], "W");
	    int counter2 = 0;
        while (token2 != NULL) { //traverse all tokens
        repair[counter2] = strdup(token2); 
        token2 = strtok(NULL, "W"); 
        if (repair[counter2] == NULL) {
            perror("Error occurred while allocating memory!!!");
            exit(1);
        }
        counter2++;
        }
        //printf("Repair 0 : %s \n", repair[0]);
        //printf("Repair 1 : %s \n", repair[1]);
        if(strcmp(repair[1], " ] ") == 0)
        {
            repair[1] = inputsec;
        }
        //repair[1] = inputsec;
        
        strcat(buffer, repair[0]); //e.g. input = l mkdir (change l with mkdir)
        strcat(buffer, repair[1]);
        aliases[i] = buffer; //changes l ls to l mkdir
}
        strcat(total, aliases[i]); //total is all the strings except the changed line l ls (is now l mkdir)
        strcat(total, "\n");
    }
        ptr2= fopen("trainlog.txt", "w");//open recorder.txt as write not append 
        fprintf(ptr2, "%s", total); //write total into ptr2
        fclose(ptr2);
        memset(departure_time, '\0', sizeof(departure_time));
        memset(inputsec, '\0', sizeof(inputsec));
        memset(str, '\0', sizeof(str));
        memset(strtop, '\0', sizeof(strtop));
        memset(buffer, '\0', sizeof(buffer));
        memset(total, '\0', sizeof(total));
        sem_post(&pass_recordedB);
        sem_post(&tunnel_log);
    }
}
//It checks(look for a message from E line if there is departure and if there is departure it write it to train log 
void departureTimeE()
{
    while (1) {
    sem_wait(&dep_recordedE);
    //printf("Dep Recorded E online");
    sem_wait(&tunnel_log);
    //printf("Dep Recorded E aktif");
    int id = trainPass_E.ID; 
    char departure_time[80]; 
    char destpoint = 'E';
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    char inputsec[400];
    int k = snprintf(inputsec, 70,
                 "    \t    %c, \t \t ", destpoint);
    
    strftime(departure_time, sizeof(departure_time), "%H:%M:%S ", timeinfo);
    strcat(inputsec, departure_time);
    strcat(inputsec, " ]");
    //printf("INPUTSEC: %s \n", inputsec);
    FILE* ptr; //read
    FILE* ptr2; //write
    char str[4096];
    char strtop[4096];
    char buffer[4096];
    char input[4096];
    int j = snprintf(input, 40,
                 " %d,", id);
    
   
    char* pointer;
    char total[4096] = "";
    strcat(input, " "); //e.g. l 'space'
    char *aliases[100]; 
    char *repair[100];
        ptr = fopen("trainlog.txt", "r"); 
    if (NULL == ptr) {
        perror("Cannot open the file!!! \n");
    }
    while (fgets(str, 4096, ptr) != NULL) { 
        strcat(strtop,str); //strtop contains all string in file
    }
    fclose(ptr);

    char *token = strtok(strtop, "\n"); //divide from line get the tokens
    int counter = 0;
    while (token != NULL) { //traverse all tokens
        aliases[counter] = strdup(token); 
        token = strtok(NULL, "\n"); 
        if (aliases[counter] == NULL) {
            perror("Error occurred while allocating memory!!!");
            exit(1);
        }
        counter++;
    }
    for (int i = 0; i < counter; i++) {
        //printf("Aliases %d, : %s \n", i, aliases[i]);
        pointer = strstr(aliases[i], input); //look for 'l ' in the line  (l 'space')
        //printf("anaaaaaaaa bulduu");
        if(pointer != NULL)
        {
	    char *token2 = strtok(aliases[i], "W");
	    int counter2 = 0;
        while (token2 != NULL) { //traverse all tokens
        repair[counter2] = strdup(token2); 
        token2 = strtok(NULL, "W"); 
        if (repair[counter2] == NULL) {
            perror("Error occurred while allocating memory!!!");
            exit(1);
        }
        counter2++;
        }
        //printf("Repair 0 : %s \n", repair[0]);
        //printf("Repair 1 : %s \n", repair[1]);
        if(strcmp(repair[1], " ] ") == 0)
        {
            repair[1] = inputsec;
        }
        //repair[1] = inputsec;
        
        strcat(buffer, repair[0]); //e.g. input = l mkdir (change l with mkdir)
        strcat(buffer, repair[1]);
        aliases[i] = buffer; //changes l ls to l mkdir
}
        strcat(total, aliases[i]); //total is all the strings except the changed line l ls (is now l mkdir)
        strcat(total, "\n");
    }
        ptr2= fopen("trainlog.txt", "w");//open recorder.txt as write not append 
        fprintf(ptr2, "%s", total); //write total into ptr2
        fclose(ptr2);
        memset(departure_time, '\0', sizeof(departure_time));
        memset(inputsec, '\0', sizeof(inputsec));
        memset(str, '\0', sizeof(str));
        memset(strtop, '\0', sizeof(strtop));
        memset(buffer, '\0', sizeof(buffer));
        memset(total, '\0', sizeof(total));
        sem_post(&pass_recordedE);
        sem_post(&tunnel_log);
    }
}
//It checks(look for a message from F line if there is departure and if there is departure it write it to train log 
void departureTimeF()
{
    while (1) {
    sem_wait(&dep_recordedF);
    //printf("Dep Recorded F online");
    sem_wait(&tunnel_log);
    //printf("Dep Recorded F aktif");
    int id = trainPass_F.ID; 
    char departure_time[80]; 
    char destpoint = 'F';
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    char inputsec[400];
    int k = snprintf(inputsec, 70,
                 "    \t    %c, \t \t ", destpoint);
    
    strftime(departure_time, sizeof(departure_time), "%H:%M:%S ", timeinfo);
    strcat(inputsec, departure_time);
    strcat(inputsec, " ]");
    //printf("INPUTSEC: %s \n", inputsec);
    FILE* ptr; //read
    FILE* ptr2; //write
    char str[4096];
    char strtop[4096];
    char buffer[4096];
    char input[4096];
    int j = snprintf(input, 40,
                 " %d,", id);
    
   
    char* pointer;
    char total[4096] = "";
    strcat(input, " "); //e.g. l 'space'
    char *aliases[100]; 
    char *repair[100];
        ptr = fopen("trainlog.txt", "r"); 
    if (NULL == ptr) {
        perror("Cannot open the file!!! \n");
    }
    while (fgets(str, 4096, ptr) != NULL) { 
        strcat(strtop,str); //strtop contains all string in file
    }
    fclose(ptr);

    char *token = strtok(strtop, "\n"); //divide from line get the tokens
    int counter = 0;
    while (token != NULL) { //traverse all tokens
        aliases[counter] = strdup(token); 
        token = strtok(NULL, "\n"); 
        if (aliases[counter] == NULL) {
            perror("Error occurred while allocating memory!!!");
            exit(1);
        }
        counter++;
    }
    for (int i = 0; i < counter; i++) {
        //printf("Aliases %d, : %s \n", i, aliases[i]);
        pointer = strstr(aliases[i], input); //look for 'l ' in the line  (l 'space')
        //printf("anaaaaaaaa bulduu");
        if(pointer != NULL)
        {
	    char *token2 = strtok(aliases[i], "W");
	    int counter2 = 0;
        while (token2 != NULL) { //traverse all tokens
        repair[counter2] = strdup(token2); 
        token2 = strtok(NULL, "W"); 
        if (repair[counter2] == NULL) {
            perror("Error occurred while allocating memory!!!");
            exit(1);
        }
        counter2++;
        }
        //printf("Repair 0 : %s \n", repair[0]);
        //printf("Repair 1 : %s \n", repair[1]);
        if(strcmp(repair[1], " ] ") == 0)
        {
            repair[1] = inputsec;
        }
        //repair[1] = inputsec;
        
        strcat(buffer, repair[0]); //e.g. input = l mkdir (change l with mkdir)
        strcat(buffer, repair[1]);
        aliases[i] = buffer; //changes l ls to l mkdir
}
        strcat(total, aliases[i]); //total is all the strings except the changed line l ls (is now l mkdir)
        strcat(total, "\n");
    }
        ptr2= fopen("trainlog.txt", "w");//open recorder.txt as write not append 
        fprintf(ptr2, "%s", total); //write total into ptr2
        fclose(ptr2);
        memset(departure_time, '\0', sizeof(departure_time));
        memset(inputsec, '\0', sizeof(inputsec));
        memset(str, '\0', sizeof(str));
        memset(strtop, '\0', sizeof(strtop));
        memset(buffer, '\0', sizeof(buffer));
        memset(total, '\0', sizeof(total));
        sem_post(&pass_recordedF);
        sem_post(&tunnel_log);
    }
}

//It is a tunnel thread, it executes tunnel line
//It search for the largest waiting queue in queues, and look at priority if tie
//After that it decide where to go and check if breakdown is happened.
//Also it send signal to logs if there is breakdown, tunnelpassing
void* tunnelThread(void* arg) {
    FromDirection* direction = (FromDirection*)arg;
    TrainQueue* queue;
    TrainQueue* queueTo;
    
    TrainQueue* queue1 = direction->queue1;
    TrainQueue* queue2 = direction->queue2;
    TrainQueue* queue3 = direction->queue3;
    TrainQueue* queue4 = direction->queue4;
    TrainQueue* queue5 = direction->queue5;
    TrainQueue* queue6 = direction->queue6;
    TrainQueue* queue7 = direction->queue7;
    TrainQueue* queue8 = direction->queue8;
    TrainQueue* queues[4] = {queue1, queue2, queue3, queue4};
    while (1) {
        sem_wait(&tunnel_sem);
        int pos;
        int max = 0;
        int leftOrRight;
        int line;
        for(int i = 0; i < 4; i++)
        {
            if(queues[i]->totalcount >= max)
            {
                max = queues[i]->totalcount;
                line = i;
                //printf("max is chosen to %d \n", max);
            }
        }
        if(max == 0)
        {
            
        }
        else
    {    
        if(queue1->totalcount==max)
        {
            queue = queue1;
            leftOrRight = 1;
            //printf("A'dan girdi tunele \n");
        }
        else if(queue2->totalcount==max)
        {
            queue = queue2;
            leftOrRight = 1;
            //printf("B'den girdi tunele \n");
        }
        else if(queue3->totalcount==max)
        {
            queue = queue3;
            leftOrRight = 0;
            //printf("E'den girdi tunele \n");
        }
        else if(queue4->totalcount==max)
        {
            queue = queue4;
            leftOrRight = 0;
            //printf("F'den girdi tunele \n");
        }
        else
        {
            for(int i = 0; i < 4; i++)
        {
            if(queues[i]->totalcount > max)
            {
                max = queues[i]->totalcount;
                //printf("max is chosen to %d \n", max);
            }
        }
            continue;
        }
    }
    
	
        if(queue != NULL && queue->totalcount > 0) {
        if(leftOrRight == 1)
        {
            //srand(time(NULL));
            if((((rand() % 2) + 1) == 1))
            {
                queueTo = queue7;
                pos = 7;
                //printf("E'yi secti \n");
            }
            else
            {
                queueTo = queue8;
                pos = 8;
                //printf("F'i secti \n");
            }
        }
        else
        {
            //srand(time(NULL));
            if((((rand() % 2) + 1) == 1))
            {
                queueTo = queue5;
                pos = 7;
                //printf("A'yi secti \n");
            }
            else
            {
                queueTo = queue6;
                pos = 8;
                //printf("B'i secti \n");
            }
        }
        
	   
            sem_wait(&pass_recorded);
            Train passengerTrain = dequeue(queue);
            totalACounter++;
            passengerTrain.pos = pos;
            int lengthofTun = passengerTrain.length;
            int passedTime = lengthofTun / passengerTrain.speed;
            passedTime++;
            if(((rand() % 10) + 1) == 10)
            {
                passedTime = passedTime +4;
                //printf("BREAKDOWN'A GIRDI INANMIYORUMMMMMMMMMM \n");
                trainPassing = passengerTrain;
                sem_post(&bd_log);
            }
            

            sleep(passedTime);
            trainPassing = passengerTrain;
            sem_post(&sem_log);
            
            enqueue(queueTo, passengerTrain);
            totalACounter--;
        }
        sem_post(&tunnel_sem);
    }
    return NULL;
}
//It takes event time, passing train ID, and trains waiting passage, and record(append) it to controllog when a train is passed in tunnel
void centerLogRecorder(void* arg)
{
    WaitingQueues* waitQue = (WaitingQueues*)arg;
    TrainQueue* queue1 = waitQue->queue1;
    TrainQueue* queue2 = waitQue->queue2;
    TrainQueue* queue3 = waitQue->queue3;
    TrainQueue* queue4 = waitQue->queue4;
    while (1) {
    sem_wait(&sem_log);
    sem_wait(&controllog);
    //printf("Biri tunel'den gecti sonunda");
    //printf("  ID : %d, ", trainPassing.ID);
    fflush(stdout);
    char result[1000] = "";
    char result1[1000] = "";
    char result2[1000] = "";
    char result3[1000] = "";
    for (int i = 0; i < queue1->totalcount; i++) {
    char id[100];
    sprintf(id, "%d", queue1->trains[i].ID);
    strcat(result, id);
        if (i != queue1->totalcount - 1) 
        {
            strcat(result, ", ");
        }
    }
    for (int i = 0; i < queue2->totalcount; i++) {
    char id[100];
    sprintf(id, "%d", queue2->trains[i].ID);
    strcat(result1, id);
        if (i != queue2->totalcount - 1) 
        {
            strcat(result1, ", ");
        }
    }
    for (int i = 0; i < queue3->totalcount; i++) {
    char id[100];
    sprintf(id, "%d", queue3->trains[i].ID);
    strcat(result2, id);
        if (i != queue3->totalcount - 1) 
        {
            strcat(result2, ", ");
        }
    }
    for (int i = 0; i < queue4->totalcount; i++) {
    char id[100];
    sprintf(id, "%d", queue4->trains[i].ID);
    strcat(result3, id);
        if (i != queue4->totalcount - 1) 
        {
            strcat(result3, ", ");
        }
    }
    FILE *file = fopen("controllog.txt", "a");
                time_t rawtime;
                struct tm *timeinfo;
                char buffer[800];
                memset(buffer, '\0', sizeof(buffer));
                char bufferID[80];
                char buffer3[80] = " ";
                char buffer2[80];
                time(&rawtime);
                timeinfo = localtime(&rawtime);
                int j = snprintf(bufferID, 50,
                 "\t \t %d,\t \t \t", trainPassing.ID);
                strftime(buffer2, sizeof(buffer), " \t    %H:%M:%S ", timeinfo);
                strcat(buffer3, buffer2);
                //printf("BUFFER BUDUR : %s \n", buffer);
                strcat(buffer, "Tunnel Passing ");
                
                strcat(buffer, buffer3);
                strcat(buffer, bufferID);
                strcat(buffer, " ");
                
                if(strcmp(result1, "") != 0) 
                {
                    if(strcmp(result, "") != 0)
                {
                    strcat(result, ", ");
                }
                    strcat(result, result1);
                }
               
                if(strcmp(result2, "") != 0)
                {
                     if(strcmp(result, "") != 0)
                {
                    strcat(result, ", ");
                }
                    strcat(result, result2);
                    
                }
                
                if(strcmp(result3, "") != 0)
                {
                    if(strcmp(result, "") != 0)
                {
                    strcat(result, ", ");
                }
                    strcat(result, result3);
                }

                strcat(buffer, result);
                //strcat(buffer, buffer4);
                fprintf(file, "[%s] \n", buffer);
                memset(buffer, '\0', sizeof(buffer));
                memset(bufferID, '\0', sizeof(bufferID));
                memset(buffer3, '\0', sizeof(buffer3));
                memset(buffer2, '\0', sizeof(buffer2));
                memset(result, '\0', sizeof(result));
                memset(result1, '\0', sizeof(result));
                memset(result2, '\0', sizeof(result));
                memset(result3, '\0', sizeof(result));

                fclose(file);
        //sleep(0.1f);
        sem_post(&pass_recorded);
        sem_post(&controllog);
        
    }
}
//It takes event time, passing train ID, and trains waiting passage, and record(append) it to controllog when breakdown happened
void breakDownRecorder(void* arg)
{
    WaitingQueues* waitQue = (WaitingQueues*)arg;
    TrainQueue* queue1 = waitQue->queue1;
    TrainQueue* queue2 = waitQue->queue2;
    TrainQueue* queue3 = waitQue->queue3;
    TrainQueue* queue4 = waitQue->queue4;
    while (1) {
    sem_wait(&bd_log);
    sem_wait(&controllog);
    //printf("Biri breakdown oldu");
    //printf("  ID : %d, ", trainPassing.ID);
    fflush(stdout);
    char result[1000] = "";
    char result1[1000] = "";
    char result2[1000] = "";
    char result3[1000] = "";
    for (int i = 0; i < queue1->totalcount; i++) {
    char id[100];
    sprintf(id, "%d", queue1->trains[i].ID);
    strcat(result, id);
        if (i != queue1->totalcount - 1) 
        {
            strcat(result, ", ");
        }
    }
    for (int i = 0; i < queue2->totalcount; i++) {
    char id[100];
    sprintf(id, "%d", queue2->trains[i].ID);
    strcat(result1, id);
        if (i != queue2->totalcount - 1) 
        {
            strcat(result1, ", ");
        }
    }
    for (int i = 0; i < queue3->totalcount; i++) {
    char id[100];
    sprintf(id, "%d", queue3->trains[i].ID);
    strcat(result2, id);
        if (i != queue3->totalcount - 1) 
        {
            strcat(result2, ", ");
        }
    }
    for (int i = 0; i < queue4->totalcount; i++) {
    char id[100];
    sprintf(id, "%d", queue4->trains[i].ID);
    strcat(result3, id);
        if (i != queue4->totalcount - 1) 
        {
            strcat(result3, ", ");
        }
    }
    FILE *file = fopen("controllog.txt", "a");
                time_t rawtime;
                struct tm *timeinfo;
                char buffer[800];
                char bufferID[80];
                memset(buffer, '\0', sizeof(buffer));
                char buffer3[80] = " ";
                char buffer2[80];
                time(&rawtime);
                timeinfo = localtime(&rawtime);
                int j = snprintf(bufferID, 60,
                 "%d, \t \t", trainPassing.ID);
                strftime(buffer2, sizeof(buffer), "\t \t%H:%M:%S \t \t ", timeinfo);
                strcat(buffer3, buffer2);
                strcat(buffer, "Breakdown      ");
                
                strcat(buffer, buffer3);
                strcat(buffer, bufferID);
                strcat(buffer, " ");
                
                 if(strcmp(result1, "") != 0) 
                {
                    if(strcmp(result, "") != 0)
                {
                    strcat(result, ", ");
                }
                    strcat(result, result1);
                }
               
                if(strcmp(result2, "") != 0)
                {
                     if(strcmp(result, "") != 0)
                {
                    strcat(result, ", ");
                }
                    strcat(result, result2);
                    
                }
                
                if(strcmp(result3, "") != 0)
                {
                    if(strcmp(result, "") != 0)
                {
                    strcat(result, ", ");
                }
                    strcat(result, result3);
                }

                strcat(buffer, result);
                //strcat(buffer, buffer4);
                fprintf(file, "[%s] \n", buffer);
                memset(buffer, '\0', sizeof(buffer));
                memset(bufferID, '\0', sizeof(bufferID));
                memset(buffer3, '\0', sizeof(buffer3));
                memset(buffer2, '\0', sizeof(buffer2));
                memset(result, '\0', sizeof(result));
                memset(result1, '\0', sizeof(result));
                memset(result2, '\0', sizeof(result));
                memset(result3, '\0', sizeof(result));
                fclose(file);
                sem_post(&controllog);
                //sleep(0.1f);
    }
}
//It takes event time, passing train ID, and trains waiting passage, and record(append) it to controllog when overload happened
void overloadRecorder(void* arg)
{
    WaitingQueues* waitQue = (WaitingQueues*)arg;
    TrainQueue* queue1 = waitQue->queue1;
    TrainQueue* queue2 = waitQue->queue2;
    TrainQueue* queue3 = waitQue->queue3;
    TrainQueue* queue4 = waitQue->queue4;
    while (1) {
    sem_wait(&ol_log);
    sem_wait(&controllog);
    //printf("Biri overload oldu");
    //printf("  ID : %d, ", trainPassing.ID);
    fflush(stdout);
    char result[1000] = "";
    char result1[1000] = "";
    char result2[1000] = "";
    char result3[1000] = "";
    for (int i = 0; i < queue1->totalcount; i++) {
    char id[100];
    sprintf(id, "%d", queue1->trains[i].ID);
    strcat(result, id);
        if (i != queue1->totalcount - 1) 
        {
            strcat(result, ", ");
        }
    }
    for (int i = 0; i < queue2->totalcount; i++) {
    char id[100];
    sprintf(id, "%d", queue2->trains[i].ID);
    strcat(result1, id);
        if (i != queue2->totalcount - 1) 
        {
            strcat(result1, ", ");
        }
    }
    for (int i = 0; i < queue3->totalcount; i++) {
    char id[100];
    sprintf(id, "%d", queue3->trains[i].ID);
    strcat(result2, id);
        if (i != queue3->totalcount - 1) 
        {
            strcat(result2, ", ");
        }
    }
    for (int i = 0; i < queue4->totalcount; i++) {
    char id[100];
    sprintf(id, "%d", queue4->trains[i].ID);
    strcat(result3, id);
        if (i != queue4->totalcount - 1) 
        {
            strcat(result3, ", ");
        }
    }
    FILE *file = fopen("controllog.txt", "a");
                time_t rawtime;
                struct tm *timeinfo;
                char buffer[800];
                char bufferID[80];
                char buffer3[80] = " ";
                char buffer2[80];
                time(&rawtime);
                timeinfo = localtime(&rawtime);
                int j = snprintf(bufferID, 30,
                 "%d, \t ", trainPassing.ID);
                strftime(buffer2, sizeof(buffer), "\t \t%H:%M:%S\t\t ", timeinfo);
                strcat(buffer3, buffer2);
                memset(buffer, '\0', sizeof(buffer));
                strcat(buffer, "System Overload ");
                
                strcat(buffer, buffer3);
                strcat(buffer, bufferID);
                strcat(buffer, " ");
                if(strcmp(result1, "") != 0) 
                {
                    if(strcmp(result, "") != 0)
                {
                    strcat(result, ", ");
                }
                    strcat(result, result1);
                }
               
                if(strcmp(result2, "") != 0)
                {
                     if(strcmp(result, "") != 0)
                {
                    strcat(result, ", ");
                }
                    strcat(result, result2);
                    
                }
                
                if(strcmp(result3, "") != 0)
                {
                    if(strcmp(result, "") != 0)
                {
                    strcat(result, ", ");
                }
                    strcat(result, result3);
                }

                strcat(buffer, result);
                //strcat(buffer, buffer4);
                fprintf(file, "[%s] \n", buffer);
                memset(buffer, '\0', sizeof(buffer));
                memset(bufferID, '\0', sizeof(bufferID));
                memset(buffer3, '\0', sizeof(buffer3));
                memset(buffer2, '\0', sizeof(buffer2));
                memset(result, '\0', sizeof(result));
                memset(result1, '\0', sizeof(result));
                memset(result2, '\0', sizeof(result));
                memset(result3, '\0', sizeof(result));
                fclose(file);
                //sleep(0.1f);
                sem_post(&controllog);
    }
}
//It takes event time, passing train ID, and trains waiting passage, and record(append) it to controllog when tunnel is cleared
void tunelClearRecorder(void* arg)
{
    WaitingQueues* waitQue = (WaitingQueues*)arg;
    TrainQueue* queue1 = waitQue->queue1;
    TrainQueue* queue2 = waitQue->queue2;
    TrainQueue* queue3 = waitQue->queue3;
    TrainQueue* queue4 = waitQue->queue4;
    while (1) {
    sem_wait(&tc_log);
    sem_wait(&controllog);
    //printf("Tunel clear oldu");
    //printf("  ID : %d, ", trainPassing.ID);
    fflush(stdout);
    char result[1000] = "";
    char result1[1000] = "";
    char result2[1000] = "";
    char result3[1000] = "";
    for (int i = 0; i < queue1->totalcount; i++) {
    char id[100];
    sprintf(id, "%d", queue1->trains[i].ID);
    strcat(result, id);
        if (i != queue1->totalcount - 1) 
        {
            strcat(result, ", ");
        }
    }
    for (int i = 0; i < queue2->totalcount; i++) {
    char id[100];
    sprintf(id, "%d", queue2->trains[i].ID);
    strcat(result1, id);
        if (i != queue2->totalcount - 1) 
        {
            strcat(result1, ", ");
        }
    }
    for (int i = 0; i < queue3->totalcount; i++) {
    char id[100];
    sprintf(id, "%d", queue3->trains[i].ID);
    strcat(result2, id);
        if (i != queue3->totalcount - 1) 
        {
            strcat(result2, ", ");
        }
    }
    for (int i = 0; i < queue4->totalcount; i++) {
    char id[100];
    sprintf(id, "%d", queue4->trains[i].ID);
    strcat(result3, id);
        if (i != queue4->totalcount - 1) 
        {
            strcat(result3, ", ");
        }
    }
    FILE *file = fopen("controllog.txt", "a");
                time_t rawtime;
                struct tm *timeinfo;
                char buffer[800];
                char bufferID[80] = "\t \t  # \t ";
                char buffer3[80] = " ";
                char buffer2[80];
                time(&rawtime);
                timeinfo = localtime(&rawtime);
                strftime(buffer2, sizeof(buffer), "\t \t %H:%M:%S ", timeinfo);
                strcat(buffer3, buffer2);
                memset(buffer, '\0', sizeof(buffer));
                strcat(buffer, "Tunnel cleared");
                
                strcat(buffer, buffer3);
                strcat(buffer, bufferID);
                strcat(buffer, " ");
                 if(strcmp(result1, "") != 0) 
                {
                    if(strcmp(result, "") != 0)
                {
                    strcat(result, ", ");
                }
                    strcat(result, result1);
                }
               
                if(strcmp(result2, "") != 0)
                {
                     if(strcmp(result, "") != 0)
                {
                    strcat(result, ", ");
                }
                    strcat(result, result2);
                    
                }
                
                if(strcmp(result3, "") != 0)
                {
                    if(strcmp(result, "") != 0)
                {
                    strcat(result, ", ");
                }
                    strcat(result, result3);
                }
                strcat(buffer, result);
                fprintf(file, "[%s] \n", buffer);
                memset(buffer, '\0', sizeof(buffer));
                memset(bufferID, '\0', sizeof(bufferID));
                memset(buffer3, '\0', sizeof(buffer3));
                memset(buffer2, '\0', sizeof(buffer2));
                memset(result, '\0', sizeof(result));
                memset(result1, '\0', sizeof(result));
                memset(result2, '\0', sizeof(result));
                memset(result3, '\0', sizeof(result));
                fclose(file);
                //sleep(0.1f);
                sem_post(&controllog);
    }
}
//It executes A to C line
//It dequeue from toA queue and enqueue to A to C line(means that it is in C point)
void* A_CThread(void* arg) {
    Direction* direction = (Direction*)arg;
    TrainQueue* queue = direction->queueFrom;
    TrainQueue* queueTo = direction->queueTo;
    while (1) {
        if (queue->totalcount > 0) {
            Train passengerTrain = dequeue(queue);
            totalACounter++;
            
            sleep(1);
            enqueue(queueTo, passengerTrain);
            totalACounter--;


        }
    }
    return NULL;
}
//It executes C to A line
//It dequeue from fromC to A queue (means that it is departed)
void* C_AThread(void* arg) {
    Direction* direction = (Direction*)arg;
    TrainQueue* queue = direction->queueFrom;
    TrainQueue* queueTo = direction->queueTo;
    while (1) {
        if (queue->totalcount > 0) {
            sem_wait(&pass_recordedA);
            Train passengerTrain = dequeue(queue);
            totalACounter++;

            trainPass_A = passengerTrain;
            sleep(1);
            sem_post(&dep_recordedA);
            totalACounter--;


        }
    }
    return NULL;
}
//It executes B to C line
//It dequeue from toB queue and enqueue to B to C line(means that it is in C point)
void* B_CThread(void* arg) {
    Direction* direction = (Direction*)arg;
    TrainQueue* queue = direction->queueFrom;
    TrainQueue* queueTo = direction->queueTo;
    while (1) {
        if (queue->totalcount > 0) {
            Train passengerTrain = dequeue(queue);
            totalBCounter++;

            sleep(1);
            enqueue(queueTo, passengerTrain);
            totalBCounter--;

            
        }
    }
    return NULL;
}
//It executes C to B line
//It dequeue from fromC to B queue (means that it is departed)
void* C_BThread(void* arg) {
    Direction* direction = (Direction*)arg;
    TrainQueue* queue = direction->queueFrom;
    TrainQueue* queueTo = direction->queueTo;
    while (1) {
        if (queue->totalcount > 0) {
            sem_wait(&pass_recordedB);
            Train passengerTrain = dequeue(queue);
            totalBCounter++;
            trainPass_B = passengerTrain;
            sleep(1);
            totalBCounter--;
            
            sem_post(&dep_recordedB);


        }
    }
    return NULL;
}
//It executes D to E line
//It dequeue from fromD to E queue (means that it is departed)
void* D_EThread(void* arg) {
    Direction* direction = (Direction*)arg;
    TrainQueue* queue = direction->queueFrom;
    TrainQueue* queueTo = direction->queueTo;

    while (1) {
        if (queue->totalcount > 0) {
            sem_wait(&pass_recordedE);
            Train passengerTrain = dequeue(queue);
            totalECounter++;

            trainPass_E = passengerTrain;
            sleep(1);
            
            sem_post(&dep_recordedE);
        }
    }
    return NULL;
}

//It executes E to D line
//It dequeue from toE queue and enqueue to E to D line(means that it is in D point)
void* E_DThread(void* arg) {
    Direction* direction = (Direction*)arg;
    TrainQueue* queue = direction->queueFrom;
    TrainQueue* queueTo = direction->queueTo;
    while (1) {
        if (queue->totalcount > 0) {
            //printf("E-D thread is working \n");
            Train passengerTrain = dequeue(queue);
            totalECounter++;

            sleep(1);
            enqueue(queueTo, passengerTrain);
        }
    }
    return NULL;
}
//It executes D to F line
//It dequeue from fromD to F queue (means that it is departed)
void* D_FThread(void* arg) {
    Direction* direction = (Direction*)arg;
    TrainQueue* queue = direction->queueFrom;
    TrainQueue* queueTo = direction->queueTo;

    while (1) {
        if (queue->totalcount > 0) {
            sem_wait(&pass_recordedF);
            Train passengerTrain = dequeue(queue);
            totalFCounter++;
            trainPass_F = passengerTrain;
            sleep(1);
            
            sem_post(&dep_recordedF);
        }
    }
    return NULL;
}
//It executes F to D line
//It dequeue from toF queue and enqueue to F to D line(means that it is in D point)
void* F_DThread(void* arg) {
    Direction* direction = (Direction*)arg;
    TrainQueue* queue = direction->queueFrom;
    TrainQueue* queueTo = direction->queueTo;
    while (1) {
        if (queue->totalcount > 0) {
            //printf("F-D thread is working \n");
            Train passengerTrain = dequeue(queue);
            totalFCounter++;

            sleep(1);
            enqueue(queueTo, passengerTrain);
        }
    }
    return NULL;
}
int main(int args, char *argv[]) {
    srand(time(NULL));
    int overload = 0;
    int ID = 0;
    //In there I created my semaphores
    //controllog is used to share controllog.txt properly, it is used by thread_centerlog, thread_breakdownlog, thread_overloadlog, thread_tclearedlog
    //sem_log is used to record tunnelpass properly (initialized as 0 signaled after train passed tunnel in tunnel thread, waited in centerLogRecorder therefore we can record after train passed tunnel)
    //bd_log is used to record breakdown properly (initialized as 0 signaled after breakdown is happened in tunnel in tunnel thread, waited in breakdownRecorder therefore we can record after train breakdowned)
    //ol_log is used to record overload properly (initialized as 0 signaled after overload is happened tunnel in main thread, waited in overloadRecorder therefore we can record after overlaod is happened)
    //tc_log is used to record tunnelcleared properly (initialized as 0 signaled after tunnel is cleared in main thread, waited in tunelClearRecorder therefore we can record after tunnel is cleared)
    //pass_recorded is referred to pass is recorded. It is waited in tunnel thread and signaled after pass is recorded.
    //dep_recordedA, B, E, F is referred to departure from A, B, E, F is recorded. It is waited in their lines thread, and signaled after departure is recorded.
    //pass_recordedA, B, E, F is referred to pass from C or D, to A, B, E ,F is recorded. It is waited in their lines between C or D thread, and signaled after pass is recorded.

    int log_ret = sem_init(&sem_log, 0, 0);
    int log_ret2 = sem_init(&bd_log, 0, 0);
    int log_ret3 = sem_init(&ol_log, 0, 0);
    int log_ret4 = sem_init(&tc_log, 0, 0);
    int log_ret5 = sem_init(&tunnel_log, 0, 1);
    int log_ret6 = sem_init(&controllog, 0, 1);
    int log_ret7 = sem_init(&tunnel_sem, 0, 1);
    int log_ret8 = sem_init(&pass_recorded, 0, 1);
    int log_ret9 = sem_init(&dep_recordedA, 0, 0);
    int log_ret10 = sem_init(&dep_recordedB, 0, 0);
    int log_ret11 = sem_init(&dep_recordedE, 0, 0);
    int log_ret12 = sem_init(&dep_recordedF, 0, 0);
    int log_ret13 = sem_init(&pass_recordedA, 0, 1);
    int log_ret14 = sem_init(&pass_recordedB, 0, 1);
    int log_ret15 = sem_init(&pass_recordedE, 0, 1);
    int log_ret16 = sem_init(&pass_recordedF, 0, 1);
    //In there I create and initialized my train queues
    //queue_to_A, B, E, F means that one train out of system is reached point of A, B, E, F 
    TrainQueue queue_to_A, queue_to_B, queue_to_E, queue_to_F;
    initializeQueue(&queue_to_A);
    initializeQueue(&queue_to_B);
    initializeQueue(&queue_to_E);
    initializeQueue(&queue_to_F);
    //In there I create and initialized my train lines
    //queue_fromX_toY, means that one train from X is reached point of Y, therefore the train is in Y
    TrainQueue queue_fromA_toC, queue_fromB_toC, queue_fromE_toD, queue_fromF_toD, queue_C_between_D, queue_fromC_toA, queue_fromC_toB, queue_fromD_toE, queue_fromD_toF;
    initializeQueue(&queue_fromA_toC);
    initializeQueue(&queue_fromB_toC);
    initializeQueue(&queue_fromE_toD);
    initializeQueue(&queue_fromF_toD);
    initializeQueue(&queue_C_between_D);
    initializeQueue(&queue_fromC_toA);
    initializeQueue(&queue_fromC_toB);
    initializeQueue(&queue_fromD_toF);
    initializeQueue(&queue_fromD_toE);
    
    //In there I create and initialized my log files, 
    //trainlog which records trainID, starting point, arrival time, length, destination point, and departure time
    FILE *file = fopen("trainlog.txt", "a");
    char buffer[800] = "Train ID Starting Point Arrival Time Length(m) Destination Point Departure Time";
    fprintf(file, "%s \n", buffer);
    fclose(file);
    //controllog which records  Event, Event Time, Train ID, Trains Waiting Passage 
    FILE *file2 = fopen("controllog.txt", "a");
    char buffer2[800] = " \t Event \t \t \t  Event Time \t Train ID \t Trains Waiting Passage";
    fprintf(file2, "%s \n", buffer2);
    fclose(file2);
    int simulation_time;
    if(strcmp(argv[1], "-s") == 0)
    {
        simulation_time = atoi(argv[2]);
    }
    else
    {
        simulation_time = 60;
    }
    
    time_t start_time = time(NULL);
    
    int system_trains_count = 0;
        //In there I create tunnelThread, this thread does C-D line's execution.
        pthread_t tunnelThreadc_d;
        FromDirection direction_c_d = {(void*)&queue_C_between_D,(void*)&queue_fromA_toC, (void*)&queue_fromB_toC,(void*)&queue_fromE_toD,(void*)&queue_fromF_toD, (void*)&queue_fromC_toA, (void*)&queue_fromC_toB,(void*)&queue_fromD_toE,(void*)&queue_fromD_toF};
        pthread_create(&tunnelThreadc_d, NULL, tunnelThread, (void*)&direction_c_d);
        
        //In there I create A to C thread, this thread does A-C line's execution.
        pthread_t tunnelThreada_c;
        Direction direction_a_c = {(void*)&queue_to_A,(void*)&queue_fromA_toC};
        pthread_create(&tunnelThreada_c, NULL, A_CThread, (void*)&direction_a_c);
        
        //In there I create B to C thread, this thread does B-C line's execution.
        pthread_t tunnelThreadb_c;
        Direction direction_b_c = {(void*)&queue_to_B,(void*)&queue_fromB_toC};
        pthread_create(&tunnelThreadb_c, NULL, B_CThread, (void*)&direction_b_c);
        
        //In there I create E to D thread, this thread does E-D line's execution.
        pthread_t tunnelThreade_d;
        Direction direction_e_d = {(void*)&queue_to_E,(void*)&queue_fromE_toD};
        pthread_create(&tunnelThreade_d, NULL, E_DThread, (void*)&direction_e_d);
        
        //In there I create F to D thread, this thread does F-D line's execution.
        pthread_t tunnelThreadf_d;
        Direction direction_f_d = {(void*)&queue_to_F,(void*)&queue_fromF_toD};
        pthread_create(&tunnelThreadf_d, NULL, F_DThread, (void*)&direction_f_d);
        
        
        //In there I create D to E thread, this thread does D-E line's execution.
        pthread_t tunnelThreadd_e;
        Direction direction_d_e = {(void*)&queue_fromD_toE,(void*)&queue_fromD_toE};
        pthread_create(&tunnelThreadd_e, NULL, D_EThread, (void*)&direction_d_e);
        
        //In there I create D to F thread, this thread does D-F line's execution.
        pthread_t tunnelThreadd_f;
        Direction direction_d_f = {(void*)&queue_fromD_toF,(void*)&queue_fromD_toF};
        pthread_create(&tunnelThreadd_f, NULL, D_FThread, (void*)&direction_d_f);
        
        //In there I create C to A thread, this thread does C-A line's execution
        pthread_t tunnelThreadc_a;
        Direction direction_c_a = {(void*)&queue_fromC_toA,(void*)&queue_fromC_toA};
        pthread_create(&tunnelThreadc_a, NULL, C_AThread, (void*)&direction_c_a);
        
        //In there I create C to B thread, this thread does C-B line's execution
        pthread_t tunnelThreadc_b;
        Direction direction_c_b = {(void*)&queue_fromC_toB,(void*)&queue_fromC_toB};
        pthread_create(&tunnelThreadc_b, NULL, C_BThread, (void*)&direction_c_b);
        
        //In there I create centerlog thread, this thread keep record of tunnel passing
        pthread_t thread_centerlog;
        WaitingQueues waitTunnel = {(void*)&queue_fromA_toC,(void*)&queue_fromB_toC, (void*)&queue_fromE_toD,(void*)&queue_fromF_toD};
        pthread_create(&thread_centerlog, NULL, centerLogRecorder, (void*)&waitTunnel);
        
        //In there I create breakdownlog thread, this thread keep record of breakdown in tunnel
        pthread_t thread_breakdownlog;
        pthread_create(&thread_breakdownlog, NULL, breakDownRecorder, (void*)&waitTunnel);
        
        //In there I create overloadlog thread, this thread keep record of overload in tunnel
        pthread_t thread_overloadlog;
        pthread_create(&thread_overloadlog, NULL, overloadRecorder, (void*)&waitTunnel);
        
        //In there I create tunnel cleared log thread, this thread keep record of tunnel cleared in tunnel
        pthread_t thread_tclearedlog;
        pthread_create(&thread_tclearedlog, NULL, tunelClearRecorder, (void*)&waitTunnel);
        
        //In there I create departureA thread, this thread keep record of departure in C to A line
        pthread_t thread_departureA;
        pthread_create(&thread_departureA, NULL, departureTimeA, (void*)&waitTunnel);
        
        //In there I create departureB thread, this thread keep record of departure in C to B line
        pthread_t thread_departureB;
        pthread_create(&thread_departureB, NULL, departureTimeB, (void*)&waitTunnel);
        
        //In there I create departureE thread, this thread keep record of departure in D to E line
        pthread_t thread_departureE;
        pthread_create(&thread_departureE, NULL, departureTimeE, (void*)&waitTunnel);
        
        //In there I create departureF thread, this thread keep record of departure in D to F line
        pthread_t thread_departureF;
        pthread_create(&thread_departureF, NULL, departureTimeF, (void*)&waitTunnel);
        
        //In there, if the simulation time is not passed, it checks breakdown, tunnel cleared. Addition to that, it creates train (means that train arrives A, B, E or F)
    while (difftime(time(NULL), start_time) < simulation_time) {
        if ((queue_to_A.totalcount + queue_fromA_toC.totalcount + queue_to_B.totalcount + queue_fromB_toC.totalcount + queue_fromE_toD.totalcount + queue_to_E.totalcount + queue_fromF_toD.totalcount + queue_to_F.totalcount + queue_C_between_D.totalcount) > overloadLimit) {
            if(overload == 0)
            {
            //printf("System Overloaded\n");
            overload = 1;
            sem_post(&ol_log);
            }
        }
        else if((queue_to_A.totalcount + queue_fromA_toC.totalcount + queue_to_B.totalcount + queue_fromB_toC.totalcount + queue_fromE_toD.totalcount + queue_to_E.totalcount + queue_fromF_toD.totalcount + queue_to_F.totalcount + queue_C_between_D.totalcount) == 0)
        {
            if(overload == 1)
            {
                overload = 0;
                //printf("Overload is solved\n");
                sem_post(&tc_log);
            }
            
        }
	//printf("TotalSec: %d \n", totalSec);
	totalSec++;
        //In there with probability p train arrives A, E, F and with 1-p probability it arrives B point.
        //When train arrives it enqueue to special queue and produce first part of log.
        float prob = atof(argv[3]);
        float p = (float)rand() / RAND_MAX;
	printf("P :  %.6f \n", p);
        int train_length = (rand() % 10 < 7) ? 100 : 200;
        if (overload != 1)
        {
            if (p < (prob/3)) {
                Train new_train = {train_length, 100, 0, ID};  // A
                
                enqueue(&queue_to_A, new_train);
                //printf("A is created \n");
                createdACounter++;
                FILE *file = fopen("trainlog.txt", "a");
                time_t rawtime;
                struct tm *timeinfo;
                char buffer[800];
                char buffer2[80];
                //char buffer3[80] = "Arrival Time: ";
                char buffer4[80];
                time(&rawtime);
                timeinfo = localtime(&rawtime);
                int j = snprintf(buffer, 20,
                 " %d,  \t \t A   \t", ID);
                int k = snprintf(buffer4, 30,
                 "  \t %d   ", new_train.length);
                strftime(buffer2, sizeof(buffer), " \t %H:%M:%S ", timeinfo);
                strcat(buffer, buffer2);
                strcat(buffer, buffer4);
                fprintf(file, "[%s W ] \n", buffer);
                fclose(file);
                ID++;
            }else if (p < ((prob/3)*2)) {
                Train new_train = {train_length, 100, 0, ID};  // E
                enqueue(&queue_to_E, new_train);
                createdECounter++;
                FILE *file = fopen("trainlog.txt", "a");
                time_t rawtime;
                struct tm *timeinfo;
                char buffer[800];
                char buffer2[80];
                //char buffer3[80] = "Arrival Time: ";
                char buffer4[80];
                time(&rawtime);
                timeinfo = localtime(&rawtime);
                int j = snprintf(buffer, 20,
                 " %d,  \t \t E   \t", ID);
                int k = snprintf(buffer4, 30,
                 "  \t %d   ", new_train.length);
                strftime(buffer2, sizeof(buffer), " \t %H:%M:%S ", timeinfo);
                strcat(buffer, buffer2);
                strcat(buffer, buffer4);
                fprintf(file, "[%s W ] \n", buffer);
                fclose(file);
                ID++;
		printf("EEE OLUSTUUU \n");
            } else if (p < prob) {
                Train new_train = {train_length, 100, 0, ID};  // F
                enqueue(&queue_to_F, new_train);
                createdFCounter++;
                FILE *file = fopen("trainlog.txt", "a");
                time_t rawtime;
                struct tm *timeinfo;
                char buffer[800];
                char buffer2[80];
                //char buffer3[80] = "Arrival Time: ";
                char buffer4[80];
                time(&rawtime);
                timeinfo = localtime(&rawtime);
                int j = snprintf(buffer, 20,
                 " %d,  \t \t F   \t", ID);
                int k = snprintf(buffer4, 30,
                 "  \t %d   ", new_train.length);
                strftime(buffer2, sizeof(buffer), " \t %H:%M:%S ", timeinfo);
                strcat(buffer, buffer2);
                strcat(buffer, buffer4);
                fprintf(file, "[%s W ] \n", buffer);
                fclose(file);
                ID++;
            } else {
                Train new_train = {train_length, 100, 0, ID};  // B
                enqueue(&queue_to_B, new_train);
                //printf("B is created \n");
                createdBCounter++;
                FILE *file = fopen("trainlog.txt", "a");
                time_t rawtime;
                struct tm *timeinfo;
                char buffer[800];
                char buffer2[80];
                //char buffer3[80] = "Arrival Time: ";
                char buffer4[80];
                time(&rawtime);
                timeinfo = localtime(&rawtime);
                int j = snprintf(buffer, 20,
                 " %d,  \t \t B   \t", ID);
                int k = snprintf(buffer4, 30,
                 "  \t %d   ", new_train.length);
                strftime(buffer2, sizeof(buffer), " \t %H:%M:%S ", timeinfo);
                strcat(buffer, buffer2);
                strcat(buffer, buffer4);
                fprintf(file, "[%s W ] \n", buffer);
                fclose(file);
                ID++;
            }
        }
        

        sleep(1);
       
    }
	return 0;
       /* //After simulation time is passed it cancel and join threads.
        pthread_cancel(tunnelThreadc_d);
        pthread_join(tunnelThreadc_d, NULL);
        pthread_cancel(tunnelThreada_c);
        pthread_join(tunnelThreada_c, NULL);
        pthread_cancel(tunnelThreadb_c);
        pthread_join(tunnelThreadb_c, NULL);
        pthread_cancel(tunnelThreadd_e);
        pthread_join(tunnelThreadd_e, NULL);
        pthread_cancel(tunnelThreadd_f);
        pthread_join(tunnelThreadd_f, NULL);
        pthread_cancel(tunnelThreadc_a);
        pthread_join(tunnelThreadc_a, NULL);
        pthread_cancel(tunnelThreadc_b);
        pthread_join(tunnelThreadc_b, NULL);
        pthread_cancel(tunnelThreade_d);
        pthread_join(tunnelThreade_d, NULL);
        pthread_cancel(tunnelThreadf_d);
        pthread_join(tunnelThreadf_d, NULL);
    
    return 0;*/
}
