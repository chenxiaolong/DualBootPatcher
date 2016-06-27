/*
 * Copyright 2012 bigbiff/Dees_Troy TeamWin
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

#ifndef _INFOMANAGER_HPP_HEADER
#define _INFOMANAGER_HPP_HEADER

#include <string>
#include <unordered_map>
#include <utility>

class InfoManager
{
public:
    InfoManager();
    explicit InfoManager(const std::string& filename);
    virtual ~InfoManager();
    void SetFile(const std::string& filename);
    void SetFileVersion(int version);
    void SetConst();
    void Clear();
    int LoadValues();
    int SaveValues();

    // Core get routines
    int GetValue(const std::string& varName, std::string& value);
    int GetValue(const std::string& varName, int& value);
    int GetValue(const std::string& varName, float& value);
    unsigned long long GetValue(const std::string& varName, unsigned long long& value);

    std::string GetStrValue(const std::string& varName);
    int GetIntValue(const std::string& varName);

    // Core set routines
    int SetValue(const std::string& varName, const std::string& value);
    int SetValue(const std::string& varName, const int value);
    int SetValue(const std::string& varName, const float value);
    int SetValue(const std::string& varName, const unsigned long long& value);

private:
    std::string File;
    std::unordered_map<std::string, std::string> mValues;
    int file_version;
    bool is_const;

};

#endif // _DATAMANAGER_HPP_HEADER
