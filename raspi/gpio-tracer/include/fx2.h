#ifndef FX2_H
#define FX2_H

#include <libusb.h>
#include <glib.h>

// Vendor Requests
#define LIBUSB_FX2_LOAD_FIRMWARE 0xA0

#define LIBUSB_REQUEST_HOST_TO_DEVICE 0x01 << 7



struct fx2_device_manager {
  libusb_context *libusb_cntxt;
  libusb_device *fx2_dev;
  libusb_device_handle *fx2_dev_handl;

  struct libusb_device_descriptor fx2_desc;
};

int send_control_command(struct fx2_device_manager *manager_instc,
                         guint8 bmRequestType, guint8 bRequest,
                         guint16 wValue, guint16 wIndex, unsigned char *data,
                         guint16 wLength);

void pretty_print_memory(char *mem, int starting_addr, size_t bytes);

int fx2_init_manager(struct fx2_device_manager *manager_instc);

int fx2_find_devices(struct fx2_device_manager *manager_instc);

int fx2_open_device(struct fx2_device_manager *manager_instc);

int fx2_close_device(struct fx2_device_manager *manager_instc);

int fx2_upload_firmware(struct fx2_device_manager *manager_instc, unsigned char *buf);

int fx2_download_firmware(struct fx2_device_manager *manager_instc, unsigned char *buf, size_t length, int verify);

/*
  performs single blocking synchronous bulk transfer
*/
int fx2_submit_bulk_transfer(struct fx2_device_manager *manager_instc);

void* fx2_transfer_loop_thread_func(void *thread_data);

int fx2_start_sampling(struct fx2_device_manager *manager_instc);

int fx2_cpu_set_reset(struct fx2_device_manager *manager_instc);

int fx2_cpu_unset_reset(struct fx2_device_manager *manager_instc);

#endif /* FX2_H */
