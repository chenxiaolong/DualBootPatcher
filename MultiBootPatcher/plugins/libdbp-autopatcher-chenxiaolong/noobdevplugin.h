#ifndef NOOBDEVPLUGIN_H
#define NOOBDEVPLUGIN_H

#include <libdbp/plugininterface.h>

#include <QtCore/QtPlugin>


class NoobdevPlugin : public QObject,
                                public IAutoPatcherFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.github.chenxiaolong.libdbp.IAutoPatcherFactory/1.0" FILE "libdbp-autopatcher-chenxiaolong.json")
    Q_INTERFACES(IAutoPatcherFactory)

public:
    NoobdevPlugin(QObject *parent = 0);

    virtual QStringList autoPatchers() const;

    virtual QSharedPointer<AutoPatcher> createAutoPatcher(const PatcherPaths * const pp,
                                                          const QString &id,
                                                          const FileInfo * const info,
                                                          const PatchInfo::AutoPatcherArgs &args) const;
};

#endif // NOOBDEVPLUGIN_H
