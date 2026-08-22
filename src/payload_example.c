#include <stdio.h>
#include <string.h>

int main()
{
    FILE *file = fopen("hacked_test.txt", "w");
    if (file == NULL)
    {
        return 1;
    }

    fprintf(file, "You have been hacked\n");
    fclose(file);

    return 0;
}