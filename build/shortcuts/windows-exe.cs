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

using System;
using System.IO;
using System.Diagnostics;
using System.Reflection;

public class WindowsLauncher {
#if (GUI)
    private const bool GUI = true;
#else
    private const bool GUI = false;
#endif

    public static int Main(string[] args) {
        string python;
        string script;
        if (GUI) {
            python = Path.Combine("pythonportable", "pythonw.exe");
            script = Path.Combine("scripts", "qtmain.py");
        } else {
            python = Path.Combine("pythonportable", "python.exe");
            script = Path.Combine("scripts", "patchfile.py");
        }

        string[] quotedArgs = new string[args.Length];
        for (int i = 0; i < args.Length; i++) {
            quotedArgs[i] = quote(args[i]);
        }

        String strPath = quote(fromApplicationDirectory(python));
        String strArgs = quote(fromApplicationDirectory(script));
        strArgs += " " + String.Join(" ", quotedArgs);

        ProcessStartInfo start = new ProcessStartInfo();
        start.FileName = strPath;
        start.Arguments = strArgs;
        start.UseShellExecute = false;

        int exitcode = 1;

        using (Process proc = Process.Start(start)) {
            if (proc != null) {
                proc.WaitForExit();
                exitcode = proc.ExitCode;
            } else {
                Console.WriteLine("Failed to start:");
                Console.WriteLine("Application: " + strPath);
                Console.WriteLine("Arguments: " + strArgs);
            }
        }

        if (!GUI) {
            Console.WriteLine("\nPress any key to exit.");
            Console.ReadKey(true);
        }

        return exitcode;
    }

    private static string getApplicationDirectory() {
        string file = Assembly.GetExecutingAssembly().Location;
        string dir = Path.GetDirectoryName(file);
        return dir;
    }

    private static string fromApplicationDirectory(string path) {
        return Path.Combine(getApplicationDirectory(), path);
    }

    private static string quote(string arg) {
        return "\"" + arg + "\"";
    }
}
