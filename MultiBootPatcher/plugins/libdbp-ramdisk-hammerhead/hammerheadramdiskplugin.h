#ifndef HAMMERHEADRAMDISKPLUGIN_H
#define HAMMERHEADRAMDISKPLUGIN_H

#include <libdbp/plugininterface.h>

#include <QtCore/QtPlugin>


class HammerheadRamdiskPlugin : public QObject,
                                public IRamdiskPatcherFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.github.chenxiaolong.libdbp.IRamdiskPatcherFactory/1.0" FILE "libdbp-ramdisk-hammerhead.json")
    Q_INTERFACES(IRamdiskPatcherFactory)

public:
    HammerheadRamdiskPlugin(QObject *parent = 0);

    virtual QStringList ramdiskPatchers() const;

    virtual QSharedPointer<RamdiskPatcher> createRamdiskPatcher(const PatcherPaths * const pp,
                                                                const QString &id,
                                                                const FileInfo * const info,
                                                                CpioFile * const cpio) const;
};

#endif // HAMMERHEADRAMDISKPLUGIN_H
