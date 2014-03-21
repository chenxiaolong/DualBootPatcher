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
