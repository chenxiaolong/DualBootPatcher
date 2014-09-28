#ifndef FILEINFO_H
#define FILEINFO_H

#include "libdbp_global.h"

#include "device.h"
#include "patchinfo.h"
#include "partitionconfig.h"

#include <QtCore/QObject>
#include <QtCore/QScopedPointer>

class FileInfoPrivate;

class LIBDBPSHARED_EXPORT FileInfo : public QObject
{
    Q_OBJECT
public:
    explicit FileInfo(QObject *parent = 0);
    ~FileInfo();

    void setFilename(const QString &path);
    void setPatchInfo(PatchInfo * const info);
    void setDevice(Device * const device);
    void setPartConfig(PartitionConfig * const config);

    QString filename() const;
    PatchInfo * patchInfo() const;
    Device * device() const;
    PartitionConfig * partConfig() const;

private:
    const QScopedPointer<FileInfoPrivate> d_ptr;
    Q_DECLARE_PRIVATE(FileInfo)
};

#endif // FILEINFO_H
