#include <stdio.h>

int global_var = 0x7f7f7f7f;

int main(int argc, char *argv[]) {
    const char *str = "there are some text for testing";
    puts(str);

    char nums[] = {'1', '2', '3', '4', '5', '6', '7', '8'};
    for (int i = 0; i < 8; i++) {
        printf("%c", nums[i]);
    }

    return 0;
}