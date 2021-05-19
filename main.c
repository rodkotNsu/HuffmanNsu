 #define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODE_ARCHIVING 'c'
#define MODE_UNPACKING 'd'
#define MAX_BLOCK_READ 180
int pow2[8] = {1, 2, 4, 8, 16, 32, 64, 128};

typedef unsigned char byte;

struct SymbolInfo {
    unsigned char symbol;
    int count;
};
struct Tree {
    struct Tree *left;
    struct Tree *right;
    char symbol;
};
struct ArrSymbol {
    struct Tree *list;
    int order;
};
struct CodeOfByte {
    char *code;
    int size;
};

void build_frequency_table(FILE *F_in, struct SymbolInfo pInfo[256], long *count_different, long *len_file);

void free_tree(struct Tree *pList);

int comp_sym_info(const struct SymbolInfo *a, const struct SymbolInfo *b);

int comp_arr_sym(const struct ArrSymbol *a, const struct ArrSymbol *b);

int building_huffman_code(struct Tree *root, struct CodeOfByte *arr_value, char *value, int deap);

struct Tree *building_huffman_tree(struct SymbolInfo pInfo[256], long different);

void formation_representation_tree(struct Tree *huffman_tree, char *buffer, int *len);

void archiving_file(FILE *F_in, FILE *F_out);

int restoration_tree(FILE *F_in, struct Tree *root, int *len_bug, char *bug);

int unpacking_archive(FILE *F_in, FILE *F_out);

void free_tree(struct Tree *pList);

void build_frequency_table(FILE *F_in, struct SymbolInfo pInfo[256], long *count_different, long *len_file);


int comp_sym_info(const struct SymbolInfo *a, const struct SymbolInfo *b) {
    if ((b->count) - (a->count) == 0)
        return b->symbol - a->symbol;
    else
        return (b->count) - (a->count);
}

int comp_arr_sym(const struct ArrSymbol *a, const struct ArrSymbol *b) {
    return (b->order) - (a->order);
}

int building_huffman_code(struct Tree *root, struct CodeOfByte *arr_value, char *value, int deap) {
    if (root == NULL || arr_value == NULL) {
        return 1;
    }
    if (root->right == NULL && root->left == NULL) {
        arr_value[(byte) root->symbol].code = calloc(deap, sizeof(char));
        if (arr_value[(byte) root->symbol].code == NULL)
            return 1;
        for (int i = 0; i < deap; i++)
            arr_value[(byte) root->symbol].code[i] = value[i];
        arr_value[(byte) root->symbol].size = deap;
    }

    if (root->right != NULL) {
        value[deap] = 1;
        building_huffman_code(root->right, arr_value, value, deap + 1);
    }

    if (root->left != NULL) {
        value[deap] = 0;
        building_huffman_code(root->left, arr_value, value, deap + 1);
    }
    return 0;
}

struct Tree *building_huffman_tree(struct SymbolInfo pInfo[256], long different) {
    qsort(pInfo, 256, sizeof(struct SymbolInfo), (int (*)(const void *, const void *)) comp_sym_info);
    struct ArrSymbol *arr = calloc(different, sizeof(struct ArrSymbol));
    if (arr == NULL)
        return NULL;
    for (long i = 0; i < different; i++) {
        arr[i].order = pInfo[i].count;
        struct Tree new_list = {.symbol = pInfo[i].symbol, .left = NULL, .right = NULL};
        arr[i].list = malloc(sizeof(struct Tree));
        if (arr[i].list != NULL)
            *(arr[i].list) = new_list;

    }
    long diff = different;
    while (diff != 1) {
        struct ArrSymbol *first_sym = &arr[diff - 2];
        struct ArrSymbol *second_sym = &arr[diff - 1];
        first_sym->order += second_sym->order;
        struct Tree new_list = {.symbol = 0, .left = first_sym->list, .right = second_sym->list};
        first_sym->list = malloc(sizeof(struct Tree));
        if (first_sym->list != NULL)
            *first_sym->list = new_list;

        second_sym->order = 0;
        qsort(arr, diff, sizeof(struct ArrSymbol), (int (*)(const void *, const void *)) comp_arr_sym);
        diff--;
    }


    return arr->list;
}

void formation_representation_tree(struct Tree *huffman_tree, char *buffer, int *len) {
    if (huffman_tree->right == NULL && huffman_tree->left == NULL) {
        (buffer)[*len] = '\0';
        (*len)++;
        (buffer)[*len] = (char) huffman_tree->symbol;
        (*len)++;
    } else {
        (buffer)[*len] = 1;
        (*len)++;
        if (huffman_tree->left != NULL) {

            formation_representation_tree(huffman_tree->left, buffer, len);
        }
        if (huffman_tree->right != NULL) {

            formation_representation_tree(huffman_tree->right, buffer, len);
        }
    }

}

void archiving_file(FILE *F_in, FILE *F_out) {
    long len_file = 0;
    long count_different = 0;

    struct SymbolInfo full = {.symbol = 0, .count = 0};
    struct SymbolInfo frequency_table[256] = {full};

    struct CodeOfByte huffman_codes[256];
    char buffer[800] = {'\0'};

    struct Tree *huffman_tree = NULL;

    long pos = ftell(F_in);
    build_frequency_table(F_in, frequency_table, &count_different, &len_file);
    fseek(F_in, pos, SEEK_SET);

    if (count_different == 0) {
        return;
    }

    huffman_tree = building_huffman_tree(frequency_table, count_different);

    if (count_different > 1)
        building_huffman_code(huffman_tree, huffman_codes, buffer, 0);
    else
        building_huffman_code(huffman_tree, huffman_codes, buffer, 1);

    fseek(F_out, 1, SEEK_SET);

    int len_buffer_for_tree = 0;
    formation_representation_tree(huffman_tree, buffer, &len_buffer_for_tree);

    //Запись образа дерева в  F_out
    byte buffer_byte = 0;
    int len_buffer = 0;
    int flag = 0;
    for (int i = 0; i < len_buffer_for_tree; i++) {
        if (flag == 0) {
            if (buffer[i] == 0) {
                len_buffer++;
                flag = 1;
            } else if (buffer[i] == 1) {
                buffer_byte += pow2[7 - len_buffer];
                len_buffer++;
            } else {
                buffer_byte += (byte) buffer[i] >> len_buffer;
                fwrite(&buffer_byte, sizeof(byte), 1, F_out);
                buffer_byte = buffer[i] << (8 - len_buffer);
                len_buffer = 8 - len_buffer;
            }
            if (len_buffer == 8) {
                fwrite(&buffer_byte, sizeof(byte), 1, F_out);
                buffer_byte = 0;
                len_buffer = 0;
            }
        } else {
            flag = 0;
            buffer_byte += (byte) buffer[i] >> len_buffer;
            fwrite(&buffer_byte, sizeof(byte), 1, F_out);
            buffer_byte = buffer[i] << (8 - len_buffer);
        }
    }
    if (len_buffer != 0)
        fwrite(&buffer_byte, sizeof(byte), 1, F_out);

    //Кодирование байтов в  F_out
    buffer_byte = '\0';
    len_buffer = 0;

    char read;
    while (fread(&read, sizeof(byte), 1, F_in)) {
        int len_code = huffman_codes[(byte) read].size;
        char *code_s = huffman_codes[(byte) read].code;

        int i = 0;
        while (len_code != i) {
            buffer_byte += code_s[i] * (pow2[7 - len_buffer]);

            len_buffer++;
            if (len_buffer == 8) {
                fwrite(&buffer_byte, sizeof(byte), 1, F_out);
                buffer_byte = '\0';
                len_buffer = 0;
            }
            i++;
        }
    }

    if (len_buffer != 0) {
        fwrite(&buffer_byte, sizeof(byte), 1, F_out);
        fseek(F_out, 0, SEEK_SET);
        byte g = (byte) (8 - len_buffer);
        fwrite(&g, sizeof(byte), 1, F_out);
    }

    free_tree(huffman_tree);
}

int restoration_tree(FILE *F_in, struct Tree *root, int *len_bug, char *bug) {
    if (*len_bug == 8) {
        if (!fread(bug, sizeof(char), 1, F_in)) {
            return 1;
        }
        *len_bug = 0;
    }
    if (((byte) (*bug) >> (7 - (*len_bug)++)) % 2 == 0) {
        root->symbol = (byte) (*bug) << *len_bug;
        if (!fread(bug, sizeof(char), 1, F_in)) {
            return 1;
        }
        root->symbol += (byte) (*bug) >> (8 - *len_bug);

    } else {
        root->symbol = 0;
        root->left = calloc(1, sizeof(struct Tree));
        restoration_tree(F_in, root->left, len_bug, bug);

        root->right = calloc(1, sizeof(struct Tree));
        restoration_tree(F_in, root->right, len_bug, bug);
    }
    return 0;
}

int unpacking_archive(FILE *F_in, FILE *F_out) {
    byte len = 0;

    if (!fread(&len, sizeof(byte), 1, F_in)) {
        return 1;
    }

    struct Tree *huffman_tree = calloc(1, sizeof(struct Tree));
    int len_buffer_tree = 8;
    char buffer_tree = 0;
    restoration_tree(F_in, huffman_tree, &len_buffer_tree, &buffer_tree);

    struct Tree *action = huffman_tree;

    int count_symbols = 0;
    char temp;
    while (fread(&temp, sizeof(char), 1, F_in)) {
        int act = 0;
        if (feof(F_in))
            break;
        char flag_eof = 0;
        char l;
        long j = ftell(F_in);
        if (!fread(&l, 1, 1, F_in) || feof(F_in)) {
            flag_eof = 1;
        };

        fseek(F_in, j, SEEK_SET);

        while (act != 8) {

            if (act == (8 - len) && flag_eof == 1)
                break;


            if ((byte) (temp << act) / 128 == 0) {
                if (action->left != NULL)
                    action = action->left;
            } else {
                if (action->right != NULL)
                    action = action->right;
            }

            if (action->left == NULL && action->right == NULL) {
                fwrite(&action->symbol, sizeof(byte), 1, F_out);

                action = huffman_tree;
                count_symbols++;
            }
            act++;
        }


    }
    free_tree(huffman_tree);
    return 0;
}

void free_tree(struct Tree *pList) {
    if (pList->right != NULL)
        free_tree(pList->right);
    if (pList->left != NULL)
        free_tree(pList->left);
    free(pList);
}

void build_frequency_table(FILE *F_in, struct SymbolInfo pInfo[256], long *count_different, long *len_file) {

    byte temp[MAX_BLOCK_READ];

    while (!feof(F_in)) {
        size_t len = fread(temp, sizeof(byte), MAX_BLOCK_READ, F_in);
        *len_file += (int) len;

        for (size_t i = 0; i < len; i++) {
            if (pInfo[temp[i]].count == 0) {
                (*count_different)++;
                pInfo[temp[i]].symbol = temp[i];
            }
            pInfo[temp[i]].count++;
        }
    }
}

int main(int argc, char *argv[]) {
    FILE *FILE_IN = NULL, *FILE_OUT = NULL;
    char *FILE_NAME_IN = "in.txt";
    char *FILE_NAME_OUT = "out.txt";

    char mode_archiving_file = -2;

    switch (argc) {
        case 4: {
            char *temp_mode = argv[1];
            FILE_NAME_IN = argv[2];
            FILE_NAME_OUT = argv[3];

            if (strlen(temp_mode) != 2)
                return 0;
            if (temp_mode[0] != '-')
                return 0;
            mode_archiving_file = temp_mode[1];

            FILE_IN = fopen(FILE_NAME_IN, "rb");
            FILE_OUT = fopen(FILE_NAME_OUT, "wb");

            if (FILE_IN == NULL || FILE_OUT == NULL) {

                if (FILE_IN != NULL)
                    fclose(FILE_IN);
                if (FILE_OUT != NULL)
                    fclose(FILE_OUT);
                return 0;
            }

        }
            break;
        case 1: {

            FILE_IN = fopen(FILE_NAME_IN, "rb");
            FILE_OUT = fopen(FILE_NAME_OUT, "wb");

            if (FILE_IN == NULL || FILE_OUT == NULL) {

                if (FILE_IN != NULL)
                    fclose(FILE_IN);
                if (FILE_OUT != NULL)
                    fclose(FILE_OUT);
                return 0;
            }

            int temp1 = fgetc(FILE_IN);
            int n = fgetc(FILE_IN);
            int n2 = fgetc(FILE_IN);
            if (temp1 != EOF && n != EOF && n2 != EOF && (char) n == '\r' && (char) n2 == '\n')
                mode_archiving_file = (char) temp1;

        }
            break;

        default:
            mode_archiving_file = -2;

    }
    if (mode_archiving_file == MODE_ARCHIVING) {
        archiving_file(FILE_IN, FILE_OUT);
        fclose(FILE_IN);
        fclose(FILE_OUT);
    }
    if (mode_archiving_file == MODE_UNPACKING) {
        unpacking_archive(FILE_IN, FILE_OUT);
        fclose(FILE_IN);
        fclose(FILE_OUT);
    }

    return 0;
}

