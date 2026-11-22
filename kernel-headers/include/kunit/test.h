/* SPDX-License-Identifier: GPL-2.0 */


#ifndef _KUNIT_TEST_H
#define _KUNIT_TEST_H

#include <kunit/assert.h>
#include <kunit/try-catch.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/kref.h>

struct kunit_resource;

typedef int (*kunit_resource_init_t)(struct kunit_resource *, void *);
typedef void (*kunit_resource_free_t)(struct kunit_resource *);


struct kunit_resource {
	void *data;
	const char *name;
	kunit_resource_free_t free;

	
	struct kref refcount;
	struct list_head node;
};

struct kunit;


#define KUNIT_LOG_SIZE	512


#define KUNIT_SUBTEST_INDENT		"    "
#define KUNIT_SUBSUBTEST_INDENT		"        "


struct kunit_case {
	void (*run_case)(struct kunit *test);
	const char *name;

	
	bool success;
	char *log;
};

static inline char *kunit_status_to_string(bool status)
{
	return status ? "ok" : "not ok";
}


#define KUNIT_CASE(test_name) { .run_case = test_name, .name = #test_name }


struct kunit_suite {
	const char name[256];
	int (*init)(struct kunit *test);
	void (*exit)(struct kunit *test);
	struct kunit_case *test_cases;

	
	struct dentry *debugfs;
	char *log;
};


struct kunit {
	void *priv;

	
	const char *name; 
	char *log; 
	struct kunit_try_catch try_catch;
	
	bool success; 
	spinlock_t lock; 
	
	struct list_head resources; 
};

static inline void kunit_set_failure(struct kunit *test)
{
	WRITE_ONCE(test->success, false);
}

void kunit_init_test(struct kunit *test, const char *name, char *log);

int kunit_run_tests(struct kunit_suite *suite);

size_t kunit_suite_num_test_cases(struct kunit_suite *suite);

unsigned int kunit_test_case_num(struct kunit_suite *suite,
				 struct kunit_case *test_case);

int __kunit_test_suites_init(struct kunit_suite * const * const suites);

void __kunit_test_suites_exit(struct kunit_suite **suites);

#if IS_BUILTIN(CONFIG_KUNIT)
int kunit_run_all_tests(void);
#else
static inline int kunit_run_all_tests(void)
{
	return 0;
}
#endif 

#ifdef MODULE

#define kunit_test_suites_for_module(__suites)				\
	static int __init kunit_test_suites_init(void)			\
	{								\
		return __kunit_test_suites_init(__suites);		\
	}								\
	module_init(kunit_test_suites_init);				\
									\
	static void __exit kunit_test_suites_exit(void)			\
	{								\
		return __kunit_test_suites_exit(__suites);		\
	}								\
	module_exit(kunit_test_suites_exit)
#else
#define kunit_test_suites_for_module(__suites)
#endif 

#define __kunit_test_suites(unique_array, unique_suites, ...)		       \
	static struct kunit_suite *unique_array[] = { __VA_ARGS__, NULL };     \
	kunit_test_suites_for_module(unique_array);			       \
	static struct kunit_suite **unique_suites			       \
	__used __section(".kunit_test_suites") = unique_array


#define kunit_test_suites(__suites...)						\
	__kunit_test_suites(__UNIQUE_ID(array),				\
			    __UNIQUE_ID(suites),			\
			    ##__suites)

#define kunit_test_suite(suite)	kunit_test_suites(&suite)

#define kunit_suite_for_each_test_case(suite, test_case)		\
	for (test_case = suite->test_cases; test_case->run_case; test_case++)

bool kunit_suite_has_succeeded(struct kunit_suite *suite);


struct kunit_resource *kunit_alloc_and_get_resource(struct kunit *test,
						    kunit_resource_init_t init,
						    kunit_resource_free_t free,
						    gfp_t internal_gfp,
						    void *context);


static inline void kunit_get_resource(struct kunit_resource *res)
{
	kref_get(&res->refcount);
}


static inline void kunit_release_resource(struct kref *kref)
{
	struct kunit_resource *res = container_of(kref, struct kunit_resource,
						  refcount);

	
	if (res->free) {
		res->free(res);
		kfree(res);
	}
}


static inline void kunit_put_resource(struct kunit_resource *res)
{
	kref_put(&res->refcount, kunit_release_resource);
}


int kunit_add_resource(struct kunit *test,
		       kunit_resource_init_t init,
		       kunit_resource_free_t free,
		       struct kunit_resource *res,
		       void *data);


int kunit_add_named_resource(struct kunit *test,
			     kunit_resource_init_t init,
			     kunit_resource_free_t free,
			     struct kunit_resource *res,
			     const char *name,
			     void *data);


static inline void *kunit_alloc_resource(struct kunit *test,
					 kunit_resource_init_t init,
					 kunit_resource_free_t free,
					 gfp_t internal_gfp,
					 void *context)
{
	struct kunit_resource *res;

	res = kzalloc(sizeof(*res), internal_gfp);
	if (!res)
		return NULL;

	if (!kunit_add_resource(test, init, free, res, context))
		return res->data;

	return NULL;
}

typedef bool (*kunit_resource_match_t)(struct kunit *test,
				       struct kunit_resource *res,
				       void *match_data);


static inline bool kunit_resource_instance_match(struct kunit *test,
						 struct kunit_resource *res,
						 void *match_data)
{
	return res->data == match_data;
}


static inline bool kunit_resource_name_match(struct kunit *test,
					     struct kunit_resource *res,
					     void *match_name)
{
	return res->name && strcmp(res->name, match_name) == 0;
}


static inline struct kunit_resource *
kunit_find_resource(struct kunit *test,
		    kunit_resource_match_t match,
		    void *match_data)
{
	struct kunit_resource *res, *found = NULL;

	spin_lock(&test->lock);

	list_for_each_entry_reverse(res, &test->resources, node) {
		if (match(test, res, (void *)match_data)) {
			found = res;
			kunit_get_resource(found);
			break;
		}
	}

	spin_unlock(&test->lock);

	return found;
}


static inline struct kunit_resource *
kunit_find_named_resource(struct kunit *test,
			  const char *name)
{
	return kunit_find_resource(test, kunit_resource_name_match,
				   (void *)name);
}


int kunit_destroy_resource(struct kunit *test,
			   kunit_resource_match_t match,
			   void *match_data);

static inline int kunit_destroy_named_resource(struct kunit *test,
					       const char *name)
{
	return kunit_destroy_resource(test, kunit_resource_name_match,
				      (void *)name);
}


void kunit_remove_resource(struct kunit *test, struct kunit_resource *res);


void *kunit_kmalloc(struct kunit *test, size_t size, gfp_t gfp);


void kunit_kfree(struct kunit *test, const void *ptr);


static inline void *kunit_kzalloc(struct kunit *test, size_t size, gfp_t gfp)
{
	return kunit_kmalloc(test, size, gfp | __GFP_ZERO);
}

void kunit_cleanup(struct kunit *test);

void kunit_log_append(char *log, const char *fmt, ...);


#define kunit_log(lvl, test_or_suite, fmt, ...)				\
	do {								\
		printk(lvl fmt, ##__VA_ARGS__);				\
		kunit_log_append((test_or_suite)->log,	fmt "\n",	\
				 ##__VA_ARGS__);			\
	} while (0)

#define kunit_printk(lvl, test, fmt, ...)				\
	kunit_log(lvl, test, KUNIT_SUBTEST_INDENT "# %s: " fmt,		\
		  (test)->name,	##__VA_ARGS__)


#define kunit_info(test, fmt, ...) \
	kunit_printk(KERN_INFO, test, fmt, ##__VA_ARGS__)


#define kunit_warn(test, fmt, ...) \
	kunit_printk(KERN_WARNING, test, fmt, ##__VA_ARGS__)


#define kunit_err(test, fmt, ...) \
	kunit_printk(KERN_ERR, test, fmt, ##__VA_ARGS__)


#define KUNIT_SUCCEED(test) do {} while (0)

void kunit_do_assertion(struct kunit *test,
			struct kunit_assert *assert,
			bool pass,
			const char *fmt, ...);

#define KUNIT_ASSERTION(test, pass, assert_class, INITIALIZER, fmt, ...) do {  \
	struct assert_class __assertion = INITIALIZER;			       \
	kunit_do_assertion(test,					       \
			   &__assertion.assert,				       \
			   pass,					       \
			   fmt,						       \
			   ##__VA_ARGS__);				       \
} while (0)


#define KUNIT_FAIL_ASSERTION(test, assert_type, fmt, ...)		       \
	KUNIT_ASSERTION(test,						       \
			false,						       \
			kunit_fail_assert,				       \
			KUNIT_INIT_FAIL_ASSERT_STRUCT(test, assert_type),      \
			fmt,						       \
			##__VA_ARGS__)


#define KUNIT_FAIL(test, fmt, ...)					       \
	KUNIT_FAIL_ASSERTION(test,					       \
			     KUNIT_EXPECTATION,				       \
			     fmt,					       \
			     ##__VA_ARGS__)

#define KUNIT_UNARY_ASSERTION(test,					       \
			      assert_type,				       \
			      condition,				       \
			      expected_true,				       \
			      fmt,					       \
			      ...)					       \
	KUNIT_ASSERTION(test,						       \
			!!(condition) == !!expected_true,		       \
			kunit_unary_assert,				       \
			KUNIT_INIT_UNARY_ASSERT_STRUCT(test,		       \
						       assert_type,	       \
						       #condition,	       \
						       expected_true),	       \
			fmt,						       \
			##__VA_ARGS__)

#define KUNIT_TRUE_MSG_ASSERTION(test, assert_type, condition, fmt, ...)       \
	KUNIT_UNARY_ASSERTION(test,					       \
			      assert_type,				       \
			      condition,				       \
			      true,					       \
			      fmt,					       \
			      ##__VA_ARGS__)

#define KUNIT_TRUE_ASSERTION(test, assert_type, condition) \
	KUNIT_TRUE_MSG_ASSERTION(test, assert_type, condition, NULL)

#define KUNIT_FALSE_MSG_ASSERTION(test, assert_type, condition, fmt, ...)      \
	KUNIT_UNARY_ASSERTION(test,					       \
			      assert_type,				       \
			      condition,				       \
			      false,					       \
			      fmt,					       \
			      ##__VA_ARGS__)

#define KUNIT_FALSE_ASSERTION(test, assert_type, condition) \
	KUNIT_FALSE_MSG_ASSERTION(test, assert_type, condition, NULL)


#define KUNIT_BASE_BINARY_ASSERTION(test,				       \
				    assert_class,			       \
				    ASSERT_CLASS_INIT,			       \
				    assert_type,			       \
				    left,				       \
				    op,					       \
				    right,				       \
				    fmt,				       \
				    ...)				       \
do {									       \
	typeof(left) __left = (left);					       \
	typeof(right) __right = (right);				       \
	((void)__typecheck(__left, __right));				       \
									       \
	KUNIT_ASSERTION(test,						       \
			__left op __right,				       \
			assert_class,					       \
			ASSERT_CLASS_INIT(test,				       \
					  assert_type,			       \
					  #op,				       \
					  #left,			       \
					  __left,			       \
					  #right,			       \
					  __right),			       \
			fmt,						       \
			##__VA_ARGS__);					       \
} while (0)

#define KUNIT_BASE_EQ_MSG_ASSERTION(test,				       \
				    assert_class,			       \
				    ASSERT_CLASS_INIT,			       \
				    assert_type,			       \
				    left,				       \
				    right,				       \
				    fmt,				       \
				    ...)				       \
	KUNIT_BASE_BINARY_ASSERTION(test,				       \
				    assert_class,			       \
				    ASSERT_CLASS_INIT,			       \
				    assert_type,			       \
				    left, ==, right,			       \
				    fmt,				       \
				    ##__VA_ARGS__)

#define KUNIT_BASE_NE_MSG_ASSERTION(test,				       \
				    assert_class,			       \
				    ASSERT_CLASS_INIT,			       \
				    assert_type,			       \
				    left,				       \
				    right,				       \
				    fmt,				       \
				    ...)				       \
	KUNIT_BASE_BINARY_ASSERTION(test,				       \
				    assert_class,			       \
				    ASSERT_CLASS_INIT,			       \
				    assert_type,			       \
				    left, !=, right,			       \
				    fmt,				       \
				    ##__VA_ARGS__)

#define KUNIT_BASE_LT_MSG_ASSERTION(test,				       \
				    assert_class,			       \
				    ASSERT_CLASS_INIT,			       \
				    assert_type,			       \
				    left,				       \
				    right,				       \
				    fmt,				       \
				    ...)				       \
	KUNIT_BASE_BINARY_ASSERTION(test,				       \
				    assert_class,			       \
				    ASSERT_CLASS_INIT,			       \
				    assert_type,			       \
				    left, <, right,			       \
				    fmt,				       \
				    ##__VA_ARGS__)

#define KUNIT_BASE_LE_MSG_ASSERTION(test,				       \
				    assert_class,			       \
				    ASSERT_CLASS_INIT,			       \
				    assert_type,			       \
				    left,				       \
				    right,				       \
				    fmt,				       \
				    ...)				       \
	KUNIT_BASE_BINARY_ASSERTION(test,				       \
				    assert_class,			       \
				    ASSERT_CLASS_INIT,			       \
				    assert_type,			       \
				    left, <=, right,			       \
				    fmt,				       \
				    ##__VA_ARGS__)

#define KUNIT_BASE_GT_MSG_ASSERTION(test,				       \
				    assert_class,			       \
				    ASSERT_CLASS_INIT,			       \
				    assert_type,			       \
				    left,				       \
				    right,				       \
				    fmt,				       \
				    ...)				       \
	KUNIT_BASE_BINARY_ASSERTION(test,				       \
				    assert_class,			       \
				    ASSERT_CLASS_INIT,			       \
				    assert_type,			       \
				    left, >, right,			       \
				    fmt,				       \
				    ##__VA_ARGS__)

#define KUNIT_BASE_GE_MSG_ASSERTION(test,				       \
				    assert_class,			       \
				    ASSERT_CLASS_INIT,			       \
				    assert_type,			       \
				    left,				       \
				    right,				       \
				    fmt,				       \
				    ...)				       \
	KUNIT_BASE_BINARY_ASSERTION(test,				       \
				    assert_class,			       \
				    ASSERT_CLASS_INIT,			       \
				    assert_type,			       \
				    left, >=, right,			       \
				    fmt,				       \
				    ##__VA_ARGS__)

#define KUNIT_BINARY_EQ_MSG_ASSERTION(test, assert_type, left, right, fmt, ...)\
	KUNIT_BASE_EQ_MSG_ASSERTION(test,				       \
				    kunit_binary_assert,		       \
				    KUNIT_INIT_BINARY_ASSERT_STRUCT,	       \
				    assert_type,			       \
				    left,				       \
				    right,				       \
				    fmt,				       \
				    ##__VA_ARGS__)

#define KUNIT_BINARY_EQ_ASSERTION(test, assert_type, left, right)	       \
	KUNIT_BINARY_EQ_MSG_ASSERTION(test,				       \
				      assert_type,			       \
				      left,				       \
				      right,				       \
				      NULL)

#define KUNIT_BINARY_PTR_EQ_MSG_ASSERTION(test,				       \
					  assert_type,			       \
					  left,				       \
					  right,			       \
					  fmt,				       \
					  ...)				       \
	KUNIT_BASE_EQ_MSG_ASSERTION(test,				       \
				    kunit_binary_ptr_assert,		       \
				    KUNIT_INIT_BINARY_PTR_ASSERT_STRUCT,       \
				    assert_type,			       \
				    left,				       \
				    right,				       \
				    fmt,				       \
				    ##__VA_ARGS__)

#define KUNIT_BINARY_PTR_EQ_ASSERTION(test, assert_type, left, right)	       \
	KUNIT_BINARY_PTR_EQ_MSG_ASSERTION(test,				       \
					  assert_type,			       \
					  left,				       \
					  right,			       \
					  NULL)

#define KUNIT_BINARY_NE_MSG_ASSERTION(test, assert_type, left, right, fmt, ...)\
	KUNIT_BASE_NE_MSG_ASSERTION(test,				       \
				    kunit_binary_assert,		       \
				    KUNIT_INIT_BINARY_ASSERT_STRUCT,	       \
				    assert_type,			       \
				    left,				       \
				    right,				       \
				    fmt,				       \
				    ##__VA_ARGS__)

#define KUNIT_BINARY_NE_ASSERTION(test, assert_type, left, right)	       \
	KUNIT_BINARY_NE_MSG_ASSERTION(test,				       \
				      assert_type,			       \
				      left,				       \
				      right,				       \
				      NULL)

#define KUNIT_BINARY_PTR_NE_MSG_ASSERTION(test,				       \
					  assert_type,			       \
					  left,				       \
					  right,			       \
					  fmt,				       \
					  ...)				       \
	KUNIT_BASE_NE_MSG_ASSERTION(test,				       \
				    kunit_binary_ptr_assert,		       \
				    KUNIT_INIT_BINARY_PTR_ASSERT_STRUCT,       \
				    assert_type,			       \
				    left,				       \
				    right,				       \
				    fmt,				       \
				    ##__VA_ARGS__)

#define KUNIT_BINARY_PTR_NE_ASSERTION(test, assert_type, left, right)	       \
	KUNIT_BINARY_PTR_NE_MSG_ASSERTION(test,				       \
					  assert_type,			       \
					  left,				       \
					  right,			       \
					  NULL)

#define KUNIT_BINARY_LT_MSG_ASSERTION(test, assert_type, left, right, fmt, ...)\
	KUNIT_BASE_LT_MSG_ASSERTION(test,				       \
				    kunit_binary_assert,		       \
				    KUNIT_INIT_BINARY_ASSERT_STRUCT,	       \
				    assert_type,			       \
				    left,				       \
				    right,				       \
				    fmt,				       \
				    ##__VA_ARGS__)

#define KUNIT_BINARY_LT_ASSERTION(test, assert_type, left, right)	       \
	KUNIT_BINARY_LT_MSG_ASSERTION(test,				       \
				      assert_type,			       \
				      left,				       \
				      right,				       \
				      NULL)

#define KUNIT_BINARY_PTR_LT_MSG_ASSERTION(test,				       \
					  assert_type,			       \
					  left,				       \
					  right,			       \
					  fmt,				       \
					  ...)				       \
	KUNIT_BASE_LT_MSG_ASSERTION(test,				       \
				    kunit_binary_ptr_assert,		       \
				    KUNIT_INIT_BINARY_PTR_ASSERT_STRUCT,       \
				    assert_type,			       \
				    left,				       \
				    right,				       \
				    fmt,				       \
				    ##__VA_ARGS__)

#define KUNIT_BINARY_PTR_LT_ASSERTION(test, assert_type, left, right)	       \
	KUNIT_BINARY_PTR_LT_MSG_ASSERTION(test,				       \
					  assert_type,			       \
					  left,				       \
					  right,			       \
					  NULL)

#define KUNIT_BINARY_LE_MSG_ASSERTION(test, assert_type, left, right, fmt, ...)\
	KUNIT_BASE_LE_MSG_ASSERTION(test,				       \
				    kunit_binary_assert,		       \
				    KUNIT_INIT_BINARY_ASSERT_STRUCT,	       \
				    assert_type,			       \
				    left,				       \
				    right,				       \
				    fmt,				       \
				    ##__VA_ARGS__)

#define KUNIT_BINARY_LE_ASSERTION(test, assert_type, left, right)	       \
	KUNIT_BINARY_LE_MSG_ASSERTION(test,				       \
				      assert_type,			       \
				      left,				       \
				      right,				       \
				      NULL)

#define KUNIT_BINARY_PTR_LE_MSG_ASSERTION(test,				       \
					  assert_type,			       \
					  left,				       \
					  right,			       \
					  fmt,				       \
					  ...)				       \
	KUNIT_BASE_LE_MSG_ASSERTION(test,				       \
				    kunit_binary_ptr_assert,		       \
				    KUNIT_INIT_BINARY_PTR_ASSERT_STRUCT,       \
				    assert_type,			       \
				    left,				       \
				    right,				       \
				    fmt,				       \
				    ##__VA_ARGS__)

#define KUNIT_BINARY_PTR_LE_ASSERTION(test, assert_type, left, right)	       \
	KUNIT_BINARY_PTR_LE_MSG_ASSERTION(test,				       \
					  assert_type,			       \
					  left,				       \
					  right,			       \
					  NULL)

#define KUNIT_BINARY_GT_MSG_ASSERTION(test, assert_type, left, right, fmt, ...)\
	KUNIT_BASE_GT_MSG_ASSERTION(test,				       \
				    kunit_binary_assert,		       \
				    KUNIT_INIT_BINARY_ASSERT_STRUCT,	       \
				    assert_type,			       \
				    left,				       \
				    right,				       \
				    fmt,				       \
				    ##__VA_ARGS__)

#define KUNIT_BINARY_GT_ASSERTION(test, assert_type, left, right)	       \
	KUNIT_BINARY_GT_MSG_ASSERTION(test,				       \
				      assert_type,			       \
				      left,				       \
				      right,				       \
				      NULL)

#define KUNIT_BINARY_PTR_GT_MSG_ASSERTION(test,				       \
					  assert_type,			       \
					  left,				       \
					  right,			       \
					  fmt,				       \
					  ...)				       \
	KUNIT_BASE_GT_MSG_ASSERTION(test,				       \
				    kunit_binary_ptr_assert,		       \
				    KUNIT_INIT_BINARY_PTR_ASSERT_STRUCT,       \
				    assert_type,			       \
				    left,				       \
				    right,				       \
				    fmt,				       \
				    ##__VA_ARGS__)

#define KUNIT_BINARY_PTR_GT_ASSERTION(test, assert_type, left, right)	       \
	KUNIT_BINARY_PTR_GT_MSG_ASSERTION(test,				       \
					  assert_type,			       \
					  left,				       \
					  right,			       \
					  NULL)

#define KUNIT_BINARY_GE_MSG_ASSERTION(test, assert_type, left, right, fmt, ...)\
	KUNIT_BASE_GE_MSG_ASSERTION(test,				       \
				    kunit_binary_assert,		       \
				    KUNIT_INIT_BINARY_ASSERT_STRUCT,	       \
				    assert_type,			       \
				    left,				       \
				    right,				       \
				    fmt,				       \
				    ##__VA_ARGS__)

#define KUNIT_BINARY_GE_ASSERTION(test, assert_type, left, right)	       \
	KUNIT_BINARY_GE_MSG_ASSERTION(test,				       \
				      assert_type,			       \
				      left,				       \
				      right,				       \
				      NULL)

#define KUNIT_BINARY_PTR_GE_MSG_ASSERTION(test,				       \
					  assert_type,			       \
					  left,				       \
					  right,			       \
					  fmt,				       \
					  ...)				       \
	KUNIT_BASE_GE_MSG_ASSERTION(test,				       \
				    kunit_binary_ptr_assert,		       \
				    KUNIT_INIT_BINARY_PTR_ASSERT_STRUCT,       \
				    assert_type,			       \
				    left,				       \
				    right,				       \
				    fmt,				       \
				    ##__VA_ARGS__)

#define KUNIT_BINARY_PTR_GE_ASSERTION(test, assert_type, left, right)	       \
	KUNIT_BINARY_PTR_GE_MSG_ASSERTION(test,				       \
					  assert_type,			       \
					  left,				       \
					  right,			       \
					  NULL)

#define KUNIT_BINARY_STR_ASSERTION(test,				       \
				   assert_type,				       \
				   left,				       \
				   op,					       \
				   right,				       \
				   fmt,					       \
				   ...)					       \
do {									       \
	typeof(left) __left = (left);					       \
	typeof(right) __right = (right);				       \
									       \
	KUNIT_ASSERTION(test,						       \
			strcmp(__left, __right) op 0,			       \
			kunit_binary_str_assert,			       \
			KUNIT_INIT_BINARY_STR_ASSERT_STRUCT(test,	       \
							assert_type,	       \
							#op,		       \
							#left,		       \
							__left,		       \
							#right,		       \
							__right),	       \
			fmt,						       \
			##__VA_ARGS__);					       \
} while (0)

#define KUNIT_BINARY_STR_EQ_MSG_ASSERTION(test,				       \
					  assert_type,			       \
					  left,				       \
					  right,			       \
					  fmt,				       \
					  ...)				       \
	KUNIT_BINARY_STR_ASSERTION(test,				       \
				   assert_type,				       \
				   left, ==, right,			       \
				   fmt,					       \
				   ##__VA_ARGS__)

#define KUNIT_BINARY_STR_EQ_ASSERTION(test, assert_type, left, right)	       \
	KUNIT_BINARY_STR_EQ_MSG_ASSERTION(test,				       \
					  assert_type,			       \
					  left,				       \
					  right,			       \
					  NULL)

#define KUNIT_BINARY_STR_NE_MSG_ASSERTION(test,				       \
					  assert_type,			       \
					  left,				       \
					  right,			       \
					  fmt,				       \
					  ...)				       \
	KUNIT_BINARY_STR_ASSERTION(test,				       \
				   assert_type,				       \
				   left, !=, right,			       \
				   fmt,					       \
				   ##__VA_ARGS__)

#define KUNIT_BINARY_STR_NE_ASSERTION(test, assert_type, left, right)	       \
	KUNIT_BINARY_STR_NE_MSG_ASSERTION(test,				       \
					  assert_type,			       \
					  left,				       \
					  right,			       \
					  NULL)

#define KUNIT_PTR_NOT_ERR_OR_NULL_MSG_ASSERTION(test,			       \
						assert_type,		       \
						ptr,			       \
						fmt,			       \
						...)			       \
do {									       \
	typeof(ptr) __ptr = (ptr);					       \
									       \
	KUNIT_ASSERTION(test,						       \
			!IS_ERR_OR_NULL(__ptr),				       \
			kunit_ptr_not_err_assert,			       \
			KUNIT_INIT_PTR_NOT_ERR_STRUCT(test,		       \
						      assert_type,	       \
						      #ptr,		       \
						      __ptr),		       \
			fmt,						       \
			##__VA_ARGS__);					       \
} while (0)

#define KUNIT_PTR_NOT_ERR_OR_NULL_ASSERTION(test, assert_type, ptr)	       \
	KUNIT_PTR_NOT_ERR_OR_NULL_MSG_ASSERTION(test,			       \
						assert_type,		       \
						ptr,			       \
						NULL)


#define KUNIT_EXPECT_TRUE(test, condition) \
	KUNIT_TRUE_ASSERTION(test, KUNIT_EXPECTATION, condition)

#define KUNIT_EXPECT_TRUE_MSG(test, condition, fmt, ...)		       \
	KUNIT_TRUE_MSG_ASSERTION(test,					       \
				 KUNIT_EXPECTATION,			       \
				 condition,				       \
				 fmt,					       \
				 ##__VA_ARGS__)


#define KUNIT_EXPECT_FALSE(test, condition) \
	KUNIT_FALSE_ASSERTION(test, KUNIT_EXPECTATION, condition)

#define KUNIT_EXPECT_FALSE_MSG(test, condition, fmt, ...)		       \
	KUNIT_FALSE_MSG_ASSERTION(test,					       \
				  KUNIT_EXPECTATION,			       \
				  condition,				       \
				  fmt,					       \
				  ##__VA_ARGS__)


#define KUNIT_EXPECT_EQ(test, left, right) \
	KUNIT_BINARY_EQ_ASSERTION(test, KUNIT_EXPECTATION, left, right)

#define KUNIT_EXPECT_EQ_MSG(test, left, right, fmt, ...)		       \
	KUNIT_BINARY_EQ_MSG_ASSERTION(test,				       \
				      KUNIT_EXPECTATION,		       \
				      left,				       \
				      right,				       \
				      fmt,				       \
				      ##__VA_ARGS__)


#define KUNIT_EXPECT_PTR_EQ(test, left, right)				       \
	KUNIT_BINARY_PTR_EQ_ASSERTION(test,				       \
				      KUNIT_EXPECTATION,		       \
				      left,				       \
				      right)

#define KUNIT_EXPECT_PTR_EQ_MSG(test, left, right, fmt, ...)		       \
	KUNIT_BINARY_PTR_EQ_MSG_ASSERTION(test,				       \
					  KUNIT_EXPECTATION,		       \
					  left,				       \
					  right,			       \
					  fmt,				       \
					  ##__VA_ARGS__)


#define KUNIT_EXPECT_NE(test, left, right) \
	KUNIT_BINARY_NE_ASSERTION(test, KUNIT_EXPECTATION, left, right)

#define KUNIT_EXPECT_NE_MSG(test, left, right, fmt, ...)		       \
	KUNIT_BINARY_NE_MSG_ASSERTION(test,				       \
				      KUNIT_EXPECTATION,		       \
				      left,				       \
				      right,				       \
				      fmt,				       \
				      ##__VA_ARGS__)


#define KUNIT_EXPECT_PTR_NE(test, left, right)				       \
	KUNIT_BINARY_PTR_NE_ASSERTION(test,				       \
				      KUNIT_EXPECTATION,		       \
				      left,				       \
				      right)

#define KUNIT_EXPECT_PTR_NE_MSG(test, left, right, fmt, ...)		       \
	KUNIT_BINARY_PTR_NE_MSG_ASSERTION(test,				       \
					  KUNIT_EXPECTATION,		       \
					  left,				       \
					  right,			       \
					  fmt,				       \
					  ##__VA_ARGS__)


#define KUNIT_EXPECT_LT(test, left, right) \
	KUNIT_BINARY_LT_ASSERTION(test, KUNIT_EXPECTATION, left, right)

#define KUNIT_EXPECT_LT_MSG(test, left, right, fmt, ...)		       \
	KUNIT_BINARY_LT_MSG_ASSERTION(test,				       \
				      KUNIT_EXPECTATION,		       \
				      left,				       \
				      right,				       \
				      fmt,				       \
				      ##__VA_ARGS__)


#define KUNIT_EXPECT_LE(test, left, right) \
	KUNIT_BINARY_LE_ASSERTION(test, KUNIT_EXPECTATION, left, right)

#define KUNIT_EXPECT_LE_MSG(test, left, right, fmt, ...)		       \
	KUNIT_BINARY_LE_MSG_ASSERTION(test,				       \
				      KUNIT_EXPECTATION,		       \
				      left,				       \
				      right,				       \
				      fmt,				       \
				      ##__VA_ARGS__)


#define KUNIT_EXPECT_GT(test, left, right) \
	KUNIT_BINARY_GT_ASSERTION(test, KUNIT_EXPECTATION, left, right)

#define KUNIT_EXPECT_GT_MSG(test, left, right, fmt, ...)		       \
	KUNIT_BINARY_GT_MSG_ASSERTION(test,				       \
				      KUNIT_EXPECTATION,		       \
				      left,				       \
				      right,				       \
				      fmt,				       \
				      ##__VA_ARGS__)


#define KUNIT_EXPECT_GE(test, left, right) \
	KUNIT_BINARY_GE_ASSERTION(test, KUNIT_EXPECTATION, left, right)

#define KUNIT_EXPECT_GE_MSG(test, left, right, fmt, ...)		       \
	KUNIT_BINARY_GE_MSG_ASSERTION(test,				       \
				      KUNIT_EXPECTATION,		       \
				      left,				       \
				      right,				       \
				      fmt,				       \
				      ##__VA_ARGS__)


#define KUNIT_EXPECT_STREQ(test, left, right) \
	KUNIT_BINARY_STR_EQ_ASSERTION(test, KUNIT_EXPECTATION, left, right)

#define KUNIT_EXPECT_STREQ_MSG(test, left, right, fmt, ...)		       \
	KUNIT_BINARY_STR_EQ_MSG_ASSERTION(test,				       \
					  KUNIT_EXPECTATION,		       \
					  left,				       \
					  right,			       \
					  fmt,				       \
					  ##__VA_ARGS__)


#define KUNIT_EXPECT_STRNEQ(test, left, right) \
	KUNIT_BINARY_STR_NE_ASSERTION(test, KUNIT_EXPECTATION, left, right)

#define KUNIT_EXPECT_STRNEQ_MSG(test, left, right, fmt, ...)		       \
	KUNIT_BINARY_STR_NE_MSG_ASSERTION(test,				       \
					  KUNIT_EXPECTATION,		       \
					  left,				       \
					  right,			       \
					  fmt,				       \
					  ##__VA_ARGS__)


#define KUNIT_EXPECT_NOT_ERR_OR_NULL(test, ptr) \
	KUNIT_PTR_NOT_ERR_OR_NULL_ASSERTION(test, KUNIT_EXPECTATION, ptr)

#define KUNIT_EXPECT_NOT_ERR_OR_NULL_MSG(test, ptr, fmt, ...)		       \
	KUNIT_PTR_NOT_ERR_OR_NULL_MSG_ASSERTION(test,			       \
						KUNIT_EXPECTATION,	       \
						ptr,			       \
						fmt,			       \
						##__VA_ARGS__)

#define KUNIT_ASSERT_FAILURE(test, fmt, ...) \
	KUNIT_FAIL_ASSERTION(test, KUNIT_ASSERTION, fmt, ##__VA_ARGS__)


#define KUNIT_ASSERT_TRUE(test, condition) \
	KUNIT_TRUE_ASSERTION(test, KUNIT_ASSERTION, condition)

#define KUNIT_ASSERT_TRUE_MSG(test, condition, fmt, ...)		       \
	KUNIT_TRUE_MSG_ASSERTION(test,					       \
				 KUNIT_ASSERTION,			       \
				 condition,				       \
				 fmt,					       \
				 ##__VA_ARGS__)


#define KUNIT_ASSERT_FALSE(test, condition) \
	KUNIT_FALSE_ASSERTION(test, KUNIT_ASSERTION, condition)

#define KUNIT_ASSERT_FALSE_MSG(test, condition, fmt, ...)		       \
	KUNIT_FALSE_MSG_ASSERTION(test,					       \
				  KUNIT_ASSERTION,			       \
				  condition,				       \
				  fmt,					       \
				  ##__VA_ARGS__)


#define KUNIT_ASSERT_EQ(test, left, right) \
	KUNIT_BINARY_EQ_ASSERTION(test, KUNIT_ASSERTION, left, right)

#define KUNIT_ASSERT_EQ_MSG(test, left, right, fmt, ...)		       \
	KUNIT_BINARY_EQ_MSG_ASSERTION(test,				       \
				      KUNIT_ASSERTION,			       \
				      left,				       \
				      right,				       \
				      fmt,				       \
				      ##__VA_ARGS__)


#define KUNIT_ASSERT_PTR_EQ(test, left, right) \
	KUNIT_BINARY_PTR_EQ_ASSERTION(test, KUNIT_ASSERTION, left, right)

#define KUNIT_ASSERT_PTR_EQ_MSG(test, left, right, fmt, ...)		       \
	KUNIT_BINARY_PTR_EQ_MSG_ASSERTION(test,				       \
					  KUNIT_ASSERTION,		       \
					  left,				       \
					  right,			       \
					  fmt,				       \
					  ##__VA_ARGS__)


#define KUNIT_ASSERT_NE(test, left, right) \
	KUNIT_BINARY_NE_ASSERTION(test, KUNIT_ASSERTION, left, right)

#define KUNIT_ASSERT_NE_MSG(test, left, right, fmt, ...)		       \
	KUNIT_BINARY_NE_MSG_ASSERTION(test,				       \
				      KUNIT_ASSERTION,			       \
				      left,				       \
				      right,				       \
				      fmt,				       \
				      ##__VA_ARGS__)


#define KUNIT_ASSERT_PTR_NE(test, left, right) \
	KUNIT_BINARY_PTR_NE_ASSERTION(test, KUNIT_ASSERTION, left, right)

#define KUNIT_ASSERT_PTR_NE_MSG(test, left, right, fmt, ...)		       \
	KUNIT_BINARY_PTR_NE_MSG_ASSERTION(test,				       \
					  KUNIT_ASSERTION,		       \
					  left,				       \
					  right,			       \
					  fmt,				       \
					  ##__VA_ARGS__)

#define KUNIT_ASSERT_LT(test, left, right) \
	KUNIT_BINARY_LT_ASSERTION(test, KUNIT_ASSERTION, left, right)

#define KUNIT_ASSERT_LT_MSG(test, left, right, fmt, ...)		       \
	KUNIT_BINARY_LT_MSG_ASSERTION(test,				       \
				      KUNIT_ASSERTION,			       \
				      left,				       \
				      right,				       \
				      fmt,				       \
				      ##__VA_ARGS__)

#define KUNIT_ASSERT_LE(test, left, right) \
	KUNIT_BINARY_LE_ASSERTION(test, KUNIT_ASSERTION, left, right)

#define KUNIT_ASSERT_LE_MSG(test, left, right, fmt, ...)		       \
	KUNIT_BINARY_LE_MSG_ASSERTION(test,				       \
				      KUNIT_ASSERTION,			       \
				      left,				       \
				      right,				       \
				      fmt,				       \
				      ##__VA_ARGS__)


#define KUNIT_ASSERT_GT(test, left, right) \
	KUNIT_BINARY_GT_ASSERTION(test, KUNIT_ASSERTION, left, right)

#define KUNIT_ASSERT_GT_MSG(test, left, right, fmt, ...)		       \
	KUNIT_BINARY_GT_MSG_ASSERTION(test,				       \
				      KUNIT_ASSERTION,			       \
				      left,				       \
				      right,				       \
				      fmt,				       \
				      ##__VA_ARGS__)


#define KUNIT_ASSERT_GE(test, left, right) \
	KUNIT_BINARY_GE_ASSERTION(test, KUNIT_ASSERTION, left, right)

#define KUNIT_ASSERT_GE_MSG(test, left, right, fmt, ...)		       \
	KUNIT_BINARY_GE_MSG_ASSERTION(test,				       \
				      KUNIT_ASSERTION,			       \
				      left,				       \
				      right,				       \
				      fmt,				       \
				      ##__VA_ARGS__)


#define KUNIT_ASSERT_STREQ(test, left, right) \
	KUNIT_BINARY_STR_EQ_ASSERTION(test, KUNIT_ASSERTION, left, right)

#define KUNIT_ASSERT_STREQ_MSG(test, left, right, fmt, ...)		       \
	KUNIT_BINARY_STR_EQ_MSG_ASSERTION(test,				       \
					  KUNIT_ASSERTION,		       \
					  left,				       \
					  right,			       \
					  fmt,				       \
					  ##__VA_ARGS__)


#define KUNIT_ASSERT_STRNEQ(test, left, right) \
	KUNIT_BINARY_STR_NE_ASSERTION(test, KUNIT_ASSERTION, left, right)

#define KUNIT_ASSERT_STRNEQ_MSG(test, left, right, fmt, ...)		       \
	KUNIT_BINARY_STR_NE_MSG_ASSERTION(test,				       \
					  KUNIT_ASSERTION,		       \
					  left,				       \
					  right,			       \
					  fmt,				       \
					  ##__VA_ARGS__)


#define KUNIT_ASSERT_NOT_ERR_OR_NULL(test, ptr) \
	KUNIT_PTR_NOT_ERR_OR_NULL_ASSERTION(test, KUNIT_ASSERTION, ptr)

#define KUNIT_ASSERT_NOT_ERR_OR_NULL_MSG(test, ptr, fmt, ...)		       \
	KUNIT_PTR_NOT_ERR_OR_NULL_MSG_ASSERTION(test,			       \
						KUNIT_ASSERTION,	       \
						ptr,			       \
						fmt,			       \
						##__VA_ARGS__)

#endif 
