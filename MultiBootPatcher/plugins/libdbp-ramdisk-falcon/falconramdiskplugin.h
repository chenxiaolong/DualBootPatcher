#ifndef FALCONRAMDISKPLUGIN_H
#define FALCONRAMDISKPLUGIN_H

#include <libdbp/plugininterface.h>

#include <QtCore/QtPlugin>


class FalconRamdiskPlugin : public QObject,
                            public IRamdiskPatcherFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.github.chenxiaolong.libdbp.IRamdiskPatcherFactory/1.0" FILE "libdbp-ramdisk-falcon.json")
    Q_INTERFACES(IRamdiskPatcherFactory)

public:
    FalconRamdiskPlugin(QObject *parent = 0);

    virtual QStringList ramdiskPatchers() const;

    virtual QSharedPointer<RamdiskPatcher> createRamdiskPatcher(const PatcherPaths * const pp,
                                                                const QString &id,
                                                                const FileInfo * const info,
                                                                CpioFile * const cpio) const;
};

#endif // FALCONRAMDISKPLUGIN_H
