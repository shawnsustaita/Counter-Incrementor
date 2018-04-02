/*
MIT License

Copyright (c) 2018 Shawn Sustaita

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


/*** INCLUDES ***/
#include <iostream>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <wait.h>
#include <fcntl.h>


/*** NAMESPACE ***/
using namespace std;


/*** FUNCTION PROTOTYPES ***/
void open_counter();
void read_counter();
void write_counter();
void acquire_lock();
void release_lock();
void increment_counter();
void reap_children();


/*** GLOBAL VARIABLES ***/
int fd;
int fork_delay;
int lock_delay;
int counter;
string file;


/*** MAIN FUNCTION ***/
int main(int argc, char *argv[]) {
	int nprocs, iterations;

	// Initialize variables
	if (argc != 6) {
		cerr << "usage: counter_incrementor nprocs iterations fork_delay lock_delay file\n";
		exit(EXIT_FAILURE);
	}
	nprocs = atoi(argv[1]);
	iterations = atoi(argv[2]);
	fork_delay = atoi(argv[3]);
	lock_delay = atoi(argv[4]);
	file = argv[5];

	// Initialize counter
	open_counter();
	counter = 0;
	write_counter();  // Non-critical write

	// Spawn nproc children (workers)
	for (int i = 0; i < nprocs; i++) {
		pid_t pid = fork();

		if (pid == 0) {  // Child
			sleep(fork_delay);  // Allow time to spawn all children

			// Increment counter iteration times
			for (int i = 0; i < iterations; i++) {
				acquire_lock();
				increment_counter();  // Critical locking IO region
				if (lock_delay > 0) {  // Hold lock to test NFSv4 lock leases
#ifdef DEBUG
					cout << "Holding lock for " << lock_delay << " second";
					cout << ((lock_delay > 1) ? "s" : "") << endl;
#endif
					sleep(lock_delay);
				}
				release_lock();
			}

			exit(EXIT_SUCCESS);  // Child exits
		}

		if (pid == -1)  // Error
			cerr << "fork failed: " << strerror(errno) << endl;
	}

	reap_children();

	// Check counter correctness
	read_counter();
	if (counter != (iterations * nprocs))
		cerr << counter << " != " << iterations * nprocs << endl;
}


void open_counter() {
	fd = open(file.c_str(), O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
	if (fd == -1) {
		cerr << "open failed: " << strerror(errno) << endl;
		exit(EXIT_FAILURE);
	}
}


// Seek to beginning of file
void seek_counter() {
	if (lseek(fd, 0, SEEK_SET) == -1) {
		cerr << "lseek failed: " << strerror(errno) << endl;
		exit(EXIT_FAILURE);
	}
}


void read_counter() {
	seek_counter();  // Seek to beginning of file
	if (read(fd, &counter, sizeof(counter)) == -1) {
		cerr << "read failed: " << strerror(errno) << endl;
		exit(EXIT_FAILURE);
	}
#ifdef DEBUG
	cout << "READ: " << counter << endl;
#endif
}


void write_counter() {
	seek_counter();  // Seek to beginning of file
	if (write(fd, &counter, sizeof(counter)) == -1) {
		cerr << "write failed: " << strerror(errno) << endl;
		exit(EXIT_FAILURE);
	}
#ifdef DEBUG
	cout << "WROTE: " << counter << endl;
#endif
}


void acquire_lock() {
	struct flock flock_t;

	flock_t.l_whence = SEEK_SET;
	flock_t.l_start = 0;
	flock_t.l_len = 0;
	flock_t.l_type = F_WRLCK;

	if (fcntl(fd, F_SETLKW, &flock_t) == -1) {
		cerr << "fcntl failed: " << strerror(errno) << " [" << errno << "]\n";
		exit(EXIT_FAILURE);
	}
}


void release_lock() {
	struct flock flock_t;

	flock_t.l_whence = SEEK_SET;
	flock_t.l_start = 0;
	flock_t.l_len = 0;
	flock_t.l_type = F_UNLCK;

	if (fcntl(fd, F_SETLKW, &flock_t) == -1) {
		cerr << "fcntl failed: " << strerror(errno) << endl;
		exit(EXIT_FAILURE);
	}
}


void increment_counter() {
	read_counter();
	counter++;
	write_counter();
}


void reap_children() {
	while (pid_t pid = wait(NULL) != -1)
		continue;
	if (errno != ECHILD)  // Unexpected error
		cerr << "wait failed: " << strerror(errno) << endl;
}
