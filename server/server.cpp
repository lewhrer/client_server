#include <fstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <syslog.h>

#include <sstream>

#include <QNetworkInterface>
#include <QSqlError>

#include "server.h"

#define DB_HOST_NAME "localhost"
#define DB_NAME "testdb"
#define DB_USER_NAME "root"
#define DB_PASSWORD "123123"

#define BUFFER_SIZE 512
#define SUCCESS 0
#define ERROR -1

std::atomic<bool> Server::m_running(true);

void Server::init(std::string conf_file)
{
    std::ifstream conf(conf_file);
    if (conf.is_open() && !conf.eof())
    {
        std::string buff{""};
        while (!conf.eof())
        {
            conf >> buff;
        
            if(buff == "port:")
            {
                conf >> buff;
                if(std::all_of(buff.begin(), buff.end(), ::isdigit) && buff.size() < 6)
                {
                    uint64_t ui = std::stoul(buff, nullptr, 0);
                    if(ui == 0 || ui > std::numeric_limits<uint16_t>::max())
                        syslog(LOG_INFO, "Config file: bad port value!");   
                    else
                        m_port = ui;
                }
            }
            else if(buff == "db-host-name:")
            {
                conf >> buff;
                db.setHostName(buff.c_str());
            }
            else if(buff == "db-name:")
            {
                conf >> buff;
                db.setDatabaseName(buff.c_str());
            }
            else if(buff == "db-user-name:")
            {
                conf >> buff;
                db.setUserName(buff.c_str());
            }
            else if(buff == "db-password:")
            {
                conf >> buff;
                db.setPassword(buff.c_str());
            }            
        }
        conf.close();
    }
    else
        syslog(LOG_INFO, "Config file open failed!");

    db_open();
}

Server::Server()
{}

void Server::init()
{
    db_init();
    db_open();
}

Server::~Server()
{
    QThreadPool::globalInstance()->waitForDone();
}

bool Server::work()
{
    if(!m_running)
        return true;

    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *result, *rp;
    if (getaddrinfo(NULL, std::to_string(m_port).c_str(), &hints, &result) != SUCCESS)
    {
        syslog(LOG_INFO,  "Error getaddrinfo");
        return false;
    }
    int sfd = 0;
    int optval = 1;
    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        fcntl(sfd, F_SETFL, O_NONBLOCK); /* Change the socket into non-blocking state	*/
        if (sfd == ERROR)
            continue;

        if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == ERROR)
            syslog(LOG_INFO,  "Error setting SO_REUSEADDR option");

        if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == SUCCESS)
            break; // SUCCESS

        close(sfd);
        return false;
    }

    if (listen(sfd, 5) == ERROR)
    {
        syslog(LOG_INFO,"Error listen\n");
        return false;
    }

    freeaddrinfo(result);
    while (m_running)
    {
        int cfd = accept(sfd, NULL, NULL);
        if (cfd < 0 && (errno != EWOULDBLOCK || errno != EAGAIN))
        {
            syslog(LOG_INFO, "Error accept");
            return false;
        }
        else if (cfd > 0)
        {
		    std::string incomming_buffer;
		    std::string buffer{""};
		    int msg_size;
		    incomming_buffer.resize(BUFFER_SIZE);
		    ssize_t bytes_received = recv(cfd, reinterpret_cast<char*>(&msg_size), sizeof(int), 0);

		    if (bytes_received == ERROR || bytes_received == 0 || msg_size == 0)
		    	continue;

		    bytes_received = 0;
		    while (bytes_received < msg_size)
		    {
		    	bytes_received += recv(cfd, const_cast<char*>(incomming_buffer.c_str()), BUFFER_SIZE, 0);
		    	buffer.append(incomming_buffer);
		    }
		    if (msg_size <= buffer.size())
		    	buffer.erase(buffer.begin() + msg_size, buffer.end());
    
		    if (-1 == bytes_received || bytes_received == 0)
		    {
                syslog(LOG_INFO,  "Client disconected");
                return false;
		    }
            syslog(LOG_INFO,  "Message received, len=%ld", bytes_received);
            
            if(db_open())
            {
                db_sender = new DB_Sender();
                db_sender->init(query, buffer);
                QThreadPool::globalInstance()->waitForDone();
                QThreadPool::globalInstance()->start(db_sender);
            }
        }
    }
    return true;
}

bool Server::db_open()
{
    if (db.isOpen())
        return true;

    if (!db.open())
    {
        syslog(LOG_ERR, "%s\n", db.lastError().text().toStdString().c_str());
        return false;
    }
    return true;
}

void Server::db_init()
{
    db.setHostName(DB_HOST_NAME);
    db.setDatabaseName(DB_NAME);
    db.setUserName(DB_USER_NAME);
    db.setPassword(DB_PASSWORD);
}

void Server::start() { m_running = true;}

void Server::stop() { m_running = false;}
