#include <fstream>
#include <sstream>

#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>

#include <QStorageInfo>
#include <QNetworkInterface>
#include <QTcpSocket>

#include "client.h"

#define TO_GB (1024 * 1024 * 1024)

Client::Client()
{
    update_all();
}

Client::~Client()
{
    if(m_thr_send)
    {
        m_thr_send->join();
        delete m_thr_send;
    }
}

void Client::init(std::string conf_file)
{
    std::ifstream conf(conf_file);
    if (conf.is_open() && !conf.eof())
    {
        std::string buff{""};
        while (!conf.eof())
        {
            conf >> buff;
            if(buff == "host:")
            {
                QHostAddress addr;
                conf >> buff;
                if (addr.setAddress(buff.c_str()))
                    m_host_ip = buff;
                else
                    syslog(LOG_INFO, "Config file: bad host ip!");    
            }
            else if(buff == "port:")
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
        }

        conf.close();
    }
    else
        syslog(LOG_INFO, "Config file open failed!");

    update_all();
}

/*
    getters
*/
std::string Client::get_user_name() const
{
    return m_client_info.user_name;
}

std::string Client::get_time() const
{
    return m_client_info.time;
}

std::string Client::get_os_name() const
{
    return m_client_info.os_name;
}

std::string Client::get_os() const
{
    return m_client_info.os;
}

uint64_t Client::get_total_RAM() const
{
    return m_client_info.total_RAM;
}

int32_t Client::get_CPU_number() const
{
    return m_client_info.cpu_number;
}

std::vector<CPU_info> Client::get_CPU_info() const
{
    return m_client_info.cpu_info;
}

std::vector<Disk_info> Client::get_disk_info() const
{
    return m_client_info.disk_info;
}

std::string Client::get_MAC() const
{
    return m_client_info.MAC;
}

std::string Client::get_IP() const
{
    return m_client_info.IP;
}

std::vector<Process_info> Client::get_process_info() const
{
    return m_client_info.process_info;
}

/*
    setters
*/
void Client::set_user_name(const std::string user_name)
{
    m_client_info.user_name = user_name;
}

void Client::set_time(const std::string time)
{
    m_client_info.time = time;
}

void Client::set_os_name(const std::string os_name)
{
    m_client_info.os_name = os_name;
}

void Client::set_os(const std::string os)
{
    m_client_info.os = os;
}

void Client::set_total_RAM(const uint64_t total_RAM)
{
    m_client_info.total_RAM = total_RAM;
}

void Client::set_CPU_number(const int32_t cpu_number)
{
    m_client_info.cpu_number = cpu_number;
}

void Client::set_CPU_info(const std::vector<CPU_info> cpu_info)
{
    m_client_info.cpu_info = cpu_info;
}

void Client::set_disk_info(const std::vector<Disk_info> disk_info)
{
    m_client_info.disk_info = disk_info;
}

void Client::set_MAC(const std::string mac)
{
    m_client_info.MAC = mac;
}

void Client::set_IP(const std::string ip)
{
    m_client_info.IP = ip;
}

void Client::set_process_info(const std::vector<Process_info> process_info)
{
    m_client_info.process_info = process_info;
}

/*
    updaters
*/
void Client::update_all()
{
    update_user_name();
    update_time();
    update_os();
    update_os_name();
    update_total_RAM();
    update_CPU_number();
    update_CPU_info();
    update_disk_info();
    update_MAC();
    update_IP();
    update_process_info();
}

void Client::update_user_name()
{
    set_user_name(std::string(getenv("USER")));
}

void Client::update_time()
{
    time_t tm =time(NULL );
    struct tm * curtime = localtime (&tm);
    char buff[100];
    strftime(buff, sizeof(buff), "%Y-%m-%d.%X", curtime);
    set_time(std::string(buff));
}

void Client::update_os()
{
    struct utsname uts;
    uname(&uts);
    set_os(uts.sysname);
}

void Client::update_os_name()
{
    set_os_name(QSysInfo::prettyProductName().toStdString());
}

void Client::update_total_RAM()
{
    struct sysinfo memInfo;
    sysinfo (&memInfo);
    set_total_RAM(memInfo.totalram/1024/1024);
}

void Client::update_CPU_number()
{
    set_CPU_number(get_nprocs_conf());
}

void Client::update_CPU_info()
{
    std::vector<CPU_info> cpu_info(get_CPU_number());
    const char* id_comand = "grep vendor_id /proc/cpuinfo | cut -d ':' -f 2";
    const char* model_name_comand = "grep 'model name' /proc/cpuinfo | cut -d ':' -f 2";
    const char* speed_comand = "grep 'cpu MHz' /proc/cpuinfo | cut -d ':' -f 2";

    FILE *fpipe;
    char* line = NULL;
    int read = 0;
    size_t len = 0;
    uint64_t i = 0;

    if ((fpipe = (FILE*)popen(id_comand, "r")) == NULL)
        syslog(LOG_INFO, "popen failed!");
    
    while ((read = getline(&line, &len, fpipe)) != -1)
        cpu_info[i++].id = std::string(line + 1 , read - 2);
    pclose(fpipe);
    
    i = 0;
    if ((fpipe = (FILE*)popen(model_name_comand, "r")) == NULL)
        syslog(LOG_INFO, "popen failed!");
    
    while ((read = getline(&line, &len, fpipe)) != -1)
        cpu_info[i++].model_name = std::string(line + 1 , read - 2);
    pclose(fpipe);

    i = 0;
    if ((fpipe = (FILE*)popen(speed_comand, "r")) == NULL)
        syslog(LOG_INFO, "popen failed!");
    
    while ((read = getline(&line, &len, fpipe)) != -1)
        cpu_info[i++].speed = std::stod(line + 1);
    pclose(fpipe);

    free(line);
    set_CPU_info(cpu_info);
}

void Client::update_disk_info()
{
    std::vector<Disk_info> disk_info;
    QList<QStorageInfo> disks = QStorageInfo::mountedVolumes();
    for (QStorageInfo& disk : disks) {
        if (disk.isValid()                                                     // check if filesystem is exist
            && disk.isReady()                                                  // check if filesystem is ready to work
            && !disk.isReadOnly()                                              // check if filesystem is not protected from writing
            && disk.bytesTotal() / TO_GB > 0)                                  // check if filesystem is not empty
        {
            Disk_info info;

            info.name = disk.device().toStdString();
            info.media_type = calculate_media_type(disk.device().toStdString());
            info.total = (double)disk.bytesTotal() / TO_GB;
            info.free = (double)disk.bytesFree() / TO_GB;
            info.used = info.total - info.free;

            disk_info.push_back(info);
        }
    }
    set_disk_info(disk_info);
}

void Client::update_MAC()
{
    QNetworkInterface res;
    QList<QNetworkInterface> infs = QNetworkInterface::allInterfaces();
    for (QNetworkInterface& inf : infs) {
        if (inf.type() == QNetworkInterface::Ethernet || inf.type() == QNetworkInterface::Wifi)
        {
            if (inf.flags().testFlag(QNetworkInterface::IsRunning))
            {
                res = inf;
                break;
            }
            else
                res = inf;
        }
    }
    set_MAC(res.hardwareAddress().toStdString());
}

void Client::update_IP()
{
    update_MAC();
    QList<QHostAddress> ips = QNetworkInterface::interfaceFromName(get_MAC().c_str()).allAddresses();
    std::string res;
    for (QHostAddress& ip : ips)
        if (!ip.isLinkLocal() && !ip.isLoopback())
            res = ip.toString().toStdString();

    set_IP(res);
}

void Client::update_process_info()
{
    std::vector<Process_info> process_info;
    const char* comand = "top -b -n1";

    FILE *fpipe;
    char* line = NULL;
    int read = 0;
    size_t len = 0;

    if ((fpipe = (FILE*)popen(comand, "r")) == NULL)
        syslog(LOG_INFO, "popen failed!");
    bool start  = false;
    while ((read = getline(&line, &len, fpipe)) != -1)
    {
        Process_info info;
        std::stringstream ss(line); 
        if(start)
        {
            ss >> info.pid >> info.user;

            std::string str = ss.str();
            size_t find = str.rfind(" ");
            if (find == std::string::npos)
                info.command = " ";
            else
                info.command = str.substr(find + 1, str.size() - find - 2);

            process_info.push_back(info);
        }
        else
            start = ss.str().find("PID USER") != std::string::npos;
    }
    pclose(fpipe);
    free(line);
    set_process_info(process_info);
}

/*
    helpers
*/
MEDIA_TYPE Client::calculate_media_type(const std::string& device) const
{
    size_t found = device.rfind("/");
    const std::string path = "/sys/block/" + (found == std::string::npos ? device : device.substr(found + 1)) + "/queue/rotational";

    std::ifstream rotational(path);
    if (!rotational.is_open() && rotational.eof()) {
        syslog(LOG_INFO, "Rotational open failed!");
        return  UNKNOWN;
    }
    int media_type;
    rotational >> media_type;
    rotational.close();

    return media_type ? HDD : SSD;
}

void Client::send_async()
{
    const size_t time_connection = 30; // time to wait for a connection
    QScopedPointer<QTcpSocket> socket(new QTcpSocket());
    socket->connectToHost(QString::fromStdString(m_host_ip), m_port);
    if (!socket->waitForConnected(time_connection))
    {
        syslog(LOG_INFO, "Cannot connect to server, host=%s , port=%d.", m_host_ip.c_str(), m_port);
        return;
    }
    syslog(LOG_INFO, "Connect to server, host=%s , port=%d.", m_host_ip.c_str(), m_port);
    
    std::string message{""};
    get_all(message);
    size_t message_size = message.size();

    if(!socket->write(reinterpret_cast<char*>(&message_size), sizeof(int)) || !socket->write(message.c_str(), message_size))
    {
        syslog(LOG_INFO, "Cannot write message with len=%ld.", message_size);
        return;
    }
    syslog(LOG_INFO, "Write message with len=%ld.", message_size);

    if(!socket->flush())
    {
        syslog(LOG_INFO, "Cannot flush message.");
        return;
    }
    syslog(LOG_INFO, "Message flush.");

    socket->disconnectFromHost();
    socket->close();
    syslog(LOG_INFO, "Close connection.");
}

void Client::send()
{
    if(m_thr_send != nullptr)
    {
        m_thr_send->join();
        delete m_thr_send;
    }

    m_thr_send = new std::thread(&Client::send_async, this);
}

void Client::get_all(std::string& str) const
{
    str = "";
    str += get_MAC() + "\n";
    str += get_os() + "\n";
    str += get_user_name() + "\n";

    str += get_time() + "\n";

    str += "'" + get_os_name() + "', '" + get_IP() + "', " 
    + std::to_string(get_CPU_number()) + ", " + std::to_string(get_total_RAM()) + "\n";

    str += std::to_string(get_CPU_number()) + "\n";

    for ( auto& i : get_CPU_info())
    {
        str += "'" + i.id + "', '" + i.model_name + "', " + std::to_string(i.speed) + "\n";
    }

    auto disks = get_disk_info();
    str += std::to_string(disks.size()) + "\n";

    for ( auto& i : disks)
    {
        str += "'" + i.name + "', '";
        switch (i.media_type)
        {
        case SSD:
            str += "SSD";
            break;
        case HDD:
            str += "HDD";
            break;
        default:
            str += "UNW";
            break;
        }
        str += "', " + std::to_string(i.total) + ", " + std::to_string(i.used) + ", " + std::to_string(i.free) + "\n";
    }

    auto processes = get_process_info();

    str += std::to_string(processes.size()) + "\n";
    for(auto& i : processes)
    {
        str += std::to_string(i.pid) + ", '" + i.user + "', '" + i.command + "'\n";
    }
}

void Client::print()
{
    std::string str = "";
    str += "'" + get_MAC() + "', '";
    str += get_os() + "', '";
    str += get_user_name() + "'\n";

    str += get_time() + "\n";

    str += "'" + get_os_name() + "', '";
    str += get_IP() + "', ";
    str += std::to_string(get_CPU_number()) + ", ";
    str += std::to_string(get_total_RAM()) + "\n";

    str += std::to_string(get_CPU_number()) + "\n";

    for ( auto& i : get_CPU_info())
    {
        str += "'" + i.id + "', '" + i.model_name + "', " + std::to_string(i.speed) + "\n";
    }

    auto disks = get_disk_info();
    str += std::to_string(disks.size()) + "\n";

    for ( auto& i : disks)
    {
        str += "'" + i.name + "', ";
        switch (i.media_type)
        {
        case SSD:
            str += "SSD";
            break;
        case HDD:
            str += "HDD";
            break;
        default:
            str += "UNW";
            break;
        }
        str += ", " + std::to_string(i.total) + ", " + std::to_string(i.used) + ", " + std::to_string(i.free) + "\n";
    }

    auto processes = get_process_info();

    str += std::to_string(processes.size()) + "\n";
    for(auto& i : processes)
    {
        str += std::to_string(i.pid) + ", '" + i.user + "', '" + i.command + "'\n";
    }
    printf("%s", str.c_str());
}