# Running InSight #



# Introduction #

InSight can be [installed](Installation.md) system-wide or run from the source directory in which it was built. For each of these options InSight has to be invoked in a different way.

# Running InSight #

InSight consists of two parts: the front-end `insight` and the daemon `insightd`. The daemon contains all the intelligence of InSight while the front-end serves only as a "dump" terminal to connect to the shell-like interface of the daemon.

## `insightd` ##

For manual inspection of a memory dump and first steps with InSight, it is sufficient to run the daemon as a stand-alone foreground application. To do so, just run

```
insightd
```

if you installed InSight globally, or, to run from the source directory you just built it, execute

```
cd insightd
./insightd.sh
```

instead. You will see the command prompt of the InSight shell:

```
>>> _
```

You can try the `help` command for a list of available commands. A detailed description can be found at the [Shell page](InSightShell.md).

For the purpose of simple batch processing, `insight` reads any available input from `stdin`, evaluates it line by line according to its [shell syntax](InSightShell.md) and exits afterwards. The following example displays a list of available commands:

```
echo help | insightd
>>> help
The following represents a complete list of valid commands:
  exit        Exits the program
  help        Displays some help for a command
  list        Lists various types of read symbols
  memory      Load or unload a memory dump
  script      Executes a QtScript script file
  show        Shows information about a symbol given by name or ID
  stats       Shows various statistics.
  symbols     Allows to load, store or parse the kernel symbols
  sysinfo     Shows information about the host.
>>> Done, exiting.
```

### Command Line Parameters for `insightd` ###

The following list explains the command line parameters for `insightd`:

| **Parameter**                        | **Description** |
|:-------------------------------------|:----------------|
| `-d`, `--daemon`                   | Start as a background process and detach console |
| `-f`, `--foreground`               | Start as daemon but don't detach console, allowing to see diagnostic messages on the console |
| `-p` _srcdir_, `--parse` _srcdir_  | Parse the [debugging symbols](LinuxDebugSymbols.md) from the given kernel source directory _srcdir_ |
| `-l` _infile_, `--load` _infile_   | Read in previously saved debugging symbols from _infile_ |
| `-m` _infile_, `--memory` _infile_ | Load a memory dump or live guest physical memory from _infile_ |
| `-h`, `--help`                     | Display list of command line parameters |

A more up-to-date list might be available using the `--help` parameter from the command line:

```
insightd --help
```

## `insight` ##

The daemon part of InSight can be run as background process without a visible shell. In that case `insight` can be used to connect to this shell and execute arbitrary [shell commands](InSightShell.md). If `insightd` is running in [daemon mode](#Command_Line_Parameters_for_insightd.md), executing `insight` will connect to the daemon and show the shell's command prompt:

```
insight
Type "exit" to leave this shell.

>>> _
```

### Command Line Parameters for `insight` ###

`insight` provides command line parameters to interact with the `insightd` deamon as well as to control it:

| **Parameter**                              | **Description** |
|:-------------------------------------------|:----------------|
| `-sp` _file_, `--parse-symbols` _srcdir_ | Parse the [debugging symbols](LinuxDebugSymbols.md) from the given kernel source directory _srcdir_ |
| `-sl` _file_, `--load-symbols` _file_    | Read in previously saved debugging symbols from _file_ |
| `-ss` _file_, `--store` _file_           | Store the currently loaded debugging symbols as _file_ |
| `-ds`, `--start`                         | Start the InSight daemon, if not already running |
| `-dk`, `--stop`                          | Stop the InSight daemon, if it is running |
| `-dr`, `--status`                        | Show the status of the InSight daemon |
| `-md`, `--list-memdumps`                 | List the currently loaded memory dumps |
| `-ml` _file_, `--load-memdump` _file_    | Load a memory dump from _file_ |
| `-mu`, `--unload-memdump` _file_|_index_ | Unload a previously memory dump loaded from _file_ or being listed in the list at index _index_ |
| `-c` _command_, `--eval` _command_       | Evaluates _command_ in InSight's [shell syntax](InSightShell.md) |
| `-h`, `--help`                           | Display list of command line parameters |