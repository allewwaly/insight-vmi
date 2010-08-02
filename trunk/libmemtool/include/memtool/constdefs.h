/*
 * constdefs.h
 *
 *  Created on: 02.08.2010
 *      Author: chrschn
 */

#ifndef CONSTDEFS_H_
#define CONSTDEFS_H_

/// File to save the history to, relative to home directory
extern const char* history_file;

/// Locking file
extern const char* lock_file;

/// Logging file
extern const char* log_file;

/// Socket to communicate between daemon and CLI tool
extern const char* sock_file;

#endif /* CONSTDEFS_H_ */
