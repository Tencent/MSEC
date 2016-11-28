#ifndef __ARCH_I386_ATOMIC__
#define __ARCH_I386_ATOMIC__

#include <sys/cdefs.h>
__BEGIN_DECLS

/* xaddl: require Modern 486+ processor */

/*
 * Make sure gcc doesn't try to be clever and move things around
 * on us. We need to use _exactly_ the address the user gave us,
 * not some alias that contains the same information.
 */
typedef struct {
    volatile int counter;
} atomic_t;

#define ATOMIC_INIT(i)	{ (i) }

/**
 * atomic_read - read atomic variable
 * @v: pointer of type atomic_t
 *
 * Atomically reads the value of @v.
 */
#define atomic_read(v)		((v)->counter)

/**
 * atomic_set - set atomic variable
 * @v: pointer of type atomic_t
 * @i: required value
 *
 * Atomically sets the value of @v to @i.
 */
#define atomic_set(v,i)		(((v)->counter) = (i))

static __inline__ int atomic_clear(atomic_t *v)
{
    int i;
    __asm__ __volatile__(
        "      xorl %0, %0\n"
        "lock; xchgl %0, %1;"
    : "=&r"(i)
                : "m"(v->counter));
    return i;
}

/**
 * atomic_add - add integer to atomic variable
 * @i: integer value to add
 * @v: pointer of type atomic_t
 *
 * Atomically adds @i to @v.
 */
static __inline__ int atomic_add(int i, atomic_t *v)
{
    int __i = i;
    __asm__ __volatile__(
        "lock; xaddl %0, %1;"
    : "=r"(i)
                : "m"(v->counter), "0"(i));
    return __i;
}

/**
 * atomic_sub - subtract the atomic variable
 * @i: integer value to subtract
 * @v: pointer of type atomic_t
 *
 * Atomically subtracts @i from @v.
 */
static __inline__ int atomic_sub(int i, atomic_t *v)
{
    return atomic_add(-i, v);
}

/**
 * atomic_sub_and_test - subtract value from variable and test result
 * @i: integer value to subtract
 * @v: pointer of type atomic_t
 *
 * Atomically subtracts @i from @v and returns
 * true if the result is zero, or false for all
 * other cases.
 */
static __inline__ int atomic_sub_and_test(int i, atomic_t *v)
{
    unsigned char c;

    __asm__ __volatile__(
        "lock; subl %2,%0; sete %1"
    : "=m"(v->counter), "=qm"(c)
                : "ir"(i), "m"(v->counter) : "memory");
    return c;
}

/**
 * atomic_inc - increment atomic variable
 * @v: pointer of type atomic_t
 *
 * Atomically increments @v by 1.
 */
static __inline__ void atomic_inc(atomic_t *v)
{
    __asm__ __volatile__(
        "lock; incl %0"
    : "=m"(v->counter)
                : "m"(v->counter));
}

/**
 * atomic_dec - decrement atomic variable
 * @v: pointer of type atomic_t
 *
 * Atomically decrements @v by 1.
 */
static __inline__ void atomic_dec(atomic_t *v)
{
    __asm__ __volatile__(
        "lock; decl %0"
    : "=m"(v->counter)
                : "m"(v->counter));
}

/**
 * atomic_dec_and_test - decrement and test
 * @v: pointer of type atomic_t
 *
 * Atomically decrements @v by 1 and
 * returns true if the result is 0, or false for all other
 * cases.
 */
static __inline__ int atomic_dec_and_test(atomic_t *v)
{
    unsigned char c;

    __asm__ __volatile__(
        "lock; decl %0; sete %1"
    : "=m"(v->counter), "=qm"(c)
                : "m"(v->counter) : "memory");
    return c != 0;
}

/**
 * atomic_inc_and_test - increment and test
 * @v: pointer of type atomic_t
 *
 * Atomically increments @v by 1
 * and returns true if the result is zero, or false for all
 * other cases.
 */
static __inline__ int atomic_inc_and_test(atomic_t *v)
{
    unsigned char c;

    __asm__ __volatile__(
        "lock; incl %0; sete %1"
    : "=m"(v->counter), "=qm"(c)
                : "m"(v->counter) : "memory");
    return c != 0;
}

/**
 * atomic_add_negative - add and test if negative
 * @v: pointer of type atomic_t
 * @i: integer value to add
 *
 * Atomically adds @i to @v and returns true
 * if the result is negative, or false when
 * result is greater than or equal to zero.
 */
static __inline__ int atomic_add_negative(int i, atomic_t *v)
{
    unsigned char c;

    __asm__ __volatile__(
        "lock; addl %2,%0; sets %1"
    : "=m"(v->counter), "=qm"(c)
                : "ir"(i), "m"(v->counter) : "memory");
    return c;
}

/**
 * atomic_add_return - add and return
 * @v: pointer of type atomic_t
 * @i: integer value to add
 *
 * Atomically adds @i to @v and returns @i + @v
 */
static __inline__ int atomic_add_return(int i, atomic_t *v)
{
    int __i = i;
    __asm__ __volatile__(
        "lock; xaddl %0, %1;"
    : "=r"(i)
                : "m"(v->counter), "0"(i));
    return i + __i;
}

static __inline__ int atomic_sub_return(int i, atomic_t *v)
{
    return atomic_add_return(-i, v);
}

#define atomic_inc_return(v)  (atomic_add_return(1,v))
#define atomic_dec_return(v)  (atomic_sub_return(1,v))

/* These are x86-specific, used by some header files */
#define atomic_clear_mask(mask, addr) \
__asm__ __volatile__("lock; andl %0,%1" \
: : "r" (~(mask)),"m" (*addr) : "memory")

#define atomic_set_mask(mask, addr) \
__asm__ __volatile__("lock; orl %0,%1" \
: : "r" (mask),"m" (*(addr)) : "memory")

__END_DECLS
#endif
