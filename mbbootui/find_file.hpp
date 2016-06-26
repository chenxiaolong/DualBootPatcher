/*
 * Copyright 2014 TeamWin
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

#ifndef Find_File_HPP
#define Find_File_HPP

#include <string>
#include <vector>

class Find_File
{
public:
    static std::string Find(const std::string& file_name, const std::string& start_path);
private:
    Find_File();
    std::string Find_Internal(const std::string& filename, const std::string& starting_path);
    std::vector<std::string> searched_dirs;
};

#endif
