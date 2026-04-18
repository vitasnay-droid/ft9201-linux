#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#define VID 0x2808
#define PID 0x9338
#define IFACE 0
#define EP_IN 0x83
#define WIDTH 64
#define HEIGHT 80
#define FRAME_SIZE (WIDTH * HEIGHT)

static int ctrl_out(libusb_device_handle *h, uint8_t req, uint16_t val, uint16_t idx)
{
    int rc = libusb_control_transfer(h, 0x40, req, val, idx, NULL, 0, 2000);
    if (rc < 0) {
        fprintf(stderr,
                "ctrl_out req=0x%02x val=0x%04x idx=0x%04x failed: %s\n",
                req, val, idx, libusb_error_name(rc));
    }
    return rc;
}

static int ctrl_in(libusb_device_handle *h,
                   uint8_t req,
                   uint16_t val,
                   uint16_t idx,
                   unsigned char *buf,
                   uint16_t len)
{
    int rc = libusb_control_transfer(h, 0xC0, req, val, idx, buf, len, 2000);
    if (rc < 0) {
        fprintf(stderr,
                "ctrl_in req=0x%02x val=0x%04x idx=0x%04x failed: %s\n",
                req, val, idx, libusb_error_name(rc));
    }
    return rc;
}

static int bulk_in(libusb_device_handle *h, unsigned char *buf, int len, unsigned int timeout_ms)
{
    int got = 0;
    int rc = libusb_bulk_transfer(h, EP_IN, buf, len, &got, timeout_ms);
    if (rc < 0) {
        fprintf(stderr, "bulk_in len=%d failed: %s\n", len, libusb_error_name(rc));
        return rc;
    }
    return got;
}

static void dump_hex(const char *label, const unsigned char *buf, size_t len)
{
    size_t i;
    printf("%s (%zu bytes):", label, len);
    for (i = 0; i < len; i++) {
        if (i % 16 == 0)
            printf("\n  ");
        printf("%02x ", buf[i]);
    }
    printf("\n");
}

static void save_pgm(const char *path, const unsigned char *img, int w, int h)
{
    FILE *f = fopen(path, "wb");
    if (!f) {
        perror("fopen pgm");
        return;
    }
    fprintf(f, "P5\n%d %d\n255\n", w, h);
    fwrite(img, 1, (size_t) w * (size_t) h, f);
    fclose(f);
}

static int wait_for_status(libusb_device_handle *h,
                           unsigned char wanted0,
                           unsigned char wanted1,
                           unsigned int tries,
                           useconds_t sleep_us,
                           unsigned char out[4])
{
    unsigned int i;

    for (i = 0; i < tries; i++) {
        memset(out, 0, 4);
        int rc = ctrl_in(h, 0x43, 0x0000, 0x0000, out, 4);
        if (rc == 4) {
            printf("status: %02x %02x %02x %02x\n", out[0], out[1], out[2], out[3]);
            if (out[0] == wanted0 && out[1] == wanted1)
                return 0;
        }
        usleep(sleep_us);
    }

    return -1;
}

static int capture_once(libusb_device_handle *h, int shot_no)
{
    unsigned char status[4] = {0};
    unsigned char hdr32[32] = {0};
    unsigned char mark4[4] = {0};
    unsigned char img[FRAME_SIZE] = {0};
    char raw_name[64];
    char pgm_name[64];
    FILE *f;
    int rc;

    printf("\n=== Capture #%d ===\n", shot_no);
    printf("Жду палец...\n");

    if (wait_for_status(h, 0x01, 0x43, 400, 50000, status) != 0) {
        fprintf(stderr, "Палец не пойман. Приложи и подержи 1-2 секунды.\n");
        return 2;
    }

    printf("Палец обнаружен. Читаю кадр...\n");

    if (ctrl_out(h, 0x34, 0x00ff, 0x0000) < 0) return 3;
    if (ctrl_out(h, 0x34, 0x0003, 0x0000) < 0) return 3;
    if (ctrl_out(h, 0x6f, 0x0020, 0x9180) < 0) return 3;

    rc = bulk_in(h, hdr32, sizeof(hdr32), 4000);
    if (rc != 32) {
        fprintf(stderr, "Ожидал 32 байта заголовка, получил %d\n", rc);
        return 3;
    }
    dump_hex("header", hdr32, sizeof(hdr32));

    if (ctrl_out(h, 0x34, 0x00ff, 0x0000) < 0) return 3;
    if (ctrl_out(h, 0x34, 0x0003, 0x0000) < 0) return 3;
    if (ctrl_out(h, 0x6f, 0x0000, 0x00ff) < 0) return 3;

    if (ctrl_out(h, 0x34, 0x00ff, 0x0000) < 0) return 3;
    if (ctrl_out(h, 0x34, 0x0003, 0x0000) < 0) return 3;
    if (ctrl_out(h, 0x6f, 0x0004, 0x9180) < 0) return 3;

    rc = bulk_in(h, mark4, sizeof(mark4), 4000);
    if (rc != 4) {
        fprintf(stderr, "Ожидал 4 байта маркера, получил %d\n", rc);
        return 3;
    }
    dump_hex("marker", mark4, sizeof(mark4));

    if (ctrl_out(h, 0x34, 0x00ff, 0x0000) < 0) return 3;
    if (ctrl_out(h, 0x34, 0x0003, 0x0000) < 0) return 3;
    if (ctrl_out(h, 0x6f, 0x1400, 0x9080) < 0) return 3;

    rc = bulk_in(h, img, FRAME_SIZE, 5000);
    if (rc != FRAME_SIZE) {
        fprintf(stderr, "Ожидал %d байт кадра, получил %d\n", FRAME_SIZE, rc);
        return 3;
    }

    snprintf(raw_name, sizeof(raw_name), "frame_%02d.raw", shot_no);
    snprintf(pgm_name, sizeof(pgm_name), "frame_%02d.pgm", shot_no);

    f = fopen(raw_name, "wb");
    if (!f) {
        perror("fopen raw");
        return 3;
    }
    fwrite(img, 1, FRAME_SIZE, f);
    fclose(f);

    save_pgm(pgm_name, img, WIDTH, HEIGHT);

    printf("Сохранено:\n  %s\n  %s\n", raw_name, pgm_name);
    printf("Жду, пока палец уберут...\n");

    if (wait_for_status(h, 0x00, 0x43, 400, 50000, status) != 0) {
        fprintf(stderr, "Не дождался состояния finger-off.\n");
        return 4;
    }

    printf("Палец убран. Можно делать следующий захват.\n");
    return 0;
}

int main(int argc, char **argv)
{
    libusb_context *ctx = NULL;
    libusb_device_handle *h = NULL;
    char cwd[PATH_MAX] = {0};
    int shots = 1;
    int rc;
    int i;

    if (argc >= 2) {
        shots = atoi(argv[1]);
        if (shots <= 0)
            shots = 1;
    }

    if (!getcwd(cwd, sizeof(cwd)))
        strncpy(cwd, ".", sizeof(cwd) - 1);

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

    printf("Устройство открыто. Каталог вывода: %s\n", cwd);
    printf("Количество захватов: %d\n", shots);

    for (i = 1; i <= shots; i++) {
        rc = capture_once(h, i);
        if (rc != 0)
            break;
    }

    libusb_release_interface(h, IFACE);
    libusb_close(h);
    libusb_exit(ctx);

    return (rc == 0) ? 0 : rc;
}
