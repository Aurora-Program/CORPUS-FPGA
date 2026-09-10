/* UART client for the fixed 1-3-9 FPGA window pilot.
 * Usage: corpus_window_uart.exe COM5 packed0 packed1 packed2 packed3 ds de do
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static int number(const char *text, unsigned long *value) {
    char *end = NULL;
    *value = strtoul(text, &end, 0);
    return end != text && *end == '\0';
}

static int read_exact(HANDLE port, uint8_t *data, DWORD length) {
    DWORD received = 0;
    while (received < length) {
        DWORD chunk = 0;
        if (!ReadFile(port, data + received, length - received, &chunk, NULL) || chunk == 0)
            return 0;
        received += chunk;
    }
    return 1;
}

static void print_trits(uint8_t byte) {
    for (unsigned shift = 0; shift < 8; shift += 2)
        printf("%u", (unsigned)((byte >> shift) & 3));
}

int main(int argc, char **argv) {
    HANDLE port;
    DCB settings = {0};
    COMMTIMEOUTS timeouts = {0};
    uint8_t request[8] = {0xa6};
    uint8_t response[8];
    unsigned long value;
    char device[64];

    if (argc != 9) {
        fprintf(stderr, "Uso: %s COM5 p0 p1 p2 p3 ds de do\n", argv[0]);
        return 2;
    }
    snprintf(device, sizeof(device), "\\\\.\\%s", argv[1]);
    for (int i = 0; i < 7; ++i) {
        if (!number(argv[i + 2], &value) || value > 0xff) {
            fprintf(stderr, "Argumento %d fuera de rango byte\n", i + 2);
            return 2;
        }
        request[i + 1] = (uint8_t)value;
    }
    port = CreateFileA(device, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                       OPEN_EXISTING, 0, NULL);
    if (port == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "No se pudo abrir %s (error %lu)\n", argv[1], GetLastError());
        return 1;
    }
    settings.DCBlength = sizeof(settings);
    if (!GetCommState(port, &settings)) return CloseHandle(port), 1;
    settings.BaudRate = CBR_115200;
    settings.ByteSize = 8;
    settings.Parity = NOPARITY;
    settings.StopBits = ONESTOPBIT;
    if (!SetCommState(port, &settings)) return CloseHandle(port), 1;
    timeouts.ReadIntervalTimeout = 100;
    timeouts.ReadTotalTimeoutConstant = 3000;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    if (!SetCommTimeouts(port, &timeouts)) return CloseHandle(port), 1;
    PurgeComm(port, PURGE_RXCLEAR | PURGE_TXCLEAR);
    {
        DWORD sent = 0;
        if (!WriteFile(port, request, sizeof(request), &sent, NULL) || sent != sizeof(request) ||
            !read_exact(port, response, sizeof(response))) {
            fprintf(stderr, "Solicitud sin respuesta completa\n");
            CloseHandle(port);
            return 1;
        }
    }
    CloseHandle(port);
    if (response[0] != 0x5b) {
        fprintf(stderr, "Cabecera inesperada: 0x%02x\n", response[0]);
        return 1;
    }
    printf("header=0x%02x\n", response[0]);
    printf("trits=");
    print_trits(response[1]);
    print_trits(response[2]);
    print_trits(response[3]);
    printf("%u\n", (unsigned)(response[4] & 3));
    printf("statuses=DS:%u DE:%u DO:%u\n",
           response[5] & 3, (response[5] >> 2) & 3, (response[5] >> 4) & 3);
    printf("areas=DS:%u DE:%u DO:%u\n",
           response[6] & 3, (response[6] >> 2) & 3, (response[6] >> 4) & 3);
    printf("needs=DS:%u DE:%u DO:%u\n",
           response[7] & 1, (response[7] >> 1) & 1, (response[7] >> 2) & 1);
    return 0;
}
