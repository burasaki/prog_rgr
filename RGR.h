#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#pragma pack(push, 1)
typedef struct {
    unsigned short bfType;
    unsigned int   bfSize;
    unsigned short bfReserved1;
    unsigned short bfReserved2;
    unsigned int   bfOffBits;
} BITMAPFILEHEADER;

typedef struct {
    unsigned int   biSize;
    int            biWidth;
    int            biHeight;
    unsigned short biPlanes;
    unsigned short biBitCount;
    unsigned int   biCompression;
    unsigned int   biSizeImage;
    int            biXPelsPerMeter;
    int            biYPelsPerMeter;
    unsigned int   biClrUsed;
    unsigned int   biClrImportant;
} BITMAPINFOHEADER;
#pragma pack(pop)

typedef struct {
    int mode;
    char input_image[256];
    char output_image[256];
    char text_to_hide[1024];
    int use_red;
    int use_green;
    int use_blue;
} confit_t;

void log_info(char* level, char* message, const char* details);
int load_config(const char* filename, confit_t* config);
int is_channel_active(int channel_index, confit_t config);
void encrypt(confit_t config);
void decrypt(confit_t config);