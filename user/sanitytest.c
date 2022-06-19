#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"

//OFRY added this whole file to test implementation 

int
main()
{
  char buf[BSIZE];
  int num_blocks, fd;

  fd = open("sanitytest.file", O_CREATE | O_WRONLY);
  if(fd < 0){
    printf("error - cannot open this sanity test file for writing\n");
    exit(-1);
  }

  num_blocks = 0;
  while(1){
    *(int*)buf = num_blocks;
    int cc = write(fd, buf, sizeof(buf));
    if(cc <= 0)
      break;
    num_blocks++;
    if (num_blocks == 12){
        printf("Done writing 12KB (direct)\n");
    }
    if (num_blocks == 12 + 256){
        printf("Done writing 268KB (single direct)\n");
    }
    if (num_blocks == 12 + 256 + 256 * 256){
        printf("Done writing 10MB\n");
    }
    if (num_blocks % 1000 == 0)
      printf("Done writing an addition of 1000 blocks\n");
  }

  printf("\n wrote %d blocks\n", num_blocks);
  if(num_blocks != 65804) {
    printf("sanitytest: file is too small\n");
    exit(-1);
  }

  close(fd);
  fd = open("sanitytest.file", O_RDONLY);
  if(fd < 0){
    printf("sanitytest: cannot re-open sanitytest.file for reading\n");
    exit(-1);
  }
  int i; 
  for(i = 0; i < num_blocks; i++){
    int cc = read(fd, buf, sizeof(buf));
    if(cc <= 0){
      printf("sanitytest: read error at block %d\n", i);
      exit(-1);
    }
    if(*(int*)buf != i){
      printf("sanitytest: read the wrong data (%d) for block %d\n",
             *(int*)buf, i);
      exit(-1);
    }
  }

  printf("sanity test done; ok\n"); 

  exit(0);
}