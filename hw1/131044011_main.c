#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <limits.h>


int open_tiff(const char *path);
int get_byte_order(const char *data);
int get_pos(const char *data, int byte_order);
int get_number_of_tags(const char *data, int pos, int byte_order);
void analyze_tiftag(const char *data, int pos, int byte_order, int number_of_tag);
int get_number_of_bits(int type);
void print_data(int val, int byte_order, int width);
int hexa_to_decimal(const char* number, int size, int byte_order);


int main(int argc, char **argv ) {
    if (argc != 2 ) {
        fprintf(stderr, "Usage:. /tiffProcessor your_tif_file.TIF \n");
        return -1;
    }
    open_tiff(argv[1]);
    return 0;
}


int open_tiff(const char *path){
    unsigned char *data;
    int fd = 0;
    int byte_order = 0;
    int pos = 0;
    int number_of_tags = 0;
    struct stat sb;

    if(stat(path, &sb) == -1){
        fprintf(stderr, "File cannot be opened for stats\n");
        return -1;
    }

    fd = open(path, O_RDONLY);
    if(fd < 0){
        fprintf(stderr, "File cannot be opened\n");
        return -1;
    }

    data = (char*) malloc(sizeof(char) * sb.st_size);


    if(read(fd, data, sb.st_size) < 0){
        fprintf(stderr, "File cannot be read\n");
    }

    /*print size*/
    //fprintf(stderr, "File Size : %d bytes\n", sb.st_size);

    /*print byte order*/
    byte_order = get_byte_order(data);

    /*print header data*/
    pos = get_pos(data, byte_order);

    number_of_tags = get_number_of_tags(data, pos, byte_order);

    /*print tif tag*/
    analyze_tiftag(data, pos + 2, byte_order, number_of_tags);

    /*print all data*/
    //print_data(data, sb.st_size);


    /*free heap*/
    free(data);
    /*close file*/
    close(fd);
}

int get_byte_order(const char *data){
    const char order[3] = "MM";

    if(strncmp(order, data, 2) == 0){
        fprintf(stderr, "Byte order: Motorola\n");
        return 0;
    }else{
        fprintf(stderr, "Byte order: Intel\n");
        return 1;
    }
}

int get_pos(const char *data, int byte_order){
    char position[128];
    int pos = 0;
    /*byte order*/
    //fprintf(stderr, "Byte order : %hhx %hhx\n", data[0], data[1]);
    /*version*/
    //fprintf(stderr, "Version : %hhx %hhx\n", data[2], data[3]);
    /*offset of the first ifd*/
    //fprintf(stderr, "first ifd: %hhx %hhx %hhx %hhx\n", data[4], data[5], data[6], data[7]);

    /*calculate it here later*!!!!!!!!!!!!*/
    sprintf(position, "%hhx%hhx%hhx%hhx", data[4], data[5], data[6], data[7]);
    pos = hexa_to_decimal(position, strlen(position), byte_order);
    return pos;
}

int get_number_of_tags(const char *data, int pos, int byte_order){
    char number_of_tags[1024];
    int tags = 0;
    sprintf(number_of_tags, "%hhx%hhx", data[pos], data[pos+1]);
    //fprintf(stderr, "Tags string : %s\n", number_of_tags);
    tags = hexa_to_decimal(number_of_tags, strlen(number_of_tags), byte_order);
    //fprintf(stderr, "Number of Tags: %d\n", tags);
    return tags;
}

void analyze_tiftag(const char *data, int pos, int byte_order, int number_of_tag){
    char size[1024];
    int row = 0;
    int off = 0, ct = 0;
    int number_of_tags_273 = 0, number_of_tags_279 = 0;
    int width = 0, height = 0, tagid = 0;
    int data_type = 0, data_count = 0, data_offset = 0;
    int bits_per_sample = 0, planar = 0, photometric = 0;
    for(int i = 0; i < number_of_tag; ++i){
//        fprintf(stderr, "->=========\nTag %d:\n", i);
//        fprintf(stderr, "->Tagid : %02x %02x\n", data[pos+i*12], data[pos+1+i*12]);
//        fprintf(stderr, "->Datatype : %hhx %hhx\n", data[pos+2+i*12], data[pos+3+i*12]);
//        fprintf(stderr, "->Datacount : %hhx %hhx %hhx %hhx\n", data[pos+4+i*12], data[pos+5+i*12], data[pos+6+i*12], data[pos+7+i*12]);
//        fprintf(stderr, "->Dataoffset : %hhx %hhx %hhx %hhx \n", data[pos+8+i*12], data[pos+9+i*12], data[pos+10+i*12], data[pos+11+i*12]);

        /*data type*/
        sprintf(size, "%hhx%hhx", data[pos+2+i*12], data[pos+3+i*12]);
        data_type = hexa_to_decimal(size, strlen(size), byte_order);

        /*data count*/
        sprintf(size, "%hhx%hhx%hhx%hhx", data[pos+4+i*12], data[pos+5+i*12], data[pos+6+i*12], data[pos+7+i*12]);
        data_count = hexa_to_decimal(size, strlen(size), byte_order);

        /*tag id*/
        sprintf(size, "%02x%02x", data[pos+i*12], data[pos+1+i*12]);
        tagid = hexa_to_decimal(size, strlen(size), byte_order);

        /*dataoffset*/
        sprintf(size, "%02x%02x%02x%02x", data[pos+8+i*12], data[pos+9+i*12], data[pos+10+i*12], data[pos+11+i*12]);
        data_offset = hexa_to_decimal(size, strlen(size), byte_order);
        //fprintf(stderr, " string Offset : %s\n", size);

        if(tagid == 256){
            /*width*/
            sprintf(size, "%hhx", data[pos+9]);
            width = hexa_to_decimal(size , strlen(size), byte_order);
            fprintf(stderr, "Width : %d pixels\n", width);
        }else if(tagid == 257){
            /*height*/
            sprintf(size, "%hhx", data[pos+12+9]);
            height = hexa_to_decimal(size , strlen(size), byte_order);
            fprintf(stderr, "Height : %d pixels\n", height);
        }else if(tagid == 258){
            /*bits per sample*/
            //fprintf(stderr, "Data type : %d\n", data_type);
            bits_per_sample = get_number_of_bits(data_type);
        }else if(tagid == 262){
            //photometric = data_offset;
            photometric = 1;
        }else if(tagid == 273){
            /*stripoffset*/
//            fprintf(stderr, "Data Count : %d\n", data_count);
//            fprintf(stderr, "Data offset : %d\n", data_offset);
            bits_per_sample = data_type;
            off= data_offset;
            number_of_tags_273 = data_count;
            //fprintf(stderr, "strip : %d\n", off);
        }else if(tagid == 279){
            ct = data_offset;
            number_of_tags_279 = data_count;
            //fprintf(stderr, "split : %d\n", ct);
        }else if(tagid == 282){
           // fprintf(stderr, "Offset : %d\n", data_offset);
        }else if(tagid == 283){
            //fprintf(stderr, "Offset : %d\n", data_offset);
        }else if(tagid == 284){
            planar = bits_per_sample;
        }
    }

    if(width >=8 && width % 8 != 0){
        row = width / 8 + 1;
    }else{
        row = width / 8;
    }


    for(int i = 0; i < number_of_tags_273 && i < number_of_tags_279; ++i){
        for(int j = 0; j * row < ct; ++j) {
            char pixel_value[1024];
            pixel_value[0] = '\0';
            for (int k = 0; k < row; ++k) {
                sprintf(pixel_value, "%s%hhx", pixel_value, data[k + off + j * row]);
            }
            if(strlen(pixel_value) != row * 2){
                pixel_value[0] = '\0';
                for (int k = 0; k < row; ++k) {
                    sprintf(pixel_value, "%s%02x", pixel_value, data[k + off + j * row]);
                }
            }
            //fprintf(stderr, "val value = %s\n", pixel_value);
            int val = hexa_to_decimal(pixel_value, strlen(pixel_value), byte_order);
            //fprintf(stderr, "data index = %d off = %d val = %u strlen = %d\n", off + j * row, off, val, strlen(pixel_value));
            print_data(val, byte_order, width);
        }
    }
}


int hexa_to_decimal(const char* number, int size, int byte_order){
    int n = 1, result = 0;
    if(byte_order == 0){
        for(int i = size-1; i >= 0; --i){
            if(number[i] == 'a'){
                result += 10 * n;
            }else if(number[i] == 'b'){
                result += 11 * n;
            }else if(number[i] == 'c'){
                result += 12 * n;
            }else if(number[i] == 'd'){
                result += 13 * n;
            }else if(number[i] == 'e'){
                result += 14 * n;
            }else if(number[i] == 'f'){
                result += 15 * n;
            }else{
                result += (number[i] - '0') * n;
            }
            n *= 16;
        }
    }else{
        for(int i = 0; i < size; ++i){
            if(number[i] == 'a'){
                result += 10 * n;
            }else if(number[i] == 'b'){
                result += 11 * n;
            }else if(number[i] == 'c'){
                result += 12 * n;
            }else if(number[i] == 'd'){
                result += 13 * n;
            }else if(number[i] == 'e'){
                result += 14 * n;
            }else if(number[i] == 'f'){
                result += 15 * n;
            }else{
                result += (number[i] - '0') * n;
            }
            n *= 16;
        }
    }

    return result;
}

/*https://www.fileformat.info/format/tiff/egff.htm#TIFF.FO*/
int get_number_of_bits(int type){
    if(type == 1 || type == 2){
        return 8;
    }else if(type == 3){
        return 16;
    }else if(type == 4){
        return 32;
    }else if(type == 5){
        return 64;
    }
}

void print_data(int val, int byte_order, int width){
    int arr[32];
    for(int i = 0; i < 32; ++i){
        if (byte_order == 1)
            arr[i] = (val >> i) & 1;
        else {
            arr[31 - i] = (val >> i) & 1;
        }
    }
    for(int k = 0; k < width; ++k){
        if (arr[k] != 0 ){
            fprintf(stderr, "%d ", 1);
        } else {
            fprintf(stderr, "%d ", 0);
        }
    }
  
    fprintf(stderr, "\n");
}
