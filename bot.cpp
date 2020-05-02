#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  
#include <fstream>
#include <string>
#include <string.h> 
#include <time.h> 
#include "include/web_request.h"

#define SIZEQUEUE 10
#define CLIENTAMOUNT 5

void *ReadControlFile(void *arg1, void *arg2);
void *ContactServer(void *arg1, void *arg2);

using namespace std;

class Queue{

private:

    string buf[SIZEQUEUE];
    uint32_t write, read;
    bool empty, full, ReaderisDone;

public: 
    mutex _mut_acces_qeue;
    condition_variable notFull, notEmpty;

public:
    Queue();
    ~Queue();

    void Add_Item(const string& NewURL);
    void Delete_Intem(string& ReadURL);
    void EndTransmission(){ ReaderisDone = true;}
    bool isEmpty() const { return empty;}
    bool isFull() const {return full;}
    bool TransmissionDone() const {return ReaderisDone;}
};


Queue::Queue()
{
    empty = true;
    full = false;
    ReaderisDone = false;
    write = 0;
    read = 0;  
}

Queue::~Queue(){}

void Queue::Add_Item(const string& NewURL)
{
    buf[write] = NewURL;
    write++;

    if(write == SIZEQUEUE)
        write = 0;
    
    if(write == read)
        full = true;
    
    empty = false;
}

void Queue::Delete_Intem(string& ReadURL)
{
    ReadURL = buf[read];
    read++;

    if(read == SIZEQUEUE)
        read = 0;
    if(read == write)
        empty = true;
    
    full = false;
}

/* Reader Thread*/
void* ReadControlFile(void *arg1 , void *arg2)
{   
    Queue *fifo = (Queue *) arg1;
    char* file = (char *) arg2;
    
    ifstream InputFD(file);
    string line;

        if( InputFD.is_open())
        {
            while(getline(InputFD,line))
            {
                char *line_C = strdup(line.c_str());

                std::unique_lock<mutex> locker_getline(fifo->_mut_acces_qeue);

                if(fifo->isFull())
                {
                    printf("Reader : FIFO FUll.\n");
                    fifo->notFull.wait(locker_getline);         //wait until Fifo not full
                }

                if(strtok(line_C,"\r\n") == NULL)
                {
                    free(line_C);
                    continue;
                }

                fifo->Add_Item(line);
                printf("Reader: read %s \n", line.c_str());
                fifo->notEmpty.notify_one();           
            }
        }
        else
        {
            perror("File opening failed...");
            exit(EXIT_FAILURE);
        }

        unique_lock<mutex> locker(fifo->_mut_acces_qeue);
        fifo->EndTransmission();

        InputFD.close();

    printf("Reader : CLOSED.\n");

        return NULL;
}

/* Client Thread*/
void* ContactServer(void *arg1, void *arg2)
{
    Queue *fifo = ( Queue *) arg1;
    int *CLientNr = (int *) arg2;
    string line;

    while(true)
    {
        unique_lock<mutex> locker(fifo->_mut_acces_qeue);

        if(fifo->TransmissionDone() == true && fifo->isEmpty() == true)
        {
            locker.unlock();               
            break;
        }
        
        else if(fifo->TransmissionDone() == false && fifo->isEmpty() == true)
        {
            //printf("client %d : FIFO empty.\n",std::thread::id);
            printf("Client %d  : FIFO empty.\n",*CLientNr);
            fifo->notEmpty.wait(locker);
        }

        fifo->Delete_Intem(line);
        printf("Client %d  : consumed \n",*CLientNr);
        fifo->notFull.notify_one();         
        locker.unlock();

        char *line_url = strdup(line.c_str());
        char *url = strdup(line.c_str());

        strtok(url,"/");                    
        char *domain = strtok(NULL, "/");    

        if( domain == NULL)
        {
            fprintf(stderr, "Extraction of domain from URL [%s] failed, skiping...\n", line_url);
        }
        else
        {
            char filename[64];
            snprintf(filename, sizeof(filename),"%i_%s.html", *CLientNr, domain);

            printf("[START] Downloading URL: %s ->> File: %s\n", line_url, filename);

            int res;

            res = webreq_download(line_url,filename);

            if (res < 0)
                fprintf(stderr, "[ERROR] URL: %s, Message: %s\n", line_url , webreq_error(res));
            else if (res != WEBREQ_HTTP_OK)
                fprintf(stderr, "[ERROR] HTTP Status %d returned for URL: %s\n", res, line_url);
            else
                printf("[DONE ] URL: %s ->> File: %s\n", line_url, filename);
        }

        free(url);
        free(line_url);
    }

    printf("CLient : %d FINISHED\n",*CLientNr);

    return NULL;
}

int main(int argc, char *argv[]){

    Queue fifo;
    char *filename = argv[1];
    struct timespec before;
    struct timespec after;

    if(argc != 7)
    {
        fprintf(stderr,"Too few arguments\n");
        exit(EXIT_FAILURE);
    }

    char *timefile = argv[6];
    char *proxydelay = argv[3];
  

    webreq_init(argc, argv);   

    clock_gettime(CLOCK_MONOTONIC,&before);

    thread ReaderThread(ReadControlFile, &fifo, filename);
    thread Clients[CLIENTAMOUNT];
    int ClientNr[CLIENTAMOUNT];

    for(int i = 0; i < CLIENTAMOUNT; i++)
    {
        ClientNr[i] = i;
        Clients[i] = thread(ContactServer,&fifo,&ClientNr[i]);
    }

    //Wait for threads to terminate execution
    ReaderThread.join();      
    for(int i = 0; i < CLIENTAMOUNT; i++)
        Clients[i].join();

    clock_gettime(CLOCK_MONOTONIC,&after);

    uint64_t before_ms = ((before.tv_sec * 1000000000) + before.tv_nsec) / 1000000;
    uint64_t after_ms =  ((after.tv_sec * 1000000000) + after.tv_nsec) /1000000;
    int64_t elapsed = after_ms - before_ms;

    printf("%lu microseconds before\n", before_ms);
    printf("%lu microseconds after\n", after_ms);
    printf("%ld microseconds elapsed\n", elapsed);

    // free mem
    webreq_cleanup();

    ofstream filetimefd (timefile, ios::app | ios::out);

    if(filetimefd.is_open())
    {
        filetimefd << "Proxy delay: " << proxydelay 
                   << ", Number of Threads: " << CLIENTAMOUNT 
                   << ", Elapsed time in ms : " << elapsed << std::endl;
        filetimefd.close();
    }
    else
    {
        printf("Unable to open %s\n",timefile);
    }
    

    exit(EXIT_SUCCESS);
}

