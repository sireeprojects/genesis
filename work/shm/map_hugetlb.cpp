#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "../utils.h"

#define ADDR (void *)(0x0UL)
#define LENGTH (1024UL*1024*1024)

static void check_bytes(char *addr) {
   printf("First hex is %x\n", *((unsigned int *)addr));
   printf("length: %d \n", LENGTH);
}

static void write_bytes(char *addr) {
   unsigned long i;

   for (i = 0; i < LENGTH; i++)
      *(addr + i) = (char)i;
}

static int read_bytes(char *addr) {
   unsigned long i;

   check_bytes(addr);
   for (i = 0; i < LENGTH; i++)
      if (*(addr + i) != (char)i) {
         printf("Mismatch at %lu\n", i);
         return 1;
      }
   return 0;
}

int main(void) {
   cea_timer timer;
   void *addr;
   int ret;

   addr = mmap(ADDR,
           LENGTH, (PROT_READ | PROT_WRITE),
           (MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB),
           -1, 0);
   if (addr == MAP_FAILED) {
      perror("mmap");
      exit(1);
   }

   printf("Returned address is %p\n", addr);
   
   timer.start();
   check_bytes((char*)addr);
   cout << "TIME TAKEN BY check_bytes: " << timer.elapsed_in_string() << endl;

   timer.start();
   write_bytes((char*)addr);
   cout << "TIME TAKEN BY write_bytes: " << timer.elapsed_in_string() << endl;
   timer.start();
   ret = read_bytes((char*)addr);
   cout << "TIME TAKEN BY read_bytes: "<< timer.elapsed_in_string() << endl;

   sleep(3);

   /* munmap() length of MAP_HUGETLB memory must be hugepage aligned */
   if (munmap(addr, LENGTH)) {
      perror("munmap");
      exit(1);
   }

   return ret;
}
