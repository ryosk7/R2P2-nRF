#include "r2p2_nrf52_usb.h"
#include "mrubyc.h"

extern const uint8_t main_task[];
bool r2p2_picoruby_init_puts_path(mrbc_vm *vm);

int main(void) {
  static const char usb_boot_banner[] = "[r2p2] usb console open\r\n";
  static const char mrbc_init_banner[] = "[r2p2] mrbc init\r\n";
  static const char task_created_banner[] = "[r2p2] task created\r\n";
  static const char puts_init_banner[] = "[r2p2] puts init begin\r\n";
  static const char puts_init_done_banner[] = "[r2p2] puts init done\r\n";
  static const char run_banner[] = "[r2p2] mrbc run\r\n";
  bool console_banner_sent = false;
  static uint8_t heap_pool[1024 * 80] __attribute__((aligned(8)));

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
      mrbc_init(heap_pool, sizeof(heap_pool));
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
      if (!r2p2_picoruby_init_puts_path(&main_tcb->vm)) {
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
