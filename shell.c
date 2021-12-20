
#include <stdio.h>
#include "filesystem.h"

int shell()
{
    char command[20], arg1[50], arg2[50];
    char temp;

    help();

    while (1)
    {
        fflush(stdout);
        printf("$ ");
        scanf("%s", command);

        if (strcmp(command, "new") == 0) {
            scanf("%s", arg1);
            scanf("%s", arg2);
            create_disc(arg1, atoi(arg2));
        }
        else if (strcmp(command, "delete") == 0) {
            scanf("%s", arg1);
            delete_disc(arg1);
        }
        else if (strcmp(command, "remove") == 0) {
            scanf("%s", arg1);
            scanf("%s", arg2);
            delete_file(arg1, arg2);
        }
        else if (strcmp(command, "ls") == 0) {
            scanf("%s", arg1);
            ls(arg1);
        }
        else if (strcmp(command, "info") == 0) {
            scanf("%s", arg1);
            info(arg1);
        }
        else if (strcmp(command, "upload") == 0) {
            scanf("%s", arg1);
            scanf("%s", arg2);
            upload(arg1, arg2);
        }
        else if (strcmp(command, "download") == 0) {
            scanf("%s", arg1);
            scanf("%s", arg2);
            download(arg1, arg2);
        }
        else if (strcmp(command, "help") == 0) {
            help();
        }
        else if (strcmp(command, "quit") == 0) {
            return 0;
        }
        else {
            printf("Command wasn't recognized. Type \"help\" for more information\n");
        }
        
        while ((temp = getchar()) != '\n' && temp != EOF) { }
    }
}
