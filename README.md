# SEVENTH

SEVENTH (Simple EVENT Here) is a minimalist C99 event loop library with no external dependencies, consisting only of one source file and one header file. You can directly integrate these two files into your source code.

> Highly referenced `libhv` project.

## Features

- GNU C99 and Linux only.
- Easy to compile and no need to link external library.
- Suitable for small and medium-sized projects.
- Consice API and easy to learn event based programming.
- Event engine based on `select`, `poll` or `epoll`.
- Support I/O events, idle events and timer events.
- User responsiable for handling I/O reads and writes.
- Support custom memory and print function.
- About 150KB dynamic library.
- Approximate performance of using system function directly.
- Built-in Linux doubly linked list implementation.
- Built-in simple heap (priority queue) implementation.
- Built-in macro based dynamic array implementation.

## Usage

### Source integrated (Easy)

Just drag `seventh.c` and `seventh.h` into your project and build the project what the fxxk you want.

### Lib integrated (Recommended)

1. Compile static and dynamic libraries:

    ```shell
    $ make
    cc -c ./src/seventh.c -o ./lib/seventh.o
    ar rc ./lib/libseventh.a ./lib/seventh.o
    cc  -g3 -O2 -Wno-sign-compare  -I./src -DEVENT_EPOLL ./src/seventh.c -fPIC -shared -o ./lib/libseventh.so

    $ ls lib
    libseventh.a  libseventh.so  seventh.o
    ```

2. Install libraries to system directories

    ```shell
    $ sudo make install
    './lib/libseventh.a' -> '/usr/local/lib/libseventh.a'
    './lib/libseventh.so' -> '/usr/local/lib/libseventh.so'
    './src/seventh.h' -> '/usr/local/include/seventh.h'
    
    # Flush ld.so cache after installation
    $ sudo ldconfig

    # Compile your program and link with `-lseventh` option
    $ gcc ... -lseventh ...
    ```

3. [Optional] If you don't have root permission or you just want to use libseventh in the project directory. e.g. Copy the lib in `[your_project]/lib` directory and copy the header file in `[your_project]/include`, then compile and link your project like `gcc ... -lseventh -L./lib -I./include -Wl,-rpath,./lib`.

## Examples

```shell
make example
cd examples
./basic
```

## APIs

```c
// event loop
evloop_t* evloop_new(int max);
int evloop_run(evloop_t* loop);
int evloop_stop(evloop_t* loop);
void evloop_free(evloop_t** pp);

// idle event
evidle_t* evidle_add(evloop_t* loop, evidle_cb cb, uint32_t repeat);
void evidle_del(evidle_t* idle);

// timer event
evtimer_t* evtimer_add(evloop_t* loop, evtimer_cb cb, uint32_t timeout_ms, uint32_t repeat);
evtimer_t* evtimer_add_period(evloop_t* loop, evtimer_cb cb, int8_t minute, int8_t hour,
                              int8_t day, int8_t week, int8_t month,
                              uint32_t repeat);
void evtimer_del(evtimer_t* timer);
void evtimer_reset(evtimer_t* timer, uint32_t etimeout_ms);

// io event
evio_t* evio_read(evloop_t* loop, int fd, evio_cb read_cb);
int evio_del(evio_t* io, int events);
```
