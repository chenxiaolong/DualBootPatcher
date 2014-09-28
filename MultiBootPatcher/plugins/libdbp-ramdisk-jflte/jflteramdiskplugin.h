#ifndef JFLTERAMDISKPLUGIN_H
#define JFLTERAMDISKPLUGIN_H

#include <libdbp/plugininterface.h>
#include <libdbp/patcherpaths.h>

#include <QtCore/QtPlugin>


class JflteRamdiskPlugin : public QObject,
                           public IRamdiskPatcherFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.github.chenxiaolong.libdbp.IRamdiskPatcherFactory/1.0" FILE "libdbp-ramdisk-jflte.json")
    Q_INTERFACES(IRamdiskPatcherFactory)

public:
    JflteRamdiskPlugin(QObject *parent = 0);

    virtual QStringList ramdiskPatchers() const;

    virtual QSharedPointer<RamdiskPatcher> createRamdiskPatcher(const PatcherPaths * const pp,
                                                                const QString &id,
                                                                const FileInfo * const info,
                                                                CpioFile * const cpio) const;
};

#endif // JFLTERAMDISKPLUGIN_H
