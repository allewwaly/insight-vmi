
#include <QCoreApplication>
#include <QDir>
#include <QDateTime>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <memtool/constdefs.h>

#include "debug.h"
#include "kernelsymbols.h"
#include "shell.h"
#include "genericexception.h"
#include "programoptions.h"


/**
 * Log a message to stderr, prepended with the current date and time.
 * @param msg the message to log
 */
void log_message(const QString& msg)
{
    std::cerr
        << QDateTime::currentDateTime().toString(Qt::SystemLocaleShortDate).toStdString()
        << " " << msg << std::endl;
}


/**
 * Signal handler for the Linux signals sent to daemon process, for more
 * signals, refer to
 * http://www.comptechdoc.org/os/linux/programming/linux_pgsignals.html
 */
void signal_handler(int sig)
{
	switch (sig) {

	case SIGHUP:
		break;

	case SIGTERM:
	    if (shell) {
	        shell->shutdown();
	        shell->wait();
	    }
		break;
	}
}


/**
 * Creates a background process out of the application, source code taken from:
 * http://www.enderunix.org/docs/eng/daemon.php
 * with some minor modifications
 */
void init_daemon()
{
	// Check if our parent's ID is already 1 (i.e., we are already daemonized
	if (getppid() == 1) {
		fprintf(stderr, "This process already is a child of \"init\".\n");
		return;
	}
	// Create a new process
	pid_t pid = fork();
	if (pid < 0) {
		fprintf(stderr, "Cannot fork this process (error no. %d), aborting.\n",
				errno);
		exit(1);
	}
	// Parent receives child's PID and exits
	if (pid > 0)
		exit(0);

	// Child continues as daemon

	// Create a new session with this process as process group leader
	pid_t sid = setsid();
	if (sid < 0) {
		fprintf(stderr, "Cannot create a new session (error no. %d), "
				"aborting.\n", errno);
		exit(2);
	}

	// Set permissions for newly created files
	umask(027);

	QDir home = QDir::home();

	// Change running directory to home
	chdir((char*) home.absolutePath().toAscii().data());

	// Create a lock file
	QByteArray lockFile = home.absoluteFilePath(lock_file).toLocal8Bit();
	int lock_fd = open(lockFile.data(), O_RDWR | O_CREAT, 0640);
	if (lock_fd < 0) {
		fprintf(stderr, "Cannot open lock file \"%s\" (error no. %d), "
				"aborting.\n", lockFile.data(), errno);
		exit(3);
	}

    // Try to lock the file
	if (lockf(lock_fd, F_TLOCK, 0) < 0) {
	    // Read in the PID
	    const int bufsize = 1024;
	    char buf[bufsize] = {0};
	    ssize_t len = read(lock_fd, buf, bufsize-1);
	    if (len < 0) {
	        fprintf(stderr, "Cannot read PID from lock file \"%s\" (error no. %d), "
	                "aborting.\n", lockFile.data(), errno);
	        exit(4);
	    }
	    for (int i = 0; i < len; ++i) {
	        if (buf[i] < '0' || buf[i] > '9') {
	            buf[i] = 0;
	            break;
	        }
	    }

		printf("Daemon already running with PID %s.\n", buf);
		exit(0);
	}
	else
        printf("Started daemon with PID %d.\n", getpid());

	// Write PID to lock file
	QByteArray myPid = QString("%1\n").arg(getpid()).toLocal8Bit();
	write(lock_fd, myPid.data(), myPid.size());

	// Register signal handler for interesting signals
	signal(SIGCHLD, SIG_IGN);       // ignore child
	signal(SIGTSTP, SIG_IGN); 		// ignore tty signals
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGHUP, signal_handler);  // catch hangup signal
	signal(SIGTERM, signal_handler); // catch kill signal

	// Close all file descriptors that the parent might have previously opened,
	// including stdin, stdout, and stderr, but exclude lock_fd
	for (int i = getdtablesize(); i >= 0; --i)
		if (i != lock_fd)
		    close(i);

	// Use /dev/null for stdin and, stdout
	int fd = open("/dev/null", O_RDWR);
	dup(fd);
	// Use log file for stderr
	QByteArray logFile = home.absoluteFilePath(log_file).toLocal8Bit();
	open(logFile.data(), O_CREAT|O_APPEND|O_WRONLY, 0640);
}


/**
 * Entry point function that sets up the environment, parses the command line
 * options, performs the requested actions and/or spawns an interactive shell.
 */
int main(int argc, char* argv[])
{
	QCoreApplication app(argc, argv);

	// Parse the command line options
	QStringList args = app.arguments();
    args.pop_front();
    if (!programOptions.parseCmdOptions(args))
        return 1;

	int ret = 0;
    bool daemonize = (programOptions.activeOptions() & opDaemonize);

	try {
		if (daemonize) {
			init_daemon();
		    // Start a new logging session
			log_message(QString("Memtool started with PID %1.").arg(getpid()));
		}

	    shell = new Shell(daemonize);
        KernelSymbols& sym = shell->symbols();

	    // Perform any initial action that might be given
	    switch (programOptions.action()) {
	    case acNone:
	        break;
	    case acParseSymbols:
	        sym.parseSymbols(programOptions.inFileName());
	        break;
	    case acLoadSymbols:
	        sym.loadSymbols(programOptions.inFileName());
	        break;
	    case acUsage:
	        ProgramOptions::cmdOptionsUsage();
	        return 0;
	    }

        // Start the interactive shell
		shell->start();

		ret = app.exec();

		if (daemonize)
            log_message(QString("Memtool exited with return code %1.").arg(ret));
		if (shell->interactive())
		    shell->out() << "Done, exiting." << endl;
	}
	catch (GenericException e) {
	    QString msg = QString("Caught exception at %1:%2\nMessage: %3")
	            .arg(e.file)
	            .arg(e.line)
	            .arg(e.message);
	    if (daemonize)
	        log_message(msg);
	    else
	        shell->err() << msg << endl;
	    return 1;
	}

	// Wait for the shell to exit
	if (shell) {
		while(!shell->isFinished()){
		 	shell->quit();
		}
		delete shell;
	}
    return ret;
}



