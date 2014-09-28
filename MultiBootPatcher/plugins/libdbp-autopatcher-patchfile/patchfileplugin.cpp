#include "patchfileplugin.h"
#include "patchfilepatcher.h"


PatchFilePlugin::PatchFilePlugin(QObject *parent) :
    QObject(parent)
{
}

QStringList PatchFilePlugin::autoPatchers() const
{
    return QStringList() << PatchFilePatcher::PatchFile;
}

QSharedPointer<AutoPatcher> PatchFilePlugin::createAutoPatcher(const PatcherPaths * const pp,
                                                               const QString &id,
                                                               const FileInfo * const info,
                                                               const PatchInfo::AutoPatcherArgs &args) const
{
    return QSharedPointer<AutoPatcher>(new PatchFilePatcher(pp, id, info, args));
}
