/* CORPUS UART client for Windows.
 * Usage: corpus_uart.exe COM5 [values] [allowed]
 * Example: corpus_uart.exe COM5 0x34 0xff
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
        if (!ReadFile(port, buffer + received, length - received, &chunk, NULL)) {
            return 0;
        }
        if (chunk == 0) {
            return 0;
        }
        received += chunk;
    }
    return 1;
}

int main(int argc, char **argv) {
    const char *port_name;
    unsigned long values = 0x34;
    unsigned long allowed = 0xff;
    char device_name[64];
    DCB settings = {0};
    COMMTIMEOUTS timeouts = {0};
    HANDLE port;
    uint8_t request[4];
    uint8_t response[4];
    uint32_t payload;

    if (argc < 2 || argc > 4) {
        fprintf(stderr, "Uso: %s COM5 [values] [allowed]\n", argv[0]);
        return 2;
    }
    port_name = argv[1];
    if (argc >= 3 && (!parse_number(argv[2], &values) || values > 0x3ff)) {
        fprintf(stderr, "values debe estar entre 0x000 y 0x3ff\n");
        return 2;
    }
    if (argc == 4 && (!parse_number(argv[3], &allowed) || allowed > 0xff)) {
        fprintf(stderr, "allowed debe estar entre 0x00 y 0xff\n");
        return 2;
    }

    snprintf(device_name, sizeof(device_name), "\\\\.\\%s", port_name);
    port = CreateFileA(device_name, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                       OPEN_EXISTING, 0, NULL);
    if (port == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "No se pudo abrir %s (error %lu)\n", port_name,
                GetLastError());
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
    request[0] = 0xa5;
    request[1] = (uint8_t)(values & 0xff);
    request[2] = (uint8_t)((values >> 8) & 0x03);
    request[3] = (uint8_t)allowed;

    {
        DWORD sent = 0;
        if (!WriteFile(port, request, sizeof(request), &sent, NULL) ||
            sent != sizeof(request)) {
            fprintf(stderr, "No se pudo enviar la solicitud\n");
            CloseHandle(port);
            return 1;
        }
    }

    if (!read_exact(port, response, sizeof(response))) {
        fprintf(stderr, "Respuesta incompleta o timeout\n");
        CloseHandle(port);
        return 1;
    }
    CloseHandle(port);

    if (response[0] != 0x5a) {
        fprintf(stderr, "Cabecera invalida: 0x%02x\n", response[0]);
        return 1;
    }

    payload = ((uint32_t)response[1] << 16) |
              ((uint32_t)response[2] << 8) | response[3];
    printf("values=0x%03x\n", (unsigned)((payload >> 14) & 0x3ff));
    printf("status=%u areas=%u\n", (unsigned)((payload >> 12) & 0x03),
           (unsigned)((payload >> 10) & 0x03));
    printf("support=0x%02x\n", (unsigned)((payload >> 2) & 0xff));
    printf("needs_base_refinement=%u direct=%u\n",
           (unsigned)((payload >> 1) & 1), (unsigned)(payload & 1));
    return 0;
}
