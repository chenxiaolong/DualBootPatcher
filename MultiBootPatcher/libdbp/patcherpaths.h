#ifndef PATCHERPATHS_H
#define PATCHERPATHS_H

#include "libdbp_global.h"

#include "device.h"
#include "partitionconfig.h"
#include "patchererror.h"
#include "patchinfo.h"

#include <QtCore/QScopedPointer>
#include <QtCore/QXmlStreamReader>

class IPatcherFactory;
class IAutoPatcherFactory;
class IRamdiskPatcherFactory;

class PatcherPathsPrivate;

class LIBDBPSHARED_EXPORT PatcherPaths
{
public:
    PatcherPaths();
    ~PatcherPaths();

    PatcherError::Error error() const;
    QString errorString() const;

    QString configFile() const;
    QString binariesDirectory() const;
    QString dataDirectory() const;
    QString initsDirectory() const;
    QString patchesDirectory() const;
    QString patchInfosDirectory() const;
    QString pluginsDirectory() const;
    QString scriptsDirectory() const;

    void setConfigFile(const QString &path);
    void setBinariesDirectory(const QString &path);
    void setDataDirectory(const QString &path);
    void setInitsDirectory(const QString &path);
    void setMiscDirectory(const QString &path);
    void setPatchesDirectory(const QString &path);
    void setPatchInfosDirectory(const QString &path);
    void setPluginsDirectory(const QString &path);
    void setScriptsDirectory(const QString &path);

    void reset();

    QString version() const;
    QList<Device *> devices() const;
    Device * deviceFromCodename(const QString &codename) const;
    QList<PatchInfo *> patchInfos(const Device * const device) const;

    QList<IPatcherFactory *> patcherFactories() const;
    QList<IAutoPatcherFactory *> autoPatcherFactories() const;
    QList<IRamdiskPatcherFactory *> ramdiskPatcherFactories() const;

    IPatcherFactory * patcherFactory(const QString &name) const;
    IAutoPatcherFactory * autoPatcherFactory(const QString &name) const;
    IRamdiskPatcherFactory * ramdiskPatcherFactory(const QString &name) const;

    QList<PartitionConfig *> partitionConfigs() const;
    PartitionConfig * partitionConfig(const QString &id) const;

    QStringList initBinaries() const;

    bool loadConfig();
    bool loadPlugins();
    bool loadPatchInfos();

private:
    bool loadPlugin(QObject *plugin);

    // Tags for main config file
    static const QString ConfigTagPatcher;
    static const QString ConfigTagVersion;
    static const QString ConfigTagDevices;
    static const QString ConfigTagDevice;
    static const QString ConfigTagName;
    static const QString ConfigTagSelinux;
    static const QString ConfigTagPartitions;
    static const QString ConfigTagPartition;
    static const QString ConfigTagPatchInfoDirs;
    static const QString ConfigTagPatchInfoDir;

    static const QString ConfigAttrCodeName;
    static const QString ConfigAttrType;

    // Tags for patchinfo files
    static const QString PatchInfoTagPatchinfo;
    static const QString PatchInfoTagMatches;
    static const QString PatchInfoTagNotMatched;
    static const QString PatchInfoTagName;
    static const QString PatchInfoTagRegex;
    static const QString PatchInfoTagExcludeRegex;
    static const QString PatchInfoTagRegexes;
    static const QString PatchInfoTagHasBootImage;
    static const QString PatchInfoTagRamdisk;
    static const QString PatchInfoTagPatchedInit;
    static const QString PatchInfoTagAutopatchers;
    static const QString PatchInfoTagAutopatcher;
    static const QString PatchInfoTagDeviceCheck;
    static const QString PatchInfoTagPartconfigs;
    static const QString PatchInfoTagInclude;
    static const QString PatchInfoTagExclude;

    static const QString PatchInfoAttrRegex;

    static const QString XmlTextTrue;
    static const QString XmlTextFalse;

    // XML parsing functions for the main config file
    void parseConfigTagPatcher(QXmlStreamReader &xml);
    void parseConfigTagVersion(QXmlStreamReader &xml);
    void parseConfigTagDevices(QXmlStreamReader &xml);
    void parseConfigTagDevice(QXmlStreamReader &xml);
    void parseConfigTagName(QXmlStreamReader &xml, Device *device);
    void parseConfigTagSelinux(QXmlStreamReader &xml, Device *device);
    void parseConfigTagPartitions(QXmlStreamReader &xml, Device *device);
    void parseConfigTagPartition(QXmlStreamReader &xml, Device *device);
    void parseConfigTagPatchInfoDirs(QXmlStreamReader &xml);
    void parseConfigTagPatchInfoDir(QXmlStreamReader &xml);

    // XML parsing functions for the patchinfo files
    bool loadPatchInfoXml(const QString &path, const QString &pathId);
    void parsePatchInfoTagPatchinfo(QXmlStreamReader &xml, PatchInfo * const info);
    void parsePatchInfoTagMatches(QXmlStreamReader &xml, PatchInfo * const info);
    void parsePatchInfoTagNotMatched(QXmlStreamReader &xml, PatchInfo * const info);
    void parsePatchInfoTagName(QXmlStreamReader &xml, PatchInfo * const info);
    void parsePatchInfoTagRegex(QXmlStreamReader &xml, PatchInfo * const info);
    void parsePatchInfoTagExcludeRegex(QXmlStreamReader &xml, PatchInfo * const info);
    void parsePatchInfoTagRegexes(QXmlStreamReader &xml, PatchInfo * const info);
    void parsePatchInfoTagHasBootImage(QXmlStreamReader &xml, PatchInfo * const info, const QString &type);
    void parsePatchInfoTagRamdisk(QXmlStreamReader &xml, PatchInfo * const info, const QString &type);
    void parsePatchInfoTagPatchedInit(QXmlStreamReader &xml, PatchInfo * const info, const QString &type);
    void parsePatchInfoTagAutopatchers(QXmlStreamReader &xml, PatchInfo * const info, const QString &type);
    void parsePatchInfoTagAutopatcher(QXmlStreamReader &xml, PatchInfo * const info, const QString &type);
    void parsePatchInfoTagDeviceCheck(QXmlStreamReader &xml, PatchInfo * const info, const QString &type);
    void parsePatchInfoTagPartconfigs(QXmlStreamReader &xml, PatchInfo * const info, const QString &type);
    void parsePatchInfoTagInclude(QXmlStreamReader &xml, PatchInfo * const info, const QString &type);
    void parsePatchInfoTagExclude(QXmlStreamReader &xml, PatchInfo * const info, const QString &type);

    const QScopedPointer<PatcherPathsPrivate> d_ptr;
    Q_DECLARE_PRIVATE(PatcherPaths)
};

#endif // PATCHERPATHS_H
