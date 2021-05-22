#include <sstream>

#include <syslog.h>

#include <QVariant>
#include <QSqlError>

#include "db_sender.h"

void DB_Sender::add_message()
{
    std::stringstream ss(message);
    std::string id;
    std::string time;
    std::string buff;
    std::string buff1;
    std::string buff2;

    std::getline(ss, buff, '\n');   // get mac
    std::getline(ss, buff1, '\n');  // get os
    std::getline(ss, buff2, '\n');  // get user

    id = add_client(buff, buff1, buff2);

    std::getline(ss, time, '\n'); // get time

    std::getline(ss, buff, '\n');  // get general
    send_query("INSERT INTO general VALUES(" + id + ", '" + time + "', " + buff + ")");

    std::getline(ss, buff, '\n');  // get count of cpus
    for(uint16_t i = 0; i < std::stoi(buff, 0, 10); ++i )
    {
        std::getline(ss, buff1, '\n');  // get cpus
        send_query("INSERT INTO cpus VALUES(" + id + ", '" + time + "', " + buff1 + ")");
    }
    
    std::getline(ss, buff, '\n');  // get count of disks
    for(uint16_t i = 0; i < std::stoi(buff, 0, 10); ++i )
    {
        std::getline(ss, buff1, '\n');  // get cpus
        send_query("INSERT INTO disks VALUES(" + id + ", '" + time + "', " + buff1 + ")");
    }

    std::getline(ss, buff, '\n');  // get count of processes
    for(uint16_t i = 0; i < std::stoi(buff, 0, 10); ++i )
    {
        std::getline(ss, buff1, '\n');  // get cpus
        send_query("INSERT INTO processes VALUES(" + id + ", '" + time + "', " + buff1 + ")");
    }
}

std::string DB_Sender::add_client(const std::string& mac, const std::string& os, const std::string& user)
{
    std::string id = get_client_id(mac, os, user);
    if(id == "")
    {
        send_query("INSERT INTO clients VALUES(null, '" + mac +  "', '" + os + "', '" + user + "')");
        std::string id = get_client_id(mac, os, user);
    }
    return id;
}

std::string DB_Sender::get_client_id(const std::string& mac, const std::string& os, const std::string& user)
{
    std::string id{""};
    if(query.exec(std::string("SELECT id FROM clients WHERE mac='" + mac + "' AND os='" + os + "' AND user='" + user + "'").c_str()) 
    && query.next())
    {
        id = query.value(0).toString().toStdString();
    }

    return id;
}

bool DB_Sender::send_query(const std::string& query_str)
{
    syslog(LOG_ERR, "%s\n", query_str.c_str());
    if(!query.exec(query_str.c_str()))
    {
        syslog(LOG_ERR, "%s\n", query.lastError().text().toStdString().c_str());
        return false;
    }
    return true;
}

void DB_Sender::run()
{
    try
    {   
        add_message();
    }
    catch(const std::exception& e)
    {
        syslog(LOG_ERR, "%s", e.what());
    }    
}
