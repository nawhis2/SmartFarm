#include "emailManage.h"
#include <chrono>
#include "sendEmail.h"
using namespace std::chrono;

static system_clock::time_point last_intrusion_time = system_clock::now() - minutes(11);
static system_clock::time_point last_fire_time = system_clock::now() - minutes(11);

void onIntrusionDetected() {
    auto now = system_clock::now();
    if (duration_cast<minutes>(now - last_intrusion_time).count() >= 10) {
        sendEmail("침입 감지", "침입이 감지되었습니다. 확인하세요.");
        last_intrusion_time = now;
    }
}

void onFireDetected() {
    auto now = system_clock::now();
    if (duration_cast<minutes>(now - last_fire_time).count() >= 10) {
        sendEmail("화재 감지", "화재가 발생했습니다. 확인하세요.");
        last_fire_time = now;
    }
}
