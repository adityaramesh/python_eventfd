try:
	from _eventfd import __doc__, Event, EFD_CLOEXEC, EFD_NONBLOCK, EFD_SEMAPHORE
except ImportError:
	from eventfd._eventfd import __doc__, Event, EFD_CLOEXEC, EFD_NONBLOCK, EFD_SEMAPHORE
