/*
 * Copyright (C) 2008-2014 Tobias Brunner
 * Copyright (C) 2008 Martin Willi
 * Hochschule fuer Technik Rapperswil
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

/**
 * @defgroup utils_i utils
 * @{ @ingroup utils
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <sys/types.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/time.h>
#include <string.h>

#ifdef WIN32
# include "compat/windows.h"
#else
# define _GNU_SOURCE
# include <arpa/inet.h>
# include <sys/socket.h>
# include <netdb.h>
# include <netinet/in.h>
# include <sched.h>
# include <poll.h>
#endif

/**
 * strongSwan program return codes
 */
#define SS_RC_LIBSTRONGSWAN_INTEGRITY	64
#define SS_RC_DAEMON_INTEGRITY			65
#define SS_RC_INITIALIZATION_FAILED		66

#define SS_RC_FIRST	SS_RC_LIBSTRONGSWAN_INTEGRITY
#define SS_RC_LAST	SS_RC_INITIALIZATION_FAILED

/**
 * Number of bits in a byte
 */
#define BITS_PER_BYTE 8

/**
 * Default length for various auxiliary text buffers
 */
#define BUF_LEN 512

/**
 * Build assertion macro for integer expressions, evaluates to 0
 */
#define BUILD_ASSERT(x) (sizeof(char[(x) ? 0 : -1]))

/**
 * Build time check to assert a is an array, evaluates to 0
 *
 * The address of an array element has a pointer type, which is not compatible
 * to the array type.
 */
#define BUILD_ASSERT_ARRAY(a) \
		BUILD_ASSERT(!__builtin_types_compatible_p(typeof(a), typeof(&(a)[0])))

#include "utils/types.h"
#include "enum.h"
#include "utils/atomics.h"
#include "utils/byteorder.h"
#include "utils/string.h"
#include "utils/memory.h"
#include "utils/strerror.h"
#include "utils/status.h"
#include "utils/path.h"
#include "utils/tty.h"
#ifdef __APPLE__
# include "compat/apple.h"
#endif

/**
 * Initialize utility functions
 */
void utils_init();

/**
 * Deinitialize utility functions
 */
void utils_deinit();

/**
 * Macro gives back larger of two values.
 */
#define max(x,y) ({ \
	typeof(x) _x = (x); \
	typeof(y) _y = (y); \
	_x > _y ? _x : _y; })

/**
 * Macro gives back smaller of two values.
 */
#define min(x,y) ({ \
	typeof(x) _x = (x); \
	typeof(y) _y = (y); \
	_x < _y ? _x : _y; })

/**
 * Call destructor of an object, if object != NULL
 */
#define DESTROY_IF(obj) if (obj) (obj)->destroy(obj)

/**
 * Call offset destructor of an object, if object != NULL
 */
#define DESTROY_OFFSET_IF(obj, offset) if (obj) obj->destroy_offset(obj, offset);

/**
 * Call function destructor of an object, if object != NULL
 */
#define DESTROY_FUNCTION_IF(obj, fn) if (obj) obj->destroy_function(obj, fn);

/**
 * Debug macro to follow control flow
 */
#define POS printf("%s, line %d\n", __FILE__, __LINE__)

/**
 * Object allocation/initialization macro, using designated initializer.
 */
#define INIT(this, ...) { (this) = malloc(sizeof(*(this))); \
						   *(this) = (typeof(*(this))){ __VA_ARGS__ }; }

/**
 * Aligning version of INIT().
 *
 * The returned pointer must be freed using free_align(), not free().
 *
 * @param this		object to allocate/initialize
 * @param align		alignment for allocation, in bytes
 * @param ...		initializer
 */
#define INIT_ALIGN(this, align, ...) { \
						(this) = malloc_align(sizeof(*(this)), align); \
						*(this) = (typeof(*(this))){ __VA_ARGS__ }; }

/**
 * Object allocation/initialization macro, with extra allocated bytes at tail.
 *
 * The extra space gets zero-initialized.
 *
 * @param this		pointer to object to allocate memory for
 * @param extra		number of bytes to allocate at end of this
 * @param ...		initializer
 */
#define INIT_EXTRA(this, extra, ...) { \
						typeof(extra) _extra = (extra); \
						(this) = malloc(sizeof(*(this)) + _extra); \
						*(this) = (typeof(*(this))){ __VA_ARGS__ }; \
						memset((this) + 1, 0, _extra); }

/**
 * Aligning version of INIT_EXTRA().
 *
 * The returned pointer must be freed using free_align(), not free().
 *
 * @param this		object to allocate/initialize
 * @param extra		number of bytes to allocate at end of this
 * @param align		alignment for allocation, in bytes
 * @param ...		initializer
 */
#define INIT_EXTRA_ALIGN(this, extra, align, ...) { \
						typeof(extra) _extra = (extra); \
						(this) = malloc_align(sizeof(*(this)) + _extra, align); \
						*(this) = (typeof(*(this))){ __VA_ARGS__ }; \
						memset((this) + 1, 0, _extra); }

/**
 * Method declaration/definition macro, providing private and public interface.
 *
 * Defines a method name with this as first parameter and a return value ret,
 * and an alias for this method with a _ prefix, having the this argument
 * safely casted to the public interface iface.
 * _name is provided a function pointer, but will get optimized out by GCC.
 */
#define METHOD(iface, name, ret, this, ...) \
	static ret name(union {iface *_public; this;} \
	__attribute__((transparent_union)), ##__VA_ARGS__); \
	static typeof(name) *_##name = (typeof(name)*)name; \
	static ret name(this, ##__VA_ARGS__)

/**
 * Same as METHOD(), but is defined for two public interfaces.
 */
#define METHOD2(iface1, iface2, name, ret, this, ...) \
	static ret name(union {iface1 *_public1; iface2 *_public2; this;} \
	__attribute__((transparent_union)), ##__VA_ARGS__); \
	static typeof(name) *_##name = (typeof(name)*)name; \
	static ret name(this, ##__VA_ARGS__)

/**
 * Callback declaration/definition macro, allowing casted first parameter.
 *
 * This is very similar to METHOD, but instead of casting the first parameter
 * to a public interface, it uses a void*. This allows type safe definition
 * of a callback function, while using the real type for the first parameter.
 */
#define CALLBACK(name, ret, param1, ...) \
	static ret _cb_##name(union {void *_generic; param1;} \
	__attribute__((transparent_union)), ##__VA_ARGS__); \
	static typeof(_cb_##name) *name = (typeof(_cb_##name)*)_cb_##name; \
	static ret _cb_##name(param1, ##__VA_ARGS__)

/**
 * This macro allows counting the number of arguments passed to a macro.
 * Combined with the VA_ARGS_DISPATCH() macro this can be used to implement
 * macro overloading based on the number of arguments.
 * 0 to 10 arguments are currently supported.
 */
#define VA_ARGS_NUM(...) _VA_ARGS_NUM(0,##__VA_ARGS__,10,9,8,7,6,5,4,3,2,1,0)
#define _VA_ARGS_NUM(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,NUM,...) NUM

/**
 * This macro can be used to dispatch a macro call based on the number of given
 * arguments, for instance:
 *
 * @code
 * #define MY_MACRO(...) VA_ARGS_DISPATCH(MY_MACRO, __VA_ARGS__)(__VA_ARGS__)
 * #define MY_MACRO1(arg) one_arg(arg)
 * #define MY_MACRO2(arg1,arg2) two_args(arg1,arg2)
 * @endcode
 *
 * MY_MACRO() can now be called with either one or two arguments, which will
 * resolve to one_arg(arg) or two_args(arg1,arg2), respectively.
 */
#define VA_ARGS_DISPATCH(func, ...) _VA_ARGS_DISPATCH(func, VA_ARGS_NUM(__VA_ARGS__))
#define _VA_ARGS_DISPATCH(func, num) __VA_ARGS_DISPATCH(func, num)
#define __VA_ARGS_DISPATCH(func, num) func ## num

/**
 * Macro to allocate a sized type.
 */
#define malloc_thing(thing) ((thing*)malloc(sizeof(thing)))

/**
 * Get the number of elements in an array
 */
#define countof(array) (sizeof(array)/sizeof((array)[0]) \
						+ BUILD_ASSERT_ARRAY(array))

/**
 * Ignore result of functions tagged with warn_unused_result attributes
 */
#define ignore_result(call) { if(call){}; }

/**
 * Assign a function as a class method
 */
#define ASSIGN(method, function) (method = (typeof(method))function)

/**
 * time_t not defined
 */
#define UNDEFINED_TIME 0

/**
 * Maximum time since epoch causing wrap-around on Jan 19 03:14:07 UTC 2038
 */
#define TIME_32_BIT_SIGNED_MAX	0x7fffffff

/**
 * Handle struct timeval like an own type.
 */
typedef struct timeval timeval_t;

/**
 * Handle struct timespec like an own type.
 */
typedef struct timespec timespec_t;

/**
 * malloc(), but returns aligned memory.
 *
 * The returned pointer must be freed using free_align(), not free().
 *
 * @param size			size of allocated data
 * @param align			alignment, up to 255 bytes, usually a power of 2
 * @return				allocated hunk, aligned to align bytes
 */
void* malloc_align(size_t size, u_int8_t align);

/**
 * Free a hunk allocated by malloc_align().
 *
 * @param ptr			hunk to free
 */
void free_align(void *ptr);

/**
 * Portable function to wait for SIGINT/SIGTERM (or equivalent).
 */
void wait_sigint();

#ifndef HAVE_CLOSEFROM
/**
 * Close open file descriptors greater than or equal to lowfd.
 *
 * @param lowfd		start closing file descriptors from here
 */
void closefrom(int lowfd);
#endif

/**
 * Get a timestamp from a monotonic time source.
 *
 * While the time()/gettimeofday() functions are affected by leap seconds
 * and system time changes, this function returns ever increasing monotonic
 * time stamps.
 *
 * @param tv		timeval struct receiving monotonic timestamps, or NULL
 * @return			monotonic timestamp in seconds
 */
time_t time_monotonic(timeval_t *tv);

/**
 * Add the given number of milliseconds to the given timeval struct
 *
 * @param tv		timeval struct to modify
 * @param ms		number of milliseconds
 */
static inline void timeval_add_ms(timeval_t *tv, u_int ms)
{
	tv->tv_usec += ms * 1000;
	while (tv->tv_usec >= 1000000 /* 1s */)
	{
		tv->tv_usec -= 1000000;
		tv->tv_sec++;
	}
}

/**
 * returns null
 */
void *return_null();

/**
 * No-Operation function
 */
void nop();

/**
 * returns TRUE
 */
bool return_true();

/**
 * returns FALSE
 */
bool return_false();

/**
 * Get the padding required to make size a multiple of alignment
 */
static inline size_t pad_len(size_t size, size_t alignment)
{
	size_t remainder;

	remainder = size % alignment;
	return remainder ? alignment - remainder : 0;
}

/**
 * Round up size to be multiple of alignment
 */
static inline size_t round_up(size_t size, size_t alignment)
{
	return size + pad_len(size, alignment);
}

/**
 * Round down size to be a multiple of alignment
 */
static inline size_t round_down(size_t size, size_t alignment)
{
	return size - (size % alignment);
}

/**
 * printf hook for time_t.
 *
 * Arguments are:
 *	time_t* time, bool utc
 */
int time_printf_hook(printf_hook_data_t *data, printf_hook_spec_t *spec,
					 const void *const *args);

/**
 * printf hook for time_t deltas.
 *
 * Arguments are:
 *	time_t* begin, time_t* end
 */
int time_delta_printf_hook(printf_hook_data_t *data, printf_hook_spec_t *spec,
						   const void *const *args);

#endif /** UTILS_H_ @}*/
