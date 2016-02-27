from distutils.core import setup

setup(
	name         = 'eventfd',
	version      = '0.1',
	description  = 'Adds support for eventfd to Python 3.',
	url          = 'https://github.com/adityaramesh/python_eventfd',
	author       = 'Aditya Ramesh',
	author_email = '_@adityaramesh.com',
	license      = 'BSD',
	packages     = ['eventfd'],
	package_data = {'eventfd': ['_eventfd.so']}
)
