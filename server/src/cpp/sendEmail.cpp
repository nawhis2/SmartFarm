#include "sendEmail.h"
#include "db.h"
#include <curl/curl.h>
#include <iostream>
#include <string>
#include <cstring>

struct UploadStatus {
    const char* data;
    size_t bytes_left;
};

size_t payload_source(char* ptr, size_t size, size_t nmemb, void* userp) {
    UploadStatus* upload = static_cast<UploadStatus*>(userp);
    size_t buffer_size = size * nmemb;

    if (upload->bytes_left == 0)
        return 0;

    size_t to_copy = std::min(buffer_size, upload->bytes_left);
    memcpy(ptr, upload->data, to_copy);
    upload->data += to_copy;
    upload->bytes_left -= to_copy;

    return to_copy;
}

void sendEmail(const std::string& subject, const std::string& message) {
    // 1) DB에서 이메일/비밀번호 조회
    MailUser cfg = check_email_pass();
    if (cfg.email[0] == '\0' || cfg.pass[0] == '\0') {
        std::cerr << "[EmailConfig] No credentials configured\n";
        return;
    }

    // 2) libcurl 초기화
    CURL *curl = curl_easy_init();
    if (!curl) {
        std::cerr << "[Email] curl init failed\n";
        return;
    }

    // 3) From/To/User/Credentials 설정
    std::string from         = cfg.email;
    std::string to           = cfg.email;   // 받는 주소를 다르게 하고 싶으면 여기만 조정
    std::string user         = cfg.email;   // SMTP 로그인 사용자
    std::string app_password = cfg.pass;    // SMTP 앱 비밀번호

    // 4) 메일 본문 준비
    std::string raw_payload =
        "To: "   + to   + "\r\n"
        "From: " + from + "\r\n"
        "Subject: " + subject + "\r\n"
        "Content-Type: text/plain; charset=UTF-8\r\n"
        "\r\n" + message + "\r\n.\r\n";
    UploadStatus upload_ctx = { raw_payload.c_str(), raw_payload.size() };

    // 5) curl 옵션
    curl_easy_setopt(curl, CURLOPT_USERNAME, user.c_str());
    curl_easy_setopt(curl, CURLOPT_PASSWORD, app_password.c_str());
    curl_easy_setopt(curl, CURLOPT_URL, "smtps://smtp.gmail.com:465");
    curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, ("<" + from + ">").c_str());

    struct curl_slist *recipients = nullptr;
    recipients = curl_slist_append(recipients, ("<" + to + ">").c_str());
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

    curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
    curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

    // 6) 전송 실행
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "[메일 전송 실패] " << curl_easy_strerror(res) << std::endl;
    }

    // 7) 정리
    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl);
}