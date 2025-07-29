#include "storage.h"

void initStorage() {
    remove(STORAGE_PATH);
}

int registerCCTV(SSL *ssl, const char *name) {
    int fd = SSL_get_fd(ssl);
    struct sockaddr_in peer;
    socklen_t plen = sizeof(peer);
    getpeername(fd, (struct sockaddr*)&peer, &plen);

    ActiveCCTV entry;
    strncpy(entry.name, name, sizeof(entry.name));
    inet_ntop(AF_INET, &peer.sin_addr, entry.ip, sizeof(entry.ip));

    return addActiveCCTV(&entry);
}

int addActiveCCTV(const ActiveCCTV *entry) {
    FILE *fp = fopen(STORAGE_PATH, "ab");
    if (!fp) return -1;
    fwrite(entry, sizeof(*entry), 1, fp);
    fclose(fp);
    return 0;
}

int removeActiveCCTV(const char *name) {
    FILE *fp = fopen(STORAGE_PATH, "rb");
    if (!fp) return -1;

    fseek(fp, 0, SEEK_END);
    long filesize = ftell(fp);
    int count = filesize / sizeof(ActiveCCTV);
    rewind(fp);

    ActiveCCTV *arr = (ActiveCCTV*)malloc(filesize);
    if (!arr) { fclose(fp); return -1; }
    fread(arr, sizeof(ActiveCCTV), count, fp);
    fclose(fp);

    fp = fopen(STORAGE_PATH, "wb");
    if (!fp) { free(arr); return -1; }

    for (int i = 0; i < count; i++) {
        if (strcmp(arr[i].name, name) != 0) {
            fwrite(&arr[i], sizeof(ActiveCCTV), 1, fp);
        }
    }

    free(arr);
    fclose(fp);
    return 0;
}

int lookupActiveCCTV(const char *name, char *out_ip_buf) {
    FILE *fp = fopen(STORAGE_PATH, "rb");
    if (!fp)
        return 0;

    ActiveCCTV tmp;
    while (fread(&tmp, sizeof(tmp), 1, fp) == 1) {
        if (strncmp(tmp.name, name, sizeof(name)) == 0) {
            // IP 문자열만 복사
            strncpy(out_ip_buf, tmp.ip, 16);
            out_ip_buf[15] = '\0';
            printf("[IP] : %s\n", out_ip_buf);
            fclose(fp);
            return 1;
        }
    }
    
    fclose(fp);
    return 0;
}
