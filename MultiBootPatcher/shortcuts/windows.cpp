/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <shlwapi.h>
#include <windows.h>
#include <cstdio>
#include <iostream>
#include <string>

#ifndef APPLICATION
#error "APPLICATION not defined!"
#endif

#ifndef GUI
// http://www.cplusplus.com/forum/windows/55426/#msg299186
void pause() __attribute__((destructor));
#endif

std::string getApplicatonDirectory() {
    char buf[MAX_PATH];
    GetModuleFileName(NULL, buf, MAX_PATH);
    std::string path(buf);
    std::string::size_type pos = path.find_last_of("\\/");
    return path.substr(0, pos);
}

std::string fromApplicationDirectory(char *relPath) {
    char buf[MAX_PATH];
    PathCombine(buf, getApplicatonDirectory().c_str(), relPath);
    return std::string(buf);
}

int mainProg(std::string progArgs) {
    std::string application = fromApplicationDirectory(APPLICATION);

    std::string args;
    args.append("\"");
    args.append(application);
    args.append("\"");

    if (progArgs.size() > 0) {
        args.append(" ");
        args.append(progArgs);
    }

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    BOOL ret = CreateProcess(
        (LPSTR) application.c_str(),
        (LPSTR) args.c_str(),
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi
    );

    if (!ret) {
        printf("CreateProcess failed (%d).\n", GetLastError());
        return 1;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return 0;
}

#ifdef GUI
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine,
                     int nCmdShow) {
    return mainProg(lpCmdLine);
}
#else
int main(int argc, char *argv[]) {
    std::string args;
    for (int i = 1; i < argc; i++) {
        args.append(" \"");
        args.append(argv[i]);
        args.append("\"");
    }
    return mainProg(args);
}
#endif

void pause() {
    std::cout << std::endl << "Press Enter to exit." << std::endl;
    std::cin.sync();
    std::cin.ignore();
}
