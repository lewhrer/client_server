#include <string>

#include <stdio.h>
#include <QThreadPool>
#include <QSqlQuery>

class DB_Sender : public QRunnable
{
    QSqlQuery query;
    std::string message;


    bool send_query(const std::string& query_str);
    void add_message();
    std::string add_client(const std::string& mac, const std::string& os, const std::string& user);
    std::string get_client_id(const std::string& mac, const std::string& os, const std::string& user);

    void run() override;
public:
    void init(QSqlQuery& query_, std::string message_)
    {
        query = query_;
        message = message_;
    }

};