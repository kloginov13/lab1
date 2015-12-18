
#include <sys/types.h>
#include <sys/stat.h>
#include "stdio.h"
#include "stdlib.h"
#include "fcntl.h"
#include "errno.h"
#include "unistd.h"
#include "syslog.h"
#include "string.h"
#include "vector"
#include "map"
#include <iostream>
#include <fstream>
#include <string>
#include "ftw.h"
#include <sstream>
#include <sys/eventfd.h>


#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/inotify.h>
int inotify_add_watch(int fd, const char *pathname, uint32_t mask);


class ToString {
protected:
	std::stringstream m_stream;
public:
	template<class T>
	ToString& operator<<(const T& arg) {
		m_stream << arg;
		return *this;
	}
	//std::string str;
	operator std::string() const {
		return m_stream.str();

	}
};

class ConfigReader {
private:
    std::string inputPath;
    u_int time;

public:
    static std::string defaultFilePath;
    ConfigReader() {};
    void read(const std::string& path = defaultFilePath) {
        std::ifstream file(path);
        if (file.is_open()) {
            std::getline(file, inputPath);
            file >> time;
        } else {
            std::string msg = "Can't read config file " + path;
            throw msg.c_str();
        }
    }

    const std::string &getInputPath() {
        return inputPath;
    }

    u_int getTime() {
        return time;
    }
};





class Daemon {

public:
    static std::vector<std::string> dir;
    static std::map<int, std::string> inDir;

    Daemon();
    void start();
    void stopDaemon() {m_isRunning = false;}
    void reloadConfig() {m_isNeedReload = true;}

private:
    bool m_isRunning;
    bool m_isNeedReload;
    int m_notifyDescriptor;
    std::string m_path;
    int m_time;

    int init();
    void clearData();
    void setWatcher(const std::string &cur_dir);
    void process();
    void displayEvent(struct inotify_event *event);
    int haveEvent();

    void readConfig(const std::string &path);
    void myLog(int type, const std::string &data){syslog(type, data.c_str());}
};

Daemon::Daemon() {
    m_notifyDescriptor = 0;
    m_isRunning = true;
    m_isNeedReload = true;
    m_time = 0;
}
void Daemon::clearData() {
    Daemon::inDir.clear();
    Daemon::dir.clear();
}

void Daemon::readConfig(const std::string &path) {
    std::ifstream file(path);
    if (file.is_open()) {
        std::string time;
        std::getline(file, m_path);
        std::getline(file, time);

    } else {
        std::string msg = ToString() << "Mistake of config reading " << path;
        throw msg.c_str();
    }
}
void Daemon::start() {
    myLog(LOG_INFO, ToString() << "Daemon has started");
    process();
}
#define EVENT struct inotify_event *
void Daemon::process() {
    while (m_isRunning) {
        if (m_isNeedReload) {
            clearData();
            if (init() < 0)
                return;
            m_isNeedReload = false;
        }
        if (haveEvent()) {
            char buffer[256];
            read(this->m_notifyDescriptor, buffer, 256);
            EVENT event = (EVENT)&buffer[(size_t) 0];
            displayEvent(event);
        }
    }
    close(this->m_notifyDescriptor);
    myLog(LOG_INFO, ToString() << "Daemon has closed");
}

int ftw_handler(const char *name, const struct stat *status, int type) {
    if (type == FTW_NS)
        return 0;

    if (type == FTW_D)
        Daemon::dir.push_back(name);

    return 0;
}
int Daemon::init() {
    try {
        readConfig("conf.txt");
    } catch (const char* msg) {
        this->m_isRunning = false;
        myLog(LOG_ERR, ToString() << "Dir Name mistake Daemon has closed" << msg);
        return -1;
    }

    ftw(m_path.c_str(), ftw_handler, 1);

    if(Daemon::dir.empty()){
        myLog(LOG_ERR, ToString() << "Dir name mistake Daemon has closed");
        return -1;
    }

    m_notifyDescriptor = inotify_init();
    if (m_notifyDescriptor < 0) {
        myLog(LOG_ERR,
              ToString() << "notify init error" << strerror(errno));
        this->m_isRunning = false;
        return -1;
    }
    for (std::size_t i = 0; i < Daemon::dir.size(); i++) {
        std::string temp = Daemon::dir.at(i);
        setWatcher(temp);
    }
    std::cout << Daemon::inDir.size() << std::endl;
    return 0;
}

void Daemon::setWatcher(const std::string &cur_dir) {
    int watch_dir_index = inotify_add_watch(m_notifyDescriptor,
                                            cur_dir.c_str(), IN_ALL_EVENTS);//add a watch to an initialized inotify instance
    if (watch_dir_index < 0) {
        myLog(LOG_ERR,
              ToString() << "add watcher to " << cur_dir << " error: "
              << strerror(errno));
    } else {
        Daemon::inDir[watch_dir_index] = cur_dir;
        myLog(LOG_INFO, ToString() << "add watcher to " << cur_dir);
    }
}

/////

/////



std::vector<std::string> Daemon::dir;
std::map<int, std::string> Daemon::inDir;






//Daemon daemon;
 int main(void) {

        /* Our process ID and Session ID */
        pid_t pid, sid;

        /* Fork off the parent process */
        pid = fork();
        if (pid < 0) {
                exit(EXIT_FAILURE);
        }
        /* If we got a good PID, then
           we can exit the parent process. */
        if (pid > 0) {
                exit(EXIT_SUCCESS);
        }

        /* Change the file mode mask */
        umask(0);

        /* Open any logs here */


        /* Create a new SID for the child process */
        sid = setsid();
        if (sid < 0) {
                /* Log the failure */
                exit(EXIT_FAILURE);
        }



        /* Change the current working directory */
        if ((chdir("/")) < 0) {
                /* Log the failure */
                exit(EXIT_FAILURE);
        }

        /* Close out the standard file descriptors */
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        /* Daemon-specific initialization goes here */


        daemon = Daemon();
        //daemon = Daemon::start();
        daemon.start();
        exit(0);

        /* The Big Loop */
//        while (1) {
//          /* Do some task here ... */

//           sleep(30); /* wait 30 seconds */
//        }

   exit(EXIT_SUCCESS);
}
