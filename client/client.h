#include <string>
#include <vector>
#include <thread>

enum MEDIA_TYPE
{
    UNKNOWN,
    HDD,
    SSD
};

struct CPU_info
{
    std::string id;
    std::string model_name;
    double speed;
};

struct Disk_info
{
    std::string name;
    MEDIA_TYPE media_type;
    /* size in GB */    
    double total;
    double used;
    double free;
};

struct Process_info
{
    int pid;
    std::string user;
    std::string command;
};

struct client_info
{
    std::string user_name;
    std::string time;
    std::string os;
    std::string os_name;
    uint64_t total_RAM;
    int32_t cpu_number;
    std::vector<CPU_info> cpu_info;
    std::vector<Disk_info> disk_info;
    std::string MAC;
    std::string IP;
    std::vector<Process_info> process_info;
};

class Client
{
private:
    client_info m_client_info;
    std::thread *m_thr_send = nullptr;
    std::string m_host_ip = "127.0.0.1";
    uint16_t m_port = 8739;
/*
    getters
*/
    std::string get_user_name() const;
    std::string get_time() const;
    std::string get_os() const;
    std::string get_os_name() const;
    uint64_t get_total_RAM() const;
    int32_t get_CPU_number() const;
    std::vector<CPU_info> get_CPU_info() const;
    std::vector<Disk_info> get_disk_info() const;
    std::string get_MAC() const;
    std::string get_IP() const;
    std::vector<Process_info> get_process_info() const;

    void get_all(std::string& str) const;

/*
    setters
*/
    void set_user_name(const std::string user_name);
    void set_time(const std::string time);
    void set_os(const std::string os);
    void set_os_name(const std::string os_name);
    void set_total_RAM(const uint64_t total_RAM);
    void set_CPU_number(const int32_t cpu_number);
    void set_CPU_info(const std::vector<CPU_info> cpu_info);
    void set_disk_info(const std::vector<Disk_info> disk_info);
    void set_MAC(const std::string mac);
    void set_IP(const std::string ip);
    void set_process_info(const std::vector<Process_info> process_info);

/*
    updaters
*/
    void update_user_name();
    void update_time();
    void update_os();
    void update_os_name();
    void update_total_RAM();
    void update_CPU_number();
    void update_CPU_info();
    void update_disk_info();
    void update_MAC();
    void update_IP();
    void update_process_info();

/*
    helpers
*/
    MEDIA_TYPE calculate_media_type(const std::string& device) const;

    void send_async();

public:
    Client();
    ~Client();
    void init(std::string conf_file);
    void send();
    void update_all();
    void print();
};