#ifndef JFLTERAMDISKPATCHER_H
#define JFLTERAMDISKPATCHER_H

#include <libdbp/cpiofile.h>
#include <libdbp/plugininterface.h>

#include <QtCore/QObject>


class JflteRamdiskPatcherPrivate;

class JflteRamdiskPatcher : public RamdiskPatcher
{
public:
    explicit JflteRamdiskPatcher(const PatcherPaths * const pp,
                                 const QString &id,
                                 const FileInfo * const info,
                                 CpioFile * const cpio);
    ~JflteRamdiskPatcher();

    static const QString AOSP;
    static const QString CXL;
    static const QString GoogleEdition;
    static const QString TouchWiz;

    virtual PatcherError::Error error() const;
    virtual QString errorString() const;

    virtual QString name() const;

    virtual bool patchRamdisk();

private:
    bool patchAOSP();
    bool patchCXL();
    bool patchGoogleEdition();
    bool patchTouchWiz();

    // chenxiaolong's noobdev
    bool cxlModifyInitTargetRc();

    // Google Edition
    bool geModifyInitRc();

    // TouchWiz
    bool twModifyInitRc();
    bool twModifyInitTargetRc();
    bool twModifyUeventdRc();
    bool twModifyUeventdQcomRc();

    // Google Edition and TouchWiz
    bool getwModifyMsm8960LpmRc();

    const QScopedPointer<JflteRamdiskPatcherPrivate> d_ptr;
    Q_DECLARE_PRIVATE(JflteRamdiskPatcher)
};
#endif // JFLTERAMDISKPATCHER_H
