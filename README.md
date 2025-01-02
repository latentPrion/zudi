# Alert: Obsolete repository:

[!WARNING]
This project is no longer its own standalone git repository.
It has been wholly absorbed into the zambesii kernel git
repository and currently lives in the programs/ subdirectory
of the Zambesii kernel source tree.

# Legacy README content follows below here:

This project is a series of sub projects, all of which are geared toward
the support of the Uniform Driver Interface in the Zambesii kernel.

Currently it comprises:
	* A binary database compiler, which parses and compiles
	  UDI udiprops driver configuration files into a machine readable
	  format on behalf of the kernel.
	* A simple shell script which gathers all udiprops files within a target
	  folder, and adds them to the binary index.
	* Minimally modified UDI C headers, which are suitable for Zambesii.

