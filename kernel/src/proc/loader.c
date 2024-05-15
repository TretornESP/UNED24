#include "../util/string.h"
#include "../util/printf.h"
#include "../memory/paging.h"
#include "../memory/heap.h"
#include "../sched/scheduler.h"

#include "loader.h"
#include "process.h"

#include <elf.h>

const char * elf_class[] = {
    "Invalid class",
    "ELF32",
    "ELF64"
};

const char * elf_data[] = {
    "Invalid data",
    "2's complement, little endian",
    "2's complement, big endian"
};

const char * elf_osabi[] = {
    "UNIX System V",
    "HP-UX",
    "NetBSD",
    "Linux",
    "GNU Hurd",
    "Solaris",
    "AIX",
    "IRIX",
    "FreeBSD",
    "Tru64",
    "Novell Modesto",
    "OpenBSD",
    "OpenVMS",
    "NonStop Kernel",
    "AROS",
    "Fenix OS",
    "CloudABI",
    "Stratus Technologies OpenVOS"
};

const char * elf_type[] = {
    "NONE",
    "REL",
    "EXEC",
    "DYN",
    "CORE"
};

const char * elf_machine[] = {
    "No machine",
    "AT&T WE 32100",
    "SPARC",
    "Intel 80386",
    "Motorola 68000",
    "Motorola 88000",
    "Reserved for future use (was EM_486)",
    "Intel 80860",
    "MIPS I Architecture",
    "IBM System/370 Processor",
    "MIPS RS3000 Little-endian",
    "Reserved for future use",
    "Reserved for future use",
    "Reserved for future use",
    "Reserved for future use",
    "Hewlett-Packard PA-RISC",
    "Reserved for future use",
    "Fujitsu VPP500",
    "Enhanced instruction set SPARC",
    "Intel 80960",
    "PowerPC",
    "64-bit PowerPC",
    "IBM System/390 Processor",
    "Reserved for future use",
    "Reserved for future use",
    "Reserved for future use",
    "Reserved for future use",
    "Reserved for future use",
    "Reserved for future use",
    "Reserved for future use",
    "Reserved for future use",
    "Reserved for future use",
    "Reserved for future use",
    "Reserved for future use",
    "Reserved for future use",
    "Reserved for future use",
    "NEC V800",
    "Fujitsu FR20",
    "TRW RH-32",
    "Motorola RCE",
    "Advanced RISC Machines ARM",
    "Digital Alpha",
    "Hitachi SH",
    "SPARC Version 9",
    "Siemens TriCore embedded processor",
    "Argonaut RISC Core, Argonaut Technologies Inc.",
    "Hitachi H8/300",
    "Hitachi H8/300H",
    "Hitachi H8S",
    "Hitachi H8/500",
    "Intel IA-64 processor architecture",
    "Stanford MIPS-X",
    "Motorola ColdFire",
    "Motorola M68HC12",
    "Fujitsu MMA Multimedia Accelerator",
    "Siemens PCP",
    "Sony nCPU embedded RISC processor",
    "Denso NDR1 microprocessor",
    "Motorola Star*Core processor",
    "Toyota ME16 processor",
    "STMicroelectronics ST100 processor",
    "Advanced Logic Corp. TinyJ embedded processor family",
    "AMD x86-64 architecture",
    "Sony DSP Processor",
    "Digital Equipment Corp. PDP-10",
    "Digital Equipment Corp. PDP-11",
    "Siemens FX66 microcontroller",
    "STMicroelectronics ST9+ 8/16 bit microcontroller",
    "STMicroelectronics ST7 8-bit microcontroller",
    "Motorola MC68HC16 Microcontroller",
    "Motorola MC68HC11 Microcontroller",
    "Motorola MC68HC08 Microcontroller",
    "Motorola MC68HC05 Microcontroller",
    "Silicon Graphics SVx",
    "STMicroelectronics ST19 8-bit microcontroller",
    "Digital VAX",
    "Axis Communications 32-bit embedded processor",
    "Infineon Technologies 32-bit embedded processor",
    "Element 14 64-bit DSP Processor",
    "LSI Logic 16-bit DSP Processor",
    "Donald Knuth's educational 64-bit processor",
    "Harvard University machine-independent object files",
    "SiTera Prism",
    "Atmel AVR 8-bit microcontroller",
    "Fujitsu FR30",
    "Mitsubishi D10V",
    "Mitsubishi D30V",
    "NEC v850",
    "Mitsubishi M32R",
    "Matsushita MN10300",
    "Matsushita MN10200",
    "picoJava",
    "OpenRISC 32-bit embedded processor",
    "ARC Cores Tangent-A5",
    "Tensilica Xtensa Architecture",
    "Alphamosaic VideoCore processor",
    "Thompson Multimedia General Purpose Processor",
    "National Semiconductor 32000 series",
    "Tenor Network TPC processor",
    "Trebia SNP 1000 processor",
    "STMicroelectronics (www.st.com) ST200 microcontroller"
};

const char * elf_version[] = {
    "Invalid version",
    "Current version"
};

uint8_t parse_elf_file(uint8_t * buffer) {
    Elf64_Ehdr * header = (Elf64_Ehdr *) buffer;
    if (memcmp(header->e_ident, ELFMAG, SELFMAG) != 0) {
        printf("Invalid ELF magic\n");
        return 0;
    }

    printf("Valid elf header\n");
    return 1;
}

void readelf_32(uint8_t * buffer, uint64_t size) {
    printf("32 Bit elf header not supported\n");
}

void elf_readelf(uint8_t * buffer, uint64_t size) {
    if (!parse_elf_file(buffer)) return;

    Elf64_Ehdr * elf_header = (Elf64_Ehdr *) buffer;
    if (elf_header->e_ident[EI_CLASS] == ELFCLASS32) {
        readelf_32(buffer, size);
        return;
    }

    //readelf_64
    printf("ELF Header:\n");
    printf("  Magic: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", elf_header->e_ident[0], elf_header->e_ident[1], elf_header->e_ident[2], elf_header->e_ident[3], elf_header->e_ident[4], elf_header->e_ident[5], elf_header->e_ident[6], elf_header->e_ident[7], elf_header->e_ident[8], elf_header->e_ident[9], elf_header->e_ident[10], elf_header->e_ident[11], elf_header->e_ident[12], elf_header->e_ident[13], elf_header->e_ident[14], elf_header->e_ident[15]);
    printf("  Class: %s\n", elf_class[elf_header->e_ident[EI_CLASS]]);
    printf("  Data: %s\n", elf_data[elf_header->e_ident[EI_DATA]]);
    printf("  Version: %s\n", elf_version[elf_header->e_ident[EI_VERSION]]);
    printf("  OS/ABI: %s\n", elf_osabi[elf_header->e_ident[EI_OSABI]]);
    printf("  ABI Version: %d\n", elf_header->e_ident[EI_ABIVERSION]);
    printf("  Type: %s\n", elf_type[elf_header->e_type]);
    printf("  Machine: %s\n", elf_machine[elf_header->e_machine]);
    printf("  Version: 0x%x\n", elf_header->e_version);
    printf("  Entry point address: 0x%x\n", elf_header->e_entry);
    printf("  Start of program headers: %d (bytes into file)\n", elf_header->e_phoff);
    printf("  Start of section headers: %d (bytes into file)\n", elf_header->e_shoff);
    printf("  Flags: 0x%x\n", elf_header->e_flags);
    printf("  Size of this header: %d (bytes)\n", elf_header->e_ehsize);
    printf("  Size of program headers: %d (bytes)\n", elf_header->e_phentsize);
    printf("  Number of program headers: %d\n", elf_header->e_phnum);
    printf("  Size of section headers: %d (bytes)\n", elf_header->e_shentsize);
    printf("  Number of section headers: %d\n", elf_header->e_shnum);
    printf("  Section header string table index: %d\n", elf_header->e_shstrndx);
}

void allocate_segment(struct task * task, Elf64_Phdr * program_header, void * buffer) {
    if (program_header->p_type != PT_LOAD) return;

    uint64_t vaddr_offset = program_header->p_vaddr & 0xfff;
    uint64_t vaddr = program_header->p_vaddr & ~0xfff;

    uint64_t total_size = program_header->p_memsz + vaddr_offset;
    uint64_t page_no = total_size / 0x1000;
    if (total_size % 0x1000) page_no++;

    uint8_t perms = PAGE_USER_BIT;
    if (program_header->p_flags & PF_W) perms |= PAGE_WRITE_BIT;
    if (!(program_header->p_flags & PF_X)) perms |= PAGE_NX_BIT;

    void * complete_buffer = kmalloc(page_no * 0x1000);
    memset(complete_buffer, 0, page_no * 0x1000);
    memcpy(complete_buffer + vaddr_offset,  (void*)(buffer+program_header->p_offset), program_header->p_filesz);

    void * physical;
    for (uint64_t i = 0; i < page_no; i++) {
        void * partial_buffer = TO_KERNEL_MAP(request_page());
        physical = virtual_to_physical(get_pml4(), partial_buffer);
        map_memory(TO_KERNEL_MAP(task->pd), (void*)(vaddr + i * 0x1000), (void*)physical, perms);
        printf("Para el proceso padre %llx mapea a %llx\n", partial_buffer + i * 0x1000, physical);
        physical = virtual_to_physical(TO_KERNEL_MAP(task->pd), (void*)(vaddr + i * 0x1000));
        printf("Para el proceso hijo  %llx mapea a %llx\n", vaddr + i * 0x1000, physical);
        memset(partial_buffer, 0, 0x1000);
        memcpy(partial_buffer, complete_buffer + i * 0x1000, 0x1000);

        printf("Dump for %llx\n", program_header->p_vaddr);
        for (int i = 0; i < 100; i++) {
            if (i % 16 == 0) printf("\n");
            printf("%02x ", ((uint8_t*)partial_buffer)[i]);
        }
        printf("\n");
    }

    kfree(complete_buffer);    

}

uint8_t elf_load_elf(uint8_t * buffer, uint64_t size, void* env) {
    if (!parse_elf_file(buffer)) return 0;

    Elf64_Ehdr * elf_header = (Elf64_Ehdr *) buffer;
    if (elf_header->e_ident[EI_CLASS] == ELFCLASS32) {
        readelf_32(buffer, size);
        return 0;
    }

    //¿Es dinamico?
    if (elf_header->e_type == ET_DYN) {
        printf("Dynamic ELF not supported\n");
        return 0;
    }

    if (elf_header->e_type != ET_EXEC) {
        printf("Invalid ELF type\n");
        return 0;
    }

    Elf64_Phdr * program_header = (Elf64_Phdr *) (buffer + elf_header->e_phoff);


    for (int i = 0; i < elf_header->e_phnum; i++) {
        if (program_header[i].p_type == PT_LOAD) {
            if (elf_header->e_entry >= program_header[i].p_vaddr && elf_header->e_entry < program_header[i].p_vaddr + program_header[i].p_memsz) {
                printf("Spawning process at 0x%x\n", elf_header->e_entry);
                struct task * task = create_task((void*)elf_header->e_entry, "ttya\0", USER_TASK);
                if (!task) {
                    printf("Failed to create task\n");
                    return 0;
                }
                
                for (int i = 0; i < elf_header->e_phnum; i++) {
                    if (program_header[i].p_type == PT_LOAD) {
                        allocate_segment(task, &program_header[i], buffer);
                        printf("Loaded segment at 0x%x with size 0x%x and flags 0x%x\n", program_header[i].p_vaddr, program_header[i].p_memsz, program_header[i].p_flags);
                    }
                }

                if (!is_present(TO_KERNEL_MAP(task->pd), (void*)elf_header->e_entry)) {
                    printf("Entry point not present\n");
                    return 0;
                }

                if (!is_user_access(TO_KERNEL_MAP(task->pd), (void*)elf_header->e_entry)) {
                    printf("Entry point not user accessible\n");
                    return 0;
                }

                if (!is_executable(TO_KERNEL_MAP(task->pd), (void*)elf_header->e_entry)) {
                    printf("Entry point not executable\n");
                    return 0;
                }

                add_task(task);

                return 1;
            }
        }
    }
}