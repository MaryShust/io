#include "kernel.h"
#include "common.h"
#include "io.h"

extern char __bss[], __bss_end[], __stack_top[];

struct sbiret
sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4, long arg5, long fid, long eid)
{
  register long a0 __asm__("a0") = arg0;
  register long a1 __asm__("a1") = arg1;
  register long a2 __asm__("a2") = arg2;
  register long a3 __asm__("a3") = arg3;
  register long a4 __asm__("a4") = arg4;
  register long a5 __asm__("a5") = arg5;
  register long a6 __asm__("a6") = fid;
  register long a7 __asm__("a7") = eid;

  __asm__ __volatile__("ecall"
                  : "=r"(a0), "=r"(a1)
                  : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
                  : "memory");
  return (struct sbiret) {.error = a0, .value = a1};
}

void
putchar(char ch)
{
  sbi_call(ch, 0, 0, 0, 0, 0, 0, SBI_ECALL_0_1_PUTCHAR);
}

int
getchar(void)
{
  struct sbiret ret;
  
  do
  {
    ret = sbi_call(0, 0, 0, 0, 0, 0, 0, SBI_ECALL_0_1_GETCHAR);
  } while (ret.error == SBI_ERR_FAILED);
  
  return (int) ret.error;
}

void
readline(char *buf, int max_len)
{
  int i = 0;
  char c;

  while (i < max_len - 1) {
    c = getchar();

    // Backspace handling
    if (c == 8 || c == 127) {
      if (i > 0) {
        i--;
        buf[i] = '\0';
        putchar(8);    // Move cursor back
        putchar(' ');  // Erase character with space
        putchar(8);    // Move cursor back again
      }
      continue;
    }

    // Enter key handling
    if (c == '\r' || c == '\n') {
      putchar('\n');
      break;
    }

    // Printable characters
    if (c >= 32 && c < 127) {
      putchar(c);
      buf[i++] = c;
    }
  }

  buf[i] = '\0';
}

void
get_sbi_version()
{
  struct sbiret ret = sbi_call(0, 0, 0, 0, 0, 0, SBI_EXT_VER, SBI_EXT_BASE);
  
  if (ret.error == SBI_SUCCESS) {
    long major = ret.value >> 24;
    long minor = ret.value & 0xFFFFFF;
    printf("\nSBI Specification Version: %d.%d.%d\n", 
           major, (minor >> 16) & 0xFF, minor & 0xFFFF);
  } else {
    printf("\nFailed to get SBI version. Error: %ld\n", ret.error);
  }
}

void
get_num_counters()
{
  struct sbiret ret = sbi_call(0, 0, 0, 0, 0, 0, SBI_EXT_CTR_NUM, SBI_EXT_PMU);
  
  if (ret.error == SBI_SUCCESS) {
    printf("\nNumber of counters: %ld\n", ret.value);
  } else {
    printf("\nFailed to get number of counters. Error: %ld\n", ret.error);
  }
}

void
get_counter_details()
{
  printf("\nEnter counter number (0-%ld): ", get_num_counters_available());
  char input[32];
  readline(input, sizeof(input));
  long counter_num = atoi(input);

  struct sbiret ret = sbi_call(counter_num, 0, 0, 0, 0, 0, SBI_EXT_CTR_DTLS, SBI_EXT_PMU);
  
  printf("\n=== Counter %ld Details ===\n", counter_num);
  
  if (ret.error == SBI_SUCCESS) {
    unsigned long counter_info = ret.value;
    int type = (counter_info >> (__riscv_xlen - 1)) & 0x1;
    int width = (counter_info >> 12) & 0x3F;
    int csr = counter_info & 0xFFF;

    printf("Type: %s\n", type ? "Firmware" : "Hardware");
    
    if (!type) {
      printf("CSR: 0x%03x\n", csr);
      printf("Width: %d bits\n", width + 1);
    } else {
      printf("CSR and width not applicable for firmware counters.\n");
    }
  } else {
    printf("Failed to get counter details. Error: %ld\n", ret.error);
  }
}

// Helper function to get the maximum counter number
long
get_num_counters_available()
{
  struct sbiret ret = sbi_call(0, 0, 0, 0, 0, 0, SBI_EXT_CTR_NUM, SBI_EXT_PMU);
  return (ret.error == SBI_SUCCESS) ? ret.value - 1 : 0;
}

void
system_shutdown()
{
  printf("\nShutting down system...\n");
  printf("\nThank you for using the program!\n");
  
  struct sbiret ret = sbi_call(0, 0, 0, 0, 0, 0, SBI_EXT_SHUTDOWN, SBI_EXT_SRST);
  
  if (ret.error != SBI_SUCCESS) {
    printf("Shutdown failed. Error: %ld\n", ret.error);
  }
  
  while(1) {
    __asm__ __volatile__("wfi");
  }
}

void
display_menu()
{
  printf("\n╔════════════════════════════════════╗\n");
  printf("║        OpenSBI Function Menu       ║\n");
  printf("╠════════════════════════════════════╣\n");
  printf("║ 1. Get SBI specification version   ║\n");
  printf("║ 2. Get number of counters           ║\n");
  printf("║ 3. Get details of a counter         ║\n");
  printf("║ 4. System shutdown                  ║\n");
  printf("╚════════════════════════════════════╝\n");
  printf("Enter option (1-4): ");
}

void
menu()
{
  while(1)
  {
    display_menu();

    char input[32];
    readline(input, sizeof(input));
    int choice = atoi(input);
    
    switch (choice)
    {
      case 1:
        get_sbi_version();
        break;
      case 2:
        get_num_counters();
        break;
      case 3:
        get_counter_details();
        break;
      case 4:
        system_shutdown();
        break;
      default:
        printf("\n❌ Invalid option. Please try again.\n");
    }
    
    printf("\n────────────────────────────────────\n");
  }
}

void
kernel_main(void)
{
  // Clear BSS section
  for(char *p = __bss; p < __bss_end; p++)
  {
    *p = 0;
  }
  
  printf("\n🎓 ITMO University\n");
  printf("📚 Laboratory Work #1\n");
  printf("💻 Input/Output Systems\n");
  printf("👨‍💻 Principles of I/O Organization without OS\n\n");

  menu();

  for(;;)
  {
    __asm__ __volatile__("wfi");
  }
}

__attribute__((section(".text.boot")))
__attribute__((naked))
void
boot(void)
{
  __asm__ __volatile__(
          "mv sp, %[stack_top]\n"
          "j kernel_main\n"
          :
          : [stack_top] "r" (__stack_top)
  );
}
