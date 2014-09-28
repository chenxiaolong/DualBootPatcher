#ifndef SYNCDAEMON_H
#define SYNCDAEMON_H

#include <Poco/Condition.h>
#include <Poco/DirectoryWatcher.h>
#include <Poco/Mutex.h>
#include <Poco/Runnable.h>
#include <Poco/Task.h>


struct rominformation {
    // Mount points
    std::string system;
    std::string cache;
    std::string data;

    // Identifiers
    std::string id;
};


struct apkinformation {
    std::string apkfile;
    std::string apkdir;
    std::string libdir;
    std::string cacheddex;
};


class SyncdaemonTask;

class SingleDelayedTask : public Poco::Runnable
{
public:
    SingleDelayedTask(SyncdaemonTask *owner, unsigned int delay);

    virtual void run();

    void execute();

    void stop();

private:
    SyncdaemonTask *_owner;
    unsigned int _delay;
    bool _canRun;
    bool _stopped;
    Poco::Mutex _mutex;
    Poco::Condition _cond;
};


class SyncdaemonTask : public Poco::Task
{
public:
    SyncdaemonTask();

    virtual ~SyncdaemonTask();

    virtual void runTask();

    void onAppDirChanged(const void *, const Poco::DirectoryWatcher::DirectoryEvent &ev);

    void onConfigFileChanged(const void *, const Poco::DirectoryWatcher::DirectoryEvent &ev);

    bool isBootedInPrimary();

    void syncPackages();

private:
    void populateRoms();

    std::string getCurrentRom();

    int getApkInformation(struct rominformation *info, std::string package,
                          struct apkinformation *apkinfo);

    void syncPackage(std::string package, std::vector<std::string> rom_ids);

private:
    Poco::DirectoryWatcher *_appDirWatcher;
    Poco::DirectoryWatcher *_configDirWatcher;
    std::vector<struct rominformation *> _roms;
    SingleDelayedTask _task;
    Poco::Thread *_thread;
};

#endif // SYNCDAEMON_H
