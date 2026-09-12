# SENG21213 Operating Systems Assignment
SE/2023/015

## Project Overview

This project is an Operating Systems assignment developed in stages from Stage 0 to Stage 4.

The project includes:

- Basic kernel startup and VGA output
- Keyboard input
- Process management
- CPU scheduling
- Interrupt Descriptor Table (IDT)
- Threads
- Mutexes
- Semaphores
- Physical memory management
- RAM disk
- File system
- File system shell commands

Stage 4 is the final stage of the assignment and adds a RAM Disk File System.

---

## Stages Completed

### Stage 0 - Starter Kernel

Implemented the basic kernel foundation and required type definitions.

### Stage 1 - Process Management and Interrupt Foundation

Implemented:

- Process creation
- Process termination
- Process table
- Basic scheduler foundation
- IDT foundation
- Keyboard input

### Stage 2 - Scheduling and Synchronization

Implemented:

- Process scheduling
- Threads
- Mutexes
- Semaphores
- Basic synchronization support

### Stage 3 - Physical Memory Manager

Implemented:

- Physical memory frame management
- Frame allocation
- Frame freeing
- Free frame counting
- Total frame counting
- `mem` and `meminfo` shell commands

### Stage 4 - RAM Disk File System

Implemented:

- 1 MB RAM disk
- Superblock
- Inode bitmap
- Data block bitmap
- Inode table
- Inode allocation and freeing
- File creation
- File opening and closing
- File writing
- File reading
- File deletion
- Directory listing
- File system shell commands

---

# Stage 4 File System

The Stage 4 file system is implemented as a RAM-based file system.

The RAM disk size is:

```text
1 MB
```

The block size is:

```text
512 bytes
```

The RAM disk contains:

```text
2048 blocks
```

The file system uses inodes and direct data block pointers.

---

## RAM Disk Layout

| Area | Block / Offset | Size |
|---|---:|---:|
| Superblock | Block 0 / `0x000` | 512 bytes |
| Inode Bitmap | Block 1 / `0x200` | 512 bytes |
| Block Bitmap | Block 2 / `0x400` | 512 bytes |
| Inode Table | Block 3 / `0x600` | 256 KB |
| Data Blocks | Block 515 / `0x40600` | Remaining space |

The file system supports up to:

```text
1024 inodes
```

Each inode is:

```text
256 bytes
```

Each inode contains:

- File size
- Direct data block pointers
- Number of allocated blocks
- File type
- File name

The current implementation uses 8 direct block pointers.

Therefore, the maximum file size supported by the current direct-block implementation is approximately:

```text
8 × 512 = 4096 bytes
```

---

# File System API

The following functions are implemented.

## Initialize File System

```c
void fs_init(void);
```

Initializes the RAM disk file system, superblock, bitmaps, inode table, and open file table.

---

## Open File

```c
int fs_open(const char *name, uint32_t flags);
```

Supported flags:

```c
#define O_RDONLY 0x01
#define O_WRONLY 0x02
#define O_CREAT  0x04
#define O_TRUNC  0x08
```

---

## Read File

```c
int fs_read(int fd, void *buffer, uint32_t count);
```

Reads data from an opened file.

---

## Write File

```c
int fs_write(int fd, const void *buffer, uint32_t count);
```

Writes data to an opened file.

The implementation allocates RAM disk data blocks when required.

---

## Close File

```c
int fs_close(int fd);
```

Closes an opened file descriptor.

---

## Delete File

```c
int fs_unlink(const char *name);
```

Deletes a file and releases its allocated inode and data blocks.

---

## List Files

```c
void fs_ls(void);
```

Lists the files currently stored in the file system.

---

# Shell Commands

The Stage 4 shell provides the following file system commands.

## `ls`

Lists files.

Example:

```text
> ls
```

Example output:

```text
readme 5 bytes
```

If there are no files:

```text
No files.
```

---

## `touch`

Creates an empty file.

Example:

```text
> touch readme
```

---

## `write`

Writes text into a file.

Example:

```text
> write readme hello
```

Expected result:

```text
File written successfully.
```

---

## `cat`

Displays the contents of a file.

Example:

```text
> cat readme
```

Output:

```text
hello
```

---

## `rm`

Deletes a file.

Example:

```text
> rm readme
```

Expected result:

```text
File deleted.
```

---

# Example File System Workflow

A basic Stage 4 workflow is:

```text
> ls
No files.

> touch readme

> write readme Hello from Stage 4
File written successfully.

> cat readme
Hello from Stage 4

> ls
readme 17 bytes

> rm readme
File deleted.

> ls
No files.
```

---

# Project Structure

Important Stage 4 files include:

```text
seng21213-os/
├── boot/
│   ├── boot.asm
│   └── boot.bin
│
├── include/
│   ├── fs.h
│   ├── pmm.h
│   ├── types.h
│   └── ...
│
├── kernel/
│   ├── kernel.c
│   ├── ramdisk.c
│   ├── ramdisk.h
│   ├── fs.c
│   ├── fs.h
│   ├── pmm.c
│   ├── process.c
│   ├── scheduler.c
│   ├── thread.c
│   ├── mutex.c
│   ├── semaphore.c
│   └── ...
│
├── Makefile
├── linker.ld
└── README.md
```

---

# Building the Project

From the project directory:

```bash
make clean
make
```

If the build is successful, the kernel image is generated.

---

# Running with QEMU

Run:

```bash
make run
```

The project uses QEMU to run the operating system.

The shell should appear after the kernel starts.

---

# Stage 4 Verification

The Stage 4 assignment requires testing file creation, writing, reading, deleting, and listing.

A recommended verification is to create at least five files.

Example:

```text
> touch file1
> touch file2
> touch file3
> touch file4
> touch file5

> write file1 First file
> write file2 Second file
> write file3 Third file
> write file4 Fourth file
> write file5 Fifth file

> ls

> cat file1
> cat file2
> cat file3
> cat file4
> cat file5

> rm file1
> rm file2
> rm file3
> rm file4
> rm file5

> ls
```

This verifies:

- File creation
- File writing
- File reading
- Directory listing
- File deletion
- Multiple-file handling

**Note:** The five-file verification should be run before the final submission if it has not already been completed.

---

# Git Version Tags

The project stages use the following tags:

```text
v0.1-stage0
v0.2-stage1
v0.3-stage2
v0.4-stage3
v0.5-stage4
```

The final Stage 4 tag is:

```text
v0.5-stage4
```

---

# Git Commands

Check the current status:

```bash
git status
```

View the commit history:

```bash
git log --oneline --decorate --graph
```

Create the final Stage 4 tag:

```bash
git tag v0.5-stage4
```

Push the main branch:

```bash
git push origin main
```

Push the Stage 4 tag:

```bash
git push origin v0.5-stage4
```

---

# Stage 4 Completion

Stage 4 is the final stage of this assignment.

The project now contains the previous Operating System features from Stages 0-3 together with the Stage 4 RAM Disk File System.

Before final submission, make sure to:

1. Build the project successfully.
2. Run the OS using QEMU.
3. Test the file system commands.
4. Perform the required five-file verification.
5. Check the Git history.
6. Make sure the final tag is:

```text
v0.5-stage4
```

7. Push the final branch and tag to the remote repository.

---

## Final Project

The final project combines:

```text
Stage 0
   ↓
Stage 1
   ↓
Stage 2
   ↓
Stage 3
   ↓
Stage 4
   ↓
Final Operating System
```

Stage 4 completes the assignment by adding a working RAM Disk File System and file management shell commands.
