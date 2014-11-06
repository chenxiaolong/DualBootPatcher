/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "patchfilepatcher.h"

#include <fstream>
#include <unordered_map>

//#include <boost/algorithm/string.hpp>
//#include <boost/regex.hpp>


/*! \cond INTERNAL */
class PatchFilePatcher::Impl
{
public:
    const PatcherPaths *pp;
    const FileInfo *info;
    PatchInfo::AutoPatcherArgs args;

    std::vector<std::string> files;

    PatcherError error;
};
/*! \endcond */


#define PATCH_FAIL_MSG "Failed to run \"patch\" command"

const std::string PatchFilePatcher::Id("PatchFile");

static const std::string ArgFile("file");


PatchFilePatcher::PatchFilePatcher(const PatcherPaths * const pp,
                                   const FileInfo * const info,
                                   const PatchInfo::AutoPatcherArgs &args)
    : m_impl(new Impl())
{
#if 0
    m_impl->pp = pp;
    m_impl->info = info;
    m_impl->args = args;

    // The arguments should have the patch to the file
    if (args.find(ArgFile) == args.end()) {
        qWarning() << "Arguments does not contain the path to a patch file";
        return;
    }

    std::string fileName(pp->patchesDirectory() + "/" + args[ArgFile]);
    std::ifstream file(fileName, std::ios::binary);

    if (file.fail()) {
        qWarning() << "Failed to open" << QString::fromStdString(fileName);
        file.close();
        return;
    }

    file.seekg(0, std::ios::end);
    auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<unsigned char> patchContents(size);

    if (!file.read(reinterpret_cast<char *>(patchContents.data()), size)) {
        qWarning() << "Failed to read" << QString::fromStdString(fileName);
        file.close();
        return;
    }

    file.close();

    // Assume the patch file is UTF-8 encoded
    std::string contents(patchContents.begin(), patchContents.end());
    std::vector<std::string> lines;
    boost::split(lines, contents, boost::is_any_of("\n"));

    const boost::regex reOrigFile("---\\s+(?P<file>.+)(?:\n|\t)");

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        boost::smatch what;

        if (boost::regex_search(*it, what, reOrigFile)) {
            std::string file = what.str("file");
            if (boost::starts_with(file, "\"") && boost::ends_with(file, "\"")) {
                file.erase(file.begin());
                file.erase(file.end() - 1);
            }

            // Skip files containing escaped characters
            if (file.find("\\") != std::string::npos) {
                continue;
            }

            // Strip leading slash (-p1)
            auto it = file.find("/");
            if (file.find("/") != std::string::npos) {
                file.erase(file.begin(), it + 1);
            }

            m_impl->files.push_back(file);
        }
    }
#else
    (void) pp;
    (void) info;
    (void) args;
#endif
}

PatchFilePatcher::~PatchFilePatcher()
{
}

PatcherError PatchFilePatcher::error() const
{
    return m_impl->error;
}

std::string PatchFilePatcher::id() const
{
    return Id;
}

std::vector<std::string> PatchFilePatcher::newFiles() const
{
    return std::vector<std::string>();
}

std::vector<std::string> PatchFilePatcher::existingFiles() const
{
    return m_impl->files;
}

bool PatchFilePatcher::patchFile(const std::string &file,
                                 std::vector<unsigned char> * const contents,
                                 const std::vector<std::string> &bootImages)
{
#if 0
    Q_D(PatchFilePatcher);
    Q_UNUSED(bootImages);

    if (d->patches.isEmpty()) {
        d->errorCode = PatcherError::CustomError;
        d->errorString = tr("Failed to parse patch file: %1").arg(d->args[ArgFile]);
        return false;
    }

    // Write the file to be patched to a temporary file
    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        d->errorCode = PatcherError::CustomError;
        d->errorString = tr("Failed to create temporary directory");
        return false;
    }

    if (d->patches.isEmpty()) {
        d->errorCode = PatcherError::CustomError;
        d->errorString = tr("The patch file is empty");
        return false;
    }

    QString path(tempDir.path() % QStringLiteral("/file"));
    QFile f(path);
    f.open(QFile::WriteOnly);
    f.write(*contents);
    f.close();

#if defined(Q_OS_ANDROID)
    QString patch = d->pp->binariesDirectory() % Sep
            % QStringLiteral("android") % Sep
            % d->info->device()->architecture() % Sep
            % QStringLiteral("patch");
#elif defined(Q_OS_WIN32)
    QString patch = d->pp->binariesDirectory() % Sep
            % QStringLiteral("windows") % Sep
            % QStringLiteral("hctap.exe");
#else
    QString patch = QStringLiteral("patch");
#endif

    QProcess process;
    process.setWorkingDirectory(tempDir.path());
    process.start(patch,
                  QStringList() << QStringLiteral("--no-backup-if-mismatch")
                                << QStringLiteral("-p1")
                                << QStringLiteral("-d")
                                << tempDir.path());
    if (!process.waitForStarted()) {
        d->errorCode = PatcherError::CustomError;
        d->errorString = tr(PATCH_FAIL_MSG);
        return false;
    }

    process.write(QStringLiteral("--- a/file\n").toUtf8());
    process.write(QStringLiteral("+++ b/file\n").toUtf8());
    process.write(d->patches[file]);
    process.closeWriteChannel();

    if (!process.waitForFinished()) {
        d->errorCode = PatcherError::CustomError;
        d->errorString = tr(PATCH_FAIL_MSG);
        return false;
    }

    // Print console output
    qDebug() << process.readAll();

    bool ret = process.exitStatus() == QProcess::NormalExit
            && process.exitCode() == 0;

    if (!ret) {
        d->errorCode = PatcherError::CustomError;
        d->errorString = tr(PATCH_FAIL_MSG);
    } else {
        f.open(QFile::ReadOnly);
        *contents = f.readAll();
        f.close();
    }

    return ret;
#else
    (void) file;
    (void) contents;
    (void) bootImages;
    return false;
#endif
}
