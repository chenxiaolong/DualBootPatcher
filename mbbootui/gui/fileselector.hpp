/*
 * Copyright 2013 bigbiff/Dees_Troy TeamWin
 * This file is part of TWRP/TeamWin Recovery Project.
 *
 * TWRP is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * TWRP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with TWRP.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "gui/scrolllist.hpp"

class GUIFileSelector : public GUIScrollList
{
public:
    GUIFileSelector(xml_node<>* node);
    virtual ~GUIFileSelector();

public:
    // Update - Update any UI component animations (called <= 30 FPS)
    //  Return 0 if nothing to update, 1 on success and contiue, >1 if full render required, and <0 on error
    virtual int Update();

    // NotifyVarChange - Notify of a variable change
    virtual int NotifyVarChange(const std::string& varName, const std::string& value);

    // SetPageFocus - Notify when a page gains or loses focus
    virtual void SetPageFocus(int inFocus);

    virtual size_t GetItemCount();
    virtual void RenderItem(size_t itemindex, int yPos, bool selected);
    virtual void NotifySelect(size_t item_selected);

protected:
    struct FileData {
        std::string fileName;
        unsigned char fileType; // Uses d_type format from struct dirent
        mode_t protection;      // Uses mode_t format from stat
        uid_t userId;
        gid_t groupId;
        off_t fileSize;
        time_t lastAccess;      // Uses time_t format from stat
        time_t lastModified;    // Uses time_t format from stat
        time_t lastStatChange;  // Uses time_t format from stat
    };

protected:
    virtual int GetFileList(const std::string& folder);
    static bool fileSort(FileData d1, FileData d2);

protected:
    std::vector<FileData> mFolderList;
    std::vector<FileData> mFileList;
    std::string mPathVar; // current path displayed, saved in the data manager
    std::string mPathDefault; // default value for the path if none is set in mPathVar
    std::string mExtn; // used for filtering the file list, for example, *.zip
    std::string mVariable; // set when the user selects an item, pull path like /path/to/foo
    std::string mSortVariable; // data manager variable used to change the sorting of files
    std::string mSelection; // set when the user selects an item without the full path like selecting /path/to/foo would just be set to foo
    int mShowFolders, mShowFiles; // indicates if the list should show folders and/or files
    int mShowNavFolders; // indicates if the list should include the "up a level" item and allow you to traverse folders (nav folders are disabled for the restore list, for instance)
    static int mSortOrder; // must be static because it is used by the static function fileSort
    ImageResource* mFolderIcon;
    ImageResource* mFileIcon;
    bool updateFileList;
};
