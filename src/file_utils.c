#include "include/file_utils.h"
#include <stdio.h>


int delete_file(const char* filename)
{
    int status;
    status = remove(filename);

    if (status == 0)
        printf("%s file deleted successfully.\n", filename);
    else
    {
        printf("Unable to delete the file\n");
        perror("Following error occurred");
    }

    return 0;
}
