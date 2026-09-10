/* Batch CORPUS UART client for Windows.
 * Usage: corpus_uart_batch.exe COM5 < cases.txt
 * Input lines: values allowed, both hexadecimal or decimal.
 * Output lines: payload as six hexadecimal digits, or ERROR.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static int parse_number(const char *text, unsigned long *value) {
    char *end = NULL;
    *value = strtoul(text, &end, 0);
    return end != text && *end == '\0';
}

static int read_exact(HANDLE port, uint8_t *buffer, DWORD length) {
    DWORD received = 0;
    while (received < length) {
        DWORD chunk = 0;
        if (!ReadFile(port, buffer + received, length - received, &chunk, NULL) || chunk == 0) {
            return 0;
        }
        received += chunk;
    }
    return 1;
}

int main(int argc, char **argv) {
    char device_name[64];
    DCB settings = {0};
    COMMTIMEOUTS timeouts = {0};
    HANDLE port;
    char line[128];

    if (argc != 2) {
        fprintf(stderr, "Uso: %s COM5 < casos.txt\n", argv[0]);
        return 2;
    }
    snprintf(device_name, sizeof(device_name), "\\\\.\\%s", argv[1]);
    port = CreateFileA(device_name, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                       OPEN_EXISTING, 0, NULL);
    if (port == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "No se pudo abrir %s (error %lu)\n", argv[1], GetLastError());
        return 1;
    }
    settings.DCBlength = sizeof(settings);
    if (!GetCommState(port, &settings)) {
        fprintf(stderr, "No se pudo leer la configuracion serie\n");
        CloseHandle(port);
        return 1;
    }
    settings.BaudRate = CBR_115200;
    settings.ByteSize = 8;
    settings.Parity = NOPARITY;
    settings.StopBits = ONESTOPBIT;
    if (!SetCommState(port, &settings)) {
        fprintf(stderr, "No se pudo configurar 115200 8N1\n");
        CloseHandle(port);
        return 1;
    }
    timeouts.ReadIntervalTimeout = 100;
    timeouts.ReadTotalTimeoutConstant = 3000;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    if (!SetCommTimeouts(port, &timeouts)) {
        fprintf(stderr, "No se pudieron configurar los tiempos de espera\n");
        CloseHandle(port);
        return 1;
    }
    PurgeComm(port, PURGE_RXCLEAR | PURGE_TXCLEAR);

    while (fgets(line, sizeof(line), stdin)) {
        unsigned long values, allowed;
        uint8_t request[4], response[4];
        DWORD sent;
        uint32_t payload;
        {
            char values_text[64], allowed_text[64];
            if (sscanf_s(line, "%63s %63s", values_text, (unsigned)_countof(values_text),
                         allowed_text, (unsigned)_countof(allowed_text)) != 2 ||
                !parse_number(values_text, &values) || !parse_number(allowed_text, &allowed) ||
                values > 0x3ff || allowed > 0xff) {
                puts("ERROR");
                continue;
            }
        }
        request[0] = 0xa5;
        request[1] = (uint8_t)values;
        request[2] = (uint8_t)(values >> 8);
        request[3] = (uint8_t)allowed;
        if (!WriteFile(port, request, sizeof(request), &sent, NULL) || sent != sizeof(request) ||
            !read_exact(port, response, sizeof(response)) || response[0] != 0x5a) {
            puts("ERROR");
            fflush(stdout);
            continue;
        }
        payload = ((uint32_t)response[1] << 16) |
                  ((uint32_t)response[2] << 8) | response[3];
        printf("%06x\n", (unsigned)payload);
        fflush(stdout);
    }
    CloseHandle(port);
    return 0;
}
