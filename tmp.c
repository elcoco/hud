
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

//#define RIPGREP_BIN_PATH "/usr/bin/rg bla"
const char *RIPGREP_BIN_PATH = "/usr/bin/rg bla";


int bever()
{
  FILE *FileOpen;
  char line[130];                                 
  FileOpen = popen("rg bla", "r");                                                                            

  while ( fgets( line, sizeof line, FileOpen)) {
    printf("%s", line);
  }

  pclose(FileOpen);
}

int main(int argc, char **argv)
{
    bever();
    FILE *pipe = popen("rg bla", "r");
    //FILE *pipe = popen("rg bla", "r");
    //FILE *pipe = popen(RIPGREP_BIN_PATH, "r");
    //if (pipe == NULL)
    //    die("Failed to run command\n");

    while (!feof(pipe)) {
        char buf[512] = "";
        if (fgets(buf, sizeof(buf), pipe) != NULL) {
            //printf(">> %s\n", buf);
            puts("bevers");
            puts(buf);
        }
    }

    pclose(pipe);
    return 0;
}

