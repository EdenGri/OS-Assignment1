#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  char* old_path;
  char* new_path;

  if(argc < 3 || argc > 4)
  {
    printf("Usage: ln old new, or, ls -l old new\n");
    exit(0);
  }
  if(strcmp(argv[1], "-s"))
  {
    if(link(argv[1], argv[2]) < 0)
      printf("link %s %s: failed\n", argv[1], argv[2]);
    exit(0);
  }
  old_path = argv[2];
  new_path = argv[3];
  if(symlink(old_path, new_path))
  {
    printf("symlink %s %s: failed\n", old_path, new_path);
  }
  exit(0);
}
