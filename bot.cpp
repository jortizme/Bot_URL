#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>   
#include <fstream>
#include <string>
#include <cstdio>
//#include <cstring>
#include <string.h> 
#include <malloc.h>
#include <sys/time.h> 
#include "web_request.h"

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
    int cnt = 0;
    

    //InputFileDescriptor.open(std::string(file),std::ifstream::in);

        if( InputFD.is_open())
        {
            while(getline(InputFD,line))
            {
                //char *line_C = new char[sizeof(line) + 1];
                //strncpy(line_C,line.c_str(),sizeof(line) + 1); ///////!!!!!!!!!!!!

                char *line_C = strdup(line.c_str());

                std::unique_lock<mutex> locker_getline(fifo->_mut_acces_qeue);

                if(fifo->isFull())
                {
                    printf("Reader : FIFO FUll.\n");
                    fifo->notFull.wait(locker_getline);         //wait until Fifo not full
                }

                //if (strtok_r(line_C,"\r\n",&line_C) == NULL)
                if(strtok(line_C,"\r\n") == NULL)
                {
                    free(line_C);
                    continue;
                }

                fifo->Add_Item(line);
                printf("Reader: read %s \n", line.c_str());
                fifo->notEmpty.notify_one();//!!!!!!!            //tell all threads that FIFO not empty
                locker_getline.unlock();                        //maybe not necessary

                cnt++;
                usleep(10000 + cnt*10000);
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

        return NULL;
}

/* Client Thread*/
void* ContactServer(void *arg1, void *arg2)
{
    Queue *fifo = ( Queue *) arg1;
    int *CLientNr = (int *) arg2;
    string line;
    int cnt = 0;


    while(true)
    {
        unique_lock<mutex> locker(fifo->_mut_acces_qeue);

        if(fifo->TransmissionDone() == true && fifo->isEmpty() == true)
        {
            locker.unlock();                //maybe not necessary 
            break;
        }
        
        if(fifo->isEmpty())
        {
            //printf("client %d : FIFO empty.\n",std::thread::id);
            printf("Client %d  : FIFO empty.\n",*CLientNr);
            fifo->notEmpty.wait(locker);
        }

        fifo->Delete_Intem(line);
        //printf("Client %d : consumed %s\n", std::thread::id,line);
        printf("Client %d  : consumed \n",*CLientNr);
        fifo->notFull.notify_one();         //the only one waiting for it is the Reader
        locker.unlock();


        //std::vector<char> charline(line.begin(), line.end());
        //charline.push_back('\0');

        //char *url =  new char[sizeof(line) + 1];
        //strncpy(url,line.c_str(),sizeof(line) + 1);

        char *url = strdup(line.c_str());

        strtok(url,"/");                    //!!!!!!!!!!!!!
        char *domain = strtok(NULL, "/");    //!!!!!!!!!!!!!

        if( domain == NULL)
        {
            fprintf(stderr, "Extraction of domain from URL [%s] failed, skiping...\n", url);
        }
        else
        {
            char filename[64];
            snprintf(filename, sizeof(filename),"%s.html",domain);

            printf("[START] Downloading URL: %s ->> File: %s\n", url, filename);

            int res;

            res = webreq_download(url,filename);

            if (res < 0)
                fprintf(stderr, "[ERROR] URL: %s, Message: %s\n", url , webreq_error(res));
            else if (res != WEBREQ_HTTP_OK)
                fprintf(stderr, "[ERROR] HTTP Status %d returned for URL: %s\n", res, url);
            else
                printf("[DONE ] URL: %s ->> File: %s\n", url, filename);

            
        }

        cnt++;
        usleep(20000 + cnt*30000);          //maybe we must wait less time because we contact the server before it
    }

    return NULL;
}

int main(int argc, char *argv[]){

    Queue fifo;
    char *filename = argv[1];
    int ClientNr[CLIENTAMOUNT];

    for(int i = 0; i < 4; i++)
        printf("%s\n",argv[i]);

    if(argc < 2)
    {
        fprintf(stderr,"Please give the control file as an argument\n");
        exit(EXIT_FAILURE);
    }

    webreq_init(argc, argv);    //maybe place it after the Clients's declaration

    
    thread ReaderThread(ReadControlFile, &fifo, filename);
    vector<thread> Clients;       //you could also declarate a normal array
    Clients.reserve(CLIENTAMOUNT);

    for(int i = 0; i < CLIENTAMOUNT; i++)
    {
        ClientNr[i] = i;
        Clients.emplace_back(ContactServer, &fifo, &ClientNr[i]);
    }
        

        //Clients.push_back(std::thread(ContactServer, &fifo));

    //Wait for threads to terminate execution
    ReaderThread.join();      
    for(int i = 0; i < CLIENTAMOUNT; i++)
        Clients[i].join();

    // free mem
    webreq_cleanup();

    exit(EXIT_SUCCESS);
}

