/* Batch client for the fixed FPGA window pilot.
 * Input: p0 p1 p2 p3 ds de do, one request per line.
 * Output: 8 response bytes as hexadecimal, or ERROR.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static int parse(const char *text, unsigned long *value) {
    char *end = NULL;
    *value = strtoul(text, &end, 0);
    return end != text && *end == '\0' && *value <= 0xff;
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

int main(int argc, char **argv) {
    char device[64], line[256];
    HANDLE port;
    DCB settings = {0};
    COMMTIMEOUTS timeouts = {0};
    if (argc != 2) {
        fprintf(stderr, "Uso: %s COM5 < casos.txt\n", argv[0]);
        return 2;
    }
    snprintf(device, sizeof(device), "\\\\.\\%s", argv[1]);
    port = CreateFileA(device, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                       OPEN_EXISTING, 0, NULL);
    if (port == INVALID_HANDLE_VALUE) return 1;
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

    while (fgets(line, sizeof(line), stdin)) {
        char *tokens[7];
        char *context = NULL;
        char *token = strtok_s(line, " \t\r\n", &context);
        uint8_t request[8], response[8];
        unsigned long value;
        int count = 0;
        DWORD sent;
        while (token && count < 7) {
            tokens[count++] = token;
            token = strtok_s(NULL, " \t\r\n", &context);
        }
        if (count != 7) {
            puts("ERROR");
            continue;
        }
        request[0] = 0xa6;
        for (int i = 0; i < 7; ++i) {
            if (!parse(tokens[i], &value)) {
                count = 0;
                break;
            }
            request[i + 1] = (uint8_t)value;
        }
        if (!count) {
            puts("ERROR");
            continue;
        }
        if (!WriteFile(port, request, sizeof(request), &sent, NULL) || sent != sizeof(request) ||
            !read_exact(port, response, sizeof(response)) || response[0] != 0x5b) {
            puts("ERROR");
            fflush(stdout);
            continue;
        }
        for (int i = 0; i < 8; ++i) printf("%02x%s", response[i], i == 7 ? "\n" : " ");
        fflush(stdout);
    }
    CloseHandle(port);
    return 0;
}
