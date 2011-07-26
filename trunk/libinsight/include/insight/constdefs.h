/*
 * constdefs.h
 *
 *  Created on: 02.08.2010
 *      Author: chrschn
 */

#ifndef CONSTDEFS_H_
#define CONSTDEFS_H_

/// Multiplexer channels used to communicate with the InSight daemon
enum MuxerChannels {
	mcStdOut,
	mcStdErr,
	mcBinary,
	mcReturn
};

/// Binary data that can be retrieved from the InSight daemon
enum BinaryData {
    bdMemDumpList,
    bdInstance
};

/// File to save the history to, relative to home directory
extern const char* mt_history_file;

/// Locking file
extern const char* mt_lock_file;

/// Logging file
extern const char* mt_log_file;

/// Socket to communicate between daemon and CLI tool
extern const char* mt_sock_file;

#endif /* CONSTDEFS_H_ */
