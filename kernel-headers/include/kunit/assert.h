/* SPDX-License-Identifier: GPL-2.0 */


#ifndef _KUNIT_ASSERT_H
#define _KUNIT_ASSERT_H

#include <linux/err.h>
#include <linux/kernel.h>

struct kunit;
struct string_stream;


enum kunit_assert_type {
	KUNIT_ASSERTION,
	KUNIT_EXPECTATION,
};


struct kunit_assert {
	struct kunit *test;
	enum kunit_assert_type type;
	int line;
	const char *file;
	struct va_format message;
	void (*format)(const struct kunit_assert *assert,
		       struct string_stream *stream);
};


#define KUNIT_INIT_VA_FMT_NULL { .fmt = NULL, .va = NULL }


#define KUNIT_INIT_ASSERT_STRUCT(kunit, assert_type, fmt) {		       \
	.test = kunit,							       \
	.type = assert_type,						       \
	.file = __FILE__,						       \
	.line = __LINE__,						       \
	.message = KUNIT_INIT_VA_FMT_NULL,				       \
	.format = fmt							       \
}

void kunit_base_assert_format(const struct kunit_assert *assert,
			      struct string_stream *stream);

void kunit_assert_print_msg(const struct kunit_assert *assert,
			    struct string_stream *stream);


struct kunit_fail_assert {
	struct kunit_assert assert;
};

void kunit_fail_assert_format(const struct kunit_assert *assert,
			      struct string_stream *stream);


#define KUNIT_INIT_FAIL_ASSERT_STRUCT(test, type) {			       \
	.assert = KUNIT_INIT_ASSERT_STRUCT(test,			       \
					   type,			       \
					   kunit_fail_assert_format)	       \
}


struct kunit_unary_assert {
	struct kunit_assert assert;
	const char *condition;
	bool expected_true;
};

void kunit_unary_assert_format(const struct kunit_assert *assert,
			       struct string_stream *stream);


#define KUNIT_INIT_UNARY_ASSERT_STRUCT(test, type, cond, expect_true) {	       \
	.assert = KUNIT_INIT_ASSERT_STRUCT(test,			       \
					   type,			       \
					   kunit_unary_assert_format),	       \
	.condition = cond,						       \
	.expected_true = expect_true					       \
}


struct kunit_ptr_not_err_assert {
	struct kunit_assert assert;
	const char *text;
	const void *value;
};

void kunit_ptr_not_err_assert_format(const struct kunit_assert *assert,
				     struct string_stream *stream);


#define KUNIT_INIT_PTR_NOT_ERR_STRUCT(test, type, txt, val) {		       \
	.assert = KUNIT_INIT_ASSERT_STRUCT(test,			       \
					   type,			       \
					   kunit_ptr_not_err_assert_format),   \
	.text = txt,							       \
	.value = val							       \
}


struct kunit_binary_assert {
	struct kunit_assert assert;
	const char *operation;
	const char *left_text;
	long long left_value;
	const char *right_text;
	long long right_value;
};

void kunit_binary_assert_format(const struct kunit_assert *assert,
				struct string_stream *stream);


#define KUNIT_INIT_BINARY_ASSERT_STRUCT(test,				       \
					type,				       \
					op_str,				       \
					left_str,			       \
					left_val,			       \
					right_str,			       \
					right_val) {			       \
	.assert = KUNIT_INIT_ASSERT_STRUCT(test,			       \
					   type,			       \
					   kunit_binary_assert_format),	       \
	.operation = op_str,						       \
	.left_text = left_str,						       \
	.left_value = left_val,						       \
	.right_text = right_str,					       \
	.right_value = right_val					       \
}


struct kunit_binary_ptr_assert {
	struct kunit_assert assert;
	const char *operation;
	const char *left_text;
	const void *left_value;
	const char *right_text;
	const void *right_value;
};

void kunit_binary_ptr_assert_format(const struct kunit_assert *assert,
				    struct string_stream *stream);


#define KUNIT_INIT_BINARY_PTR_ASSERT_STRUCT(test,			       \
					    type,			       \
					    op_str,			       \
					    left_str,			       \
					    left_val,			       \
					    right_str,			       \
					    right_val) {		       \
	.assert = KUNIT_INIT_ASSERT_STRUCT(test,			       \
					   type,			       \
					   kunit_binary_ptr_assert_format),    \
	.operation = op_str,						       \
	.left_text = left_str,						       \
	.left_value = left_val,						       \
	.right_text = right_str,					       \
	.right_value = right_val					       \
}


struct kunit_binary_str_assert {
	struct kunit_assert assert;
	const char *operation;
	const char *left_text;
	const char *left_value;
	const char *right_text;
	const char *right_value;
};

void kunit_binary_str_assert_format(const struct kunit_assert *assert,
				    struct string_stream *stream);


#define KUNIT_INIT_BINARY_STR_ASSERT_STRUCT(test,			       \
					    type,			       \
					    op_str,			       \
					    left_str,			       \
					    left_val,			       \
					    right_str,			       \
					    right_val) {		       \
	.assert = KUNIT_INIT_ASSERT_STRUCT(test,			       \
					   type,			       \
					   kunit_binary_str_assert_format),    \
	.operation = op_str,						       \
	.left_text = left_str,						       \
	.left_value = left_val,						       \
	.right_text = right_str,					       \
	.right_value = right_val					       \
}

#endif 
