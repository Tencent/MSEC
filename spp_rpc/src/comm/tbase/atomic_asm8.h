#ifndef __ARCH_I386_ATOMIC8__
#define __ARCH_I386_ATOMIC8__

#include <sys/cdefs.h>
__BEGIN_DECLS

/* cmpxchg8b: require Pentium or above */

/*
 * Make sure gcc doesn't try to be clever and move things around
 * on us. We need to use _exactly_ the address the user gave us,
 * not some alias that contains the same information.
 */
typedef struct {
    volatile int64_t counter;
} atomic8_t;

#define ATOMIC_INIT(i)	{ (i) }

/**
 * atomic8_read - read atomic variable
 * @v: pointer of type atomic8_t
 *
 * Atomically reads the value of @v.
 */
static __inline__ int64_t atomic8_read(atomic8_t *v)
{
    int64_t r;
    __asm__ __volatile__(
        "       xorl            %%eax, %%eax\n"
        "       xorl            %%edx, %%edx\n"
        "       xorl            %%ebx, %%ebx\n"
        "       xorl            %%ecx, %%ecx\n"
        "lock;  cmpxchg8b       %1\n"
    : "=A"(r)
                : "o"(v->counter)
                : "memory", "ebx", "ecx", "cc");
    return r;
}

/**
 * atomic8_set - set atomic variable
 * @v: pointer of type atomic8_t
 * @i: required value
 *
 * Atomically sets the value of @v to @i.
 */
static __inline__ void atomic8_set(atomic8_t *v, int64_t i)
{
    int il = i & 0xFFFFFFFF;
    int ih = i >> 32;
#if 0
    int *v2 = (int *)v;
    __asm__ __volatile__(
        "1:     movl            %0, %%eax\n"
        "       movl            %1, %%edx\n"
        "lock;  cmpxchg8b       %0\n"
        "       jnz             1b\n"
    : "+o"(v2[0]), "+o"(v2[1])
                : "b"(il), "c"(ih)
                : "memory", "eax", "edx", "cc");
#else
    __asm__ __volatile__(
        "1:     repz; nop\n" // aka pause
        "lock;  cmpxchg8b       %0\n"
        "       jnz             1b\n"
    : "+o"(v->counter)
                : "A"(v->counter), "b"(il), "c"(ih)
                : "memory", "cc");
#endif
}

/**
 * atomic8_set - set atomic variable
 * @v: pointer of type atomic8_t
 * @i: required value
 *
 * Atomically sets the value of @v to @i.
 */
static __inline__ int64_t atomic8_clear(atomic8_t *v)
{
    int64_t r;
    __asm__ __volatile__(
        "1:     xor		%%ebx, %%ebx\n"
        "       xor		%%ecx, %%ecx\n"
        "lock;  cmpxchg8b       %0\n"
        "       jnz             1b\n"
    : "+o"(v->counter), "=A"(r)
                : "A"(v->counter)
                : "memory", "ebx", "ecx", "cc");
    return r;
}

/**
 * atomic8_add - add integer to atomic variable
 * @i: integer value to add
 * @v: pointer of type atomic8_t
 *
 * Atomically adds @i to @v.
 */
static __inline__ int64_t atomic8_add(int64_t i, atomic8_t *v)
{
    int il = i & 0xFFFFFFFF;
    int ih = i >> 32;
#if 0
    int *v2 = (int *)v;
    __asm__ __volatile__(
        "       movl            %0, %%eax\n"
        "       movl            %1, %%edx\n"
        "1:     movl            %2, %%ebx\n"
        "       movl            %3, %%ecx\n"
        "       addl            %%eax, %%ebx\n"
        "       adcl            %%edx, %%ecx\n"
        "lock;  cmpxchg8b       %0\n"
        "       jnz             1b"
    : "+o"(v2[0]), "+o"(v2[1])
                : "g"(il), "g"(ih)
                : "memory", "eax", "ebx", "ecx", "edx", "cc");
#else
    int64_t r;
    __asm__ __volatile__(
        "1:     movl            %3, %%ebx\n"
        "       movl            %4, %%ecx\n"
        "       addl            %%eax, %%ebx\n"
        "       adcl            %%edx, %%ecx\n"
        "lock;  cmpxchg8b       %0\n"
        "       jnz             1b\n"
    : "+o"(v->counter), "=A"(r)
                : "A"(v->counter), "g"(il), "g"(ih)
                : "memory", "ebx", "ecx", "cc");
    return r;
#endif
}

/**
 * atomic8_add_return - add and return
 * @v: pointer of type atomic8_t
 * @i: integer value to add
 *
 * Atomically adds @i to @v and returns @i + @v
 */
static __inline__ int64_t atomic8_add_return(int64_t i, atomic8_t *v)
{
    int64_t r;
    int il = i & 0xFFFFFFFF;
    int ih = i >> 32;
#if 0

    int *v2 = (int *)v;
    __asm__ __volatile__(
        "       movl            %0, %%eax\n"
        "       movl            %1, %%edx\n"
        "1:     movl            %3, %%ebx\n"
        "       movl            %4, %%ecx\n"
        "       addl            %%eax, %%ebx\n"
        "       adcl            %%edx, %%ecx\n"
        "lock;  cmpxchg8b       %0\n"
        "       jnz             1b\n"
        "       movl            %%ebx, %%eax\n"
        "       movl            %%ecx, %%edx\n"
    : "+o"(v2[0]), "+o"(v2[1]), "=&A"(r)
                : "g"(il), "g"(ih)
                : "memory", "ebx", "ecx", "cc");
#else
    __asm__ __volatile__(
        "1:     movl            %3, %%ebx\n"
        "       movl            %4, %%ecx\n"
        "       addl            %%eax, %%ebx\n"
        "       adcl            %%edx, %%ecx\n"
        "lock;  cmpxchg8b       %0\n"
        "       jnz             1b\n"
        "       movl            %%ebx, %%eax\n"
        "       movl            %%ecx, %%edx\n"
    : "+o"(v->counter), "=A"(r)
                : "A"(v->counter), "g"(il), "g"(ih)
                : "memory", "ebx", "ecx", "cc");
#endif
    return r;
}

/**
 * atomic8_sub - subtract the atomic variable
 * @i: integer value to subtract
 * @v: pointer of type atomic8_t
 *
 * Atomically subtracts @i from @v.
 */
static __inline__ int64_t atomic8_sub(int64_t i, atomic8_t *v)
{
    return atomic8_add(-i, v);
}

/**
 * atomic8_sub_and_test - subtract value from variable and test result
 * @i: integer value to subtract
 * @v: pointer of type atomic8_t
 *
 * Atomically subtracts @i from @v and returns
 * true if the result is zero, or false for all
 * other cases.
 */
static __inline__ int atomic8_sub_and_test(int i, atomic8_t *v)
{
    return atomic8_add_return(-i, v) == 0;
}

/**
 * atomic8_inc - increment atomic variable
 * @v: pointer of type atomic8_t
 *
 * Atomically increments @v by 1.
 */
static __inline__ int64_t atomic8_inc(atomic8_t *v)
{
    return atomic8_add(1, v);
}

/**
 * atomic8_dec - decrement atomic variable
 * @v: pointer of type atomic8_t
 *
 * Atomically decrements @v by 1.
 */
static __inline__ int64_t atomic8_dec(atomic8_t *v)
{
    return atomic8_add(-1, v);
}

/**
 * atomic8_dec_and_test - decrement and test
 * @v: pointer of type atomic8_t
 *
 * Atomically decrements @v by 1 and
 * returns true if the result is 0, or false for all other
 * cases.
 */
static __inline__ int atomic8_dec_and_test(atomic8_t *v)
{
    return atomic8_add_return(-1, v) == 0;
}

/**
 * atomic8_inc_and_test - increment and test
 * @v: pointer of type atomic8_t
 *
 * Atomically increments @v by 1
 * and returns true if the result is zero, or false for all
 * other cases.
 */
static __inline__ int atomic8_inc_and_test(atomic8_t *v)
{
    return atomic8_add_return(1, v) == 0;
}

/**
 * atomic8_add_negative - add and test if negative
 * @v: pointer of type atomic8_t
 * @i: integer value to add
 *
 * Atomically adds @i to @v and returns true
 * if the result is negative, or false when
 * result is greater than or equal to zero.
 */
static __inline__ int atomic8_add_negative(int64_t i, atomic8_t *v)
{
    return atomic8_add_return(i, v) < 0;
}

static __inline__ int64_t atomic8_sub_return(int64_t i, atomic8_t *v)
{
    return atomic8_add_return(-i, v);
}

#define atomic8_inc_return(v)  (atomic8_add_return(1,v))
#define atomic8_dec_return(v)  (atomic8_sub_return(1,v))

__END_DECLS
#endif
