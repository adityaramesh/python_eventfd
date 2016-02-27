# Overview

Adds support for eventfd to Python 3.

# Usage

	from eventfd import Event, EFD_CLOEXEC, EFD_NONBLOCK

	e = Event(0, EFD_CLOEXEC | EFD_NONBLOCK)
	e.read()    # Raises BlockingIOError
	e.write(10)
	e.read()    # Returns 10
	e.read()    # Raises BlockingIOError
	e.close()   # Automatically called when garbage-collected.

# Requirements

Linux kernel version 2.6.30 or higher.

# Installation

First, define the following environment variables:

- `CXX`: Path to a version of g++ or clang++ that is C++14-compliant.
- `PYTHON_INCLUDE_PATH`: Path to the include directory of the active Python installation.

Then run the following commands:

	rake
	pip install .
