#include "device.h"
#include "device_p.h"


const QString DevicePrivate::Unchanged = QStringLiteral("unchanged");

Device::Device() : d_ptr(new DevicePrivate())
{
    Q_D(Device);

    // Other architectures currently aren't supported
    d->architecture = QStringLiteral("armeabi-v7a");
}

Device::~Device()
{
    // Destructor so d_ptr is destroyed
}

QString Device::codename() const
{
    Q_D(const Device);

    return d->codename;
}

QString Device::name() const
{
    Q_D(const Device);

    return d->name;
}

QString Device::architecture() const
{
    Q_D(const Device);

    return d->architecture;
}

QString Device::selinux() const
{
    Q_D(const Device);

    if (d->selinux == DevicePrivate::Unchanged) {
        return QString();
    }

    return d->selinux;
}

QString Device::partition(const QString &which) const
{
    Q_D(const Device);

    if (d->partitions.contains(which)) {
        return d->partitions[which];
    } else {
        return QString();
    }
}

QStringList Device::partitionTypes() const
{
    Q_D(const Device);

    return d->partitions.keys();
}
