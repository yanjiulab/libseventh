#ifndef __SEVENTH_H__
#define __SEVENTH_H__

#include <assert.h> // for assert
#include <errno.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h> // for NULL
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <unistd.h> // usleep

// -----------------------------------------------------------------------------
// User custom macros
// -----------------------------------------------------------------------------

#define printd     printf

#define EV_MALLOC  malloc
#define EV_CALLOC  calloc
#define EV_REALLOC realloc
#define EV_FREE    free

// -----------------------------------------------------------------------------
// utils
// -----------------------------------------------------------------------------

// memory
#define EV_ALLOC(ptr, size)                   \
    do {                                      \
        *(void**)&(ptr) = EV_CALLOC(size, 1); \
    } while (0)

#define EV_ALLOC_SIZEOF(ptr) EV_ALLOC(ptr, sizeof(*(ptr)))

void* ev_zrealloc(void* oldptr, size_t newsize, size_t oldsize);

// math
#define LD(v)  ((long)(v))
#define LU(v)  ((unsigned long)(v))
#define LLD(v) ((long long)(v))
#define LLU(v) ((unsigned long long)(v))

// #define MAX(a, b) ((a) > (b) ? (a) : (b))
// #define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b)                          \
    ({                                     \
        typeof(a) _max_a = (a);            \
        typeof(b) _max_b = (b);            \
        _max_a > _max_b ? _max_a : _max_b; \
    })
#define MIN(a, b)                          \
    ({                                     \
        typeof(a) _min_a = (a);            \
        typeof(b) _min_b = (b);            \
        _min_a < _min_b ? _min_a : _min_b; \
    })

static inline unsigned long ceil2e(unsigned long num) {
    // 2**0 = 1
    if (num == 0 || num == 1)
        return 1;
    unsigned long n = num - 1;
    int e = 1;
    while (n >>= 1)
        ++e;
    unsigned long ret = 1;
    while (e--)
        ret <<= 1;
    return ret;
}

// -----------------------------------------------------------------------------
// datetime
// -----------------------------------------------------------------------------
#define SECONDS_PER_MINUTE 60
#define SECONDS_PER_HOUR   3600
#define SECONDS_PER_DAY    86400  // 24*3600
#define SECONDS_PER_WEEK   604800 // 7*24*3600

#define ev_sleep(s)        sleep(s)
#define ev_msleep(ms)      usleep((ms) * 1000)
#define ev_usleep(us)      usleep(us)
#define ev_delay(ms)       ev_msleep(ms)

time_t cron_next_timeout(int minute, int hour, int day, int week, int month);

static inline unsigned long long gettimeofday_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * (unsigned long long)1000 + tv.tv_usec / 1000;
}
static inline unsigned long long gettimeofday_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * (unsigned long long)1000000 + tv.tv_usec;
}

extern unsigned long long gethrtime_us();

// -----------------------------------------------------------------------------
// Simple doubly linked list implementation.
// -----------------------------------------------------------------------------

#ifndef prefetch
#ifdef __GNUC__
#define prefetch(x) __builtin_prefetch(x)
#else
#define prefetch(x) (void)0
#endif
#endif

#ifndef offsetof
#define offsetof(type, member) ((size_t)(&((type*)0)->member))
#endif

#ifndef offsetofend
#define offsetofend(type, member) (offsetof(type, member) + sizeof(((type*)0)->member))
#endif

#ifndef container_of
#define container_of(ptr, type, member) ((type*)((char*)(ptr)-offsetof(type, member)))
#endif

struct list_head {
    struct list_head *next, *prev;
};
#define list_node list_head

// TODO: <sys/queue.h> defined LIST_HEAD
#ifndef LIST_HEAD
#define LIST_HEAD(name) struct list_head name = {&(name), &(name)}
#endif

#define INIT_LIST_HEAD list_init
static inline void list_init(struct list_head* list) {
    list->next = list;
    list->prev = list;
}

/*
 * Create a new list.
 */
static struct list_head* list_new(void) {
    struct list_head* new_list = calloc(1, sizeof(struct list_head));
    new_list->next = new_list;
    new_list->prev = new_list;
    return new_list;
}

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_add(struct list_head* n, struct list_head* prev, struct list_head* next) {
    next->prev = n;
    n->next = next;
    n->prev = prev;
    prev->next = n;
}

/**
 * list_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void list_add(struct list_head* n, struct list_head* head) {
    __list_add(n, head, head->next);
}

/**
 * list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void list_add_tail(struct list_head* n, struct list_head* head) {
    __list_add(n, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_del(struct list_head* prev, struct list_head* next) {
    next->prev = prev;
    prev->next = next;
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty() on entry does not return true after this, the entry is
 * in an undefined state.
 */
static inline void __list_del_entry(struct list_head* entry) {
    __list_del(entry->prev, entry->next);
}

static inline void list_del(struct list_head* entry) {
    __list_del(entry->prev, entry->next);
    // entry->next = NULL;
    // entry->prev = NULL;
}

/**
 * list_replace - replace old entry by new one
 * @old : the element to be replaced
 * @new : the new element to insert
 *
 * If @old was empty, it will be overwritten.
 */
static inline void list_replace(struct list_head* old, struct list_head* n) {
    n->next = old->next;
    n->next->prev = n;
    n->prev = old->prev;
    n->prev->next = n;
}

static inline void list_replace_init(struct list_head* old, struct list_head* n) {
    list_replace(old, n);
    INIT_LIST_HEAD(old);
}

/**
 * list_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static inline void list_del_init(struct list_head* entry) {
    __list_del_entry(entry);
    INIT_LIST_HEAD(entry);
}

/**
 * list_move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
static inline void list_move(struct list_head* list, struct list_head* head) {
    __list_del_entry(list);
    list_add(list, head);
}

/**
 * list_move_tail - delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 */
static inline void list_move_tail(struct list_head* list, struct list_head* head) {
    __list_del_entry(list);
    list_add_tail(list, head);
}

/**
 * list_is_last - tests whether @list is the last entry in list @head
 * @list: the entry to test
 * @head: the head of the list
 */
static inline int list_is_last(const struct list_head* list, const struct list_head* head) {
    return list->next == head;
}

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int list_empty(const struct list_head* head) {
    return head->next == head;
}

/**
 * list_empty_careful - tests whether a list is empty and not being modified
 * @head: the list to test
 *
 * Description:
 * tests whether a list is empty _and_ checks that no other CPU might be
 * in the process of modifying either member (next or prev)
 *
 * NOTE: using list_empty_careful() without synchronization
 * can only be safe if the only activity that can happen
 * to the list entry is list_del_init(). Eg. it cannot be used
 * if another CPU could re-list_add() it.
 */
static inline int list_empty_careful(const struct list_head* head) {
    struct list_head* next = head->next;
    return (next == head) && (next == head->prev);
}

/**
 * list_rotate_left - rotate the list to the left
 * @head: the head of the list
 */
static inline void list_rotate_left(struct list_head* head) {
    struct list_head* first;

    if (!list_empty(head)) {
        first = head->next;
        list_move_tail(first, head);
    }
}

/**
 * list_is_singular - tests whether a list has just one entry.
 * @head: the list to test.
 */
static inline int list_is_singular(const struct list_head* head) {
    return !list_empty(head) && (head->next == head->prev);
}

static inline void __list_cut_position(struct list_head* list, struct list_head* head, struct list_head* entry) {
    struct list_head* new_first = entry->next;
    list->next = head->next;
    list->next->prev = list;
    list->prev = entry;
    entry->next = list;
    head->next = new_first;
    new_first->prev = head;
}

/**
 * list_cut_position - cut a list into two
 * @list: a new list to add all removed entries
 * @head: a list with entries
 * @entry: an entry within head, could be the head itself
 *	and if so we won't cut the list
 *
 * This helper moves the initial part of @head, up to and
 * including @entry, from @head to @list. You should
 * pass on @entry an element you know is on @head. @list
 * should be an empty list or a list you do not care about
 * losing its data.
 *
 */
static inline void list_cut_position(struct list_head* list, struct list_head* head, struct list_head* entry) {
    if (list_empty(head))
        return;
    if (list_is_singular(head) && (head->next != entry && head != entry))
        return;
    if (entry == head)
        INIT_LIST_HEAD(list);
    else
        __list_cut_position(list, head, entry);
}

static inline void __list_splice(const struct list_head* list, struct list_head* prev, struct list_head* next) {
    struct list_head* first = list->next;
    struct list_head* last = list->prev;

    first->prev = prev;
    prev->next = first;

    last->next = next;
    next->prev = last;
}

/**
 * list_splice - join two lists, this is designed for stacks
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void list_splice(const struct list_head* list, struct list_head* head) {
    if (!list_empty(list))
        __list_splice(list, head, head->next);
}

/**
 * list_splice_tail - join two lists, each list being a queue
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void list_splice_tail(struct list_head* list, struct list_head* head) {
    if (!list_empty(list))
        __list_splice(list, head->prev, head);
}

/**
 * list_splice_init - join two lists and reinitialise the emptied list.
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * The list at @list is reinitialised
 */
static inline void list_splice_init(struct list_head* list, struct list_head* head) {
    if (!list_empty(list)) {
        __list_splice(list, head, head->next);
        INIT_LIST_HEAD(list);
    }
}

/**
 * list_splice_tail_init - join two lists and reinitialise the emptied list
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * Each of the lists is a queue.
 * The list at @list is reinitialised
 */
static inline void list_splice_tail_init(struct list_head* list, struct list_head* head) {
    if (!list_empty(list)) {
        __list_splice(list, head->prev, head);
        INIT_LIST_HEAD(list);
    }
}

/**
 * list_entry - get the struct for this entry
 * @ptr:	the &struct list_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 */
#define list_entry(ptr, type, member)       container_of(ptr, type, member)

/**
 * list_first_entry - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define list_first_entry(ptr, type, member) list_entry((ptr)->next, type, member)

/**
 * list_for_each	-	iterate over a list
 * @pos:	the &struct list_head to use as a loop cursor.
 * @head:	the head for your list.
 */
#define list_for_each(pos, head)            for (pos = (head)->next; prefetch(pos->next), pos != (head); pos = pos->next)

/**
 * __list_for_each	-	iterate over a list
 * @pos:	the &struct list_head to use as a loop cursor.
 * @head:	the head for your list.
 *
 * This variant differs from list_for_each() in that it's the
 * simplest possible list iteration code, no prefetching is done.
 * Use this for code that knows the list to be very short (empty
 * or 1 entry) most of the time.
 */
#define __list_for_each(pos, head)          for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * list_for_each_prev	-	iterate over a list backwards
 * @pos:	the &struct list_head to use as a loop cursor.
 * @head:	the head for your list.
 */
#define list_for_each_prev(pos, head)       for (pos = (head)->prev; prefetch(pos->prev), pos != (head); pos = pos->prev)

/**
 * list_for_each_safe - iterate over a list safe against removal of list entry
 * @pos:	the &struct list_head to use as a loop cursor.
 * @n:		another &struct list_head to use as temporary storage
 * @head:	the head for your list.
 */
#define list_for_each_safe(pos, n, head)    for (pos = (head)->next, n = pos->next; pos != (head); pos = n, n = pos->next)

/**
 * list_for_each_prev_safe - iterate over a list backwards safe against removal of list entry
 * @pos:	the &struct list_head to use as a loop cursor.
 * @n:		another &struct list_head to use as temporary storage
 * @head:	the head for your list.
 */
#define list_for_each_prev_safe(pos, n, head) \
    for (pos = (head)->prev, n = pos->prev; prefetch(pos->prev), pos != (head); pos = n, n = pos->prev)

/**
 * list_foreach	-	iterate over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define list_foreach(pos, head, member)                                                                            \
    for (pos = list_entry((head)->next, typeof(*pos), member); prefetch(pos->member.next), &pos->member != (head); \
         pos = list_entry(pos->member.next, typeof(*pos), member))

/**
 * list_foreach_reverse - iterate backwards over list of given type.
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define list_foreach_reverse(pos, head, member)                                                                    \
    for (pos = list_entry((head)->prev, typeof(*pos), member); prefetch(pos->member.prev), &pos->member != (head); \
         pos = list_entry(pos->member.prev, typeof(*pos), member))

/**
 * list_prepare_entry - prepare a pos entry for use in list_foreach_continue()
 * @pos:	the type * to use as a start point
 * @head:	the head of the list
 * @member:	the name of the list_struct within the struct.
 *
 * Prepares a pos entry for use as a start point in list_foreach_continue().
 */
#define list_prepare_entry(pos, head, member) ((pos) ?: list_entry(head, typeof(*pos), member))

/**
 * list_foreach_continue - continue iteration over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Continue to iterate over list of given type, continuing after
 * the current position.
 */
#define list_foreach_continue(pos, head, member)                                                                       \
    for (pos = list_entry(pos->member.next, typeof(*pos), member); prefetch(pos->member.next), &pos->member != (head); \
         pos = list_entry(pos->member.next, typeof(*pos), member))

/**
 * list_foreach_continue_reverse - iterate backwards from the given point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Start to iterate over list of given type backwards, continuing after
 * the current position.
 */
#define list_foreach_continue_reverse(pos, head, member)                                                               \
    for (pos = list_entry(pos->member.prev, typeof(*pos), member); prefetch(pos->member.prev), &pos->member != (head); \
         pos = list_entry(pos->member.prev, typeof(*pos), member))

/**
 * list_foreach_from - iterate over list of given type from the current point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Iterate over list of given type, continuing from current position.
 */
#define list_foreach_from(pos, head, member) \
    for (; prefetch(pos->member.next), &pos->member != (head); pos = list_entry(pos->member.next, typeof(*pos), member))

/**
 * list_foreach_safe - iterate over list of given type safe against removal of list entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define list_foreach_safe(pos, n, head, member)                                                                        \
    for (pos = list_entry((head)->next, typeof(*pos), member), n = list_entry(pos->member.next, typeof(*pos), member); \
         &pos->member != (head); pos = n, n = list_entry(n->member.next, typeof(*n), member))

/**
 * list_foreach_safe_continue - continue list iteration safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Iterate over list of given type, continuing after current point,
 * safe against removal of list entry.
 */
#define list_foreach_safe_continue(pos, n, head, member)           \
    for (pos = list_entry(pos->member.next, typeof(*pos), member), \
        n = list_entry(pos->member.next, typeof(*pos), member);    \
         &pos->member != (head); pos = n, n = list_entry(n->member.next, typeof(*n), member))

/**
 * list_foreach_safe_from - iterate over list from current point safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Iterate over list of given type from current point, safe against
 * removal of list entry.
 */
#define list_foreach_safe_from(pos, n, head, member)                                     \
    for (n = list_entry(pos->member.next, typeof(*pos), member); &pos->member != (head); \
         pos = n, n = list_entry(n->member.next, typeof(*n), member))

/**
 * list_foreach_safe_reverse - iterate backwards over list safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Iterate backwards over list of given type, safe against removal
 * of list entry.
 */
#define list_foreach_safe_reverse(pos, n, head, member)                                                                \
    for (pos = list_entry((head)->prev, typeof(*pos), member), n = list_entry(pos->member.prev, typeof(*pos), member); \
         &pos->member != (head); pos = n, n = list_entry(n->member.prev, typeof(*n), member))

/**
 * list_safe_reset_next - reset a stale list_foreach_safe loop
 * @pos:	the loop cursor used in the list_foreach_safe loop
 * @n:		temporary storage used in list_foreach_safe
 * @member:	the name of the list_struct within the struct.
 *
 * list_safe_reset_next is not safe to use in general if the list may be
 * modified concurrently (eg. the lock is dropped in the loop body). An
 * exception to this is if the cursor element (pos) is pinned in the list,
 * and list_safe_reset_next is called after re-taking the lock and before
 * completing the current iteration of the loop body.
 */
#define list_safe_reset_next(pos, n, member) n = list_entry(pos->member.next, typeof(*pos), member)

// -----------------------------------------------------------------------------
// Simple dynamic array implementation.
// at: random access by pos
// effective: push_back, pop_back, del_nomove, swap
// ineffective: add, del
// -----------------------------------------------------------------------------
#define ARRAY_INIT_SIZE                      16

#define ARRAY_DECL(type, atype)                                                                 \
    struct atype {                                                                              \
        type* ptr;                                                                              \
        size_t size;                                                                            \
        size_t maxsize;                                                                         \
    };                                                                                          \
    typedef struct atype atype;                                                                 \
                                                                                                \
    static inline type* atype##_data(atype* p) {                                                \
        return p->ptr;                                                                          \
    }                                                                                           \
                                                                                                \
    static inline int atype##_size(atype* p) {                                                  \
        return p->size;                                                                         \
    }                                                                                           \
                                                                                                \
    static inline int atype##_maxsize(atype* p) {                                               \
        return p->maxsize;                                                                      \
    }                                                                                           \
                                                                                                \
    static inline int atype##_empty(atype* p) {                                                 \
        return p->size == 0;                                                                    \
    }                                                                                           \
                                                                                                \
    static inline type* atype##_at(atype* p, int pos) {                                         \
        if (pos < 0) {                                                                          \
            pos += p->size;                                                                     \
        }                                                                                       \
        assert(pos >= 0 && pos < p->size);                                                      \
        return p->ptr + pos;                                                                    \
    }                                                                                           \
                                                                                                \
    static inline type* atype##_front(atype* p) {                                               \
        return p->size == 0 ? NULL : p->ptr;                                                    \
    }                                                                                           \
                                                                                                \
    static inline type* atype##_back(atype* p) {                                                \
        return p->size == 0 ? NULL : p->ptr + p->size - 1;                                      \
    }                                                                                           \
                                                                                                \
    static inline void atype##_init(atype* p, int maxsize) {                                    \
        p->size = 0;                                                                            \
        p->maxsize = maxsize;                                                                   \
        EV_ALLOC(p->ptr, sizeof(type) * maxsize);                                               \
    }                                                                                           \
                                                                                                \
    static inline void atype##_clear(atype* p) {                                                \
        p->size = 0;                                                                            \
        memset(p->ptr, 0, sizeof(type) * p->maxsize);                                           \
    }                                                                                           \
                                                                                                \
    static inline void atype##_cleanup(atype* p) {                                              \
        EV_FREE(p->ptr);                                                                        \
        p->size = p->maxsize = 0;                                                               \
    }                                                                                           \
                                                                                                \
    static inline void atype##_resize(atype* p, int maxsize) {                                  \
        if (maxsize == 0)                                                                       \
            maxsize = ARRAY_INIT_SIZE;                                                          \
        p->ptr = (type*)ev_zrealloc(p->ptr, sizeof(type) * maxsize, sizeof(type) * p->maxsize); \
        p->maxsize = maxsize;                                                                   \
    }                                                                                           \
                                                                                                \
    static inline void atype##_double_resize(atype* p) {                                        \
        atype##_resize(p, p->maxsize * 2);                                                      \
    }                                                                                           \
                                                                                                \
    static inline void atype##_push_back(atype* p, type* elem) {                                \
        if (p->size == p->maxsize) {                                                            \
            atype##_double_resize(p);                                                           \
        }                                                                                       \
        p->ptr[p->size] = *elem;                                                                \
        p->size++;                                                                              \
    }                                                                                           \
                                                                                                \
    static inline void atype##_pop_back(atype* p) {                                             \
        assert(p->size > 0);                                                                    \
        p->size--;                                                                              \
    }                                                                                           \
                                                                                                \
    static inline void atype##_add(atype* p, type* elem, int pos) {                             \
        if (pos < 0) {                                                                          \
            pos += p->size;                                                                     \
        }                                                                                       \
        assert(pos >= 0 && pos <= p->size);                                                     \
        if (p->size == p->maxsize) {                                                            \
            atype##_double_resize(p);                                                           \
        }                                                                                       \
        if (pos < p->size) {                                                                    \
            memmove(p->ptr + pos + 1, p->ptr + pos, sizeof(type) * (p->size - pos));            \
        }                                                                                       \
        p->ptr[pos] = *elem;                                                                    \
        p->size++;                                                                              \
    }                                                                                           \
                                                                                                \
    static inline void atype##_del(atype* p, int pos) {                                         \
        if (pos < 0) {                                                                          \
            pos += p->size;                                                                     \
        }                                                                                       \
        assert(pos >= 0 && pos < p->size);                                                      \
        p->size--;                                                                              \
        if (pos < p->size) {                                                                    \
            memmove(p->ptr + pos, p->ptr + pos + 1, sizeof(type) * (p->size - pos));            \
        }                                                                                       \
    }                                                                                           \
                                                                                                \
    static inline void atype##_del_nomove(atype* p, int pos) {                                  \
        if (pos < 0) {                                                                          \
            pos += p->size;                                                                     \
        }                                                                                       \
        assert(pos >= 0 && pos < p->size);                                                      \
        p->size--;                                                                              \
        if (pos < p->size) {                                                                    \
            p->ptr[pos] = p->ptr[p->size];                                                      \
        }                                                                                       \
    }                                                                                           \
                                                                                                \
    static inline void atype##_swap(atype* p, int pos1, int pos2) {                             \
        if (pos1 < 0) {                                                                         \
            pos1 += p->size;                                                                    \
        }                                                                                       \
        if (pos2 < 0) {                                                                         \
            pos2 += p->size;                                                                    \
        }                                                                                       \
        type tmp = p->ptr[pos1];                                                                \
        p->ptr[pos1] = p->ptr[pos2];                                                            \
        p->ptr[pos2] = tmp;                                                                     \
    }

// -----------------------------------------------------------------------------
// Simple heap implementation.
// -----------------------------------------------------------------------------

struct heap_node {
    struct heap_node* parent;
    struct heap_node* left;
    struct heap_node* right;
};

typedef int (*heap_compare_fn)(const struct heap_node* lhs, const struct heap_node* rhs);

struct heap {
    struct heap_node* root;
    int nelts;
    // if compare is less_than, root is min of heap
    // if compare is larger_than, root is max of heap
    heap_compare_fn compare;
};

void heap_init(struct heap* heap, heap_compare_fn fn);
void heap_insert(struct heap* heap, struct heap_node* node);
void heap_remove(struct heap* heap, struct heap_node* node);
void heap_dequeue(struct heap* heap);

// -----------------------------------------------------------------------------
// Event
// -----------------------------------------------------------------------------
typedef struct evloop evloop_t;
typedef struct event event_t;
// NOTE: The following structures are subclasses of event_t,
// inheriting event_t data members and function members.
typedef struct evidle evidle_t;
typedef struct evtimer evtimer_t;
typedef struct evtimeout evtimeout_t;
typedef struct evperiod evperiod_t;
typedef struct evio evio_t;

typedef void (*event_cb)(event_t*);
typedef void (*evtimer_cb)(evtimer_t*);
typedef void (*evidle_cb)(evidle_t*);
typedef void (*evio_cb)(evio_t*);

typedef enum {
    EVENT_TYPE_NONE = 0,
    EVENT_TYPE_IO = 0x00000001,
    EVENT_TYPE_TIMEOUT = 0x00000010,
    EVENT_TYPE_PERIOD = 0x00000020,
    EVENT_TYPE_TIMER = EVENT_TYPE_TIMEOUT | EVENT_TYPE_PERIOD,
    EVENT_TYPE_IDLE = 0x00000100,
    EVENT_TYPE_CUSTOM = 0x00000400, // 1024
} event_type_t;

typedef enum {
    EVLOOP_STATUS_STOP,
    EVLOOP_STATUS_RUNNING
} evloop_status_t;

// #define EVENT_SELECT                   0
// #define EVENT_POLL                     1
// #define EVENT_EPOLL                    2

/* Event priority */
#define EVENT_LOWEST_PRIORITY          (-5)
#define EVENT_LOW_PRIORITY             (-3)
#define EVENT_NORMAL_PRIORITY          0
#define EVENT_HIGH_PRIORITY            3
#define EVENT_HIGHEST_PRIORITY         5
#define EVENT_PRIORITY_SIZE            (EVENT_HIGHEST_PRIORITY - EVENT_LOWEST_PRIORITY + 1)
#define EVENT_PRIORITY_INDEX(priority) (priority - EVENT_LOWEST_PRIORITY)

ARRAY_DECL(evio_t*, io_array);

#define IO_ARRAY_INIT_SIZE 1024
struct evloop {
    uint32_t flags;
    evloop_status_t status;
    uint64_t start_ms;     // ms
    uint64_t start_hrtime; // us
    uint64_t end_hrtime;
    uint64_t cur_hrtime;
    uint64_t loop_cnt;
    long pid;
    long tid;
    void* userdata;
    // private:
    //  events
    uint32_t nactives;
    uint32_t npendings;
    // pendings: with priority as array.index
    event_t* pendings[EVENT_PRIORITY_SIZE];
    // idles
    struct list_head idles;
    uint32_t nidles;
    // timers
    struct heap timers;     // monotonic time
    struct heap realtimers; // realtime
    uint32_t ntimers;
    // ios: with fd as array.index
    struct io_array ios;
    uint32_t nios;
    // one loop per thread, so one readbuf per loop is OK.
    // buf_t readbuf;
    void* iowatcher;
};

#define EVENT_FLAGS       \
    unsigned destroy : 1; \
    unsigned active : 1;  \
    unsigned pending : 1;

#define EVENT_FIELDS            \
    evloop_t* loop;             \
    event_type_t event_type;    \
    uint64_t event_id;          \
    event_cb cb;                \
    void* userdata;             \
    void* privdata;             \
    struct event* pending_next; \
    int priority;               \
    EVENT_FLAGS

struct event {
    EVENT_FIELDS
};

struct evidle {
    EVENT_FIELDS
    uint32_t repeat;
    // private:
    struct list_node node;
};

#define TIMER_FIELDS       \
    EVENT_FIELDS           \
    uint32_t repeat;       \
    uint64_t next_timeout; \
    struct heap_node node;

struct evtimer {
    TIMER_FIELDS
};

struct evtimeout {
    TIMER_FIELDS
    uint32_t timeout;
};

struct evperiod {
    TIMER_FIELDS
    int8_t minute;
    int8_t hour;
    int8_t day;
    int8_t week;
    int8_t month;
};

struct evio {
    EVENT_FIELDS
    // flags
    unsigned ready : 1;
    unsigned connected : 1;
    unsigned closed : 1;
    unsigned accept : 1;
    unsigned connect : 1;
    unsigned connectex : 1; // for ConnectEx/DisconnectEx
    unsigned recv : 1;
    unsigned send : 1;
    unsigned recvfrom : 1;
    unsigned sendto : 1;
    unsigned close : 1;
    unsigned alloced_readbuf : 1; // for evio_alloc_readbuf
    unsigned alloced_ssl_ctx : 1; // for evio_new_ssl_ctx
    // public:
    // evio_type_e io_type;
    uint32_t id; // fd cannot be used as unique identifier, so we provide an id
    int fd;
    int error;
    int events;
    int revents;
    struct sockaddr* localaddr;
    struct sockaddr* peeraddr;
    uint64_t last_read_hrtime;
    uint64_t last_write_hrtime;
//     // read
// fifo_buf_t readbuf;
//     unsigned int read_flags;
//     // for eio_read_until
//     union {
//         unsigned int read_until_length;
//         unsigned char read_until_delim;
//     };
//     uint32_t max_read_bufsize;
//     uint32_t small_readbytes_cnt; // for readbuf autosize
//     // write
//     struct write_queue write_queue;
//     pthread_mutex_t write_mutex; // lock write and write_queue
//     uint32_t write_bufsize;
//     uint32_t max_write_bufsize;
//     // callbacks
// read_cb read_cb;
// write_cb write_cb;
// close_cb close_cb;
// accept_cb accept_cb;
// connect_cb connect_cb;
//     // timers
//     int connect_timeout;    // ms
//     int close_timeout;      // ms
//     int read_timeout;       // ms
//     int write_timeout;      // ms
//     int keepalive_timeout;  // ms
//     int heartbeat_interval; // ms
//     eio_send_heartbeat_fn heartbeat_fn;
//     etimer_t* connect_timer;
//     etimer_t* close_timer;
//     etimer_t* read_timer;
//     etimer_t* write_timer;
//     etimer_t* keepalive_timer;
//     etimer_t* heartbeat_timer;
//     // upstream
//     struct eio_s* upstream_io; // for eio_setup_upstream
//     // unpack
//     unpack_setting_t* unpack_setting; // for eio_set_unpack
//     // ssl
//     void* ssl;      // for eio_set_ssl
//     void* ssl_ctx;  // for eio_set_ssl_ctx
//     char* hostname; // for hssl_set_sni_hostname
//     // context
//     void* ctx; // for eio_context / eio_set_context
// private:
#if defined(EVENT_POLL) || defined(EVENT_KQUEUE)
    int event_index[2]; // for poll,kqueue
#endif
};

// evloop
#define EVLOOP_FLAG_RUN_ONCE                   0x00000001
#define EVLOOP_FLAG_AUTO_FREE                  0x00000002
#define EVLOOP_FLAG_QUIT_WHEN_NO_ACTIVE_EVENTS 0x00000004
evloop_t* evloop_new(int flags);
void evloop_free(evloop_t** pp);
int evloop_run(evloop_t* loop);
int evloop_stop(evloop_t* loop);
void evloop_update_time(evloop_t* loop);
uint64_t evloop_now(evloop_t* loop);        // s
uint64_t evloop_now_ms(evloop_t* loop);     // ms
uint64_t evloop_now_us(evloop_t* loop);     // us
uint64_t evloop_now_hrtime(evloop_t* loop); // us

// event
#define event_loop(ev)                (((event_t*)(ev))->loop)
#define event_type(ev)                (((event_t*)(ev))->event_type)
#define event_id(ev)                  (((event_t*)(ev))->event_id)
#define event_cb(ev)                  (((event_t*)(ev))->cb)
#define event_priority(ev)            (((event_t*)(ev))->priority)
#define event_userdata(ev)            (((event_t*)(ev))->userdata)

#define event_set_id(ev, id)          ((event_t*)(ev))->event_id = id
#define event_set_cb(ev, cb)          ((event_t*)(ev))->cb = cb
#define event_set_priority(ev, prio)  ((event_t*)(ev))->priority = prio
#define event_set_userdata(ev, udata) ((event_t*)(ev))->userdata = (void*)udata

uint64_t evloop_next_event_id();

#define TIMER_ENTRY(p) container_of(p, evtimer_t, node)
#define EVENT_ENTRY(p) container_of(p, event_t, pending_node)
#define IDLE_ENTRY(p)  container_of(p, evidle_t, node)

#define EVENT_ACTIVE(ev)      \
    if (!ev->active) {        \
        ev->active = 1;       \
        ev->loop->nactives++; \
    }

#define EVENT_INACTIVE(ev)    \
    if (ev->active) {         \
        ev->active = 0;       \
        ev->loop->nactives--; \
    }

#define EVENT_PENDING(ev)                                                              \
    do {                                                                               \
        if (!ev->pending) {                                                            \
            ev->pending = 1;                                                           \
            ev->loop->npendings++;                                                     \
            event_t** phead = &ev->loop->pendings[EVENT_PRIORITY_INDEX(ev->priority)]; \
            ev->pending_next = *phead;                                                 \
            *phead = (event_t*)ev;                                                     \
        }                                                                              \
    } while (0)

#define EVENT_ADD(loop, ev, cb)                \
    do {                                       \
        ev->loop = loop;                       \
        ev->event_id = evloop_next_event_id(); \
        ev->cb = (event_cb)cb;                 \
        EVENT_ACTIVE(ev);                      \
    } while (0)

#define EVENT_DEL(ev)       \
    do {                    \
        EVENT_INACTIVE(ev); \
        if (!ev->pending) { \
            EV_FREE(ev);    \
        }                   \
    } while (0)

#define EVENT_RESET(ev)   \
    do {                  \
        ev->destroy = 0;  \
        EVENT_ACTIVE(ev); \
        ev->pending = 0;  \
    } while (0)

// evidle
evidle_t* evidle_add(evloop_t* loop, evidle_cb cb, uint32_t repeat);
void evidle_del(evidle_t* idle);

// evtimer
evtimer_t* evtimer_add(evloop_t* loop, evtimer_cb cb, uint32_t timeout_ms, uint32_t repeat);
/*
 * minute   hour    day     week    month       cb
 * 0~59     0~23    1~31    0~6     1~12
 *  -1      -1      -1      -1      -1          cron.minutely
 *  30      -1      -1      -1      -1          cron.hourly
 *  30      1       -1      -1      -1          cron.daily
 *  30      1       15      -1      -1          cron.monthly
 *  30      1       -1       5      -1          cron.weekly
 *  30      1        1      -1      10          cron.yearly
 */
evtimer_t* evtimer_add_period(evloop_t* loop, evtimer_cb cb, int8_t minute, int8_t hour,
                              int8_t day, int8_t week, int8_t month,
                              uint32_t repeat);
void evtimer_del(evtimer_t* timer);
void evtimer_reset(evtimer_t* timer, uint32_t etimeout_ms);

// evio
#define EV_READ  0x0001
#define EV_WRITE 0x0004
#define EV_RDWR  (EV_READ | EV_WRITE)

int iowatcher_init(evloop_t* loop);
int iowatcher_cleanup(evloop_t* loop);
int iowatcher_add_event(evloop_t* loop, int fd, int events);
int iowatcher_del_event(evloop_t* loop, int fd, int events);
int iowatcher_poll_events(evloop_t* loop, int timeout);

const char* evio_engine();
evio_t* evio_get(evloop_t* loop, int fd);
int evio_add(evio_t* io, evio_cb cb, int events);
int evio_del(evio_t* io, int events);
void evio_ready(evio_t* io);
evio_t* evio_read(evloop_t* loop, int fd, evio_cb read_cb);
evio_t* evio_write(evloop_t* loop, int fd, evio_cb write_cb);
// evio_t* evio_write();

#endif