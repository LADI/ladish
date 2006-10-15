/* This file is part of Raul.  Copyright (C) 2006 Dave Robillard.
 * 
 * Raul is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Raul is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef RAUL_PROCESS_H
#define RAUL_PROCESS_H

#include <string>
#include <iostream>
#include <unistd.h>

/** A child process.
 *
 * \ingroup raul
 */
class Process
{
public:

	/** Launch a sub process.
	 *
	 * @param command can be a typical shell command with parameters, the PATH is searched etc.
	 */
	bool launch(std::string command)
	{
		const string executable = (command.find(" ") != string::npos)
			? command.substr(0, command.find(" "))
			: command;

		std::cerr << "Launching child process '" << executable << "' with command line '"
			<< command << "'" << endl;

		// Use the same double fork() trick as JACK to prevent zombie children
		const int err = fork();

		if (err == 0) {
			// (child)

			// close all nonstandard file descriptors
			const int max_fds = getdtablesize();
			int fd;
			for (fd = 3; fd < max_fds; ++fd)
				close(fd);

			switch (fork()) {

				case 0:
					// (grandchild)
					setsid();
					execlp(executable.c_str(), command.c_str(), NULL);
					_exit(-1);

				case -1:
					// (second) fork failed, there is no grandchild
					_exit (-1);

					/* exit the child process here */
				default: 
					_exit (0);
			}

		return (err > 0);
	}

private:
	// .... no :) (undefined)
	Process();
	Process(const Process&);
	Process& operator=(const Process&);
};


#endif // RAUL_PROCESS_H
