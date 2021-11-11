#ifndef FX2_H
#define FX2_H

#include <libusb.h>
#include <glib.h>

// Vendor Requests
#define LIBUSB_FX2_LOAD_FIRMWARE 0xA0

#define LIBUSB_REQUEST_HOST_TO_DEVICE 0x01 << 7

typedef void (*fx2_packet_callback_fn)(uint8_t *packet_data, int length, void *user_data);



struct fx2_device_manager {
  libusb_context *libusb_cntxt;
  libusb_device *fx2_dev;
  libusb_device_handle *fx2_dev_handl;
  struct libusb_device_descriptor fx2_desc;

  GThread *event_handler_thread;
};

struct transfer_data {
  uint8_t transfer_finished;
  size_t  transfer_num;
  fx2_packet_callback_fn callback;
  void *cb_user_data;
};

struct fx2_bulk_transfer_config {
  uint16_t endpoint_addr;
  fx2_packet_callback_fn packet_handler_cb;
  void *transfer_cb_user_data;

  size_t transfer_size;
  size_t transfer_num;

  struct libusb_transfer **transfers;
  struct transfer_data *user_data;
  unsigned char **transfer_buffers;
};

int send_control_command(struct fx2_device_manager *manager_instc,
                         guint8 bmRequestType, guint8 bRequest,
                         guint16 wValue, guint16 wIndex, unsigned char *data,
                         guint16 wLength);

void pretty_print_memory(char *mem, int starting_addr, size_t bytes);

int fx2_init_manager(struct fx2_device_manager *manager_instc);

int fx2_deinit_manager(struct fx2_device_manager *manager_instc);

int fx2_find_devices(struct fx2_device_manager *manager_instc);

int fx2_open_device(struct fx2_device_manager *manager_instc);

int fx2_close_device(struct fx2_device_manager *manager_instc);

int fx2_upload_firmware(struct fx2_device_manager *manager_instc, unsigned char *buf);

int fx2_download_firmware(struct fx2_device_manager *manager_instc,
                          unsigned char *buf, size_t length, int verify);

int fx2_create_bulk_transfer(struct fx2_device_manager *manager_instc,
                             struct fx2_bulk_transfer_config *transfer_cnfg,
                             size_t transfer_num, size_t transfer_size);

int fx2_set_bulk_transfer_packet_callback(struct fx2_bulk_transfer_config *transfer_cnfg,
                            fx2_packet_callback_fn packet_handler,
                            void *user_data);

int fx2_submit_bulk_transfer(struct fx2_bulk_transfer_config *transfer_cnfg);

int fx2_free_bulk_transfer(struct fx2_bulk_transfer_config *transfer_cnfg);

/*
  performs single blocking synchronous bulk transfer
*/
void* fx2_transfer_main_loop(void *thread_data);

int fx2_cpu_set_reset(struct fx2_device_manager *manager_instc);

int fx2_cpu_unset_reset(struct fx2_device_manager *manager_instc);

#endif /* FX2_H */
