#include <iostream>
#include <cstdio>
#include <cstdlib>

int main() {
    FILE* pipe = popen("python ./hand_side.py", "r");
    if (!pipe) {
        std::cerr << "Error opening pipe." << std::endl;
        return 1;
    }

    int value = 123;
    char ch = '0';
    while(ch != '#')
        fscanf(pipe, "%c", &ch);
    fscanf(pipe, "%c", &ch);
    printf("Please input hand 0\n");
    while(value != 0) {
        fscanf(pipe, "%d", &value);
        printf("value = %d\n", value);
    }
    printf("Please input hand 1\n");
    while(value != 1)
        fscanf(pipe, "%d", &value);
    printf("Please input hand 2\n");
    while(value != 2)
        fscanf(pipe, "%d", &value);
    printf("Please input hand 3\n");
    while(value != 3)
        fscanf(pipe, "%d", &value);
    printf("Please input hand 4\n");
    while(value != 4)
        fscanf(pipe, "%d", &value);
    printf("Input finished!\n");

    fclose(pipe);
    return 0;
}
