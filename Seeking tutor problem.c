#include <stdio.h>
#include<conio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include<math.h>
#include<stdarg.h>
#include <string.h>
#include <pthread.h>
#include<signal.h>
#include <semaphore.h>
#define SIZELIM 100000

int newstdarriveQueue[SIZELIM];
int donetutorQueue[SIZELIM];
int priorityQueue[SIZELIM][2];
int st_priority[SIZELIM];
int std_ids[SIZELIM];
int ttr_ids[SIZELIM];
sem_t sem_std;
sem_t sem_co;
sem_t sem_ttrTOstd[SIZELIM];
pthread_mutex_t holdseat;
pthread_mutex_t holdqueue;
pthread_mutex_t completedtutoringqueue;
int numofstudents=0;
int numberoftutors=0;
int numberofhelps=0;
int numberofchairs=0;
int occupied_numberofchairs=0;
int completed=0;
int allrequests=0;
int currenttutoring=0;
void *thread1_student(void *student_id){
    int id_student=*(int*)student_id;
    while(1){
        if(st_priority[id_student-1]>=numberofhelps) {
            pthread_mutex_lock(&holdseat);
            completed++;
            pthread_mutex_unlock(&holdseat);
            sem_post(&sem_std);
            printf("****student %d terminated****\n",id_student);
            pthread_exit(NULL);
        }
        pthread_mutex_lock(&holdseat);
        if(occupied_numberofchairs>=numberofchairs){
            printf("Student update>> Student %d found no empty chair. Will try again later.\n",id_student);
            pthread_mutex_unlock(&holdseat);
            continue;
        }
        occupied_numberofchairs++;
        allrequests++;
        newstdarriveQueue[id_student-1]=allrequests;
        printf("Student update>> Student %d takes a seat. Empty chairs = %d\n",id_student,numberofchairs-occupied_numberofchairs);
        pthread_mutex_unlock(&holdseat);
        sem_post(&sem_std);
        while(donetutorQueue[id_student-1]==-1);
        sem_wait(&sem_ttrTOstd[id_student-1]);
        printf("Student update>> Student %d received help from Tutor %d.\n",id_student,donetutorQueue[id_student-1]-numofstudents);
        pthread_mutex_lock(&completedtutoringqueue);
        donetutorQueue[id_student-1]=-1;
        pthread_mutex_unlock(&completedtutoringqueue);
        pthread_mutex_lock(&holdseat);
        st_priority[id_student-1]++;
        pthread_mutex_unlock(&holdseat);
    }
}
void *thread2_tutor(void *tutor_id){
    int id_tutor=*(int*)tutor_id;
    int stdTTRtimes;
    int stdSeq;
    int id_student;
while(1){
        if(completed==numofstudents){
            printf("****tutor terminated****\n");
            pthread_exit(NULL);
        }
        stdTTRtimes=numberofhelps-1;
        stdSeq=numofstudents*numberofhelps+1;
        id_student=-1;
        sem_wait(&sem_co);
        pthread_mutex_lock(&holdseat);
        int i;
        for(i=0;i<numofstudents;i++){
            if(priorityQueue[i][0]>-1 && priorityQueue[i][0]<=stdTTRtimes
               && priorityQueue[i][1]<stdSeq){
                stdTTRtimes=priorityQueue[i][0];
                stdSeq=priorityQueue[i][1];
                id_student=std_ids[i];
            }
        }
        if(id_student==-1) {
            pthread_mutex_unlock(&holdseat);
            continue;
        }
        priorityQueue[id_student-1][0]=-1;
        priorityQueue[id_student-1][1]=-1;
        occupied_numberofchairs--;
        currenttutoring++;
        pthread_mutex_unlock(&holdseat);
        pthread_mutex_lock(&holdseat);
        currenttutoring--;
        printf("Tutor update>> Student %d tutored by Tutor %d. Students tutored now = %d\n",id_student,id_tutor-numofstudents,currenttutoring);
        pthread_mutex_unlock(&holdseat);
        pthread_mutex_lock(&completedtutoringqueue);
        donetutorQueue[id_student-1]=id_tutor;
        pthread_mutex_unlock(&completedtutoringqueue);
        sem_post(&sem_ttrTOstd[id_student-1]);
    }
}
void *thread3_coordinator(){
    while(1){
        if(completed==numofstudents){
            int i;
            for(i=0;i<numberoftutors;i++){
	        sem_post(&sem_co);
            }
            printf("****coordinator terminated****\n");
            pthread_exit(NULL);
        }
        sem_wait(&sem_std);
        pthread_mutex_lock(&holdseat);
        int i;
        for(i=0;i<numofstudents;i++){
            if(newstdarriveQueue[i]>-1){
                priorityQueue[i][0]=st_priority[i];
                priorityQueue[i][1]=newstdarriveQueue[i];
        printf("Coordinator update>> Student %d with priority %d in the queue. Waiting students now = %d\n",std_ids[i],st_priority[i],occupied_numberofchairs);
                newstdarriveQueue[i]=-1;
                sem_post(&sem_co);
      }
    }
  pthread_mutex_unlock(&holdseat);
}
}
int main() {
    printf("Enter total number of students\n");
    scanf("%d",&numofstudents);
    printf("Enter total number of Tutors\n");
    scanf("%d",&numberoftutors);
    printf("Enter total number of Cahirs\n");
    scanf("%d",&numberofchairs);
    printf("Enter maximum number of helps a student receives\n");
    scanf("%d",&numberofhelps);
    if(numofstudents > SIZELIM || numberoftutors > SIZELIM){
        printf("Max student number is: %d; Max tutor number is: %d\n", SIZELIM, SIZELIM);
        exit(-1);
    }
    int i;
    for(i=0;i<numofstudents;i++){
        newstdarriveQueue[i]=-1;
        donetutorQueue[i]=-1;
        priorityQueue[i][0]=-1;
        priorityQueue[i][1]=-1;
        st_priority[i]=0;
    }
    sem_init(&sem_std,0,0);
    sem_init(&sem_co,0,0);
    pthread_mutex_init(&holdseat,NULL);
    pthread_mutex_init(&holdqueue,NULL);
    pthread_mutex_init(&completedtutoringqueue,NULL);
    for(i=0;i<numofstudents;i++){
        sem_init(&sem_ttrTOstd[i],0,0);
    }
    pthread_t students[numofstudents];
    pthread_t tutors[numberoftutors];
    pthread_t coordinator;
    assert(pthread_create(&coordinator,NULL,thread3_coordinator,NULL)==0);
    for(i = 0; i < numofstudents; i++)
    {
        std_ids[i] = i + 1;
        assert(pthread_create(&students[i], NULL, thread1_student, (void*) &std_ids[i])==0);
    }
    for(i = 0; i < numberoftutors; i++)
    {
        ttr_ids[i] = i + numofstudents + 1;
        assert(pthread_create(&tutors[i], NULL, thread2_tutor, (void*) &ttr_ids[i])==0);
    }
    pthread_join(coordinator, NULL);
for(i =0; i < numofstudents; i++)
    {
        pthread_join(students[i],NULL);
    }
for(i =0; i < numberoftutors; i++)
    {
        pthread_join(tutors[i],NULL);
    }
return 0;
}
