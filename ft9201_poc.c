#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define VID 0x2808
#define PID 0x9338
#define IFACE 0
#define EP_IN 0x83
#define WIDTH 64
#define HEIGHT 80
#define FRAME_SIZE (WIDTH * HEIGHT)

static int ctrl_out(libusb_device_handle *h, uint8_t req, uint16_t val, uint16_t idx) {
    int rc = libusb_control_transfer(h, 0x40, req, val, idx, NULL, 0, 2000);
    if (rc < 0) {
        fprintf(stderr, "ctrl_out req=0x%02x val=0x%04x idx=0x%04x failed: %s\n",
                req, val, idx, libusb_error_name(rc));
    }
    return rc;
}

static int ctrl_in(libusb_device_handle *h, uint8_t req, uint16_t val, uint16_t idx,
                   unsigned char *buf, uint16_t len) {
    int rc = libusb_control_transfer(h, 0xC0, req, val, idx, buf, len, 2000);
    if (rc < 0) {
        fprintf(stderr, "ctrl_in req=0x%02x val=0x%04x idx=0x%04x failed: %s\n",
                req, val, idx, libusb_error_name(rc));
    }
    return rc;
}

static int bulk_in(libusb_device_handle *h, unsigned char *buf, int len) {
    int got = 0;
    int rc = libusb_bulk_transfer(h, EP_IN, buf, len, &got, 4000);
    if (rc < 0) {
        fprintf(stderr, "bulk_in len=%d failed: %s\n", len, libusb_error_name(rc));
        return rc;
    }
    return got;
}

static void save_pgm(const char *path, const unsigned char *img, int w, int h) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        perror("fopen pgm");
        return;
    }
    fprintf(f, "P5\n%d %d\n255\n", w, h);
    fwrite(img, 1, w * h, f);
    fclose(f);
}

static void dump4(const unsigned char *b) {
    printf("%02x %02x %02x %02x\n", b[0], b[1], b[2], b[3]);
}

int main(void) {
    libusb_context *ctx = NULL;
    libusb_device_handle *h = NULL;
    unsigned char st[4] = {0};
    unsigned char hdr32[32] = {0};
    unsigned char mark4[4] = {0};
    unsigned char img[FRAME_SIZE] = {0};
    int rc;

    rc = libusb_init(&ctx);
    if (rc < 0) {
        fprintf(stderr, "libusb_init failed: %s\n", libusb_error_name(rc));
        return 1;
    }

    h = libusb_open_device_with_vid_pid(ctx, VID, PID);
    if (!h) {
        fprintf(stderr, "Device %04x:%04x not found\n", VID, PID);
        libusb_exit(ctx);
        return 1;
    }

    libusb_set_auto_detach_kernel_driver(h, 1);

    rc = libusb_claim_interface(h, IFACE);
    if (rc < 0) {
        fprintf(stderr, "claim_interface failed: %s\n", libusb_error_name(rc));
        libusb_close(h);
        libusb_exit(ctx);
        return 1;
    }

    printf("Устройство открыто. Жду касание пальцем...\n");

    int seen_ready = 0;
    for (int i = 0; i < 400; i++) {
        memset(st, 0, sizeof(st));
        rc = ctrl_in(h, 0x43, 0x0000, 0x0000, st, 4);
        if (rc == 4) {
            printf("status: ");
            dump4(st);
            if (st[0] == 0x01 && st[1] == 0x43) {
                seen_ready = 1;
                break;
            }
        }
        usleep(50000);
    }

    if (!seen_ready) {
        fprintf(stderr, "Палец не пойман. Попробуй приложить и подержать 1-2 секунды.\n");
        libusb_release_interface(h, IFACE);
        libusb_close(h);
        libusb_exit(ctx);
        return 2;
    }

    printf("Палец обнаружен. Читаю кадр...\n");

    if (ctrl_out(h, 0x34, 0x00ff, 0x0000) < 0) goto fail;
    if (ctrl_out(h, 0x34, 0x0003, 0x0000) < 0) goto fail;
    if (ctrl_out(h, 0x6f, 0x0020, 0x9180) < 0) goto fail;

    rc = bulk_in(h, hdr32, sizeof(hdr32));
    if (rc != 32) {
        fprintf(stderr, "Ожидал 32 байта, получил %d\n", rc);
        goto fail;
    }

    if (ctrl_out(h, 0x34, 0x00ff, 0x0000) < 0) goto fail;
    if (ctrl_out(h, 0x34, 0x0003, 0x0000) < 0) goto fail;
    if (ctrl_out(h, 0x6f, 0x0000, 0x00ff) < 0) goto fail;

    if (ctrl_out(h, 0x34, 0x00ff, 0x0000) < 0) goto fail;
    if (ctrl_out(h, 0x34, 0x0003, 0x0000) < 0) goto fail;
    if (ctrl_out(h, 0x6f, 0x0004, 0x9180) < 0) goto fail;

    rc = bulk_in(h, mark4, sizeof(mark4));
    if (rc != 4) {
        fprintf(stderr, "Ожидал 4 байта, получил %d\n", rc);
        goto fail;
    }

    if (ctrl_out(h, 0x34, 0x00ff, 0x0000) < 0) goto fail;
    if (ctrl_out(h, 0x34, 0x0003, 0x0000) < 0) goto fail;
    if (ctrl_out(h, 0x6f, 0x1400, 0x9080) < 0) goto fail;

    rc = bulk_in(h, img, FRAME_SIZE);
    if (rc != FRAME_SIZE) {
        fprintf(stderr, "Ожидал %d байт кадра, получил %d\n", FRAME_SIZE, rc);
        goto fail;
    }

    FILE *f = fopen("frame.raw", "wb");
    if (!f) {
        perror("fopen raw");
        goto fail;
    }
    fwrite(img, 1, FRAME_SIZE, f);
    fclose(f);

    save_pgm("frame.pgm", img, WIDTH, HEIGHT);

    printf("Готово:\n");
    printf("  %s/frame.raw\n", getenv("PWD"));
    printf("  %s/frame.pgm\n", getenv("PWD"));

    libusb_release_interface(h, IFACE);
    libusb_close(h);
    libusb_exit(ctx);
    return 0;

fail:
    libusb_release_interface(h, IFACE);
    libusb_close(h);
    libusb_exit(ctx);
    return 3;
}
