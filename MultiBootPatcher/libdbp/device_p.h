#ifndef DEVICE_P_H
#define DEVICE_P_H

#include <QtCore/QHash>
#include <QtCore/QString>

class DevicePrivate
{
public:
    static const QString Unchanged;

    QString codename;
    QString name;
    QString architecture;
    QString selinux;

    QHash<QString, QString> partitions;
};


#endif // DEVICE_P_H
