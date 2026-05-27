#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include <cmocka.h>

#include "../RGR.h"

void create_dummy_bmp(const char* filename, int width, int height) {
    FILE *f = fopen(filename, "wb");
    assert_non_null(f);

    BITMAPFILEHEADER bfh = {0};
    bfh.bfType = 0x4D42;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bfh.bfSize = bfh.bfOffBits + (width * height * 3);

    BITMAPINFOHEADER bih = {0};
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = width;
    bih.biHeight = height;
    bih.biPlanes = 1;
    bih.biBitCount = 24;

    fwrite(&bfh, sizeof(BITMAPFILEHEADER), 1, f);
    fwrite(&bih, sizeof(BITMAPINFOHEADER), 1, f);

    char zero_pixel[3] = {0, 0, 0};
    for (int i = 0; i < width * height; i++) {
        fwrite(zero_pixel, 1, 3, f);
    }
    fclose(f);
}

static void test_load_config_success(void **state) {
    FILE *f = fopen("test_config.txt", "w");
    assert_non_null(f);
    fprintf(f, "mode = 1\n"
               "input_image = input.bmp\n"
               "encrypt_image = output.bmp\n"
               "text_to_hide = Hello\n"
               "use_red = 1\n"
               "use_green = 0\n"
               "use_blue = 1");
    fclose(f);

    confit_t config = {0};
    int result = load_config("test_config.txt", &config);

    assert_int_equal(result, 1);
    assert_int_equal(config.mode, 1);
    assert_string_equal(config.input_image, "input.bmp");
    assert_string_equal(config.output_image, "output.bmp");
    assert_string_equal(config.text_to_hide, "Hello");
    assert_int_equal(config.use_red, 1);
    assert_int_equal(config.use_green, 0);
    assert_int_equal(config.use_blue, 1);

    remove("test_config.txt");
}

static void test_load_config_invalid_format(void **state) {
    FILE *f = fopen("bad_config.txt", "w");
    assert_non_null(f);
    fprintf(f, "mode = NotANumber\ninput_image = img.bmp\n");
    fclose(f);

    confit_t config = {0};
    int result = load_config("bad_config.txt", &config);

    assert_int_equal(result, 0);
    remove("bad_config.txt");
}

static void test_load_config_missing_file(void **state) {
    confit_t config = {0};
    int result = load_config("non_existent_config_file.txt", &config);
    assert_int_equal(result, 0);
}


static void test_is_channel_active_combinations(void **state) {
    confit_t config = {0};
        config.use_blue = 1; config.use_green = 0; config.use_red = 0;
    assert_int_equal(is_channel_active(0, config), 1);
    assert_int_equal(is_channel_active(1, config), 0);
    assert_int_equal(is_channel_active(2, config), 0);

    config.use_blue = 0; config.use_green = 1; config.use_red = 0;
    assert_int_equal(is_channel_active(0, config), 0);
    assert_int_equal(is_channel_active(1, config), 1);
    assert_int_equal(is_channel_active(2, config), 0);

    assert_int_equal(is_channel_active(5, config), 0);
    assert_int_equal(is_channel_active(-1, config), 0);
}


static void test_encrypt_overflow_protection(void **state) {
    create_dummy_bmp("small.bmp", 2, 2);

    confit_t config = {0};
    config.mode = 1;
    strcpy(config.input_image, "small.bmp");
    strcpy(config.output_image, "small_out.bmp");
    strcpy(config.text_to_hide, "BBBBBBBBBBBBBBBBBBiiiiiiiiiiiiiiiigggggggggggggg Text");
    config.use_red = 1;
    config.use_green = 1;
    config.use_blue = 1;

    encrypt(config);

    FILE *log_f = fopen("log_infos.log_info", "r");
    assert_non_null(log_f);
    
    char line[256];
    int error_found = 0;
    while (fgets(line, sizeof(line), log_f)) {
        if (strstr(line, "Переполнение!") != NULL) {
            error_found = 1;
            break;
        }
    }
    fclose(log_f);
    assert_int_equal(error_found, 1);

    remove("small.bmp");
    remove("small_out.bmp");
    remove("log_infos.log_info");
}

static void test_encrypt_missing_input_image(void **state) {
    confit_t config = {0};
    strcpy(config.input_image, "this_file_does_not_exist.bmp");
    strcpy(config.output_image, "should_not_be_created.bmp");
    strcpy(config.text_to_hide, "Test");
    config.use_red = 1;

    encrypt(config);

    FILE *f = fopen("should_not_be_created.bmp", "rb");
    assert_null(f);
    
    remove("log_infos.log_info");
}


static void test_steganography_single_channel(void **state) {
    create_dummy_bmp("integ_in.bmp", 20, 20);

    confit_t config = {0};
    strcpy(config.input_image, "integ_in.bmp");
    strcpy(config.output_image, "integ_out.bmp");
    strcpy(config.text_to_hide, "Secret123");
    config.use_red = 0;
    config.use_green = 1;
    config.use_blue = 0;

    encrypt(config);
    decrypt(config);

    FILE *log_f = fopen("log_infos.log_info", "r");
    assert_non_null(log_f);
    
    char line[256];
    int text_found = 0;
    while (fgets(line, sizeof(line), log_f)) {
        if (strstr(line, "Secret123") != NULL) {
            text_found = 1;
            break;
        }
    }
    fclose(log_f);
    assert_int_equal(text_found, 1);

    remove("integ_in.bmp");
    remove("integ_out.bmp");
    remove("log_infos.log_info");
}

static void test_encrypt_empty_string(void **state) {
    create_dummy_bmp("empty_test.bmp", 5, 5);

    confit_t config = {0};
    strcpy(config.input_image, "empty_test.bmp");
    strcpy(config.output_image, "empty_test_out.bmp");
    strcpy(config.text_to_hide, "");
    config.use_red = 1;
    config.use_green = 1;
    config.use_blue = 1;

    encrypt(config);
    decrypt(config);

    FILE *log_f = fopen("log_infos.log_info", "r");
    assert_non_null(log_f);
    
    char line[256];
    int empty_decoded_logged = 0;
    while (fgets(line, sizeof(line), log_f)) {
        if (strstr(line, "Результат дешифрования:") != NULL) {
            empty_decoded_logged = 1;
            break;
        }
    }
    fclose(log_f);
    assert_int_equal(empty_decoded_logged, 1);

    remove("empty_test.bmp");
    remove("empty_test_out.bmp");
    remove("log_infos.log_info");
}


int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_load_config_success),
        cmocka_unit_test(test_load_config_invalid_format),
        cmocka_unit_test(test_load_config_missing_file),
        cmocka_unit_test(test_is_channel_active_combinations),
        cmocka_unit_test(test_encrypt_overflow_protection),
        cmocka_unit_test(test_encrypt_missing_input_image),
        cmocka_unit_test(test_steganography_single_channel),
        cmocka_unit_test(test_encrypt_empty_string),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}