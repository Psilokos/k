#ifndef CQUEUE_H
# define CQUEUE_H

# include "compiler.h"

# define cqueue_def(type, size) \
    struct cqueue \
    { \
        type    data[size]; \
        type *  read_ptr; \
        type *  write_ptr; \
        u8      full; \
    }

# define cqueue_init(q) \
    do { \
        q.read_ptr = q.data; \
        q.write_ptr = q.data; \
        q.full = 0; \
    } while (0)

# define cqueue_can_read(q) \
    q.read_ptr != q.write_ptr || q.full

# define cqueue_can_write(q) \
    !q.full

# define _cqueue_advance_ptr(q, qptr) \
    do { \
        if (++qptr >= q.data + array_size(q.data)) \
            qptr = q.data; \
    } while (0);

# define cqueue_read(q, data) \
    do { \
        data = *q.read_ptr; \
        _cqueue_advance_ptr(q, q.read_ptr); \
        q.full = 0; \
    } while (0)

# define cqueue_write(q, data) \
    do { \
        *q.write_ptr = data; \
        _cqueue_advance_ptr(q, q.write_ptr); \
        if (q.write_ptr == q.read_ptr) \
            q.full = 1; \
    } while (0)

# define cqueue_write_back(q, data) \
    do { \
        if (q.read_ptr++ == q.data) \
            q.read_ptr = q.data + array_size(q.data) - 1; \
        *q.read_ptr = data; \
    } while (0)

#endif
