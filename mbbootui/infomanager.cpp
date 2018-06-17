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

#include <sstream>
#include <string>
#include <unordered_map>

#include <cstdlib>

#include "mblog/logging.h"

#include "infomanager.hpp"

#define LOG_TAG "mbbootui/infomanager"

InfoManager::InfoManager()
{
    file_version = 0;
    is_const = false;
}

InfoManager::InfoManager(const std::string& filename)
{
    file_version = 0;
    is_const = false;
    SetFile(filename);
}

InfoManager::~InfoManager()
{
    Clear();
}

void InfoManager::SetFile(const std::string& filename)
{
    File = filename;
}

void InfoManager::SetFileVersion(int version)
{
    file_version = version;
}

void InfoManager::SetConst()
{
    is_const = true;
}

void InfoManager::Clear()
{
    mValues.clear();
}

int InfoManager::LoadValues()
{
    std::string str;

    // Read in the file, if possible
    FILE* in = fopen(File.c_str(), "rbe");
    if (!in) {
        LOGI("InfoManager file '%s' not found.", File.c_str());
        return -1;
    } else {
        LOGI("InfoManager loading from '%s'.", File.c_str());
    }

    if (file_version) {
        int read_file_version;
        if (fread(&read_file_version, 1, sizeof(int), in) != sizeof(int)) {
            goto error;
        }
        if (read_file_version != file_version) {
            LOGI("InfoManager file version has changed, not reading file");
            goto error;
        }
    }

    while (!feof(in)) {
        std::string Name;
        std::string Value;
        unsigned short length;
        char array[513];

        if (fread(&length, 1, sizeof(unsigned short), in) != sizeof(unsigned short)) {
            goto error;
        }
        if (length >= sizeof(array) - 1) {
            goto error;
        }
        if (fread(array, 1, length, in) != length) {
            goto error;
        }
        array[length + 1] = '\0';
        Name = array;

        if (fread(&length, 1, sizeof(unsigned short), in) != sizeof(unsigned short)) {
            goto error;
        }
        if (length >= sizeof(array) - 1) {
            goto error;
        }
        if (fread(array, 1, length, in) != length) {
            goto error;
        }
        array[length + 1] = '\0';
        Value = array;

        auto pos = mValues.find(Name);
        if (pos != mValues.end()) {
            pos->second = Value;
        } else {
            mValues.insert(make_pair(Name, Value));
        }
    }
error:
    fclose(in);
    return 0;
}

int InfoManager::SaveValues()
{
    if (File.empty()) {
        return -1;
    }

    LOGI("InfoManager saving '%s'", File.c_str());
    FILE* out = fopen(File.c_str(), "wbe");
    if (!out) {
        return -1;
    }

    if (file_version) {
        fwrite(&file_version, 1, sizeof(int), out);
    }

    for (auto iter = mValues.begin(); iter != mValues.end(); ++iter) {
        unsigned short length = (unsigned short) iter->first.length() + 1;
        fwrite(&length, 1, sizeof(unsigned short), out);
        fwrite(iter->first.c_str(), 1, length, out);
        length = (unsigned short) iter->second.length() + 1;
        fwrite(&length, 1, sizeof(unsigned short), out);
        fwrite(iter->second.c_str(), 1, length, out);
    }
    fclose(out);
    return 0;
}

int InfoManager::GetValue(const std::string& varName, std::string& value)
{
    std::string localStr = varName;

    auto pos = mValues.find(localStr);
    if (pos == mValues.end()) {
        return -1;
    }

    value = pos->second;
    return 0;
}

int InfoManager::GetValue(const std::string& varName, int& value)
{
    std::string data;

    if (GetValue(varName,data) != 0) {
        return -1;
    }

    value = atoi(data.c_str());
    return 0;
}

int InfoManager::GetValue(const std::string& varName, float& value)
{
    std::string data;

    if (GetValue(varName,data) != 0) {
        return -1;
    }

    value = atof(data.c_str());
    return 0;
}

unsigned long long InfoManager::GetValue(const std::string& varName, unsigned long long& value)
{
    std::string data;

    if (GetValue(varName,data) != 0) {
        return -1;
    }

    value = strtoull(data.c_str(), nullptr, 10);
    return 0;
}

// This function will return an empty string if the value doesn't exist
std::string InfoManager::GetStrValue(const std::string& varName)
{
    std::string retVal;

    GetValue(varName, retVal);
    return retVal;
}

// This function will return 0 if the value doesn't exist
int InfoManager::GetIntValue(const std::string& varName)
{
    std::string retVal;
    GetValue(varName, retVal);
    return atoi(retVal.c_str());
}

int InfoManager::SetValue(const std::string& varName, const std::string& value)
{
    // Don't allow empty names or numerical starting values
    if (varName.empty() || (varName[0] >= '0' && varName[0] <= '9')) {
        return -1;
    }

    auto pos = mValues.find(varName);
    if (pos == mValues.end()) {
        mValues.insert(make_pair(varName, value));
    } else if (!is_const) {
        pos->second = value;
    }

    return 0;
}

int InfoManager::SetValue(const std::string& varName, const int value)
{
    std::ostringstream valStr;
    valStr << value;
    return SetValue(varName, valStr.str());
}

int InfoManager::SetValue(const std::string& varName, const float value)
{
    std::ostringstream valStr;
    valStr << value;
    return SetValue(varName, valStr.str());
}

int InfoManager::SetValue(const std::string& varName, const unsigned long long& value)
{
    std::ostringstream valStr;
    valStr << value;
    return SetValue(varName, valStr.str());
}
