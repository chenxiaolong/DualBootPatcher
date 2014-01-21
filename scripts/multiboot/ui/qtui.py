import sys

listener = None
current_progress = 0


def add_task(task):
    listener.trigger_add_task.emit(task)


def set_task(task):
    listener.trigger_set_task.emit(task)
    clear()


# Details about task
def details(msg):
    listener.trigger_details.emit(msg)


def clear():
    listener.trigger_clear.emit()


# Miscellaneous info
def info(msg):
    listener.trigger_info.emit(msg)


# Failed to run command
def command_error(output="", error=""):
    listener.trigger_command_error.emit(output, error)


# Process failed
def failed(msg):
    listener.trigger_failed.emit(msg)


# Process succeeded
def succeeded(msg):
    listener.trigger_succeeded.emit(msg)


# Set maximum progress
def max_progress(num):
    global current_progress
    listener.trigger_max_progress.emit(num)
    current_progress = 0


# Increment progress counter
def progress():
    global current_progress
    current_progress += 1
    listener.trigger_set_progress.emit(current_progress)
