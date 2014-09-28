#ifndef QCOMRAMDISKPLUGIN_H
#define QCOMRAMDISKPLUGIN_H

#include <libdbp/plugininterface.h>

#include <QtCore/QtPlugin>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>


class QcomRamdiskPlugin : public QObject,
                          public IRamdiskPatcherFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.github.chenxiaolong.libdbp.IRamdiskPatcherFactory/1.0" FILE "libdbp-ramdisk-qcom.json")
    Q_INTERFACES(IRamdiskPatcherFactory)

public:
    QcomRamdiskPlugin(QObject *parent = 0);

    virtual QStringList ramdiskPatchers() const;

    virtual QSharedPointer<RamdiskPatcher> createRamdiskPatcher(const PatcherPaths * const pp,
                                                                const QString &id,
                                                                const FileInfo * const info,
                                                                CpioFile * const cpio) const;
};

#endif // QCOMRAMDISKPLUGIN_H
