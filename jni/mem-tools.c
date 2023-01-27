#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>

int debug = 1;
char* dev = "/dev/mem";
char* VERSION = "0.0.1";

#define MIN(a,b) ((a) < (b) ? (a) : (b))

// open a memory device and map it to a pointer
void *openMem(char* dev, off_t target, off_t size, int write) {
    int fd = open(dev, write ? O_RDWR : O_RDONLY);
    if (fd < 0) {
        if (debug) printf("Failed to open memory\n");
        return NULL;
    }

    void *memory = mmap64(NULL, size, write ? (PROT_READ | PROT_WRITE) : (PROT_READ), MAP_SHARED, fd, target);
    if (memory == MAP_FAILED) {
        if (debug) printf("Failed to map memory %p\n", memory);
        return NULL;
    }

    if (debug) printf("Mapped Address: %ld to %p (%ld bytes) \n", target, memory, size);

    close(fd);

    return memory;
}

// check if a block of memory matches a given char array
int checkIfMemoryEquals(char* dev, off_t target, size_t size, size_t offset, char *data) {
    
    void *memory = openMem(dev, target, size, 0);
    if (memory == NULL) {
        return 0;
    }

    printf("Checking Memory... \n");

    int length = MIN(size, strlen(data));

    int result = memcmp(data, memory + offset, length);

    munmap(memory, size);

    if (result == 0) {
        return 1;
    } else {
        return 0;
    }
}

// function that reads file into a buffer
void *readFile(char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        printf("Failed to open file %s\n", filename);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = malloc(length + 1);
    if (buffer == NULL) {
        printf("Failed to allocate memory for file %s\n", filename);
        return NULL;
    }

    fread(buffer, 1, length, file);
    fclose(file);

    return buffer;
}

// function that writes a char array to a file
int writeFile(char *filename, char *data, size_t size) {
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        printf("Failed to open file %s\n", filename);
        return 0;
    }

    fwrite(data, 1, size, file);
    fclose(file);

    return 1;
}

// read a block of memory and return it as a char array
char *readMem(char* dev, off_t target, size_t size, size_t offset) {
    void *memory = openMem(dev, target, size, 0);
    if (memory == NULL) {
        return NULL;
    }

    char *data = malloc(size);
    if (data == NULL) {
        printf("Failed to allocate memory\n");
        return NULL;
    }

    memcpy(data, memory + offset, size);

    munmap(memory, size);

    return data;
}

// calculate the absolute target address based on the page size and target address
off_t calculateAbsoluteTarget(off_t target) {
    off_t page_size = getpagesize();
    off_t page_offset = target % page_size;
    target = target - page_offset;
     
    if(debug) printf("Page size: %ld\n", page_size);
    if(debug) printf("Page offset: %ld\n", page_offset);
    if(debug) printf("Target: %ld\n", target);

    return target;
}

// convert a hex string to a char array
char *hexStrToBin(const char *str) {
    size_t len = strlen(str);
    char *data = malloc(len / 2);
    char *pos = data;

    while (*str && str[1]) {
        sscanf(str, "%2hhx", pos);
        ++pos;
        str += 2;
    }

    return data;
}


//// ROUTES ////

// function that reads a block of memory and spits it out to stdout
int R_memToStdout(char* dev, off_t target, size_t size, size_t offset) {
    char *data = readMem(dev, target, size, offset);
    if (data == NULL) {
        return 0;
    }

    fwrite(data, 1, size, stdout);
    
    return 1;
}

// function that reads a block of memory and writes it to a file
int R_memToFile(char* dev, off_t target, size_t size, size_t offset, char *filename) {
    char *data = readMem(dev, target, size, offset);
    if (data == NULL) {
        return 0;
    }

    int result = writeFile(filename, data, size);

    return result;
}

// function that reads a file and writes it to a block of memory
int R_fileToMem(char* dev, off_t target, size_t size, size_t offset, void *file) {
    // if the size is 0, read the whole file

    void *memory = openMem(dev, target, size, 1);
    if (memory == NULL) {
        return 0;
    }

    memcpy(memory + offset, file, size);

    munmap(memory, size);

    return 1;
}

// char to mem  
int R_stdinToMem(char* dev, off_t target, size_t size, size_t offset, char *stdin) {
    void *memory = openMem(dev, target, size, 1);
    if (memory == NULL) {
        return 0;
    }

    memcpy(memory + offset, stdin, size);

    munmap(memory, size);

    return 1;
}

// function that compares a block of memory to a file
int R_memToFileCompare(char* dev, off_t target, size_t size, size_t offset, void *file) {
    char *data = readMem(dev, target, size, offset);
    if (data == NULL) {
        return 0;
    }

    int result = checkIfMemoryEquals(dev, target, size, offset, file);

    return result;
}


int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Invalid command, See 'mem-tools help' for more\n");
        return 1;
    }

    char *command = argv[1];
    // if the command is not NULL, check if it's a valid command [read, write, file2mem, mem2file, compare]
    if (
        strcmp(command, "read") != 0 && 
        strcmp(command, "write") != 0 && 
        strcmp(command, "file2mem") != 0 && 
        strcmp(command, "mem2file") != 0 && 
        strcmp(command, "compare") != 0 && 
        strcmp(command, "help") != 0 &&
        strcmp(command, "version") != 0
    ) {
        printf("Invalid command: %s\nSee 'mem-tools help' for more\n", command);
        return 1;
    }

    if (strcmp(command, "read") == 0) {
        // the command is read, check if the arguments are valid
        // ex: mem-tools read 0x17F3D000 0x473100
        // ex: mem-tools read <address> <size>
        if (argc != 4) {
            printf("Invalid arguments for command 'read'\nSee 'mem-tools help' for more\n");
            return 0;
        }

        // parse the arguments
        off_t reqTarget = strtoul(argv[2], NULL, 16);
        size_t size = strtoul(argv[3], NULL, 16);

        // calculate the absolute target address
        off_t target = calculateAbsoluteTarget(reqTarget);

        // get the offset
        size_t offset = reqTarget - target;


        // read the memory and print it to stdout
        return R_memToStdout(dev, target, size, offset);
    } 

    if (strcmp(command, "write") == 0) {
        // the command is write, check if the arguments are valid
        // ex: mem-tools write 0x17F3D000 0xffffff
        // ex: mem-tools write <address> <data>
        if (argc != 4) {
            printf("Invalid arguments for command 'write'\nSee 'mem-tools help' for more\n");
            return 0;
        }

        // parse the arguments
        off_t reqTarget = strtoul(argv[2], NULL, 16);
        
        char *data = hexStrToBin(argv[3]);
        
        int size = strlen(data);

        // calculate the absolute target address
        off_t target = calculateAbsoluteTarget(reqTarget);

        // get the offset
        size_t offset = reqTarget - target;

        // read the memory and print it to stdout
        return R_memToFile(dev, target, size, offset, data);
    }

    if (strcmp(command, "file2mem") == 0) {
        // the command is file2mem, check if the arguments are valid
        // ex: mem-tools file2mem 0x17F3D000 /path/to/file
        // ex: mem-tools file2mem <address> <file>
        if (argc != 4) {
            printf("Invalid arguments for command 'file2mem'\nSee 'mem-tools help' for more\n");
            return 0;
        }

        // parse the arguments
        off_t reqTarget = strtoul(argv[2], NULL, 16);

        void *file = readFile(argv[3]);
        if (file == NULL) {
            return 0;
        }

        int size = strlen(file);

        // calculate the absolute target address
        off_t target = calculateAbsoluteTarget(reqTarget);

        // get the offset
        size_t offset = reqTarget - target;

        // read the memory and print it to stdout
        return R_fileToMem(dev, target, size, offset, file);
    }

    if (strcmp(command, "mem2file") == 0) {
        // the command is mem2file, check if the arguments are valid
        // ex: mem-tools mem2file 0x17F3D000 <size> /path/to/file
        // ex: mem-tools mem2file <address> <size> <file>
        if (argc != 5) {
            printf("Invalid arguments for command 'mem2file'\nSee 'mem-tools help' for more\n");
            return 0;
        }

        // parse the arguments
        off_t reqTarget = strtoul(argv[2], NULL, 16);
        if (debug) printf("reqTarget: %lx\n", reqTarget);
        size_t size = strtoul(argv[3], NULL, 16);
        if (debug) printf("size: %lx\n", size);

        // calculate the absolute target address
        off_t target = calculateAbsoluteTarget(reqTarget);
        if (debug) printf("target: %lx\n", target);

        // get the offset
        size_t offset = reqTarget - target;
        if (debug) printf("offset: %lx\n", offset);

        // read the memory and print it to stdout
        return R_memToFile(dev, target, size, offset, argv[4]);
    }

    if (strcmp(command, "compare") == 0) {
        // the command is compare, check if the arguments are valid
        // ex: mem-tools compare 0x17F3D000 /path/to/file
        // ex: mem-tools compare <address> <file>
        if (argc != 5) {
            printf("Invalid arguments for command 'compare'\nSee 'mem-tools help' for more\n");
            return 0;
        }

        // parse the arguments
        off_t reqTarget = strtoul(argv[2], NULL, 16);
        void *file = readFile(argv[3]);
        if (file == NULL) {
            return 0;
        }

        int size = strlen(file);

        // calculate the absolute target address
        off_t target = calculateAbsoluteTarget(reqTarget);

        // get the offset
        size_t offset = reqTarget - target;

        // read the memory and print it to stdout
        int result = R_memToFileCompare(dev, target, size, offset, file);
        if (result == 0) {
            printf("The memory and the file are the same\n");
        } else {
            printf("The memory and the file are different\n");
        }
        return result;
    }

    if (strcmp(command, "help") == 0) {
        // the command is help, print the help message
        printf("Usage: mem-tools <command> <arguments>\n");
        printf("Commands:\n");
        printf("  read <address> <size> - read memory from address to size\n");
        printf("  write <address> <data> - write data to address\n");
        printf("  file2mem <address> <file> - write file to address\n");
        printf("  mem2file <address> <size> <file> - read memory from address to size and write it to file\n");
        printf("  compare <address> <file> - compare memory from address to file\n");
        printf("  help - print this help message\n");
        printf("  version - print the version of mem-tools\n");
        return 0;
    }

    if (strcmp(command, "version") == 0) {
        // the command is version, print the version
        printf("mem-tools version %s\n", VERSION);
        return 0;
    }

    // in theory, this is unreachable - but just in case
    printf("Invalid command: %s\nSee 'mem-tools help' for more\n", command);
    return 1;
}