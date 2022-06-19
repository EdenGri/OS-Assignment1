#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"

int
main()
{
  const char* old_path = "/cat";
  const char* new_path= "/new_cat";
  symlink(old_path, new_path);
  struct stat st;
  char buf[256];
  readlink(new_path, buf, 256);
  int fd = open(new_path, O_RDONLY| O_NOFOLLOW);
  fstat(fd, &st);
  printf("%s %d %d %d\n", new_path, st.type, st.ino, st.size);

  exit(0);
}