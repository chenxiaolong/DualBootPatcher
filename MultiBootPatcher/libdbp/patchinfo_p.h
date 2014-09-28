#ifndef PATCHINFO_P_H
#define PATCHINFO_P_H

#include "patchinfo.h"

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>

class PatchInfoPrivate
{
public:
    PatchInfoPrivate();

    QString path;

    // The name of the ROM
    QString name;

    // Regular expressions for matching the filename
    QStringList regexes;

    // If the regexes are over-encompassing, these regexes are used to
    // exclude the filename
    QStringList excludeRegexes;

    // List of regexes used for conditionals
    QStringList condRegexes;

    // Whether the patchinfo has a <not-matched> elements (acts as the
    // "else" statement for the conditionals)
    bool hasNotMatchedElement;

    // ** For the variables below, use hashmap[Default] to get the default
    //    values and hashmap[regex] with the index in condRegexes to get
    //    the overridden values.
    //
    // NOTE: If the variable is a list, the "overridden" values are used
    //       in addition to the default values

    // List of autopatcher plugins to use
    QHash<QString, PatchInfo::AutoPatcherItems> autoPatchers;

    // Whether or not the file contains (a) boot image(s)
    QHash<QString, bool> hasBootImage;

    // Attempt to autodetect boot images (finds .img files and checks their headers)
    QHash<QString, bool> autodetectBootImages;

    // List of manually specified boot images
    QHash<QString, QStringList> bootImages;

    // Ramdisk plugin to use
    QHash<QString, QString> ramdisk;

    // Name of alternative binary to use
    QHash<QString, QString> patchedInit;

    // Whether or not device checks/asserts should be kept
    QHash<QString, bool> deviceCheck;

    // List of supported partition configurations
    QHash<QString, QStringList> supportedConfigs;
};

#endif // PATCHINFO_P_H
