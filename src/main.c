#include <stddef.h>
#include <stdint.h>

#include "r2p2_nrf52_usb.h"
#include "mrubyc.h"
#include "picoruby_runtime.h"

extern const uint8_t main_task[];

/*
 * The Ruby heap is not a fixed array. It takes all the RAM the linker left
 * between .data/.bss and the bottom of the stack (`_heap_end`), minus a
 * strip at the top for the newlib heap:
 *
 *   .data/.bss | mruby/c pool (auto-sized) | newlib strip | stack
 *   ^end                                   ^_heap_end - reserve
 *
 * Anything added to .data/.bss (a gem, OpenThread) shrinks the pool by
 * itself. The newlib strip grows upward from its base, so its unused top
 * sits next to the stack: a stack that overruns its 24 KB eats into that
 * slack before it reaches the pool.
 *
 * The strip receives everything libc allocates: the UART ring buffers
 * (about 5 bytes per byte of rx_buffer_size), LittleFS caches, the stdio
 * buffer and dtoa. 16 KB still fits rx_buffer_size: 2048 on top of those.
 */
extern char end;       /* end of .data/.bss */
extern char _heap_end; /* bottom of the stack */

#if !defined(R2P2_NEWLIB_HEAP_RESERVE)
  #define R2P2_NEWLIB_HEAP_RESERVE (16 * 1024)
#endif

#if !defined(R2P2_HEAP_MIN_SIZE)
  #define R2P2_HEAP_MIN_SIZE (64 * 1024)
#endif

/*
 * Replace the libnosys _sbrk, which grows the newlib heap from `end` with
 * no limit at all -- straight through the mruby/c pool. A request that
 * does not fit the strip returns -1, so malloc returns NULL rather than
 * corrupting the pool.
 *
 * newlib_heap_end is not static so a debugger can read how much of the
 * strip is in use: p newlib_heap_end - (&_heap_end - reserve)
 */
char *newlib_heap_end;

void *_sbrk(int incr) {
  char *limit = &_heap_end;
  char *base = limit - R2P2_NEWLIB_HEAP_RESERVE;
  if (newlib_heap_end == NULL) {
    newlib_heap_end = base;
  }
  char *prev = newlib_heap_end;
  char *next = newlib_heap_end + incr;
  if (next > limit || next < base) {
    return (void *)-1;
  }
  newlib_heap_end = next;
  return prev;
}

int main(void) {
  static const char usb_boot_banner[] = "[r2p2] usb console open\r\n";
  static const char mrbc_init_banner[] = "[r2p2] mrbc init\r\n";
  static const char task_created_banner[] = "[r2p2] task created\r\n";
  static const char puts_init_banner[] = "[r2p2] puts init begin\r\n";
  static const char puts_init_done_banner[] = "[r2p2] puts init done\r\n";
  static const char run_banner[] = "[r2p2] mrbc run\r\n";
  bool console_banner_sent = false;
  /* mruby/c needs an 8-byte aligned base. */
  uintptr_t heap_lo = ((uintptr_t)&end + 7u) & ~(uintptr_t)7u;
  uintptr_t heap_hi =
    ((uintptr_t)&_heap_end - R2P2_NEWLIB_HEAP_RESERVE) & ~(uintptr_t)7u;

  r2p2_usb_init();
  while (1) {
    r2p2_usb_task();

    if (!console_banner_sent &&
        r2p2_usb_channel_connected(R2P2_USB_CHANNEL_CONSOLE)) {
      r2p2_usb_write(R2P2_USB_CHANNEL_CONSOLE,
        (const uint8_t *)usb_boot_banner,
        sizeof(usb_boot_banner) - 1);
      console_banner_sent = true;

      r2p2_usb_write(R2P2_USB_CHANNEL_CONSOLE,
        (const uint8_t *)mrbc_init_banner,
        sizeof(mrbc_init_banner) - 1);
      if (heap_hi <= heap_lo || heap_hi - heap_lo < R2P2_HEAP_MIN_SIZE) {
        static const char heap_too_small[] = "[r2p2] FATAL: Ruby heap too small\r\n";
        r2p2_usb_write(R2P2_USB_CHANNEL_CONSOLE,
          (const uint8_t *)heap_too_small,
          sizeof(heap_too_small) - 1);
        break;
      }
      mrbc_init((void *)heap_lo, (unsigned int)(heap_hi - heap_lo));
      mrbc_tcb *main_tcb = mrbc_create_task(main_task, 0);
      if (!main_tcb) {
        static const char create_task_failed[] = "mrbc_create_task failed\r\n";
        r2p2_usb_write(R2P2_USB_CHANNEL_CONSOLE,
          (const uint8_t *)create_task_failed,
          sizeof(create_task_failed) - 1);
        break;
      }

      r2p2_usb_write(R2P2_USB_CHANNEL_CONSOLE,
        (const uint8_t *)task_created_banner,
        sizeof(task_created_banner) - 1);
      mrbc_set_task_name(main_tcb, "main_task");
      r2p2_usb_write(R2P2_USB_CHANNEL_CONSOLE,
        (const uint8_t *)puts_init_banner,
        sizeof(puts_init_banner) - 1);
      if (!r2p2_picoruby_init_runtime(&main_tcb->vm)) {
        static const char init_failed[] = "picoruby puts init failed\r\n";
        r2p2_usb_write(R2P2_USB_CHANNEL_CONSOLE,
          (const uint8_t *)init_failed,
          sizeof(init_failed) - 1);
        break;
      }

      r2p2_usb_write(R2P2_USB_CHANNEL_CONSOLE,
        (const uint8_t *)puts_init_done_banner,
        sizeof(puts_init_done_banner) - 1);
      r2p2_usb_write(R2P2_USB_CHANNEL_CONSOLE,
        (const uint8_t *)run_banner,
        sizeof(run_banner) - 1);
      mrbc_run();
    }
  }
}
