#ifndef JFLTEPATCHERPLUGIN_H
#define JFLTEPATCHERPLUGIN_H

#include <libdbp/plugininterface.h>

#include <QtCore/QtPlugin>


class JfltePlugin : public QObject,
                    public IAutoPatcherFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.github.chenxiaolong.libdbp.IAutoPatcherFactory/1.0" FILE "libdbp-autopatcher-jflte.json")
    Q_INTERFACES(IAutoPatcherFactory)

public:
    JfltePlugin(QObject *parent = 0);

    virtual QStringList autoPatchers() const;

    virtual QSharedPointer<AutoPatcher> createAutoPatcher(const PatcherPaths * const pp,
                                                          const QString &id,
                                                          const FileInfo * const info,
                                                          const PatchInfo::AutoPatcherArgs &args) const;
};

#endif // JFLTEPATCHERPLUGIN_H
