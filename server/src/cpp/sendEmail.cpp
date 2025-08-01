#include "db.h"
#include <curl/curl.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
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

// Base64 인코딩 함수
std::string base64_encode(const std::vector<unsigned char>& input) {
    static const char* table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string output;
    size_t i = 0;
    while (i < input.size()) {
        int val = 0, valb = -6;
        for (int j = 0; j < 3; ++j) {
            val <<= 8;
            if (i < input.size()) val |= input[i++];
            else val |= 0;
            valb += 8;
        }
        for (int j = 0; j < 4; ++j) {
            if (valb >= 0)
                output.push_back(table[(val >> valb) & 0x3F]);
            else
                output.push_back('=');
            valb -= 6;
        }
    }
    return output;
}

void sendEmail(const std::string& subject,
               const std::string& message,
               const std::string& imagePath) {
    // 1) DB에서 이메일/비밀번호 조회
    MailUser cfg = check_email_pass();
    if (cfg.email[0] == '\0' || cfg.pass[0] == '\0') {
        std::cerr << "[EmailConfig] No credentials configured\n";
        return;
    }

    std::string from = cfg.email;
    std::string to   = cfg.email;
    std::string user = cfg.email;
    std::string pass = cfg.pass;

    // 2) 이미지 파일 읽고 base64 인코딩
    std::ifstream file(imagePath, std::ios::binary);
    if (!file) {
        std::cerr << "[Image] Failed to open file: " << imagePath << std::endl;
        return;
    }
    std::vector<unsigned char> buffer((std::istreambuf_iterator<char>(file)), {});
    std::string base64Image = base64_encode(buffer);

    // 3) MIME 메시지 구성
    std::string boundary = "BOUNDARY_STRING";
    std::ostringstream payload;

    payload << "Content-Type: multipart/related; boundary=\"" << boundary << "\"\r\n";
    payload << "\r\n--" << boundary << "\r\n";
    payload << "Content-Type: text/html; charset=\"UTF-8\"\r\n\r\n";
    payload << "<html><body>";
    payload << "<h3>" << subject << "</h3>";
    payload << "<p>" << message << "</p>";
    payload << "<img src=\"cid:image1\">";
    payload << "</body></html>\r\n";

    payload << "--" << boundary << "\r\n";
    payload << "Content-Type: image/jpeg\r\n";
    payload << "Content-Transfer-Encoding: base64\r\n";
    payload << "Content-ID: <image1>\r\n";
    payload << "Content-Disposition: inline; filename=\"intrusion.jpg\"\r\n\r\n";
    payload << base64Image << "\r\n";
    payload << "--" << boundary << "--\r\n";

    std::string payloadStr = payload.str();
    UploadStatus upload_ctx = { payloadStr.c_str(), payloadStr.size() };

    // 4) libcurl 설정 및 전송
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "[Email] curl init failed\n";
        return;
    }

    curl_easy_setopt(curl, CURLOPT_USERNAME, user.c_str());
    curl_easy_setopt(curl, CURLOPT_PASSWORD, pass.c_str());
    curl_easy_setopt(curl, CURLOPT_URL, "smtps://smtp.gmail.com:465");
    curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, ("<" + from + ">").c_str());

    struct curl_slist* recipients = nullptr;
    recipients = curl_slist_append(recipients, ("<" + to + ">").c_str());
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

    curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
    curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
        std::cerr << "[메일 전송 실패] " << curl_easy_strerror(res) << std::endl;

    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl);
}
