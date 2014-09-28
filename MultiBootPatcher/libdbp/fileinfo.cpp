#include "fileinfo.h"
#include "fileinfo_p.h"
#include "patcherpaths.h"

#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>

FileInfo::FileInfo(QObject *parent) :
    QObject(parent), d_ptr(new FileInfoPrivate())
{
}

FileInfo::~FileInfo()
{
    // Destructor so d_ptr is destroyed
}

void FileInfo::setFilename(const QString &path)
{
    Q_D(FileInfo);

    d->filename = path;
}

void FileInfo::setPatchInfo(PatchInfo * const info)
{
    Q_D(FileInfo);

    d->patchinfo = info;
}

void FileInfo::setDevice(Device * const device)
{
    Q_D(FileInfo);

    d->device = device;
}

void FileInfo::setPartConfig(PartitionConfig * const config)
{
    Q_D(FileInfo);

    d->partconfig = config;
}

QString FileInfo::filename() const
{
    Q_D(const FileInfo);

    return d->filename;
}

PatchInfo * FileInfo::patchInfo() const
{
    Q_D(const FileInfo);

    return d->patchinfo;
}

Device * FileInfo::device() const
{
    Q_D(const FileInfo);

    return d->device;
}

PartitionConfig * FileInfo::partConfig() const
{
    Q_D(const FileInfo);

    return d->partconfig;
}
