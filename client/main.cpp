#include <iostream>
#include <atomic>
#include <unistd.h>
#include <sys/types.h>
#include <syslog.h>
#include <stdlib.h>
#include <signal.h>

#include "client.h"

#define TIME_INTERVAL 15

std::atomic<bool> running(true);

void signal_handler(int sig)
{
    syslog(LOG_INFO, "Received %d signal.", sig);
    running = false;
}

int main(int argc, char *argv[])
{
    signal(SIGHUP, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);

    if (argc > 2)
    {
        printf("Bad usage!!!\nYou can pass only config file or nothing!!!\n");
        return 0;
    }

    pid_t pid = 0;
    pid = fork();
    if (pid < 0)
    {
        printf("Fork error = %d\n", pid);
        return 0;
    }

    if (pid == 0)
    {   
        printf("Deamon started\n");
        
        syslog(LOG_INFO, "Deamon started\n");
        if (setsid() == -1)
        {
            perror("setsid");
            return 0;
        }
        if (chdir("/") == -1)
        {
            perror("chdir");
            return 0;
        }

        Client client;
        if(argc == 2)
            client.init(argv[1]);

        while(running)
        {
            client.send();
            // client.print();
            sleep(TIME_INTERVAL);
            client.update_all();
        }
        syslog(LOG_INFO, "Client deamon stop\n");

        return 0;
    }
    else{
        return 0;
    }
}