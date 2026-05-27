#include "RGR.h"

int main() {
    log_info("INFO", "Запуск программы", "");

    confit_t config;
    if (!load_config("../config.txt", &config)) {
        log_info("ERROR", "Ошибка структуры конфига или файл config.txt не найден", "");
        return 1;
    }

    if (!config.use_red && !config.use_green && !config.use_blue) {
        log_info("ERROR", "Ни один канал не выбран для работы", "");
        return 1;
    }

    if (config.mode == 1) {
        encrypt(config);
    } else if (config.mode == 2) {
        decrypt(config);
    } else {
        log_info("ERROR", "Неверный режим mode. Ожидается 1 или 2", "");
    }

    log_info("INFO", "Программа завершила работу", "");
    return 0;
}