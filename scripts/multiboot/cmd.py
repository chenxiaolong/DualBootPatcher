import multiboot.debug as debug

import subprocess


def run_command(command,
                stdin_data=None,
                cwd=None,
                universal_newlines=True):
    debug.debug("Running command: " + str(command))

    try:
        process = subprocess.Popen(
            command,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            cwd=cwd,
            universal_newlines=universal_newlines
        )
        output, error = process.communicate(input=stdin_data)

        exit_code = process.returncode

        return (exit_code, output, error)

    except:
        raise Exception("Failed to run command: \"%s\"" % ' '.join(command))
