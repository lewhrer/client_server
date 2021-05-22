#include <string>
#include <atomic>

#include <QSqlDatabase>
#include <QSqlQuery>

#include "db_sender.h"

#define DB_DRIVER "QMYSQL"

class Server
{
private:
    uint16_t m_port = 8739;
    static std::atomic<bool> m_running;

    QSqlDatabase db = QSqlDatabase::addDatabase(DB_DRIVER);
    QSqlQuery query;
    DB_Sender* db_sender;

    void db_init();
    bool db_open();

public:
    Server();
    ~Server();
    void init();
    void init(std::string conf_file);

    bool work();
    static void start();
    static void stop();
};