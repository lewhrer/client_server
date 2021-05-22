#include <stdio.h>
#include <syslog.h>
#include <signal.h>
#include <unistd.h>

#include "server.h"

void signal_handler(int sig)
{
    syslog(LOG_INFO, "Received %d signal.", sig);
    Server::stop();
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

        Server server;
        if(argc == 2)
            server.init(std::string(argv[1]));
        else
            server.init();

        syslog(LOG_INFO, "System monitor server deamon start!!!");
        printf("System monitor server deamon start!!!\n");

        server.start();

        while(!server.work()) { }

        syslog(LOG_INFO,  "System monitor server deamon stop!!!");
        return 0;
    }
    else
    {
        return 0;
    }
    return 0;
}