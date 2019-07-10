* DBus Mockery

Currently a work in Progress.

** Summary
The DBus Mockery library wants to make interfacing with a DBus API almost as simple as defining a C++ interface.
The DBus protocol is entirely hidden from the user. 

Will probably come with an example definition of the BlueZ interface (which this was made for).

** Build
This project uses cmake.
You require libsystemd, because the systemd sd-bus library is used underneath.

** Examples
WIP