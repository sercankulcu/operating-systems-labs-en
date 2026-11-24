# Operating Systems in C

C code for the undergraduate **Operating Systems** course at Giresun University, Department of Computer Engineering. Names are the ones used in the English edition of the course (lecture notes and slides), so each program can be read side by side with them.

Course page, lecture notes, slides, study questions and simulators: <https://sercankulcu.github.io/teaching/operating-systems/?lang=en>

The same programs with Turkish identifiers, for the Turkish edition of the course, are in [operating-systems-labs](https://github.com/sercankulcu/operating-systems-labs).

The repository has two parts:

- **`ChapterNN/`**: the code shown on the slides of chapter NN, as small runnable programs. Function, variable and structure names follow the slides. C-style pseudocode on the slides (`wait`/`signal`, `test_and_set`, `enter_region` …) is completed into working code with pthreads and C11 atomics. Kernel code (`spin_lock`, `kmalloc`, `task_struct`, interrupt handlers …) is shown as a small user-space simulation that keeps the same names.
- **`labNN/`**: lab exercises, each with its own `Makefile`.

## Chapters

| Folder | Chapter | Programs |
|---|---|---|
| `Chapter01` | Introduction | Spooling queue |
| `Chapter02` | Operating System Structures | System-call dispatch, `read`/`write`, file copy, a minimal shell, `link`/`unlink`, microkernel messages |
| `Chapter03` | Processes | `fork` and its return value, `fork`/`exec`/`waitpid`, output redirection, shared memory, pipes and pipelines, a `task_struct` model, context switching, memory regions of a process, multiprogramming |
| `Chapter04` | Threads | Dispatcher/worker threads, `pthread` creation, arguments and results, sum with threads, pop-up (detached) threads, `thread_yield`, thread-local storage, Amdahl's law, OpenMP-style loops with pthreads |
| `Chapter05` | CPU Scheduling | FCFS, exponential averaging, round robin, lottery, PCS/SCS, load balancing, Liu-Layland test, EDF, POSIX real-time scheduling, CFS `vruntime` |
| `Chapter06` | Synchronization | Lock variable, strict alternation, Peterson, memory model, `test_and_set`, `compare_and_swap`, futex, semaphores, lost wake-up, bounded buffer, readers-writers, dining philosophers, monitors and condition variables, POSIX semaphores, lock-free stack |
| `Chapter07` | Deadlocks | Lock ordering, deadlock scenarios, cycle detection, deadlock detection, banker's algorithm, livelock, atomicity and order violations |
| `Chapter08` | Memory Management | Direct-mapped cache, effective access time, dynamic loading, best fit, bitmap free space, MMU and two-level address translation |
| `Chapter09` | Virtual Memory | Address translation, copy-on-write after `fork`, `mmap`, clock algorithm, page-fault frequency, locality, buddy allocator |
| `Chapter10` | File Systems | `stat`, `open`/`read`/`close`, file copy, FAT chains, bitmap block allocation, buffer cache, journaling recovery |
| `Chapter11` | I/O | Polling, programmed, interrupt-driven and DMA I/O, device-independent I/O, ring buffer, `epoll` event loop, timer list, disk scheduling, RAID 5, port and memory-mapped I/O |
| `Chapter12` | Multiprocessor Systems | False sharing, test-and-test-and-set lock, hypercube routing, Linda `in` |
| `Chapter13` | Security | Diffie-Hellman, access control lists, least privilege, TOTP, stack overflow and stack canary, TOCTOU |
| `Chapter14` | Operating System Design | Algorithmic code, event loop, static tables, hiding hardware, reentrancy, early error checks, bit counting, path cache, KASAN shadow memory, lockdep |
| `Chapter15` | Linux | Pipelines, signal handling, SLUB free list, driver interface, link counts, file permissions |

The Java examples on the slides (threads, `synchronized`, deadlocks, ticket lock) are in the companion repository [operating-systems-java-en](https://github.com/sercankulcu/operating-systems-java-en).

## Running the examples

The programs need Linux (glibc). Every file is a standalone program with a `main` function. No program waits for keyboard input or needs root, and every program removes the temporary files, FIFOs and shared-memory objects it creates. To compile and run one program:

```bash
cd Chapter03
gcc -std=gnu11 -Wall -Wextra -O2 -pthread fork_wait.c -o fork_wait -lm -lrt
./fork_wait
```

Process IDs, addresses, timings and the results of race conditions change from run to run; the comments say where.

`ci/build.sh` compiles and runs every program in the `ChapterNN` folders. GitHub Actions runs it on every push with both `gcc` (warnings are errors) and `clang`.

## Contact

For questions or suggestions, contact [Sercan Külcü](https://github.com/sercankulcu) or open an issue on GitHub.
