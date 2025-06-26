#include "storage.h"

void init_storage(void) {
    remove(STORAGE_PATH);
}

int register_cctv(SSL *ssl, const char *name) {
    int fd = SSL_get_fd(ssl);
    struct sockaddr_in peer;
    socklen_t plen = sizeof(peer);
    getpeername(fd, (struct sockaddr*)&peer, &plen);

    ActiveCCTV entry;
    strncpy(entry.name, name, sizeof(entry.name));
    inet_ntop(AF_INET, &peer.sin_addr, entry.ip, sizeof(entry.ip));

    return add_active_cctv(&entry);
}

int add_active_cctv(const ActiveCCTV *entry) {
    FILE *fp = fopen(STORAGE_PATH, "ab");
    if (!fp) return -1;
    fwrite(entry, sizeof(*entry), 1, fp);
    fclose(fp);
    return 0;
}

int remove_active_cctv(const char *name) {
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

int lookup_active_cctv(const char *name, char *out_ip_buf, size_t buf_sz) {
    FILE *fp = fopen(STORAGE_PATH, "rb");
    if (!fp)
        return 0;

    ActiveCCTV tmp;
    while (fread(&tmp, sizeof(tmp), 1, fp) == 1) {
        if (strcmp(tmp.name, name) == 0) {
            // IP 문자열만 복사
            strncpy(out_ip_buf, tmp.ip, buf_sz);
            out_ip_buf[buf_sz-1] = '\0';
            fclose(fp);
            return 1;
        }
    }
    
    fclose(fp);
    return 0;
}
