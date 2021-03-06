#ifndef AWS_COMMON_COMMON_H
#define AWS_COMMON_COMMON_H

/*
 * Copyright 2010-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <aws/common/exports.h>
#include <aws/common/config.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h> /* for abort() */

#ifndef AWS_STATIC_IMPL
/*
 * In order to allow us to export our inlinable methods in a DLL/.so, we have a designated .c
 * file where this AWS_STATIC_IMPL macro will be redefined to be non-static.
 */
#    define AWS_STATIC_IMPL static inline
#endif

#ifdef __cplusplus
#    define AWS_EXTERN_C_BEGIN extern "C" {
#    define AWS_EXTERN_C_END }
#else
#    define AWS_EXTERN_C_BEGIN
#    define AWS_EXTERN_C_END
#endif

#define AWS_CONCAT(A, B) A ## B

#define AWS_ZERO_STRUCT(object)                                                                                        \
        do {                                                                                                           \
            memset(&(object), 0, sizeof(object));                                                                      \
        } while (0)
#define AWS_ZERO_ARRAY(array)                                                                                          \
        do {                                                                                                           \
            memset((void *)array, 0, sizeof(array));                                                                   \
        } while (0)
#define AWS_ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))


#define AWS_CACHE_LINE 64

/**
 * Format macro for strings of a specified length.
 * Allows non null-terminated strings to be used with the printf family of functions.
 * Ex: printf("scheme is " PRInSTR, 4, "http://example.org"); // ouputs: "scheme is http"
 */
#define PRInSTR "%.*s"

#if defined(_MSC_VER)
#    include <malloc.h>
#    define AWS_ALIGN(alignment) __declspec(align(alignment))
#    define AWS_LIKELY(x) x
#    define AWS_UNLIKELY(x) x
#    define AWS_FORCE_INLINE __forceinline
#    define AWS_VARIABLE_LENGTH_ARRAY(type, name, length) type *name = _alloca(sizeof(type) * length)
#    define AWS_DECLSPEC_NORETURN __declspec(noreturn)
#    define AWS_ATTRIBUTE_NORETURN
#else
#    if defined(__GNUC__) || defined(__clang__)
#        define AWS_ALIGN(alignment) __attribute__((aligned(alignment)))
#        define AWS_TYPE_OF(a) __typeof__(a)
#        define AWS_LIKELY(x) __builtin_expect(!!(x), 1)
#        define AWS_UNLIKELY(x) __builtin_expect(!!(x), 0)
#        define AWS_FORCE_INLINE __attribute__((always_inline))
#        define AWS_DECLSPEC_NORETURN
#        define AWS_ATTRIBUTE_NORETURN __attribute__((noreturn))
#        if defined(__cplusplus)
#            define AWS_VARIABLE_LENGTH_ARRAY(type, name, length) type *name = alloca(sizeof(type) * length)
#        else
#            define AWS_VARIABLE_LENGTH_ARRAY(type, name, length) type name[length];
#        endif
#    endif
#endif

/* If this is C++, restrict isn't supported. If this is not at least C99 on gcc and clang, it isn't supported.
 * If visual C++ building in C mode, the restrict definition is __restrict.
 * This just figures all of that out based on who's including this header file. */
#if defined(__cplusplus)
#    define AWS_RESTRICT
#else
#    if defined(_MSC_VER)
#        define AWS_RESTRICT __restrict
#    else
#        if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#            define AWS_RESTRICT restrict
#        else
#            define AWS_RESTRICT
#        endif
#    endif
#endif

#define AWS_CACHE_ALIGN AWS_ALIGN(AWS_CACHE_LINE)

#include <aws/common/assert.inl>

#define GET_MACRO(_1,_2,NAME,...) NAME

/**
 * Define function contracts.
 * When the code is being verified using CBMC these contracts are formally verified;
 * When the code is built in debug mode, they are checked as much as possible using assertions
 * When the code is built in production mode, they are not checked.
 * Violations of the function contracts are undefined behaviour.
 */
#ifdef CBMC
#define AWS_PRECONDITION_2(cond, explanation) __CPROVER_precondition((cond), (explanation))
#define AWS_PRECONDITION_1(cond) __CPROVER_precondition((cond), #cond " check failed")
#define AWS_POSTCONDITION_2(cond, explanation) __CPROVER_assert((cond), (explanation))
#define AWS_POSTCONDITION_1(cond) __CPROVER_assert((cond), #cond " check failed")
#define AWS_MEM_IS_READABLE(base, len) __CPROVER_r_ok((base), (len))
#define AWS_MEM_IS_WRITABLE(base, len) __CPROVER_w_ok((base), (len))
#else
#define AWS_PRECONDITION_2(cond, expl) AWS_ASSERT(cond)
#define AWS_PRECONDITION_1(cond) AWS_ASSERT(cond)
#define AWS_POSTCONDITION_2(cond, expl) AWS_ASSERT(cond)
#define AWS_POSTCONDITION_1(cond) AWS_ASSERT(cond)
/* the C runtime does not give a way to check these properties,
 * but we can at least check that the pointer is valid */
#define AWS_MEM_IS_READABLE(base, len) (((len) == 0) || (base))
#define AWS_MEM_IS_WRITABLE(base, len) (((len) == 0) || (base))
#endif

// The UNUSED is used to silence the complains of GCC for zero arguments in variadic macro
#define AWS_PRECONDITION(...) GET_MACRO(__VA_ARGS__, AWS_PRECONDITION_2, AWS_PRECONDITION_1, UNUSED)(__VA_ARGS__)
#define AWS_POSTCONDITION(...) GET_MACRO(__VA_ARGS__, AWS_POSTCONDITION_2, AWS_POSTCONDITION_1, UNUSED)(__VA_ARGS__)

#define AWS_OBJECT_PTR_IS_READABLE(ptr) AWS_MEM_IS_READABLE((ptr), sizeof(*ptr))
#define AWS_OBJECT_PTR_IS_WRITABLE(ptr) AWS_MEM_IS_WRITABLE((ptr), sizeof(*ptr))

#ifndef NO_STDBOOL
#    include <stdbool.h>
#else
#    ifndef __cplusplus
#        define bool _Bool
#        define true 1
#        define false 0
#    elif defined(__GNUC__) && !defined(__STRICT_ANSI__)

#        define _Bool bool
#        if __cplusplus < 201103L
/* For C++98, define bool, false, true as a GNU extension. */
#            define bool bool
#            define false false
#            define true true
#        endif
#    endif
#endif

#ifndef NO_STDINT
#    include <stdint.h>
/* Android defines SIZE_MAX in limits.h, not stdint.h */
#    ifdef ANDROID
#        include <limits.h>
#    endif
#else
#    if defined(__x86_64__) || defined(_M_AMD64) || defined(__aarch64__) || defined(__ia64__) ||                   \
            defined(__powerpc64__)
#        define PTR_SIZE 8
#    else
#        define PTR_SIZE 4
#    endif

typedef signed char int8_t;
typedef short int int16_t;
typedef int int32_t;
#    if (PTR_SIZE == 8)
typedef long int int64_t;
#    else
typedef long long int int64_t;
#    endif

typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;

typedef unsigned int uint32_t;

#    if (PTR_SIZE == 8)
typedef unsigned long int uint64_t;
#    else
typedef unsigned long long int uint64_t;
#    endif

#    if (PTR_SIZE == 8)
typedef long int intptr_t;
typedef unsigned long int uintptr_t;
#    else
typedef int intptr_t;
typedef unsigned int uintptr_t;
#    endif

#    if (PTR_SIZE == 8)
#        define __INT64_C(c) c##L
#        define __UINT64_C(c) c##UL
#    else
#        define __INT64_C(c) c##LL
#        define __UINT64_C(c) c##ULL
#    endif

#    define INT8_MIN (-128)
#    define INT16_MIN (-32767 - 1)
#    define INT32_MIN (-2147483647 - 1)
#    define INT64_MIN (-__INT64_C(9223372036854775807) - 1)
#    define INT8_MAX (127)
#    define INT16_MAX (32767)
#    define INT32_MAX (2147483647)
#    define INT64_MAX (__INT64_C(9223372036854775807))
#    define UINT8_MAX (255)
#    define UINT16_MAX (65535)
#    define UINT32_MAX (4294967295U)
#    define UINT64_MAX (__UINT64_C(18446744073709551615))

AWS_STATIC_ASSERT(sizeof(uint64_t) == 8);
AWS_STATIC_ASSERT(sizeof(uint32_t) == 4);
AWS_STATIC_ASSERT(sizeof(uint16_t) == 2);
AWS_STATIC_ASSERT(sizeof(uint8_t) == 1);
AWS_STATIC_ASSERT(sizeof(int64_t) == 8);
AWS_STATIC_ASSERT(sizeof(int32_t) == 4);
AWS_STATIC_ASSERT(sizeof(int16_t) == 2);
AWS_STATIC_ASSERT(sizeof(int8_t) == 1);
AWS_STATIC_ASSERT(sizeof(uintptr_t) == sizeof(void *));
AWS_STATIC_ASSERT(sizeof(intptr_t) == sizeof(void *));
AWS_STATIC_ASSERT(sizeof(char) == 1);
#endif

#if defined(_MSC_VER)
typedef int64_t aws_off_t;
#else
#   if _FILE_OFFSET_BITS == 64 || _POSIX_C_SOURCE >= 200112L
typedef off_t aws_off_t;
#   else
typedef long aws_off_t;
#   endif
#endif

AWS_STATIC_ASSERT(sizeof(int64_t) >= sizeof(aws_off_t));

/**
 * from a pointer and a type of the struct containing the node
 * this will get you back to the pointer of the object. member is the name of
 * the instance of struct aws_linked_list_node in your struct.
 */
#define AWS_CONTAINER_OF(ptr, type, member) ((type *)((uint8_t *)(ptr)-offsetof(type, member)))

/*
 * Due to the recursive inclusion of error.h and common.h, we need to define these
 * before including error.h
 */
#define AWS_OP_SUCCESS (0)
#define AWS_OP_ERR (-1)

#include <aws/common/error.h>

#if defined(_MSC_VER)
#    define AWS_THREAD_LOCAL __declspec(thread)
#else
#    define AWS_THREAD_LOCAL __thread
#endif

/* Allocator structure. An instance of this will be passed around for anything needing memory allocation */
struct aws_allocator {
    void *(*mem_acquire)(struct aws_allocator *allocator, size_t size);
    void (*mem_release)(struct aws_allocator *allocator, void *ptr);
    /* Optional method; if not supported, this pointer must be NULL */
    void *(*mem_realloc)(struct aws_allocator *allocator, void *oldptr, size_t oldsize, size_t newsize);
    /* Optional method; if not supported, this pointer must be NULL */
    void *(*mem_calloc)(struct aws_allocator *allocator, size_t num, size_t size);
    void *impl;
};

/* Avoid pulling in CoreFoundation headers in a header file. */
#ifdef __MACH__
struct __CFAllocator;
typedef const struct __CFAllocator *CFAllocatorRef;
#endif

AWS_EXTERN_C_BEGIN

AWS_COMMON_API
struct aws_allocator *aws_default_allocator(void);

#ifdef __MACH__
/**
 * Wraps a CFAllocator around aws_allocator. For Mac only. Use this anytime you need a CFAllocatorRef for interacting
 * with Apple Frameworks. Unfortunately, it allocates memory so we can't make it static file scope, be sure to call
 * aws_wrapped_cf_allocator_destroy when finished.
 */
AWS_COMMON_API
CFAllocatorRef aws_wrapped_cf_allocator_new(struct aws_allocator *allocator);

/**
 * Cleans up any resources alloced in aws_wrapped_cf_allocator_new.
 */
AWS_COMMON_API
void aws_wrapped_cf_allocator_destroy(CFAllocatorRef allocator);
#endif

/**
 * Returns at least `size` of memory ready for usage or returns NULL on failure.
 */
AWS_COMMON_API
void *aws_mem_acquire(struct aws_allocator *allocator, size_t size);

/**
 * Allocates a block of memory for an array of num elements, each of them size bytes long, and initializes all its bits to zero. 
 * Returns null on failure.
 */
AWS_COMMON_API
void *aws_mem_calloc(struct aws_allocator *allocator, size_t num, size_t size);

/**
 * Allocates many chunks of bytes into a single block. Expects to be called with alternating void ** (dest), size_t
 * (size). The first void ** will be set to the root of the allocation. Alignment is assumed to be sizeof(intmax_t).
 *
 * This is useful for allocating structs using the pimpl pattern, as you may allocate the public object and impl object
 * in the same contiguous block of memory.
 *
 * Returns a pointer to the allocation.
 */
AWS_COMMON_API
void *aws_mem_acquire_many(struct aws_allocator *allocator, size_t count, ...);

/**
 * Releases ptr back to whatever allocated it.
 */
AWS_COMMON_API
void aws_mem_release(struct aws_allocator *allocator, void *ptr);

/*
 * Attempts to adjust the size of the pointed-to memory buffer from oldsize to
 * newsize. The pointer (*ptr) may be changed if the memory needs to be
 * reallocated.
 *
 * If reallocation fails, *ptr is unchanged, and this method raises an
 * AWS_ERROR_OOM error.
 */
AWS_COMMON_API
int aws_mem_realloc(struct aws_allocator *allocator, void **ptr, size_t oldsize, size_t newsize);
/*
 * Maintainer note: The above function doesn't return the pointer (as with
 * standard C realloc) as this pattern becomes error-prone when OOMs occur.
 * In particular, we want to avoid losing the old pointer when an OOM condition
 * occurs, so we prefer to take the old pointer as an in/out reference argument
 * that we can leave unchanged on failure.
 */

/*
 * Loads error strings for debugging and logging purposes.
 */
AWS_COMMON_API
void aws_load_error_strings(void);

/**
 * Securely zeroes a memory buffer. This function will attempt to ensure that
 * the compiler will not optimize away this zeroing operation.
 */
AWS_COMMON_API
void aws_secure_zero(void *pBuf, size_t bufsize);

AWS_EXTERN_C_END

enum aws_common_error {
    AWS_ERROR_SUCCESS = 0,
    AWS_ERROR_OOM,
    AWS_ERROR_UNKNOWN,
    AWS_ERROR_SHORT_BUFFER,
    AWS_ERROR_OVERFLOW_DETECTED,
    AWS_ERROR_UNSUPPORTED_OPERATION,
    AWS_ERROR_INVALID_BUFFER_SIZE,
    AWS_ERROR_INVALID_HEX_STR,
    AWS_ERROR_INVALID_BASE64_STR,
    AWS_ERROR_INVALID_INDEX,
    AWS_ERROR_THREAD_INVALID_SETTINGS,
    AWS_ERROR_THREAD_INSUFFICIENT_RESOURCE,
    AWS_ERROR_THREAD_NO_PERMISSIONS,
    AWS_ERROR_THREAD_NOT_JOINABLE,
    AWS_ERROR_THREAD_NO_SUCH_THREAD_ID,
    AWS_ERROR_THREAD_DEADLOCK_DETECTED,
    AWS_ERROR_MUTEX_NOT_INIT,
    AWS_ERROR_MUTEX_TIMEOUT,
    AWS_ERROR_MUTEX_CALLER_NOT_OWNER,
    AWS_ERROR_MUTEX_FAILED,
    AWS_ERROR_COND_VARIABLE_INIT_FAILED,
    AWS_ERROR_COND_VARIABLE_TIMED_OUT,
    AWS_ERROR_COND_VARIABLE_ERROR_UNKNOWN,
    AWS_ERROR_CLOCK_FAILURE,
    AWS_ERROR_LIST_EMPTY,
    AWS_ERROR_DEST_COPY_TOO_SMALL,
    AWS_ERROR_LIST_EXCEEDS_MAX_SIZE,
    AWS_ERROR_LIST_STATIC_MODE_CANT_SHRINK,
    AWS_ERROR_PRIORITY_QUEUE_FULL,
    AWS_ERROR_PRIORITY_QUEUE_EMPTY,
    AWS_ERROR_PRIORITY_QUEUE_BAD_NODE,
    AWS_ERROR_HASHTBL_ITEM_NOT_FOUND,
    AWS_ERROR_INVALID_DATE_STR,
    AWS_ERROR_INVALID_ARGUMENT,
    AWS_ERROR_RANDOM_GEN_FAILED,
    AWS_ERROR_MALFORMED_INPUT_STRING,
    AWS_ERROR_UNIMPLEMENTED,
    AWS_ERROR_INVALID_STATE,
    AWS_ERROR_ENVIRONMENT_GET,
    AWS_ERROR_ENVIRONMENT_SET,
    AWS_ERROR_ENVIRONMENT_UNSET,
    AWS_ERROR_END_COMMON_RANGE = 0x03FF
};

#endif /* AWS_COMMON_COMMON_H */
