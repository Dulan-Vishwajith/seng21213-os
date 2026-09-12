/* =============================================================================
 * SENG21213-OS :: Main Kernel  (Stage 0 – Foundations)
 * File   : kernel/kernel.c
 *
 * PURPOSE
 *   This is the heart of your operating system. Right now it:
 *     1. Initialises VGA text-mode display
 *     2. Initialises the keyboard driver
 *     3. Prints a splash screen
 *     4. Runs a minimal interactive shell ("ksh")
 *
 * ASSIGNMENT MILESTONES  (what YOU will add in later lectures)
 *   Lecture  9  – Process Management  →  process.h / process.c / scheduler.c
 *   Lecture 10  – Threads             →  thread.h  / thread.c
 *   Lecture 11  – Memory Management   →  pmm.h     / pmm.c / vmm.c
 *   Lecture 12  – File System         →  fs.h      / fs.c
 *
 * CODING CONVENTION
 *   - Prefix kernel-internal functions with k_ (e.g. k_strcmp)
 *   - All driver APIs live in their own .h/.c pair
 *   - NEVER call malloc – use the PMM you build in Lecture 11
 * ============================================================================*/

#include "vga.h"
#include "keyboard.h"
#include "../include/types.h"
#include "process.h"
#include "scheduler.h"
#include "thread.h"
#include "mutex.h"
#include "semaphore.h"
#include "pmm.h"
#include "ramdisk.h"
#include "fs.h"
/* ---------------------------------------------------------------------------
 * Forward declarations of shell commands
 * --------------------------------------------------------------------------*/
static void cmd_help(void);
static void cmd_clear(void);
static void cmd_about(void);
static void cmd_echo(const char *args);
static void cmd_mem(void);
static void cmd_meminfo(void);
static void cmd_ps(void);
static void cmd_kill(const char *args);
static void cmd_sched(void);
static void cmd_ls(void);
static void cmd_touch(const char *args);
static void cmd_write(const char *args);
static void cmd_cat(const char *args);
static void cmd_rm(const char *args);



/* ---------------------------------------------------------------------------
 * Utility: minimal string helpers (no libc in a freestanding kernel!)
 * --------------------------------------------------------------------------*/
static int k_strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) { a++; b++; }
    return (uint8_t)*a - (uint8_t)*b;
}

static int k_strncmp(const char *a, const char *b, size_t n) {
    while (n-- && *a && (*a == *b)) { a++; b++; }
    return n == (size_t)-1 ? 0 : (uint8_t)*a - (uint8_t)*b;
}

static size_t k_strlen(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

static int k_atoi(const char *s) {
    int value = 0;

    while (*s >= '0' && *s <= '9') {
        value = value * 10 + (*s - '0');
        s++;
    }

    return value;
}



/* Skip leading spaces */
static const char *k_ltrim(const char *s) {
    while (*s == ' ') s++;
    return s;
}

/* ---------------------------------------------------------------------------
 * Splash Screen
 * --------------------------------------------------------------------------*/
static void print_splash(void) {
    vga_clear(VGA_BLACK);

    /* Top banner box */
    vga_draw_box(0, 0, 7, 80, VGA_LIGHT_MAGENTA);

    vga_set_cursor(1, 2);
    vga_puts_color("  SENG21213-OS  |  Computer Architecture & Operating Systems",
                   VGA_YELLOW, VGA_BLACK);

    vga_set_cursor(2, 2);
    vga_puts_color("  Stage 0: Kernel Foundations", VGA_LIGHT_CYAN, VGA_BLACK);

    vga_set_cursor(3, 2);
    vga_puts_color("  Faculty of Engineering – Department of Software Engineering",
                   VGA_LIGHT_GREY, VGA_BLACK);

    vga_set_cursor(4, 2);
    vga_puts_color("  Built by students, for students.  Type 'help' to begin.",
                   VGA_LIGHT_GREEN, VGA_BLACK);

    vga_set_cursor(5, 2);
    vga_puts_color("  CPU: i686 (32-bit Protected Mode)  |  Display: VGA 80x25",
                   VGA_DARK_GREY, VGA_BLACK);

    vga_set_cursor(8, 0);
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_puts("  Welcome! This kernel was compiled from source and booted entirely\n");
    vga_puts("  from bare metal. There is no Linux or Windows underneath – only\n");
    vga_puts("  the code you and your team write.\n");
    vga_puts("\n");
    vga_puts("  Assignment milestones to implement:\n");
    vga_puts_color("    [L09] ", VGA_YELLOW, VGA_BLACK);
    vga_puts("Process Management  – PCB, ready queue, round-robin scheduler\n");
    vga_puts_color("    [L10] ", VGA_YELLOW, VGA_BLACK);
    vga_puts("Threads & Sync      – kernel threads, mutex, semaphore\n");
    vga_puts_color("    [L11] ", VGA_YELLOW, VGA_BLACK);
    vga_puts("Memory Management   – physical page allocator, virtual memory\n");
    vga_puts_color("    [L12] ", VGA_YELLOW, VGA_BLACK);
    vga_puts("File System         – RAM disk, FAT-like directory structure\n");
    vga_puts("\n");
}

/* ---------------------------------------------------------------------------
 * Shell command implementations
 * --------------------------------------------------------------------------*/
static void cmd_help(void) {
    vga_puts_color("\n  SENG21213-OS Shell Commands\n", VGA_YELLOW, VGA_BLACK);
    vga_puts("  ─────────────────────────────────────────────\n");
    vga_puts("  help    – Show this help message\n");
    vga_puts("  clear   – Clear the screen\n");
    vga_puts("  about   – About this OS and course\n");
    vga_puts("  echo    – Echo text to screen\n");
    vga_puts("  mem     – Memory map (stub)\n");
    vga_puts("  meminfo - Show physical memory information\n");
    vga_puts_color("\n  Milestones (to implement):\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ps      – [L09] List processes\n");
    vga_puts("  kill    – [L09] Terminate a process\n");
    vga_puts("  threads – [L10] List kernel threads\n");
    vga_puts("  free    – [L11] Show free memory\n");
    vga_puts("  ls      – [L12] List files\n");
    vga_puts("  cat     – [L12] Print file contents\n\n");
    vga_puts("  sched  - Run scheduler\n");
}

static void cmd_clear(void) {
    vga_clear(VGA_BLACK);
}

static void cmd_about(void) {
    vga_puts_color("\n  About SENG21213-OS\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ─────────────────────────────────────────────\n");
    vga_puts("  Architecture : x86 (i686), 32-bit Protected Mode\n");
    vga_puts("  Bootloader   : Custom MBR (NASM)\n");
    vga_puts("  Kernel       : Freestanding C (GCC, no libc)\n");
    vga_puts("  VM Target    : QEMU (qemu-system-i386)\n");
    vga_puts("  Course       : SENG 21213 – Sem 2\n");
    vga_puts("  Reference    : Stallings, OS: Internals & Design Principles\n\n");
}

static void cmd_echo(const char *args) {
    vga_puts("  ");
    vga_puts(args);
    vga_puts("\n");
}

static void cmd_mem(void) {
    /* Stage 0 stub – students implement the real PMM in Lecture 11 */
    vga_puts_color("\n  Memory Map (stub – implement PMM in Lecture 11)\n",
                   VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ─────────────────────────────────────────────\n");
    vga_puts("  0x00000000 – 0x000FFFFF  :  First 1 MB (reserved/BIOS)\n");
    vga_puts("  0x00100000 – 0x00EFFFFF  :  Extended memory (usable ~14 MB)\n");
    vga_puts("  0x00F00000 – 0x00FFFFFF  :  BIOS / ROM area\n");
    vga_puts("  0xB8000    – 0xBFFFF     :  VGA frame buffer\n");
    vga_puts_color("\n  TODO: Use BIOS int 0x15, EAX=0xE820 to get real memory map\n\n",
                   VGA_YELLOW, VGA_BLACK);
}



static void k_print_uint(uint32_t n) {
    char buffer[11];
    int i = 0;

    if (n == 0) {
        vga_puts("0");
        return;
    }

    while (n > 0) {
        buffer[i++] = (char)('0' + (n % 10));
        n /= 10;
    }

    while (i > 0) {
        vga_putchar(buffer[--i]);
    }
}

static void cmd_meminfo(void)
{
    uint32_t total;
    uint32_t free;
    uint32_t used;

    total = pmm_total_frames();
    free = pmm_free_frames();
    used = total - free;

    vga_puts_color("\nPhysical Memory Information\n",
                   VGA_LIGHT_CYAN, VGA_BLACK);

    vga_puts("--------------------------------\n");

    vga_puts("Total: ");
    k_print_uint(total * 4);
    vga_puts(" KB\n");

    vga_puts("Used : ");
    k_print_uint(used * 4);
    vga_puts(" KB\n");

    vga_puts("Free : ");
    k_print_uint(free * 4);
    vga_puts(" KB\n\n");
}




static void cmd_sched(void)
{
    int current;

    scheduler_schedule();
    current = scheduler_get_current();

    if (current >= 0) {
        vga_puts_color("Scheduled PID: ", VGA_LIGHT_GREEN, VGA_BLACK);
        vga_printf("%d\n", process_table[current].pid);
    } else {
        vga_puts_color("No READY process found.\n",
                       VGA_LIGHT_RED, VGA_BLACK);
    }
}



static void cmd_ps(void) {
    int i;

    vga_puts_color("\nPID    STATE       TICKS\n",
                   VGA_LIGHT_CYAN, VGA_BLACK);

    vga_puts("-----------------------------\n");

    for (i = 0; i < MAX_PROCESSES; i++) {

        if (process_table[i].state != PROCESS_UNUSED) {

            /* Print PID */
            k_print_uint(process_table[i].pid);

            vga_puts("      ");

            /* Print process state */
            if (process_table[i].state == PROCESS_READY) {
                vga_puts_color("READY", VGA_LIGHT_GREEN, VGA_BLACK);
            }
            else if (process_table[i].state == PROCESS_RUNNING) {
                vga_puts_color("RUNNING", VGA_LIGHT_CYAN, VGA_BLACK);
            }
            else if (process_table[i].state == PROCESS_TERMINATED) {
                vga_puts_color("TERMINATED", VGA_LIGHT_RED, VGA_BLACK);
            }

            vga_puts("       ");

            /* Print ticks */
            k_print_uint(process_table[i].ticks);

            vga_puts("\n");
        }
    }

    vga_puts("\n");
}



static void cmd_kill(const char *args) {
    int pid;

    args = k_ltrim(args);

    if (k_strlen(args) == 0) {
        vga_puts_color(
            "Usage: kill <pid>\n",
            VGA_YELLOW,
            VGA_BLACK
        );
        return;
    }

    pid = k_atoi(args);

    if (pid <= 0) {
        vga_puts_color(
            "Invalid PID.\n",
            VGA_LIGHT_RED,
            VGA_BLACK
        );
        return;
    }

    process_terminate(pid);

    vga_puts_color(
        "Process terminated.\n",
        VGA_LIGHT_GREEN,
        VGA_BLACK
    );
}







static void cmd_ls(void)
{
    inode_t entries[32];
    int count;
    int i;

    count = fs_ls(entries, 32);

    if (count < 0) {
        vga_puts_color(
            "  ls: filesystem error\n",
            VGA_LIGHT_RED,
            VGA_BLACK
        );
        return;
    }

    if (count == 0) {
        vga_puts("  No files.\n");
        return;
    }

    vga_puts_color(
        "\n  NAME                         SIZE\n",
        VGA_LIGHT_CYAN,
        VGA_BLACK
    );

    vga_puts("  -----------------------------------\n");

    for (i = 0; i < count; i++) {
        vga_puts("  ");
        vga_puts(entries[i].name);

        vga_puts("                         ");

        k_print_uint(entries[i].size);

        vga_puts(" bytes\n");
    }

    vga_puts("\n");
}









static void cmd_touch(const char *args)
{
    int fd;
    const char *name;

    name = k_ltrim(args);

    if (k_strlen(name) == 0) {
        vga_puts_color(
            "  Usage: touch <filename>\n",
            VGA_YELLOW,
            VGA_BLACK
        );
        return;
    }

    /*
     * Create the file if it does not exist.
     */
    fd = fs_open(name, O_WRONLY | O_CREAT);

    if (fd < 0) {
        vga_puts_color(
            "  touch: failed to create file\n",
            VGA_LIGHT_RED,
            VGA_BLACK
        );
        return;
    }

    fs_close(fd);

    vga_puts_color(
        "  File created.\n",
        VGA_LIGHT_GREEN,
        VGA_BLACK
    );
}







static void cmd_write(const char *args)
{
    char filename[28];
    const char *p;
    const char *text;
    int fd;
    int i;
    int written;

    p = k_ltrim(args);

    if (k_strlen(p) == 0) {
        vga_puts_color(
            "  Usage: write <filename> <text>\n",
            VGA_YELLOW,
            VGA_BLACK
        );
        return;
    }

    /*
     * Extract filename.
     */
    i = 0;

    while (*p != '\0' && *p != ' ' && i < 27) {
        filename[i] = *p;
        i++;
        p++;
    }

    filename[i] = '\0';

    /*
     * Filename too long.
     */
    if (*p != '\0' && *p != ' ') {
        vga_puts_color(
            "  write: filename too long\n",
            VGA_LIGHT_RED,
            VGA_BLACK
        );
        return;
    }

    /*
     * Skip spaces before the text.
     */
    while (*p == ' ') {
        p++;
    }

    text = p;

    if (k_strlen(text) == 0) {
        vga_puts_color(
            "  Usage: write <filename> <text>\n",
            VGA_YELLOW,
            VGA_BLACK
        );
        return;
    }

    /*
     * Open existing file or create it.
     *
     * O_TRUNC makes this command replace old contents.
     */
    fd = fs_open(
        filename,
        O_WRONLY | O_CREAT | O_TRUNC
    );

    if (fd < 0) {
        vga_puts_color(
            "  write: failed to open file\n",
            VGA_LIGHT_RED,
            VGA_BLACK
        );
        return;
    }

    written = fs_write(
        fd,
        text,
        (int)k_strlen(text)
    );

    fs_close(fd);

    if (written < 0) {
        vga_puts_color(
            "  write: failed\n",
            VGA_LIGHT_RED,
            VGA_BLACK
        );
        return;
    }

    vga_puts_color(
        "  File written successfully.\n",
        VGA_LIGHT_GREEN,
        VGA_BLACK
    );
}




static void cmd_cat(const char *args)
{
    static char buffer[INODE_DIRECT * BLOCK_SIZE + 1];

    const char *name;
    int fd;
    int bytes;
    int i;

    name = k_ltrim(args);

    if (k_strlen(name) == 0) {
        vga_puts_color(
            "  Usage: cat <filename>\n",
            VGA_YELLOW,
            VGA_BLACK
        );
        return;
    }

    fd = fs_open(name, O_RDONLY);

    if (fd < 0) {
        vga_puts_color(
            "  cat: file not found\n",
            VGA_LIGHT_RED,
            VGA_BLACK
        );
        return;
    }

    bytes = fs_read(
        fd,
        buffer,
        INODE_DIRECT * BLOCK_SIZE
    );

    fs_close(fd);

    if (bytes < 0) {
        vga_puts_color(
            "  cat: read error\n",
            VGA_LIGHT_RED,
            VGA_BLACK
        );
        return;
    }

    buffer[bytes] = '\0';

    vga_puts("  ");
    
    for (i = 0; i < bytes; i++) {
        vga_putchar(buffer[i]);
    }

    vga_puts("\n");
}







static void cmd_rm(const char *args)
{
    const char *name;

    name = k_ltrim(args);

    if (k_strlen(name) == 0) {
        vga_puts_color(
            "  Usage: rm <filename>\n",
            VGA_YELLOW,
            VGA_BLACK
        );
        return;
    }

    if (fs_unlink(name) != 0) {
        vga_puts_color(
            "  rm: file not found or delete failed\n",
            VGA_LIGHT_RED,
            VGA_BLACK
        );
        return;
    }

    vga_puts_color(
        "  File deleted.\n",
        VGA_LIGHT_GREEN,
        VGA_BLACK
    );
}











/* ---------------------------------------------------------------------------
 * Shell process
 * --------------------------------------------------------------------------*/
static char  shell_buf[256];
static char  prompt[] = "\n  ksh> ";

static void shell_run(void) {
    vga_puts_color("\n  Kernel Shell ready. Type 'help' for commands.\n",
                   VGA_LIGHT_GREEN, VGA_BLACK);

    while (true) {
        vga_puts_color(prompt, VGA_LIGHT_GREEN, VGA_BLACK);
        kb_readline(shell_buf, sizeof(shell_buf));

        /* Trim leading whitespace */
        const char *cmd = k_ltrim(shell_buf);
        if (k_strlen(cmd) == 0) continue;

        /* Dispatch */
        if (k_strcmp(cmd, "help")  == 0) { cmd_help();  continue; }
        if (k_strcmp(cmd, "clear") == 0) { cmd_clear(); continue; }
        if (k_strcmp(cmd, "about") == 0) { cmd_about(); continue; }
        if (k_strcmp(cmd, "mem")   == 0) { cmd_mem();   continue; }
        if (k_strcmp(cmd, "meminfo") == 0) { cmd_meminfo(); continue; }


        if (k_strncmp(cmd, "echo ", 5) == 0) {
            cmd_echo(k_ltrim(cmd + 5));
            continue;
        }

if (k_strcmp(cmd, "ps") == 0) {
    cmd_ps();
    continue;
}


if (k_strncmp(cmd, "kill ", 5) == 0) {
    cmd_kill(cmd + 5);
    continue;
}

if (k_strcmp(cmd, "sched") == 0) {
    cmd_sched();
    continue;
}


if (k_strcmp(cmd, "ls") == 0) {
    cmd_ls();
    continue;
}

if (k_strncmp(cmd, "touch ", 6) == 0) {
    cmd_touch(cmd + 6);
    continue;
}

if (k_strncmp(cmd, "write ", 6) == 0) {
    cmd_write(cmd + 6);
    continue;
}

if (k_strncmp(cmd, "cat ", 4) == 0) {
    cmd_cat(cmd + 4);
    continue;
}

if (k_strncmp(cmd, "rm ", 3) == 0) {
    cmd_rm(cmd + 3);
    continue;
}




        vga_puts_color("  Unknown command: ", VGA_LIGHT_RED, VGA_BLACK);
        vga_puts(cmd);
        vga_puts("\n  Type 'help' for a list of commands.\n");
    }

}








static void process_one(void) {
    while (1) {
        asm volatile("hlt");
    }
}

static void process_two(void) {
    while (1) {
        asm volatile("hlt");
    }
}




/* ---------------------------------------------------------------------------
 * Kernel entry point – called from kernel_entry.asm
 * --------------------------------------------------------------------------*/
void kernel_main(void) {
    vga_init();
    kb_init();
    process_init();
    pmm_init();
    ramdisk_init();
    fs_init();




process_create(process_one);
process_create(process_two);

    scheduler_init();
    thread_init();
    print_splash();
    shell_run();

    /* Should never reach here */
    __asm__ __volatile__("hlt");
}
