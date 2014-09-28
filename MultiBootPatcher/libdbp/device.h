#ifndef DEVICE_H
#define DEVICE_H

#include "libdbp_global.h"

#include <QtCore/QScopedPointer>
#include <QtCore/QStringList>

class DevicePrivate;
class PatcherPaths;

class LIBDBPSHARED_EXPORT Device
{
    friend class PatcherPaths;

public:
    Device();
    ~Device();

    QString codename() const;
    QString name() const;
    QString architecture() const;
    QString selinux() const;
    QString partition(const QString &which) const;
    QStringList partitionTypes() const;

private:
    const QScopedPointer<DevicePrivate> d_ptr;
    Q_DECLARE_PRIVATE(Device)
};

#endif // DEVICE_H
