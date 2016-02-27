#include <array>
#include <cerrno>
#include <cstdint>

#include <unistd.h>
#include <sys/eventfd.h>

#include <Python.h>

#define EXPORT_SYMBOL __attribute__((visibility("default")))

namespace {

struct event_data;

using obj_ptr = PyObject*;
using event_ptr = event_data*;

struct event_data
{
	// Macro that provides a refcount and a pointer to an object that specifies this object's
	// type.
	PyObject_HEAD
	int fd;
};

extern "C"
auto event_close(event_ptr self) noexcept
{
	if (self->fd == 0)
		return Py_BuildValue("");

	auto res = 0;
	do res = ::close(self->fd);
	while (res == -1 and errno == EINTR);

	if (res == 0 or (res == -1 and errno == EBADF))
		self->fd = 0;
	if (res == 0)
		return Py_BuildValue("");

	PyErr_SetFromErrno(PyExc_OSError);
	return obj_ptr{};
}

/*
** Deallocates the memory for a new object instance. This function is called after destruction.
*/
extern "C"
auto event_dealloc(event_ptr self) noexcept
{
	event_close(self);
	Py_XDECREF(self->fd);

	// Py_TYPE accesses the ob_type member of the object, so that we can deallocate this
	// instance.
	Py_TYPE(self)->tp_free((PyObject*)self);
}

/*
** Allocates the memory for a new object instance. This function is called before initialization.
*/
extern "C"
auto event_new(PyTypeObject* type, obj_ptr, obj_ptr) noexcept
{
	/*
	** Allocates memory for this instance of ``Event``. The amount of memory allocated is given
	** by ``tp_basicsize + n * tp_itemsize``, where ``n`` is the number of items of size
	** ``tp_itemsize`` (for variable-length objects).
	*/
	auto self = (event_ptr)type->tp_alloc(type, 0);
	self->fd = 0;
	return (obj_ptr)self;
}

extern "C"
auto event_init(event_ptr self, obj_ptr args, obj_ptr) noexcept
{
	auto initval = unsigned{};
	auto flags   = int{};

	if (not PyArg_ParseTuple(args, "ii", &initval, &flags))
		return -1;

	auto res = ::eventfd(initval, flags);

	if (res > 0) {
		self->fd = res;
		return 0;
	}

	PyErr_SetFromErrno(PyExc_OSError);
	return -1;
}

extern "C"
auto event_fileno(event_ptr self) noexcept
{ return PyLong_FromLong(self->fd); }

extern "C"
auto event_read(event_ptr self) noexcept
{
	auto v = uint64_t{};
	auto res = ssize_t{};

	do res = ::read(self->fd, &v, 8);
	while (res == -1 and errno == EINTR);

	if (res > 0)
		return PyLong_FromLong(v);

	PyErr_SetFromErrno(PyExc_OSError);
	return obj_ptr{};
}

extern "C"
auto event_write(event_ptr self, obj_ptr args) noexcept
{
	auto v = uint64_t{};
	if (not PyArg_ParseTuple(args, "i", &v))
		return Py_BuildValue("");

	auto res = int{};
	do res = ::write(self->fd, &v, 8);
	while (res == -1 and errno == EINTR);

	if (res > 0)
		return Py_BuildValue("");

	PyErr_SetFromErrno(PyExc_OSError);
	return obj_ptr{};
}

auto event_methods = std::array<PyMethodDef, 5>{{
	{"fileno", (PyCFunction)event_fileno, METH_NOARGS,
	"Returns the underlying file descriptor."},

	{"read", (PyCFunction)event_read, METH_NOARGS,
	"Reads an 8-byte integer from the underlying eventfd object."},

	{"write", (PyCFunction)event_write, METH_VARARGS,
	"Writes an 8-byte integer to the underlying eventfd object."},

	{"close", (PyCFunction)event_close, METH_NOARGS,
	"Closes the event file descriptor."},

	// Sentinel.
	{nullptr}
}};

/*
** We can't make this object const, since the members initialized by ``PyVarObject_HEAD_INIT`` will
** be changed over the course of execution of the program. When the reference count reaches zero,
** the class definition itself will be garbage collected.
*/
auto event_def = PyTypeObject
{
	/*
	** The first three fields of ``PyTypeObject`` (after macro expansion) are the following: ::
	**
	** 	Py_ssize_t ob_refcnt;
	** 	struct _typeobject *ob_type;
	** 	Py_ssize_t ob_size;
	**
	** - ``ob_refcnt`` is set to one by the macro.
	** - ``ob_type`` is the metatype of the class. It is set to ``&PyType_Type`` by the macro.
	** - ``ob_size`` is not used, and is present only for backward compatibility.
	*/
	PyVarObject_HEAD_INIT(nullptr, 0)

	// tp_name
	"eventfd.Event",

	// tp_basicsize (amount of memory allocated by ``PyObject_New()``
	sizeof(event_data),

	// tp_itemsize (for variable length objects, like lists and strings)
	0,

	// tp_dealloc (optional custom ``__del__``; called when refcount reaches zero)
	(destructor)event_dealloc,

	// tp_print (called when printed to real file; don't use)
	nullptr,

	// tp_getattr (gets attr from ``char*``; deprecated)
	nullptr,

	// tp_setattr (sets attr from ``char*``; deprecated)
	nullptr,

	// tp_as_async (support for async/await/yield)
	nullptr,

	// tp_repr (``__repr__``)
	nullptr,

	// tp_as_number (abstract protocol support)
	nullptr,

	// tp_as_sequence (abstract protocol support)
	nullptr,

	// tp_as_mapping  (abstract protocol support)
	nullptr,

	// tp_hash (``__hash__``)
	nullptr,

	// tp_call (like ``operator()()`` in C++)
	nullptr,

	// tp_str (``__str__``)
	nullptr,

	// tp_getattro (like ``tp_getattr``, but takes ``PyObject*`` instead)
	nullptr,

	// tp_setattro (like ``tp_setattr``, bu takes ``PyObject*`` instead)
	nullptr,

	// tp_as_buffer (abstract protocol support)
	nullptr,

	// tp_flags (bitmask used for a variety of purposes). Note that since we haven't OR'd in
	// ``Py_TPFLAGS_BASETYPE``, the class is final.
	Py_TPFLAGS_DEFAULT,

	// tp_doc (docstring for the class)
	"Manages an eventfd resource.",

	// tp_traverse (support for cyclic GC, if needed)
	nullptr,

	// tp_clear (support for cyclic GC, if needed)
	nullptr,

	// tp_richcompare (used for object comparison)
	nullptr,

	// tp_weaklistoffset (support for weak references)
	0,

	// tp_iter (``__iter__``)
	nullptr,

	// tp_iternext (``__next__``)
	nullptr,

	// tp_methods
	event_methods.data(),

	// tp_members (pointer to structure describing class members). Since we don't want the
	// descriptor to be directly accessible, we leave this alone.
	nullptr,

	// tp_getset
	nullptr,

	// tp_base (defaults to ``&PyBaseObject_Type``)
	nullptr,

	// tp_dict (``__dict__``)
	nullptr,

	// tp_descr_get (function to get description)
	nullptr,

	// tp_descr_set (function to set description)
	nullptr,

	// tp_dictoffset (offset to the dict member in the data struct)
	0,

	// tp_init (``__init__``)
	(initproc)event_init,

	// tp_alloc (optional custom instance allocation function)
	nullptr,

	// tp_new (``__new__``)
	event_new
};

auto module_def = PyModuleDef{
	PyModuleDef_HEAD_INIT,

	 // m_name
	"_eventfd",

	// m_doc
	"Adds support for eventfd to Python 3.",

	// m_size (size of per-interpreter state of the module, or -1 if the module keeps its state
	// in global variables).
	-1,

	// m_methods 
	nullptr,

	// m_slots
	nullptr,

	// m_traverse (support for cyclic GC)
	nullptr,

	// m_clear (support for cyclic GC)
	nullptr,

	// m_free (custom module deallocation function)
	nullptr
};

extern "C" EXPORT_SYMBOL
auto PyInit__eventfd() noexcept
{
	if (PyType_Ready(&event_def) == -1)
		return obj_ptr{};

	auto mod = PyModule_Create(&module_def);
	if (not mod)
		return obj_ptr{};

	auto c = obj_ptr{};

	#define define_constant(name)              \
		c = PyLong_FromLong(name);         \
		if (not c)                         \
			return obj_ptr{};          \
		PyModule_AddObject(mod, #name, c);

	define_constant(EFD_CLOEXEC)
	define_constant(EFD_NONBLOCK)
	define_constant(EFD_SEMAPHORE)

	#undef define_constant

	Py_INCREF(&event_def);
	PyModule_AddObject(mod, "Event", (obj_ptr)&event_def);
	return mod;
}

}
