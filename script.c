#include "RGR.h"


void log_info(char* level, char* message, const char* details) {
    FILE* log_infoFile = fopen("log_infos.log_info", "a");
    
    time_t raw_time;
    time(&raw_time);
    struct tm* t = localtime(&raw_time);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);

    fprintf(log_infoFile, "[%s] [%s] %s %s\n", time_str, level, message, details);
    printf("%s %s\n", message, details);

    fclose(log_infoFile);
}

int load_config(const char* filename, confit_t* config) {
    FILE* f = fopen(filename, "r");
    if (!f) return 0;

    int read_count = fscanf(f,
        "mode = %d\n"
        "input_image = %255s\n"
        "encrypt_image = %255s\n"
        "text_to_hide = %1023[^\n]\n"
        "use_red = %d\n"
        "use_green = %d\n"
        "use_blue = %d",
        &config->mode,
        config->input_image,
        config->output_image,
        config->text_to_hide,
        &config->use_red,
        &config->use_green,
        &config->use_blue
    );

    fclose(f);
    return (read_count == 7);
}

int is_channel_active(int channel_index, confit_t config) {
    if (channel_index == 0 && config.use_blue)  return 1;
    if (channel_index == 1 && config.use_green) return 1;
    if (channel_index == 2 && config.use_red)   return 1;
    return 0;
}

void encrypt(confit_t config) {
    log_info("INFO", "Старт шифрования", "");
    
    FILE *inFile = fopen(config.input_image, "rb");
    if (!inFile) {
        log_info("ERROR", "Не удалось открыть картинку", config.input_image);
        return;
    }

    BITMAPFILEHEADER file_header;
    BITMAPINFOHEADER info_header;

    fread(&file_header, sizeof(BITMAPFILEHEADER), 1, inFile);
    fread(&info_header, sizeof(BITMAPINFOHEADER), 1, inFile);

    
    int remaing_color_size = file_header.bfSize - file_header.bfOffBits;
    unsigned char *pixels_buffer = malloc(remaing_color_size);
       if (!pixels_buffer) {
        log_info("ERROR", "Не удалось выделить память для буфера файла", "");
        fclose(inFile);
        return;
    }

    fread(pixels_buffer, 1, remaing_color_size, inFile);
    fclose(inFile);

    int len_text = strlen(config.text_to_hide) + 1;
    int required_bits = len_text * 8;



    int total_pixels = info_header.biWidth * info_header.biHeight;
    int active_channels = config.use_red + config.use_green + config.use_blue;
    int available_bytes = total_pixels * active_channels;

    if (required_bits > available_bytes) {
        log_info("ERROR", "Переполнение! Текст слишком большой для выбранных каналов", "");
        free(pixels_buffer);
        return;
    }



    int text_index = 0;
    int bit_index = 0;
    int pixel_counter = 0;

    for (int i = 0; i < remaing_color_size; i++) {
        int current_channel = pixel_counter % 3;

        if (text_index < len_text && is_channel_active(current_channel, config)) {
            int current_bit = (config.text_to_hide[text_index] >> (7 - bit_index)) & 1;
            pixels_buffer[i] = (pixels_buffer[i] & 0xFE) | current_bit;

            bit_index++;
            if (bit_index == 8) {
                bit_index = 0;
                text_index++;
            }
        }
        pixel_counter++;
    }

    FILE *outFile = fopen(config.output_image, "wb");
    if (!outFile) {
        log_info("ERROR", "Не удалось создать файл", config.output_image);
        fclose(inFile);
        return;
    }

    fwrite(&file_header, sizeof(BITMAPFILEHEADER), 1, outFile);
    fwrite(&info_header, sizeof(BITMAPINFOHEADER), 1, outFile);
    fwrite(pixels_buffer, 1, remaing_color_size, outFile);


    log_info("INFO", "Шифрование успешно завершено. Файл сохранен", config.output_image);
    fclose(outFile);
}

void decrypt(confit_t config) {
    log_info("INFO", "Старт дешифрования", "");

    FILE *inFile = fopen(config.output_image, "rb");
    if (!inFile) {
        log_info("ERROR", "Не удалось открыть файл", config.input_image);
        return;
    }

    BITMAPFILEHEADER file_header;
    BITMAPINFOHEADER info_header;

    fread(&file_header, sizeof(BITMAPFILEHEADER), 1, inFile);
    fread(&info_header, sizeof(BITMAPINFOHEADER), 1, inFile);

    fseek(inFile, file_header.bfOffBits, SEEK_SET);

    char current_byte;
    char decoded_char = 0;
    int bit_index = 0;
    int pixel_counter = 0;
    
    char result_text[1024] = {0};
    int result_index = 0;

    while (fread(&current_byte, 1, 1, inFile) && result_index < 1023) {
        int current_channel = pixel_counter % 3;

        if (is_channel_active(current_channel, config)) {
            int bit = current_byte & 1;
            decoded_char = (decoded_char << 1) | bit;
            bit_index++;

            if (bit_index == 8) {
                if (decoded_char == '\0') break;
                result_text[result_index++] = decoded_char;
                decoded_char = 0;
                bit_index = 0;
            }
        }
        pixel_counter++;
    }

    log_info("INFO", "Результат дешифрования:", result_text);
    fclose(inFile);
}