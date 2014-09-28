#ifndef CORERAMDISKPLUGIN_H
#define CORERAMDISKPLUGIN_H

#include <libdbp/plugininterface.h>

#include <QtCore/QtPlugin>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>


class CoreRamdiskPlugin : public QObject,
                          public IRamdiskPatcherFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.github.chenxiaolong.libdbp.IRamdiskPatcherFactory/1.0" FILE "libdbp-ramdisk-common.json")
    Q_INTERFACES(IRamdiskPatcherFactory)

public:
    CoreRamdiskPlugin(QObject *parent = 0);

    virtual QStringList ramdiskPatchers() const;

    virtual QSharedPointer<RamdiskPatcher> createRamdiskPatcher(const PatcherPaths * const pp,
                                                                const QString &id,
                                                                const FileInfo * const info,
                                                                CpioFile * const cpio) const;
};

#endif // CORERAMDISKPLUGIN_H
