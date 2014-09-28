#ifndef STANDARDPATCHERPLUGIN_H
#define STANDARDPATCHERPLUGIN_H

#include <libdbp/plugininterface.h>

#include <QtCore/QtPlugin>


class StandardPlugin : public QObject,
                       public IAutoPatcherFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.github.chenxiaolong.libdbp.IAutoPatcherFactory/1.0" FILE "libdbp-autopatcher-standard.json")
    Q_INTERFACES(IAutoPatcherFactory)

public:
    StandardPlugin(QObject *parent = 0);

    virtual QStringList autoPatchers() const;

    virtual QSharedPointer<AutoPatcher> createAutoPatcher(const PatcherPaths * const pp,
                                                          const QString &id,
                                                          const FileInfo * const info,
                                                          const PatchInfo::AutoPatcherArgs &args) const;
};

#endif // STANDARDPATCHERPLUGIN_H
