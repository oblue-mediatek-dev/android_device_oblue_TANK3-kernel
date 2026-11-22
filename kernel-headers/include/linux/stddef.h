/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_STDDEF_H
#define _LINUX_STDDEF_H

#include <uapi/linux/stddef.h>

#undef NULL
#define NULL ((void *)0)

enum {
	false	= 0,
	true	= 1
};

#undef offsetof
#ifdef __compiler_offsetof
#define offsetof(TYPE, MEMBER)	__compiler_offsetof(TYPE, MEMBER)
#else
#define offsetof(TYPE, MEMBER)	((size_t)&((TYPE *)0)->MEMBER)
#endif


#define sizeof_field(TYPE, MEMBER) sizeof((((TYPE *)0)->MEMBER))


#define offsetofend(TYPE, MEMBER) \
	(offsetof(TYPE, MEMBER)	+ sizeof_field(TYPE, MEMBER))


#define struct_group(NAME, MEMBERS...)	\
	__struct_group(, NAME, , MEMBERS)


#define struct_group_attr(NAME, ATTRS, MEMBERS...) \
	__struct_group(, NAME, ATTRS, MEMBERS)


#define struct_group_tagged(TAG, NAME, MEMBERS...) \
	__struct_group(TAG, NAME, , MEMBERS)

#endif
