/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

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
