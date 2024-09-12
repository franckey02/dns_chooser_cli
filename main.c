#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>

// Define the DNS services to test
#define CLOUDFLARE_DNS1 "1.1.1.1"
#define CLOUDFLARE_DNS2 "1.0.0.1"
#define OPENDNS_DNS1 "208.67.222.222"
#define OPENDNS_DNS2 "208.67.220.220"
#define QUAD9_DNS "9.9.9.11"

// Define el dominio de prueba
#define TEST_DOMAIN "example.com"

// Función para resolver un dominio utilizando un servicio DNS específico
int resolve_domain(const char *dns_server, const char *domain, struct timespec *start, struct timespec *end) {
    struct sockaddr_in dns_addr;
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    inet_pton(AF_INET, dns_server, &dns_addr.sin_addr);
    dns_addr.sin_family = AF_INET;
    dns_addr.sin_port = htons(53);

    char query[256];
    sprintf(query, "%s%s", domain, "\0");

    // Iniciar temporizador
    clock_gettime(CLOCK_MONOTONIC, start);

    sendto(sockfd, query, strlen(query), 0, (struct sockaddr *)&dns_addr, sizeof(dns_addr));

    // Detiene temporizador
    clock_gettime(CLOCK_MONOTONIC, end);

    close(sockfd);

    return 0;
}

// Función para calcular el tiempo de respuesta en milisegundos
int calculate_response_time(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000;
}

// Función para seleccionar el servicio DNS más rápido
const char *select_fastest_dns(int cloudflare_time, int opendns_time, int quad9_time) {
    if (cloudflare_time <= opendns_time && cloudflare_time <= quad9_time) {
        return CLOUDFLARE_DNS1;
    } else if (opendns_time <= cloudflare_time && opendns_time <= quad9_time) {
        return OPENDNS_DNS1;
    } else {
        return QUAD9_DNS;
    }
}

// Función para actualizar /etc/resolv.conf con el servicio DNS más rápido
void update_resolv_conf(const char *fastest_dns) {
    FILE *fp;
    fp = fopen("/etc/resolv.conf", "w");
    if (fp == NULL) {
        perror("fopen");
        return;
    }

    fprintf(fp, "nameserver %s\n", fastest_dns);
    fclose(fp);
}

int main() {
    struct timespec start, end;
    int cloudflare_time, opendns_time, quad9_time;

    resolve_domain(CLOUDFLARE_DNS1, TEST_DOMAIN, &start, &end);
    cloudflare_time = calculate_response_time(start, end);

    resolve_domain(OPENDNS_DNS1, TEST_DOMAIN, &start, &end);
    opendns_time = calculate_response_time(start, end);

    resolve_domain(QUAD9_DNS, TEST_DOMAIN, &start, &end);
    quad9_time = calculate_response_time(start, end);

    const char *fastest_dns = select_fastest_dns(cloudflare_time, opendns_time, quad9_time);

    printf("Fastest DNS: %s\n", fastest_dns);

    update_resolv_conf(fastest_dns);

    return 0;
}