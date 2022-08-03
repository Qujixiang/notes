# 第一部分、二进制格式

## 1. 二进制剖析

系统运行的机器码被称作**二进制码**。包含二进制可执行程序的文件被称作**二进制可执行文件**。

### 1.1 C编译过程

C语言源代码转换为二进制文件需要四个步骤：**预处理**、**编译**、**汇编**、**链接**。

![C编译过程](assets/Practical-Binary-Analysis/image-20220702135143990.png)

#### 1.1.1 预处理阶段

为提高编码效率，C语言通过预处理程序提供了一些语言功能，经过预处理之后的源代码才能进行真正意义上的词法分析。预处理常以'#'开头。

预处理操作包括：

- 文件包含：将被包含的文件内容复制到此处。

  ```c
  #include <文件名> // 在编译器安装目录中搜索文件
  #include "文件名" // 在当前工作目录内搜索文件
  ```

  

- 宏定义：用比较简洁的字符串替换冗长不易识别的字符串。

  ```c
  #define 标识符 符号序列 // 使指定标识符的预处理生效
  #define 标识符(参数) 符号序列 // 带参数的宏定义
  #undef 标识符 // 使指定标识符的预处理失效，宏标识符将被视为无效信息
  ```

  

- 条件编译：让编译器有选择地忽略部分代码不进行编译。

  ```c
  #if 常量表达式 / #ifdef 标识符 / #ifndef 标识符
  文本
  #elif 常量表达式 / #else
  文本
  #endif
  ```

  

### 1.1.2 编译阶段

预处理阶段完成后，将对代码进行编译。编译阶段将预处理过的代码翻译为汇编语言代码，并且大部分编译器可以在此阶段按照优化等级对汇编代码进行优化，例如gcc的-O0到-O3。并可以指定汇编代码的格式，例如gcc默认使用AT&T语法格式的汇编代码，gcc的-masm=intel使用Intel语法格式的汇编代码。



### 1.1.3 汇编阶段

汇编阶段将汇编代码转换为**目标文件**或者**模块**。目标文件是**可重定位文件**，可重定位文件不需要放置在内存的特定位置，相反，它可以随意在内存中移动。



### 1.1.4 链接阶段

链接阶段将所有目标文件链接成一个二进制可执行文件，链接阶段有时还包含额外的优化过程，称作**链接时优化**(LTO, link-time optimization)。

在链接阶段之前，还不知道引用的代码和数据的地址，所以目标文件仅包含**重定位符号**，重定位符号指示了如何处理函数和变量的引用，依赖重定位符号的引用被称作**符号引用**。

链接器会解析大部分符号引用，对库的引用取决于库的类型。库分为**静态库**和**动态库/共享库**，静态库将被合并到可执行文件中，动态库只在内存中存在一份，所以所有程序共享动态库，链接阶段不能解析对动态库的符号引用，当可执行文件加载到内存时，**动态链接器**会解析对动态库的符号引用。



## 1.2 符号和二进制文件

### 1.2.1 查看符号信息

```bash
$ readelf --syms a.out
```



二进制格式和调试信息格式：

| Binary Format | Debugging Format| Difference |
| ------------- | ---- | ---- |
| ELF | DWARF | unsually embedded within the binary |
| PE | PDB | a separate symbol file |



调试符号信息：

- 源代码和二进制指令之间的映射
- 函数的参数
- 栈帧



PDB文件的信息：

- Public symbols (typically all functions, static and global variables)
- A list of object files that are responsible for sections of code in the executable
- Frame pointer optimization information (FPO)
- Name and type information for local variables and data structures
- Source file and line number information



stripped PDB文件的信息：

- Public symbols (typically only non-static functions and global variables)
- A list of object files that are responsible for sections of code in the executable
- Frame pointer optimization information (FPO)



### 1.2.2 去掉二进制文件不必要的符号信息

```bash
$ strip --strip-all a.out
```



## 1.3 反汇编二进制文件

### 1.3.1 查看目标文件

```bash
$ objdump -sj .rodata compilation_example.o              # 查看.rodata段的内容
$ objdump -M intel -d compilataion_example.o             # 以Intel的语法格式反汇编目标文件的代码
$ objdump --relocs compilataion_example.o                # 查看目标文件的重定位信息
```



### 1.3.2 解析二进制可执行文件

```bash
$ objdump -M intel -d a.out
```

去掉不必要的符号信息后的二进制文件保留了段，但是所 有函数都被合并到了代码段。



## 1.4 加载并执行二进制文件

![在Linux上加载ELF格式的二进制文件](assets/Practical-Binary-Analysis/image-20220710192640386.png)

运行二进制文件的过程：

1. 建立一个新进程
2. 操作系统映射一个解释器到进程虚拟内存
3. 内核将控制权交给解释器，解释器开始在用户空间工作
4. 解释器将二进制文件加载到虚拟内存空间
5. 解析二进制文件，找到二进制文件所用的动态库，并将其映射到虚拟内存空间
6. 重定位，将二进制文件代码段指向动态库的引用修改为正确的地址
7. 寻找二进制程序的入口，并将控制权交给二进制程序，二进制程序开始执行



## 2. ELF格式

ELF (Executable and Linkable Format，可执行和链接格式)，ELF包含四种类型的内容：

1. 一个**可执行头部(executable header)**
2. 一系列可选的**程序头(program header)**
3. 许多**节(section)**
4. 一系列可选的**节头(section header)**



**64位ELF二进制文件概览**

![64位ELF二进制文件概览](assets/Practical-Binary-Analysis/image-20220729094002496.png)



### 2.1 可执行头(Executable Header)

每个ELF文件都以可执行头开始，可执行头表明这是一个ELF格式文件，并表明了它是哪一类ELF文件，以及去哪里找到其它内容。



```bash
$ readelf -h a.out
ELF Header:
  Magic:   7f 45 4c 46 02 01 01 00 00 00 00 00 00 00 00 00
  Class:                             ELF64
  Data:                              2's complement, little endian
  Version:                           1 (current)
  OS/ABI:                            UNIX - System V
  ABI Version:                       0
  Type:                              DYN (Position-Independent Executable file)
  Machine:                           Advanced Micro Devices X86-64
  Version:                           0x1
  Entry point address:               0x1060
  Start of program headers:          64 (bytes into file)
  Start of section headers:          13992 (bytes into file)
  Flags:                             0x0
  Size of this header:               64 (bytes)
  Size of program headers:           56 (bytes)
  Number of program headers:         13
  Size of section headers:           64 (bytes)
  Number of section headers:         31
  Section header string table index: 30
```



#### ELF64_Ehdr

```c
#define EI_NIDENT	16

typedef struct
{
  unsigned char e_ident[EI_NIDENT];     /* Magic number and other info */
  Elf64_Half    e_type;                 /* Object file type */
  Elf64_Half    e_machine;              /* Architecture */
  Elf64_Word    e_version;              /* Object file version */
  Elf64_Addr    e_entry;                /* Entry point virtual address */
  Elf64_Off     e_phoff;                /* Program header table file offset */
  Elf64_Off     e_shoff;                /* Section header table file offset */
  Elf64_Word    e_flags;                /* Processor-specific flags */
  Elf64_Half    e_ehsize;               /* ELF header size in bytes */
  Elf64_Half    e_phentsize;            /* Program header table entry size */
  Elf64_Half    e_phnum;                /* Program header table entry count */
  Elf64_Half    e_shentsize;            /* Section header table entry size */
  Elf64_Half    e_shnum;                /* Section header table entry count */
  Elf64_Half    e_shstrndx;             /* Section header string table index */
} Elf64_Ehdr;
```

- e_ident：前四个字节为幻数(magic value)，表明这是一个ELF格式文件。e_ident各个位的含义:

| Name          | Value | Purpose                                                      |
| ------------- | ----- | ------------------------------------------------------------ |
| EI_MAG0       | 0     | 幻数，0x7f                                                   |
| EI_MAG1       | 1     | 幻数，'E'                                                    |
| EI_MAG2       | 2     | 幻数，'L'                                                    |
| EI_MAG3       | 3     | 幻数，'F'                                                    |
| EI_CLASS      | 4     | 32位或64位的目标文件                                         |
| EI_DATA       | 5     | 大端或小端编码                                               |
| EI_VERSION    | 6     | ELF文件头的版本号                                            |
| EI_OSABI      | 7     | ABI扩展标识符，0表示UNIX System V ABI，其它值表示使用了其它操作系统的扩展 |
| EI_ABIVERSION | 8     | ABI版本                                                      |
| EI_PAD        | 9     | 填充开始位，置0，保留                                        |

- e_type：表明文件类型，文件类型有以下几种：

| Name      | Value  | Meaning      |
| --------- | ------ | ------------ |
| ET_NONE   | 0      | 无文件类型   |
| ET_REL    | 1      | 重定位文件   |
| ET_EXEC   | 2      | 可执行文件   |
| ET_DYN    | 3      | 共享目标文件 |
| ET_CORE   | 4      | 核心文件     |
| ET_LOPROC | 0xff00 | 由处理器指定 |
| ET_HIPROC | 0xffff | 由处理器指定 |

- e_machine：表明该文件所需的机器架构。

| Name           | Value | Meaning                |
| -------------- | ----- | ---------------------- |
| EM_NONE        | 0     | 无架构                 |
| EM_M32         | 1     | AT&T WE 32100          |
| EM_SPARC       | 2     | SPARC                  |
| EM_386         | 3     | Intel架构              |
| EM_64K         | 4     | Motorola 68000         |
| EM_88K         | 5     | Motorola 88000         |
| EM_860         | 7     | Intel 80860            |
| EM_MIPS        | 8     | MIPS RS3000 Big-Endian |
| EM_MIPS_RS4_BE | 10    | MIPS RS4000 Big-Endian |
| RESERVED       | 11-16 | 保留备用               |

- e_version：表明目标文件的版本。1表示文件的原始格式，更高的数字可以用来表明扩展文件的格式。

| Name       | Value | Meaning  |
| ---------- | ----- | -------- |
| EV_NONE    | 0     | 非法版本 |
| EV_CURRENT | 1     | 当前版本 |

- e_entry：指出系统应该将控制权转交给的虚拟地址，然后进程开始执行。如果文件没有入口地址，则该项为0。

- e_phoff：Program Header Table在文件中的偏移量，如果文件没有Program Header Table，则该项为0。

- e_shoff：Section Header Table在文件中的偏移量，如果文件没有Section Header Table，则该项为0。

- e_flags：与处理器架构相关的标志位。对于x86架构，e_flags常设置为0，不用标志位。对于ARM架构，e_flags可以设置文件格式惯例、栈的组织方式等关于接口的额外细节信息并告知给嵌入式操作系统。

- e_ehsize：ELF Header的大小。

- e_phentsize：Program Header Table的每一项的大小。所有表项的大小相同。

- e_phnum：Program Header Table的项目数量。e_phentsize * e_phnum表明了Program Header Table的总大小。如果文件中没有Program Header Table，则e_phnum为0。

- e_shentsize：Section Header Table的每一项的大小。所有表项的大小相同。

- e_shnum：Section Header Table的项目数量。e_shentsize * e_shnum表明了Section Header Table的总大小。如果文件中没有Section Header Table，则e_shnum为0。

- e_shstrndx：Section Name String Table(.shstrtab)节在Section Header Table里的索引下标。Section Name String Table(.shstrtab)储存着所有节的名称。

```bash
$ readelf -x .shstrtab a.out

Hex dump of section '.shstrtab':
  0x00000000 002e7379 6d746162 002e7374 72746162 ..symtab..strtab
  0x00000010 002e7368 73747274 6162002e 696e7465 ..shstrtab..inte
  0x00000020 7270002e 6e6f7465 2e676e75 2e70726f rp..note.gnu.pro
  0x00000030 70657274 79002e6e 6f74652e 676e752e perty..note.gnu.
  0x00000040 6275696c 642d6964 002e6e6f 74652e41 build-id..note.A
  0x00000050 42492d74 6167002e 676e752e 68617368 BI-tag..gnu.hash
  0x00000060 002e6479 6e73796d 002e6479 6e737472 ..dynsym..dynstr
  0x00000070 002e676e 752e7665 7273696f 6e002e67 ..gnu.version..g
  0x00000080 6e752e76 65727369 6f6e5f72 002e7265 nu.version_r..re
  0x00000090 6c612e64 796e002e 72656c61 2e706c74 la.dyn..rela.plt
  0x000000a0 002e696e 6974002e 706c742e 676f7400 ..init..plt.got.
  0x000000b0 2e706c74 2e736563 002e7465 7874002e .plt.sec..text..
  0x000000c0 66696e69 002e726f 64617461 002e6568 fini..rodata..eh
  0x000000d0 5f667261 6d655f68 6472002e 65685f66 _frame_hdr..eh_f
  0x000000e0 72616d65 002e696e 69745f61 72726179 rame..init_array
  0x000000f0 002e6669 6e695f61 72726179 002e6479 ..fini_array..dy
  0x00000100 6e616d69 63002e64 61746100 2e627373 namic..data..bss
  0x00000110 002e636f 6d6d656e 7400              ..comment.
```



### 2.2 节头(Section Headers)

ELF二进制文件将代码和数据从逻辑上划分为多个连续的、不重叠的块，这种块称之为**节(section)**。每个节都由**节头(section header)**进行描述，节头指定了节的属性和位置。**节头表(section header table)**里保存了所有节的节头信息。

节头仅用于链接阶段，故不需要链接的ELF文件可以不要节头表。相似的，程序头表，也即段表，仅用于执行阶段，故不需要执行的ELF文件可以不要程序头表。



#### Elf64_Shdr

```c
typedef struct
{
  Elf64_Word    sh_name;                /* Section name (string tbl index) */
  Elf64_Word    sh_type;                /* Section type */
  Elf64_Xword   sh_flags;               /* Section flags */
  Elf64_Addr    sh_addr;                /* Section virtual addr at execution */
  Elf64_Off     sh_offset;              /* Section file offset */
  Elf64_Xword   sh_size;                /* Section size in bytes */
  Elf64_Word    sh_link;                /* Link to another section */
  Elf64_Word    sh_info;                /* Additional section information */
  Elf64_Xword   sh_addralign;           /* Section alignment */
  Elf64_Xword   sh_entsize;             /* Entry size if section holds table */
} Elf64_Shdr;
```

- sh_name：节的名称，它的值是节的名称在字符串表(string table, .strtab)中的索引。

- sh_type：节的类型。

| Name         | Value      | Meaning                                                      |
| ------------ | ---------- | ------------------------------------------------------------ |
| SHT_NULL     | 0          | 没有相关的节与之对应，其它字段的值未定义。                   |
| SHT_PROGBITS | 1          | 该节存储着程序定义的信息，它的格式和含义仅由程序定义。       |
| SHT_SYMTAB   | 2          | 符号表，存放符号信息。参见[Elf64_Sym](####Elf64_Dyn)。       |
| SHT_STRTAB   | 3          | 字符串表，存放符号名称和程序中用到的字符串。                 |
| SHT_RELA     | 4          | 重定位节，包含重定位入口。参见[Elf64_Rela](####Elf64_Rela)。 |
| SHT_HASH     | 5          | 这样的节中包含一个符号哈希表，参与动态链接的目标文件必须有一个哈希表。 |
| SHT_DYNAMIC  | 6          | 包含动态链接的信息。参见[Elf64_Dyn](####Elf64_Dyn)。         |
| SHT_NOTE     | 7          | 标记文件的信息。                                             |
| SHT_NOBITS   | 8          | 这种节不含任何字节，也不占用文件空间，节头中的**sh_offset**字段只是概念上的偏移 |
| SHT_REL      | 9          | 重定位节，包含重定位条目。参见[Elf64_Rel](####Elf64_Rel)。   |
| SHT_SHLIB    | 10         | 保留，语义未指定。                                           |
| SHT_DYNSYM   | 11         | 用于动态链接的符号表，是symbol table的子集。                 |
| SHT_LOPROC   | 0x70000000 | 保留，由处理器指定语义。                                     |
| SHT_HIPROC   | 0x7fffffff | 保留，由处理器指定语义。                                     |
| SHT_LOUSER   | 0x80000000 | 由应用程序指定语义。                                         |
| SHT_HIUSER   | 0xffffffff | 由应用程序指定语义。                                         |

- sh_flags：表明了节的属性。

| Name          | Value      | Meaning                  |
| ------------- | ---------- | ------------------------ |
| SHF_WRITE     | 0x1        | 该节包含可写的数据       |
| SHF_ALLOC     | 0x2        | 该节占用内存空间         |
| SHF_EXECINSTR | 0x4        | 该节包含可执行的机器指令 |
| SHF_MASKPROC  | 0xf0000000 | 高四位由处理器指定语义   |

- sh_addr：如果这个节会出现在进程的内存映像中，则该项指出了节的第一个字节的内存地址。否则，该项为0。

- sh_offset：这个节在文件中的偏移量。

- sh_size：节的大小。SHT_NOBITS类型的节的sh_size可能为0，但是它所占的实际空间为0。

- sh_link：它的值为Section Header Table的索引下标。它的含义由节的类型决定。

- sh_info：它保存了额外信息。它的含义由节的类型决定。

**sh_link**和**sh_info**的含义：

| sh_type                | sh_link                                | sh_info                  |
| ---------------------- | -------------------------------------- | ------------------------ |
| SHT_DYNAMIC            | 该节中的条目所使用的字符串表的节头索引 | 0                        |
| SHT_HASH               | 哈希表所适用的符号表的节头索引         | 0                        |
| SHT_REL、SHT_RELA      | 相关符号表的节头索引                   | 需要重定位的节的节头索引 |
| SHT_SYMTAB、SHT_DYNSYM | 由操作系统指定                         | 由操作系统指定           |
| other                  | SHN_UNDEF                              | 0                        |

- sh_addralign：表明了节需要对齐的字节数。0和1表示不对齐，其它的值必须为2的指数倍。

- sh_entsize：类似符号表等节拥有固定表项大小的表，该字段指明了表项的大小。0表示该节不含有固定表项大小的表。



### 2.3 节(Sections)

ELF文件由一系列的节组成，每一节都有不同的含义，负责不同的工作。

```bash
$ readelf --setions --wide a.out
There are 31 section headers, starting at offset 0x36a8:

Section Headers:
  [Nr] Name              Type            Address          Off    Size   ES Flg Lk Inf Al
  [ 0]                   NULL            0000000000000000 000000 000000 00      0   0  0
  [ 1] .interp           PROGBITS        0000000000000318 000318 00001c 00   A  0   0  1
  [ 2] .note.gnu.property NOTE            0000000000000338 000338 000030 00   A  0   0  8
  [ 3] .note.gnu.build-id NOTE            0000000000000368 000368 000024 00   A  0   0  4
  [ 4] .note.ABI-tag     NOTE            000000000000038c 00038c 000020 00   A  0   0  4
  [ 5] .gnu.hash         GNU_HASH        00000000000003b0 0003b0 000024 00   A  6   0  8
  [ 6] .dynsym           DYNSYM          00000000000003d8 0003d8 0000a8 18   A  7   1  8
  [ 7] .dynstr           STRTAB          0000000000000480 000480 00008f 00   A  0   0  1
  [ 8] .gnu.version      VERSYM          0000000000000510 000510 00000e 02   A  6   0  2
  [ 9] .gnu.version_r    VERNEED         0000000000000520 000520 000030 00   A  7   1  8
  [10] .rela.dyn         RELA            0000000000000550 000550 0000c0 18   A  6   0  8
  [11] .rela.plt         RELA            0000000000000610 000610 000018 18  AI  6  24  8
  [12] .init             PROGBITS        0000000000001000 001000 00001b 00  AX  0   0  4
  [13] .plt              PROGBITS        0000000000001020 001020 000020 10  AX  0   0 16
  [14] .plt.got          PROGBITS        0000000000001040 001040 000010 10  AX  0   0 16
  [15] .plt.sec          PROGBITS        0000000000001050 001050 000010 10  AX  0   0 16
  [16] .text             PROGBITS        0000000000001060 001060 000121 00  AX  0   0 16
  [17] .fini             PROGBITS        0000000000001184 001184 00000d 00  AX  0   0  4
  [18] .rodata           PROGBITS        0000000000002000 002000 00000f 00   A  0   0  4
  [19] .eh_frame_hdr     PROGBITS        0000000000002010 002010 00003c 00   A  0   0  4
  [20] .eh_frame         PROGBITS        0000000000002050 002050 0000cc 00   A  0   0  8
  [21] .init_array       INIT_ARRAY      0000000000003db8 002db8 000008 08  WA  0   0  8
  [22] .fini_array       FINI_ARRAY      0000000000003dc0 002dc0 000008 08  WA  0   0  8
  [23] .dynamic          DYNAMIC         0000000000003dc8 002dc8 0001f0 10  WA  7   0  8
  [24] .got              PROGBITS        0000000000003fb8 002fb8 000048 08  WA  0   0  8
  [25] .data             PROGBITS        0000000000004000 003000 000010 00  WA  0   0  8
  [26] .bss              NOBITS          0000000000004010 003010 000008 00  WA  0   0  1
  [27] .comment          PROGBITS        0000000000000000 003010 000026 01  MS  0   0  1
  [28] .symtab           SYMTAB          0000000000000000 003038 000378 18     29  18  8
  [29] .strtab           STRTAB          0000000000000000 0033b0 0001dd 00      0   0  1
  [30] .shstrtab         STRTAB          0000000000000000 00358d 00011a 00      0   0  1
Key to Flags:
  W (write), A (alloc), X (execute), M (merge), S (strings), I (info),
  L (link order), O (extra OS processing required), G (group), T (TLS),
  C (compressed), x (unknown), o (OS specific), E (exclude),
  D (mbind), l (large), p (processor specific)
```



#### 特殊的节

| Name        | Type           | Attributes                 | Meaning                                                      |
| ----------- | -------------- | -------------------------- | ------------------------------------------------------------ |
| .bss        | SHT_NOBITS     | SHF_ALLOC+SHF_WRITE        | 存放未初始化的全局变量                                       |
| .comment    | SHT_PROGBITS   | none                       | 版本控制信息                                                 |
| .data       | SHT_PROGBITS   | SHF_ALLOC + SHF_WRITE      | 存放初始化过的全局变量                                       |
| .data1      | SHT_PROGBITS   | SHF_ALLOC + SHF_WRITE      | 存放初始化过的全局变量                                       |
| .debug      | SHT_PROGBITS   | none                       | 符号调试信息                                                 |
| .dynamic    | SHT_DYNAMIC    | SHF_ALLOC( + SHF_WRITE)    | 存放动态链接信息                                             |
| .dynsym     | SHT_DYNSYM     | SHF_ALLOC                  | 存放动态链接用的符号信息                                     |
| .dynstr     | SHT_STRTAB     | SHF_ALLOC                  | 存放动态链接用的符号名称                                     |
| .hash       | SHT_HASH       | SHF_ALLOC                  | 符号哈希表                                                   |
| .line       | SHT_PROGBITS   | none                       | 符号调试的行号信息                                           |
| .note       | SHT_NOTE       | none                       |                                                              |
| .rodata     | SHT_PROGBITS   | SHF_ALLOC                  | 存放只读数据                                                 |
| .rodata1    | SHT_PROGBITS   | SHF_ALLOC                  | 存放只读数据                                                 |
| .shstrtab   | SHT_STRTAB     | none                       | 存放节的名称                                                 |
| .strtab     | SHT_STRTAB     | (SHF_ALLOC)                | 存放字符串，主要存放符号名                                   |
| .symtab     | SHT_SYMTAB     | (SHF_ALLOC)                | 符号表，存放符号描述信息                                     |
| .text       | SHT_PROGBITS   | SHF_ALLOC \+ SHF_EXECINSTR | 存放机器指令                                                 |
| .interp     | SHT_PROGBITS   | SHF_ALLOC                  | 存放程序解释器的路径                                         |
| .plt        | SHT_PROGBITS   | SHF_ALLOC \+ SHF_EXECINSTR | 对外部函数的调用将指向该段的同名函数，以实现动态链接的懒加载 |
| .got        | SHT_PROGBITS   | SHF_ALLOC+SHF_WRITE        | 存放外部变量引用信息                                         |
| .got.plt    | SHT_PROGBITS   | SHF_ALLOC+SHF_WRITE        | 存放外部函数引用信息                                         |
| .rel.*      | SHT_RELA       | SHF_ALLOC                  | 重定位表，存放重定位信息                                     |
| .rela.*     | SHT_REL        | SHF_ALLOC                  | 重定位表，存放重定位信息                                     |
| .init       | SHT_PROGBITS   | SHF_ALLOC \+ SHF_EXECINSTR | 程序执行前的初始化代码                                       |
| .fini       | SHT_PROGBITS   | SHF_ALLOC \+ SHF_EXECINSTR | 程序执行完毕后的析构代码                                     |
| .init_array | SHT_INIT_ARRAY | SHF_ALLOC + SHF_WRITE      | 作为数据段存放自定义初始化函数的指针                         |
| .fini_array | SHT_FINI_ARRAY | SHF_ALLOC + SHF_WRITE      | 作为数据段存放自定义析构函数的指针                           |



#### 符号表(Symbol Table)

##### Elf64_Sym

```c
typedef struct elf64_sym {
  Elf64_Word st_name;		/* Symbol name, index in string tbl */
  unsigned char	st_info;	/* Type and binding attributes */
  unsigned char	st_other;	/* No defined meaning, 0 */
  Elf64_Half st_shndx;		/* Associated section index */
  Elf64_Addr st_value;		/* Value of the symbol */
  Elf64_Xword st_size;		/* Associated symbol size */
} Elf64_Sym;
```

- st_name：符号名，在字符串表的索引。

- st_info：包含符号类型和绑定信息。低4位为类型信息，其余高位为绑定信息。

```c
#define ELF_ST_BIND(x)		((x) >> 4)
#define ELF_ST_TYPE(x)		(((unsigned int) x) & 0xf)
```

**符号绑定**

| Name       | Value | Meaning                              |
| ---------- | ----- | ------------------------------------ |
| STB_LOCAL  | 0     | 局部符号，仅目标文件内部可见         |
| STB_GLOBAL | 1     | 全局符号，所有链接的目标文件都可见   |
| STB_WEAK   | 2     | 弱符号类似于全局符号，但其优先级较低 |

**符号类型**

| Name        | Value | Meaning                                                      |
| ----------- | ----- | ------------------------------------------------------------ |
| STT_NOTYPE  | 0     | 未指定符号类型。                                             |
| STT_OBJECT  | 1     | 此符号与变量、数组等数据对象关联。                           |
| STT_FUNC    | 2     | 此符号与函数或其他可执行代码关联。                           |
| STT_SECTION | 3     | 此符号与节关联。此类型的符号表各项主要用于重定位，并且通常具有 `STB_LOCAL` 绑定。 |
| STT_FILE    | 4     | 通常，符号的名称会指定与目标文件关联的源文件的名称。文件符号具有 `STB_LOCAL` 绑定和节索引 `SHN_ABS`。 |
| STT_COMMON  | 5     | 此符号标记未初始化的通用块。此符号的处理与 `STT_OBJECT` 的处理完全相同。 |
| STT_TLS     | 6     | 此符号指定线程局部存储实体。定义后，此符号可为符号指明指定的偏移，而不是实际地址。 |

- st_other：未定义，置0。

- st_shndx：符号所在节的节头索引。

**特殊的索引:**

| Name       | Value  | Meaning                                                      |
| ---------- | ------ | ------------------------------------------------------------ |
| SHN_UNDEF  | 0      | 此节表索引表示未定义符号。链接器将此目标文件与已定义该符号的另一目标文件合并时，此文件中对该符号的引用将与该定义绑定。 |
| SHN_ABS    | 0xfff1 | 此符号具有不会由于重定位而发生更改的绝对值。                 |
| SHN_COMMON | 0xfff2 | 此符号标记尚未分配的通用块。与节的 `sh_addralign` 成员类似，符号的值也会指定对齐约束。链接器在值为 `st_value` 的倍数的地址为符号分配存储空间。符号的大小会指明所需的字节数。 |

- st_value：具体含义取决于上下文，可能是一个绝对值、一个地址等。

  - 在可重定位文件中，若`st_shndx`为`SHN_COMMON`，则`st_value`为对齐约束值。

  - 在可重定位文件中，若`st_shndx`为正常的索引值，则`st_value`为该符号在`st_shndx`所标识的节中的偏移量。

  - 在可执行文件和共享文件中，`st_value`为虚拟地址。为使这些文件的符号更适用于运行时链接程序，所以节偏移替换为虚拟地址。


- st_size：符号所表示的对象的大小。



#### 重定位表(Relocation Table)

重定位是将符号引用与符号定义联系起来的过程。例如，当一个程序调用一个函数时，相关的调用指令必须在执行时将控制权转移到适当的目标地址。换句话说，可重定位文件必须有描述如何修改其部分内容的信息，从而使可执行文件和共享对象文件能够为进程映像持有正确的信息。重定位表就是这些信息。

##### Elf64_Rel

```c
typedef struct
{
  Elf64_Addr	r_offset;		/* Address */
  Elf64_Xword	r_info;			/* Relocation type and symbol index */
} Elf64_Rel;
```



##### Elf64_Rela

```c
typedef struct elf64_rela {
  Elf64_Addr r_offset;	/* Location at which to apply the action */
  Elf64_Xword r_info;	/* index and type of relocation */
  Elf64_Sxword r_addend;	/* Constant addend used to compute value */
} Elf64_Rela;
```

- r_offset：这个成员给出了重定位操作的位置。对于一个可重定位的文件，该值为从节的开始到需要重定位的存储单元的字节偏移。对于可执行文件或共享目标，该值是需要重定位的存储单元的虚拟地址。对于不同的文件类型，`r_offset`有不同的含义：

  - 在可重定位文件中，`r_offset`保存了一个在节中的偏移值。即，重定位节本身描述了如何修改另一个节，`r_offset`指明了修改另一个节中的哪个存储单元。

  - 在可执行文件和共享目标文件中，为了让重定位的条目对动态链接器更有用，所以`r_offset`保存了一个虚拟地址。


- r_info：此成员指定必须对其进行重定位的符号表索引以及要应用的重定位类型。例如，调用指令的重定位项包含所调用的函数的符号表索引。如果索引是未定义的符号索引 `STN_UNDEF`，则重定位将使用零作为符号值。可以使用`ELF64_R_TYPE` 或 `ELF64_R_SYM`获取重定位类型和符号。

```c
#define ELF64_R_SYM(i)			((i) >> 32)
#define ELF64_R_TYPE(i)			((i) & 0xffffffff)
```

- r_addend：此成员指定常量加数，用于计算将存储在可重定位字段中的值。



#### 动态节(.dynamic)

如果目标文件参与动态链接，则其程序头表将包含一个类型为 `PT_DYNAMIC` 的元素。此段包含 `.dynamic` 节。特殊符号 `_DYNAMIC` 用于标记包含以下结构的数组的节。此段包含了程序所需的依赖项，以及动态链接所需的GOT、PLT、符号哈希表、字符串表、符号表、重定位表等信息。

##### Elf64_Dyn

```c
typedef struct {
  Elf64_Sxword d_tag;		/* entry tag value */
  union {
    Elf64_Xword d_val;
    Elf64_Addr d_ptr;
  } d_un;
} Elf64_Dyn;
```

d_un的具体含义取决于d_tag，d_tag表示各个表项的类型。



#### 符号哈希表(Symbol Hash Table)

符号哈希表根据符号名通过哈希函数快速查找符号在符号表中的位置。

哈希表的结构如下：

```
+---------------------------------------------+
|                   nbucket                   |
|---------------------------------------------|
|                   nchain                    |
|---------------------------------------------|
|                  bucket[0]                  |
|                    ...                      |
|              bucket[nbucket-1]              |
|---------------------------------------------|
|                  chain[0]                   |
|                    ...                      |
|               chain[nchain-1]               |
+---------------------------------------------+
```

`bucket`数组共有`nbucket`项，`chain`数组共有`nchain`项。

符号哈希表的工作原理如下：

1. 给定`symbol name`；
2. 通过`hash function`算出哈希值`x`；
3. 令`y=bucket[x%nbucket]`；
4. 将`y`当作符号表的下标查看符号表条目是否正确，若正确，则查找完毕；若不正确，则令`y=chain[y]`。重复步骤4，直至找到正确的符号表条目或者条目的值为`STN_UNDEF`。



#### 全局偏移表(Global Offset Table)

装载时重定位是解决动态模块中含有绝对地址引用的办法之一，但是代码无法在多个进程之间共享，因为不同进程的地址空间不同，重定位后的代码所包含的绝对地址只对一个程序适用，对其它程序不适用，这样就失去了动态链接节省内存的优势。所以我们希望程序模块中共享的指令部分在装载时不需要因为装载地址的改变而改变，意在将指令中需要被修改的部分分离出来，和数据部分放在一起，这样指令部分就可以保持不变，而数据部分可以在每个进程中拥有一个副本，这种方案目前就被称为**地址无关代码(PIC, Position Independent Code)**的技术。

为了实现地址无关代码，可将指令分成四种类型：

- 模块内函数调用或跳转：通过相对地址调用。
- 模块内数据访问：通过`__i686.get_pc_thunk.cx`获取当前`PC`值，并加上一个偏移量，从而达到数据访问的”相对寻址“。
- 模块间数据访问：在数据段里建一个指向外部变量的指针数组，也被称为**全局偏移表(GOT, Global Offset Table)**，当代码需要引用该外部变量时，可以通过`GOT`中对应的项间接引用。
- 模块间函数调用或跳转：通过`__i686.get_pc_thunk.cx`获取当前`PC`值，并加上一个偏移量，从而得到外部函数地址在`GOT`中的偏移，然后间接调用。

`.got`保存的是对外部数据的引用，`.got.plt`保存的是对外部函数的引用。

`.got.plt`的前三项是有特殊含义的：

- `.got.plt[0]`: 本`ELF`的动态段(`.dynamic`)的内存地址
- `.got.plt[1]`: 本`ELF`的`link_map`数据结构描述符地址
- `.got.plt[2]`: `_dl_runtime_resolve`函数的地址

第二项和第三项由动态链接器在装载共享模块的时候负责将它们初始化。

`GOT`用于存放外部变量或函数的地址。初始时`GOT`存放的是跳转到`GOT`的那条指令的下一条指令地址，当动态链接器把外部变量或函数所在的模块装入进程内存地址空间后，修改`GOT`中对应的项，将其改为外部变量或函数的实际地址。`GOT`作为数据具有读写权限，而代码段通过相对地址的方式访问`GOT`以访问外部变量或函数，从而避免了在代码段直接使用绝对地址，从此就实现了`PIC`。



#### 延迟加载(Lazy Binding) & 过程链接表(Procedure Linkage Table)

动态链接虽然比静态链接产生的文件更小，更节省内存，但是它是以牺牲一部分性能为代价的。动态链接对于全局变量和外部数据的访问都要先进行`GOT`定位，然后间接寻址，对于模块间的定位也要先定位`GOT`，然后再进行间接跳转，如此一来，程序的运行速度必然减慢；另一个减慢速度的原因是在装载程序时，动态链接器都要进行一次链接工作，寻找并装载所需要的共享目标，然后进行符号查找和地址重定位等工作。这些原因影响着动态链接的性能。

一个程序中的很多函数可能并不会在程序刚执行时就用到，所以不必一开始就将所有函数都链接好，可以等到函数第一次被用到时才进行绑定，这种技术叫做**延迟绑定(Lazy Binding)**。ELF使用**过程链接表(PLT, Procedure Linkage Table)**来实现延迟绑定。

延迟绑定的流程：

1. 将所需要动态链接的符号在`.rel.plt`段的下标压入栈中
2. 将模块的`link_map`数据结构描述符压入栈
3. 调用动态链接器的`_dl_runtime_resolve`函数完成符号解析和重定位工作
4. `_dl_runtime_resolve`将实际地址填入到`ext_fun@got`中

```assembly
# PLT段伪代码如下：
.plt
common@plt:
  push *(GOT + 8)  # 将GOT[1]压入栈中，即本模块的link_map描述符
  jmp *(GOT + 16)  # 跳转到GOT[2]即_dl_runtime_resolve执行

ext_fun@plt:
  jmp *(ext_fun@got)
  push <ext_fun符号在.rel.plt中的下标> # 第一次调用该函数会执行以下两条指令，以后调用将不会执行以下两条指令
  jmp common@plt
  
# 动态链接前GOT段伪代码如下:
.got.plt
ext_fun@got:
  <jmp到该地址的指令的下一条指令的地址> # _dl_runtime_resolve将ext_fun的实际地址填入到这里
  
# 动态链接后GOT段伪代码如下:
.got.plt
ext_fun@got:
  <ext_fun的实际地址>
```



### 2.4 程序头(Program Headers)

与节头表(Section Header Table)把二进制文件看作是节(section)的组合相反，程序头表(Program Header Table)把二进制文件看作是段(segment)的组合。节仅用于静态链接，而段用于操作系统和动态链接器执行ELF文件，并定位相关代码和数据，以及决定把哪些段装载到虚拟内存空间中。ELF文件头中的`e_phoff`指出了程序头表的位置，`e_phentsize`指出了程序头表的表项的大小，`e_phnum`指出了程序头表的表项的个数。

由于段提供了执行视图，所以段仅用于可执行文件，而不用于不可执行的文件，比如需要重定位的目标文件。

程序头表使用`Elf64_Phdr`结构体表示程序头表的项目。

#### Elf64_Phdr

```c
typedef struct
{
  Elf64_Word	p_type;			/* Segment type */
  Elf64_Word	p_flags;		/* Segment flags */
  Elf64_Off	    p_offset;		/* Segment file offset */
  Elf64_Addr	p_vaddr;		/* Segment virtual address */
  Elf64_Addr	p_paddr;		/* Segment physical address */
  Elf64_Xword	p_filesz;		/* Segment size in file */
  Elf64_Xword	p_memsz;		/* Segment size in memory */
  Elf64_Xword	p_align;		/* Segment alignment */
} Elf64_Phdr;
```

- p_type：这个字段表明了这个段的类型，或者如何解释这个段的信息。

| Name       | Value | Meaning                                                      |
| ---------- | ----- | ------------------------------------------------------------ |
| PT_NULL    | 0     | 未用                                                         |
| PT_LOAD    | 1     | 表示这是一个可装载的段                                       |
| PT_DYNAMIC | 2     | 表示这个段含有动态链接信息                                   |
| PT_INTERP  | 3     | 指定了解释器(interpreter)的路径                              |
| PT_NOTE    | 4     | 指定了辅助信息的位置和大小                                   |
| PT_SHLIB   | 5     | 保留                                                         |
| PT_PHDR    | 6     | 说明了程序头表在文件和内存映像中的位置和大小。该表项优先于其它可装载段表项 |

- p_flags：与段权限相关的标志。

| Name        | Value      | Meaning        |
| ----------- | ---------- | -------------- |
| PF_X        | 1          | 可执行         |
| PF_W        | 2          | 可写           |
| PF_R        | 4          | 可读           |
| PF_MASKOS   | 0x0ff00000 | 由操作系统指定 |
| PF_MASKPROC | 0xf0000000 | 由处理器指定   |

- p_offset：此段在文件中的偏移量。

- p_vaddr：此段在内存中的虚拟地址。
- p_paddr：此段在与物理寻址相关的系统中的物理地址。
- p_filesz：此段在文件中的大小。
- p_memsz：此段在内存中的大小。
- p_align：可装入的进程段必须具有 `p_vaddr` 和 `p_offset` 的同余值（以页面大小为模数）。此成员可提供一个值，用于在内存和文件中根据该值对齐各段。值 `0` 和 `1` 表示无需对齐。另外，`p_align` 应为 `2` 的正整数幂，并且 `p_vaddr` 应等于 `p_offset`（以 `p_align` 为模数）。



## 3. PE格式

PE格式用于Windows，是COFF格式的变体，所以PE也被称作PE/COFF，在被ELF取代前，一直被*nix使用。64位版本的PE叫做PE32+，与原PE格式区别很小。因为PE和ELF都源于COFF，所以PE和ELF有很多相似的地方。



**PE32+格式**：

![PE32+格式](assets/Practical-Binary-Analysis/image-20220730152624387.png)

### 3.1 MS-DOS Header & Stub

Microsoft为了兼容DOS，保留了MS-DOS Header。当PE刚问世的时候，为了让用户更清晰地从MS-DOS二进制格式过渡到PE格式，Microsoft让每个PE文件都以MS-DOS头作为开始，所以从狭义上讲，PE文件可以视为MS-DOS文件。MS-DOS Header最主要地作用就是描述如何加载并执行MS-DOS Stub。在MS-DOS环境下，用户执行PE文件时，运行的不是PE的主程序，而是MS-DOS stub里的指令。MS-DOS Header中的`e_lfanew`字段指出了PE Header在文件中的位置。



### 3.2 PE/COFF头(PE/COFF Header)

PE/COFF Header分为三个部分：

- PE signature
- PE file header
- PE optional header



#### 3.2.1 PE签名(PE Signature)

与ELF Header中的`e_ident`中的幻数(magic value)相似，PE Signature的值为“PE\0\0"。表明这是一个PE格式文件。



#### 3.2.2 PE文件头(PE File Header)

PE File Header描述了文件的通用属性。



##### IMAGE_FILE_HEADER

```c
typedef struct _IMAGE_FILE_HEADER {
      WORD Machine;
      WORD NumberOfSections;
      DWORD TimeDateStamp;
      DWORD PointerToSymbolTable;
      DWORD NumberOfSymbols;
      WORD SizeOfOptionalHeader;
      WORD Characteristics;
} IMAGE_FILE_HEADER,*PIMAGE_FILE_HEADER;
```

- Machine：类似ELF的`e_machine`，说明运行该文件需要的机器架构。
- NumberOfSections：节头表中表项的数量。
- TimeDateStamp：文件创建时间戳。
- PointerToSymbolTable：符号表在文件中的偏移量。
- NumberOfSymbols：符号表中表项的数量。
- SizeOfOptionalHeader：PE Optional Header的大小。
- Characteristics：描述了二进制文件的编码方式（大端 or 小端），是否是动态链接库，是否被去除了与执行无关的信息。



#### 3.2.3 PE可选头(PE Optional Header)

尽管名字里带有Optional，但是对于可执行文件来说，这部分是必须的，对于目标文件来说，这部分可以没有。



##### IMAGE_OPTIONAL_HEADER64

```c
typedef struct _IMAGE_OPTIONAL_HEADER64 {
      WORD Magic;                       // 机器型号，判断是32位或64位
      BYTE MajorLinkerVersion;          // 链接器主要版本号
      BYTE MinorLinkerVersion;          // 链接器次要版本号
      DWORD SizeOfCode;                 // 代码节的总大小
      DWORD SizeOfInitializedData;      // 已初始化数据节的大小
      DWORD SizeOfUninitializedData;    // 未初始化数据节的大小
      DWORD AddressOfEntryPoint;        // 程序入口地址(相对虚拟地址)
      DWORD BaseOfCode;                 // 代码节的基地址(相对虚拟地址)
      ULONGLONG ImageBase;              // 程序希望的基地址
      DWORD SectionAlignment;           // 内存中的节对齐
      DWORD FileAlignment;              // 文件中的节对齐
      WORD MajorOperatingSystemVersion; // 操作系统主版本号
      WORD MinorOperatingSystemVersion; // 操作系统次版本号
      WORD MajorImageVersion;           // PE主版本号
      WORD MinorImageVersion;           // PE次版本号
      WORD MajorSubsystemVersion;       // 子系统主版本号
      WORD MinorSubsystemVersion;       // 子系统次版本号
      DWORD Win32VersionValue;          // 32位系统版本号值
      DWORD SizeOfImage;                // 程序在内存中占用的大小
      DWORD SizeOfHeaders;              // MS-DOS stub, PE头和节头之和向上取整到FileAlignment的整数倍
      DWORD CheckSum;                   // 校验和
      WORD Subsystem;                   // 文件的子系统
      WORD DllCharacteristics;          // Dll文件属性
      ULONGLONG SizeOfStackReserve;     // 预留的栈的大小
      ULONGLONG SizeOfStackCommit;      // 立即申请的栈的大小
      ULONGLONG SizeOfHeapReserve;      // 预留的堆的大小
      ULONGLONG SizeOfHeapCommit;       // 立即申请的堆的大小
      DWORD LoaderFlags;                // 调试相关的标志位
      DWORD NumberOfRvaAndSizes;        // 数据目录结构的项目数量
      IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES]; // 数据目录
} IMAGE_OPTIONAL_HEADER64,*PIMAGE_OPTIONAL_HEADER64;
```



#### 3.2.4 节(Sections)

一些节与ELF文件中的节的作用和属性相似，比如`.text`, `.code`, `.bss`等。还有`.reloc`同`.rel`一样是重定位节，`.rdata`同`.rodata`是只读数据段。

PE特有而ELF没有的节中比较重要的是`.edata`和`.idata`。`DataDirectory`中的`Export Directory`和`Import Directory`指向这两个节。`.idata`节说明了二进制文件从共享库导入了哪些符号，`.edata`节说明了二进制文件导出了哪些符号。因此，当解析外部符号引用时，会先检查共享库是否导出了该符号。这两节有时也会被合并到`.rdata`节。

当加载器解析了依赖库之后，它会把解析的地址写到**导入地址表(IAT, Import Address Table)**中。`IAT`是`.idata`的一部分，初始时指向导入符号的名称或标识号，之后动态加载器会把它们替换成真实的指向导入函数或数据的指针。对外部函数的调用会被转换为对一个`thunk`的调用，`thunk`是对该外部函数的通过`IAT`的间接跳转。跳转的地址都存放在`.idata`的`Import Directory`中。

下例中的代码是用`MinGW`编译，所以使用了`nop`指令填充，使得代码以8字节对齐，从而使得访问时只需要一次访存即可获取整个指令。而用`MSVC`编译后的PE文件会用`int3`指令填充，该指令用于调试，在没有调试器而执行时会导致程序崩溃，但是`jmp`指令跳转到其它地方，而不会执行其之后的`int3`指令，所以可以用`int3`指令在这里填充。

```bash
$ objdump -M intel -d 3_1_pe_header.exe

...

0000000000402a40 <vfprintf>:
  402a40:       ff 25 16 59 00 00       jmp    QWORD PTR [rip+0x5916]        # 40835c <__imp_vfprintf>
  402a46:       90                      nop
  402a47:       90                      nop

0000000000402a48 <strncmp>:
  402a48:       ff 25 06 59 00 00       jmp    QWORD PTR [rip+0x5906]        # 408354 <__imp_strncmp>
  402a4e:       90                      nop
  402a4f:       90                      nop

0000000000402a50 <strlen>:
  402a50:       ff 25 f6 58 00 00       jmp    QWORD PTR [rip+0x58f6]        # 40834c <__imp_strlen>
  402a56:       90                      nop
  402a57:       90                      nop

0000000000402a58 <signal>:
  402a58:       ff 25 e6 58 00 00       jmp    QWORD PTR [rip+0x58e6]        # 408344 <__imp_signal>
  402a5e:       90                      nop
  402a5f:       90                      nop

0000000000402a60 <puts>:
  402a60:       ff 25 d6 58 00 00       jmp    QWORD PTR [rip+0x58d6]        # 40833c <__imp_puts>
  402a66:       90                      nop
  402a67:       90                      nop
 
 ...
 
 $ objdump -h 3_1_pe_header.exe
 
3_1_pe_header.exe:     file format pei-x86-64

Sections:
Idx Name          Size      VMA               LMA               File off  Algn
  0 .text         00001cc8  0000000000401000  0000000000401000  00000400  2**4
                  CONTENTS, ALLOC, LOAD, READONLY, CODE, DATA
  1 .data         000000d0  0000000000403000  0000000000403000  00002200  2**4
                  CONTENTS, ALLOC, LOAD, DATA
  2 .rdata        000004d0  0000000000404000  0000000000404000  00002400  2**5
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  3 .pdata        00000270  0000000000405000  0000000000405000  00002a00  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  4 .xdata        000001f4  0000000000406000  0000000000406000  00002e00  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  5 .bss          00000980  0000000000407000  0000000000407000  00000000  2**5
                  ALLOC
  6 .idata        0000076c  0000000000408000  0000000000408000  00003000  2**2
                  CONTENTS, ALLOC, LOAD, DATA
  7 .CRT          00000068  0000000000409000  0000000000409000  00003800  2**3
                  CONTENTS, ALLOC, LOAD, DATA
  8 .tls          00000010  000000000040a000  000000000040a000  00003a00  2**3
                  CONTENTS, ALLOC, LOAD, DATA
  9 .debug_aranges 00000050  000000000040b000  000000000040b000  00003c00  2**4
                  CONTENTS, READONLY, DEBUGGING
 10 .debug_info   00001f08  000000000040c000  000000000040c000  00003e00  2**0
                  CONTENTS, READONLY, DEBUGGING
 11 .debug_abbrev 00000149  000000000040e000  000000000040e000  00005e00  2**0
                  CONTENTS, READONLY, DEBUGGING
 12 .debug_line   00000222  000000000040f000  000000000040f000  00006000  2**0
                  CONTENTS, READONLY, DEBUGGING
 13 .debug_frame  00000048  0000000000410000  0000000000410000  00006400  2**3
                  CONTENTS, READONLY, DEBUGGING
 14 .debug_str    0000009b  0000000000411000  0000000000411000  00006600  2**0
                  CONTENTS, READONLY, DEBUGGING
```



## 4. 用libbfd构建二进制加载器

BFD是Binary format descriptor的缩写，即二进制文件格式描述符，是很多可执行文件相关二进制工具（如nm、objdump、ar、as等命令）的基础库。bfd库可以用来分析、创建、修改二进制文件，支持多种平台（如x86、arm等）及多种二进制格式（如elf、core、so等）。



### 4.1 二进制加载接口

首先创建一个头文件，定义相关的类和函数。引入要使用的头文件，并且定义3个类名。定义一个Binary类，作为整个二进制文件的抽象，定义Section和Symbol类，作为节和符号的抽象。

```c
#include <stdint.h>
#include <string>
#include <vector>

class Symbol;
class Section;
class Binary;
```



### 4.2 Symbol

Symbol类与二进制文件的符号相关，ELF文件中的符号表包括局部和全局变量、函数、重定位表达式及对象等，此处只解析函数符号。

```c
class Symbol {
    public:
        enum SymbolType {
            SYM_TYPE_UKN = 0,
            SYM_TYPE_FUNC = 1
        };
        
        Symbol(): type(SYM_TYPE_UKN), addr(0) {}

        SymbolType type;
        std::string name;
        uint64_t addr;
};
```



### 4.3 Section

Section类围绕二进制文件的节进行实现。例如Linux上的ELF格式文件，由一系列二进制节组织而成。使用readelf都会显示相关的基本信息，包括节头表里的索引、节的名称和类型，除此以外，还可以查看节的虚拟地址、文件偏移及大小、节的标志等等信息。

```c
class Section {
    public:
        enum SectionType {
            SEC_TYPE_NONE = 0,
            SEC_TYPE_CODE = 1,
            SEC_TYPE_DATA = 2
        };

        Section(): binary(NULL), type(SEC_TYPE_NONE), vma(0), size(0), bytes(NULL) {}
    
        Binary *binary;
        std::string name;
        SectionType type;
        uint64_t vma;
        uint64_t size;
        uint8_t *bytes;
};
```



### 4.4 Binary

这个类作为整个二进制文件的抽象，其中包含了二进制文件的文件名、类型、平台架构、位宽、入口点地址、节及符号。

```c
class Binary {
    public:
        enum BinaryType {
            BIN_TYPE_AUTO = 0,
            BIN_TYPE_ELF = 1,
            BIN_TYPE_PE = 2
        };

        enum BinaryArch {
            ARCH_NONE = 0,
            ARCH_X86 = 1
        };

        Binary(): type(BIN_TYPE_AUTO), arch(ARCH_NONE), bits(0), entry(0) {}

        std::string filename;
        BinaryType type;
        std::string type_str;
        BinaryArch arch;
        std::string arch_str;
        unsigned bits;
        uint64_t entry;
        std::vector<Section> sections;
        std::vector<Symbol> symbols;
};
```



### 4.5 加载二进制文件

定义加载器的两个入口函数`load_binary`和`unload_binary`函数。用于加载二进制文件，加载完毕后释放二进制文件的内存。`load_binary`解析由文件名指定的二进制文件，并将其加载到`Binary`对象中，其中调用了`load_binary_bfd`函数，将在后续实现。

`unload_binary`负责释放资源，实际上就是将`Binary`中`malloc`出的内存空间都释放。每个`Section`对象都要开辟一段空间来保存原始字节，将`bytes`成员释放掉即可。

```c
int load_binary(std::string &fname, Binary *bin, Binary::BinaryType type) {
    return load_binary_bfd(fname, bin, type);
}

void unload_binary(Binary *bin) {
    size_t i;
    Section *sec;

    for (i = 0; i < bin->sections.size(); i++) {
        sec = &bin->sections[i];
        if (sec->bytes) {
            free(sec->bytes);
        }
    }
}
```



### 4.6 打开二进制文件

如下实现了一个`open_bfd`函数，使用`libbfd`通过文件名(`fname`参数)确定二进制文件的属性，并将其打开，然后返回该二进制文件的句柄。事先要使用`bfd_init`函数来初始化内部结构，`open_inited`标识是否已经初始化。通过调用`bfd_openr`函数以文件名打开二进制文件，该函数第二个参数指定了文件类型，传入`NULL`则表示让`libbfd`自动确定二进制文件类型。`bfd_openr`返回一个指向`bfd`类型的文件句柄指针，这是`libbfd`的根数据结构，如果打开发生错误，则为`NULL`。使用`bfd_get_error`函数得到最近的错误类型，返回`bfd_error_type`对象，与预定义的错误标识符进行比较。

```c
static bfd* open_bfd(std::string &fname) {
    static int bfd_inited = 0;
    bfd *bfd_h;

    if (!bfd_inited) {
        bfd_init();
        bfd_inited = 1;
    }

    bfd_h = bfd_openr(fname.c_str(), NULL);
    if (!bfd_h) {
        fprintf(stderr, "failed to open binary '%s' (%s)\n",
            fname.c_str(), bfd_errmsg(bfd_get_error()));
        return NULL;
    }

    if (!bfd_check_format(bfd_h, bfd_object)) {
        fprintf(stderr, "file '%s' does not look like an executable (%s)\n",
            fname.c_str(), bfd_errmsg(bfd_get_error()));
        return NULL;
    }

    bfd_set_error(bfd_error_no_error);

    if (bfd_get_flavour(bfd_h) == bfd_target_unknown_flavour) {
        fprintf(stderr, "unrecognized format for binary '%s' (%s)\n",
            fname.c_str(), bfd_errmsg(bfd_get_error()));
        return NULL;
    }

    return bfd_h;
}
```

获得文件句柄后，可用`bfd_check_format`函数检查二进制文件格式。该函数传入`bfd`句柄和`bfd_format`值。

`bfd_format`定义如下:

```c
typedef enum bfd_format
{
  bfd_unknown = 0,	/* File format is unknown.  */
  bfd_object,		/* Linker/assembler/compiler output.  */
  bfd_archive,		/* Object archive file.  */
  bfd_core,		    /* Core dump.  */
  bfd_type_end		/* Marks the end; don't use it!  */
} bfd_format;
```

`bfd_object`对应了可执行文件、可重定位对象和共享库。

最后通过`bfd_get_flavour`函数检查二进制文件的格式。

```c
enum bfd_flavour
{
  bfd_target_unknown_flavour,
  bfd_target_aout_flavour,
  bfd_target_coff_flavour,
  bfd_target_ecoff_flavour,
  bfd_target_xcoff_flavour,
  bfd_target_elf_flavour,
  bfd_target_tekhex_flavour,
  bfd_target_srec_flavour,
  bfd_target_verilog_flavour,
  bfd_target_ihex_flavour,
  bfd_target_som_flavour,
  bfd_target_os9k_flavour,
  bfd_target_versados_flavour,
  bfd_target_msdos_flavour,
  bfd_target_ovax_flavour,
  bfd_target_evax_flavour,
  bfd_target_mmo_flavour,
  bfd_target_mach_o_flavour,
  bfd_target_pef_flavour,
  bfd_target_pef_xlib_flavour,
  bfd_target_sym_flavour
};
```

`bfd_target_coff_flavour`是微软的PE文件格式，`bfd_target_elf_flavour`是ELF文件格式，如果二进制格式未知，就返回`bfd_target_unknown_flavour`。

通过上述流程，可以打开一个有效的二进制文件。



### 4.7 解析基本属性

将二进制文件中的一些重要属性加载到Binary中。

```c
static int load_binary_bfd(std::string &fname, Binary *bin, Binary::BinaryType type) {
    int ret;
    bfd *bfd_h;
    const bfd_arch_info_type *bfd_info;

    bfd_h = open_bfd(fname);
    if (!bfd_h) {
        goto fail;
    }

    bin->filename = std::string(fname);
    bin->entry = bfd_get_start_address(bfd_h);
    bin->type_str = std::string(bfd_h->xvec->name);
    switch(bfd_h->xvec->flavour) {
        case bfd_target_elf_flavour:
            bin->type = Binary::BIN_TYPE_ELF;
            break;
        case bfd_target_coff_flavour:
            bin->type = Binary::BIN_TYPE_PE;
            break;
        case bfd_target_unknown_flavour:
        default:
            fprintf(stderr, "unsupported binary type (%s)\n", bfd_h->xvec->name);
            goto fail;
    }

    bfd_info = bfd_get_arch_info(bfd_h);
    bin->arch_str = std::string(bfd_info->printable_name);
    switch(bfd_info->mach) {
        case bfd_mach_i386_i386:
            bin->arch = Binary::ARCH_X86;
            bin->bits = 32;
            break;
        case bfd_mach_x86_64:
            bin->arch = Binary::ARCH_X86;
            bin->bits = 64;
            break;
        default: 
            fprintf(stderr, "unsupported architecture (%s)\n",
                bfd_info->printable_name);
            goto fail;
    }

    load_symbols_bfd(bfd_h, bin);
    load_dynsym_bfd(bfd_h, bin);
    if (load_sections_bfd(bfd_h, bin) < 0) goto fail;

    ret = 0;
    goto cleanup;

fail:
    ret = -1;
    
cleanup:
    if (bfd_h) bfd_close(bfd_h);

    return ret;
}
```

`load_binary_bfd`函数中，首先使用`open_bfd`函数打开fname参数指定的二进制文件，并获得该二进制文件的bfd句柄。bin是Binary指针，是二进制文件的抽象。获取一些基本信息，对该对象进行赋值。用`bfd_get_start_address`来获取入口点地址，即返回了bfd对象中start_address字段的值。此处bfd_h中的xvec实际上指向一个bfd_target结构，其中就包含了二进制文件的各种信息：

```c
typedef struct bfd_target
{
  /* Identifies the kind of target, e.g., SunOS4, Ultrix, etc.  */
  char *name;
 
 /* The "flavour" of a back end is a general indication about
    the contents of a file.  */
  enum bfd_flavour flavour;
 
  /* The order of bytes within the data area of a file.  */
  enum bfd_endian byteorder;
 
 /* The order of bytes within the header parts of a file.  */
  enum bfd_endian header_byteorder;
 
  /* A mask of all the flags which an executable may have set -
     from the set <<BFD_NO_FLAGS>>, <<HAS_RELOC>>, ...<<D_PAGED>>.  */
  flagword object_flags;
 
 /* A mask of all the flags which a section may have set - from
    the set <<SEC_NO_FLAGS>>, <<SEC_ALLOC>>, ...<<SET_NEVER_LOAD>>.  */
  flagword section_flags;
 
 /* The character normally found at the front of a symbol.
    (if any), perhaps `_'.  */
  char symbol_leading_char;
 
 /* The pad character for file names within an archive header.  */
  char ar_pad_char;
 
  /* The maximum number of characters in an archive header.  */
  unsigned char ar_max_namelen;
 
  /* How well this target matches, used to select between various
     possible targets when more than one target matches.  */
  unsigned char match_priority;
 
  /* Entries for byte swapping for data. These are different from the
     other entry points, since they don't take a BFD as the first argument.
     Certain other handlers could do the same.  */
  bfd_uint64_t   (*bfd_getx64) (const void *);
  bfd_int64_t    (*bfd_getx_signed_64) (const void *);
  void           (*bfd_putx64) (bfd_uint64_t, void *);
  bfd_vma        (*bfd_getx32) (const void *);
  bfd_signed_vma (*bfd_getx_signed_32) (const void *);
  void           (*bfd_putx32) (bfd_vma, void *);
  bfd_vma        (*bfd_getx16) (const void *);
  bfd_signed_vma (*bfd_getx_signed_16) (const void *);
  void           (*bfd_putx16) (bfd_vma, void *);
 
  /* Byte swapping for the headers.  */
  bfd_uint64_t   (*bfd_h_getx64) (const void *);
  bfd_int64_t    (*bfd_h_getx_signed_64) (const void *);
  void           (*bfd_h_putx64) (bfd_uint64_t, void *);
  bfd_vma        (*bfd_h_getx32) (const void *);
  bfd_signed_vma (*bfd_h_getx_signed_32) (const void *);
  void           (*bfd_h_putx32) (bfd_vma, void *);
  bfd_vma        (*bfd_h_getx16) (const void *);
  bfd_signed_vma (*bfd_h_getx_signed_16) (const void *);
  void           (*bfd_h_putx16) (bfd_vma, void *);
  ......
}
```

使用`bfd_get_arch_info`获取到平台架构信息，返回一个结构体：

```c
typedef struct bfd_arch_info
{
  int bits_per_word;
  int bits_per_address;
  int bits_per_byte;
  enum bfd_architecture arch;
  unsigned long mach;
  const char *arch_name;
  const char *printable_name;
  unsigned int section_align_power;
  /* TRUE if this is the default machine for the architecture.
     The default arch should be the first entry for an arch so that
     all the entries for that arch can be accessed via <<next>>.  */
  bfd_boolean the_default;
  const struct bfd_arch_info * (*compatible) (const struct bfd_arch_info *,
                                              const struct bfd_arch_info *);
 
  bfd_boolean (*scan) (const struct bfd_arch_info *, const char *);
 
  /* Allocate via bfd_malloc and return a fill buffer of size COUNT.  If
     IS_BIGENDIAN is TRUE, the order of bytes is big endian.  If CODE is
     TRUE, the buffer contains code.  */
  void *(*fill) (bfd_size_type count, bfd_boolean is_bigendian,
                 bfd_boolean code);
 
  const struct bfd_arch_info *next;
} bfd_arch_info_type;
```

其中的mach字段标识了平台架构，如果该字段为`bfd_mach_i386_i386`，说明它是一个32位x86架构的二进制文件，则在Binary设置相应的字段，如果mach为`bfd_mach_x86_64`，说明它是一个64位x86架构下的二进制文件，则在Binary中设置字段。



### 4.8 加载静态符号

加载二进制文件的静态符号表。

```c
static int load_symbols_bfd(bfd *bfd_h, Binary *bin) {
    int ret = 0;
    long n, nsyms, i;
    asymbol **bfd_symtab;
    Symbol *sym;
    
    n = bfd_get_symtab_upper_bound(bfd_h);
    if (n < 0) {
        fprintf(stderr, "failed to read symtab (%s)\n",
            bfd_errmsg(bfd_get_error()));
        goto fail;
    } else if (n) {
        bfd_symtab = (asymbol**)malloc(n);
        if (!bfd_symtab) {
            fprintf(stderr, "out of memory\n");
            goto fail;
        }
        nsyms = bfd_canonicalize_symtab(bfd_h, bfd_symtab);
        if (nsyms < 0) {
            fprintf(stderr, "failed to read symtab (%s)\n",
                bfd_errmsg(bfd_get_error()));
            goto fail;
        }
        for (i = 0; i < nsyms; i++) {
            if (bfd_symtab[i]->flags & BSF_FUNCTION) {
                bin->symbols.push_back(Symbol());
                sym = &bin->symbols.back();
                sym->type = Symbol::SYM_TYPE_FUNC;
                sym->name = std::string(bfd_symtab[i]->name);
                sym->addr = bfd_asymbol_value(bfd_symtab[i]);
            }
        }
    }

    ret = 0;
    goto cleanup;

fail:
    ret = -1;

cleanup:
    if (bfd_symtab) free(bfd_symtab);

    return ret;
}
```

`load_symbols_bfd`负责填充asymbol指针数组，然后将信息复制到Binary对象中。

`load_symbols_bfd`的输入参数是bfd句柄和用于存储符号信息的Binary对象，因此要开辟内存空间来存放符号指针。`bfd_get_symtab_upper_bound`函数会返回要分配的字节数，使用`malloc`函数来为代表符号表的二级指针分配内存空间，可以用`bfd_canonicalize_symtab`来填充符号表，将bfd句柄和要填充的符号表(asymbol**)作为参数，返回符号个数，如果返回的是负数则说明发生了错误。

此处只对函数符号感兴趣，通过检查`BSF_FUNCTION`标志来判断符号是否为一个函数符号，如果是则放入Binary对象中symbols中，并获取其起始地址，赋值给对应的Symbol对象。将数据加载到Symbol对象中后就释放原来为符号表申请的空间。



### 4.9 加载动态空间

从动态符号表中加载符号。

```c
static int load_dynsym_bfd(bfd *bfd_h, Binary *bin) {
    int ret = 0;
    long n, nsyms, i;
    asymbol **bfd_dynsym;
    Symbol *sym;
    
    n = bfd_get_dynamic_symtab_upper_bound(bfd_h);
    if (n < 0) {
        fprintf(stderr, "failed to read symtab (%s)\n",
            bfd_errmsg(bfd_get_error()));
        goto fail;
    } else if (n) {
        bfd_dynsym = (asymbol**)malloc(n);
        if (!bfd_dynsym) {
            fprintf(stderr, "out of memory\n");
            goto fail;
        }
        nsyms = bfd_canonicalize_dynamic_symtab(bfd_h, bfd_dynsym);
        if (nsyms < 0) {
            fprintf(stderr, "failed to read symtab (%s)\n",
                bfd_errmsg(bfd_get_error()));
            goto fail;
        }
        for (i = 0; i < nsyms; i++) {
            if (bfd_dynsym[i]->flags & BSF_FUNCTION) {
                bin->symbols.push_back(Symbol());
                sym = &bin->symbols.back();
                sym->type = Symbol::SYM_TYPE_FUNC;
                sym->name = std::string(bfd_dynsym[i]->name);
                sym->addr = bfd_asymbol_value(bfd_dynsym[i]);
            }
        }
    }

    ret = 0;
    goto cleanup;

fail:
    ret = -1;

cleanup:
    if (bfd_dynsym) free(bfd_dynsym);

    return ret;
}
```

与加载静态符号的流程相似，首先获取要分配的字节数，然后使用`malloc`进行内存空间的分配。使用`bfd_canonicalize_dynamic_symtab`来填充符号表，返回符号数。

在for循环中遍历符号表，判断是否为函数符号，如果是则存入Binary对象中



### 4.10 加载节信息

加载二进制文件的节。

```c
static int load_sections_bfd(bfd* bfd_h, Binary *bin) {
    flagword bfd_flags;
    uint64_t vma, size;
    const char *secname;
    asection *bfd_sec;
    Section *sec;
    Section::SectionType sectype;

    for (bfd_sec = bfd_h->sections; bfd_sec; bfd_sec = bfd_sec->next) {
        bfd_flags = bfd_sec->flags;

        sectype = Section::SEC_TYPE_NONE;
        if (bfd_flags & SEC_CODE) {
            sectype = Section::SEC_TYPE_CODE;
        } else if (bfd_flags & SEC_DATA) {
            sectype = Section::SEC_TYPE_DATA;
        } else {
            continue;
        }
        vma = bfd_section_vma(bfd_sec);
        size = bfd_section_size(bfd_sec);
        secname = bfd_section_name(bfd_sec);
        if (!secname) secname = "<unnamed>";

        bin->sections.push_back(Section());
        sec = &bin->sections.back();
        sec->binary = bin;
        sec->name = std::string(secname);
        sec->type = sectype;
        sec->vma = vma;
        sec->size = size;
        sec->bytes = (uint8_t*)malloc(size);
        if (!sec->bytes) {
            fprintf(stderr, "out of memory\n");
            return -1;
        }

        if (!bfd_get_section_contents(bfd_h, bfd_sec, sec->bytes, 0, size)) {
            fprintf(stderr, "failed to read section '%s' (%s)\n",
                secname, bfd_errmsg(bfd_get_error()));
            return -1;
        }
    }

    return 0;
}
```

libbfd使用一个名为asection的数据结构来保存节信息，也称为bfd_section结构。在内部，libbfd通过asection链表表示所有的节。遍历这个链表，先使用`bfd_get_section_flags`函数来获取标志位，通过检查这个标志位，来判断节的类型。

接下来要获取节的虚拟地址、大小、名称及其原始字节数。使用`bfd_section_vma`函数来获取节的虚拟基址，`bfd_section_size`函数返回节的大小，`bfd_section_name`函数返回节的名称。Section对象中的bytes存储节的原始字节，使用`malloc`为Section对象的bytes指针开辟空间。`bfd_get_section_contents`函数将节的所有原始字节数据复制到Section对象的bytes中。



###  4. 11 练习

#### 4.11.1 十六进制形式输出指定段

参照`xxd`、`objdump`的输出形式，按照`虚拟内存地址:十六进制 字符`的形式对段的内容进行输出。

```c
for (i = 0; (i < bin.sections.size()) && (bin.sections[i].name != argv[2]); i++) {}
if (i == bin.sections.size()) {
    printf("\nThere is not a section called '%s', please check it and try again!\n", argv[2]);
    goto cleanup;
}
sec = &bin.sections[i];
printf("\n%s(%dbytes):\n", argv[2], sec->size);
for (size = 0, vma = sec->vma; size < sec->size; size += 16, vma += 16) {
    printf("  0x%016jx: ", vma);
    if (sec->size - size < 16) {
        for (i = 0; i < sec->size - size; i++) {
            printf("%02x%s", sec->bytes[size + i], i % 2 ? " " : "");
        }
        for (; i < 16; i++) {
            printf("  %s", i % 2 ? " " : "");
        }
        for (i = 0; i < sec->size - size; i++) {
            if (sec->bytes[size + i] >= 0x21 && sec->bytes[size + i] <= 0x7e) {
                printf("%c", sec->bytes[size + i]);
            } else {
                printf(".");
            }
        }
    } else {
        for (i = 0; i < 16; i++) {
            printf("%02x%s", sec->bytes[size + i], i % 2 ? " " : "");
        }
        for (i = 0; i < 16; i++) {
            if (sec->bytes[size + i] >= 0x21 && sec->bytes[size + i] <= 0x7e) {
                printf("%c", sec->bytes[size + i]);
            } else {
                printf(".");
            }
        }
    }
    printf("\n");
}
```



#### 4.11.2 检查弱符号

对于C/C++语言来说，编译器默认函数和初始化了的全局变量为**强符号**，未初始化的全局变量为**弱符号**。链接器会按照如下规则处理与选择被多次定义的全局符号：

1. 不允许强符号多次被定义。
2. 如果一个符号在某个目标文件中是强符号，在其它文件中是弱符号，那么选择强符号。
3. 如果一个符号在所有目标文件中都是弱符号，那么选择其中占用空间最大的一个。

在`bfd.h`中，`bfd_symbol`结构体的定义如下：

```c
typedef struct bfd_symbol {
    struct bfd *the_bfd;
    const char *name;
    symvalue value;
    flagword flags;
    struct bfd_section *section;
    union {
      void *p;
      bfd_vma i;
    } udata;
} asymbol;
```

其中，关于标志位`flags`的宏定义有：

```c
/* Attributes of a symbol.  */
#define BSF_NO_FLAGS            0

  /* The symbol has local scope; <<static>> in <<C>>. The value
     is the offset into the section of the data.  */
#define BSF_LOCAL               (1 << 0)

  /* The symbol has global scope; initialized data in <<C>>. The
     value is the offset into the section of the data.  */
#define BSF_GLOBAL              (1 << 1)

  /* The symbol has global scope and is exported. The value is
     the offset into the section of the data.  */
#define BSF_EXPORT              BSF_GLOBAL /* No real difference.  */

  /* A normal C symbol would be one of:
     <<BSF_LOCAL>>, <<BSF_UNDEFINED>> or <<BSF_GLOBAL>>.  */

  /* The symbol is a debugging record. The value has an arbitrary
     meaning, unless BSF_DEBUGGING_RELOC is also set.  */
#define BSF_DEBUGGING           (1 << 2)

  /* The symbol denotes a function entry point.  Used in ELF,
     perhaps others someday.  */
#define BSF_FUNCTION            (1 << 3)

  /* Used by the linker.  */
#define BSF_KEEP                (1 << 5)

  /* An ELF common symbol.  */
#define BSF_ELF_COMMON          (1 << 6)

  /* A weak global symbol, overridable without warnings by
     a regular global symbol of the same name.  */
#define BSF_WEAK                (1 << 7)

  /* This symbol was created to point to a section, e.g. ELF's
     STT_SECTION symbols.  */
#define BSF_SECTION_SYM         (1 << 8)

  /* The symbol used to be a common symbol, but now it is
     allocated.  */
#define BSF_OLD_COMMON          (1 << 9)

  /* In some files the type of a symbol sometimes alters its
     location in an output file - ie in coff a <<ISFCN>> symbol
     which is also <<C_EXT>> symbol appears where it was
     declared and not at the end of a section.  This bit is set
     by the target BFD part to convey this information.  */
#define BSF_NOT_AT_END          (1 << 10)

  /* Signal that the symbol is the label of constructor section.  */
#define BSF_CONSTRUCTOR         (1 << 11)

  /* Signal that the symbol is a warning symbol.  The name is a
     warning.  The name of the next symbol is the one to warn about;
     if a reference is made to a symbol with the same name as the next
     symbol, a warning is issued by the linker.  */
#define BSF_WARNING             (1 << 12)

  /* Signal that the symbol is indirect.  This symbol is an indirect
     pointer to the symbol with the same name as the next symbol.  */
#define BSF_INDIRECT            (1 << 13)

  /* BSF_FILE marks symbols that contain a file name.  This is used
     for ELF STT_FILE symbols.  */
#define BSF_FILE                (1 << 14)

  /* Symbol is from dynamic linking information.  */
#define BSF_DYNAMIC             (1 << 15)

  /* The symbol denotes a data object.  Used in ELF, and perhaps
     others someday.  */
#define BSF_OBJECT              (1 << 16)

  /* This symbol is a debugging symbol.  The value is the offset
     into the section of the data.  BSF_DEBUGGING should be set
     as well.  */
#define BSF_DEBUGGING_RELOC     (1 << 17)

  /* This symbol is thread local.  Used in ELF.  */
#define BSF_THREAD_LOCAL        (1 << 18)

  /* This symbol represents a complex relocation expression,
     with the expression tree serialized in the symbol name.  */
#define BSF_RELC                (1 << 19)

  /* This symbol represents a signed complex relocation expression,
     with the expression tree serialized in the symbol name.  */
#define BSF_SRELC               (1 << 20)

  /* This symbol was created by bfd_get_synthetic_symtab.  */
#define BSF_SYNTHETIC           (1 << 21)

  /* This symbol is an indirect code object.  Unrelated to BSF_INDIRECT.
     The dynamic linker will compute the value of this symbol by
     calling the function that it points to.  BSF_FUNCTION must
     also be also set.  */
#define BSF_GNU_INDIRECT_FUNCTION (1 << 22)
  /* This symbol is a globally unique data object.  The dynamic linker
     will make sure that in the entire process there is just one symbol
     with this name and type in use.  BSF_OBJECT must also be set.  */
#define BSF_GNU_UNIQUE          (1 << 23)

  /* This section symbol should be included in the symbol table.  */
#define BSF_SECTION_SYM_USED    (1 << 24)
```

其中的`BSF_WEAK`即为弱符号标志。可以使用如下代码判断一个符号是否为弱符号：

```c
if (sym->flags & BSF_WEAK) {
    // 该符号是弱符号
} else {
    // 该符号不是弱符号
}
```



#### 4.11.3 打印数据符号

要区分全局和局部数据，以及函数符号，只需要检查符号的标志位：

- `BSF_LOCAL`：C语言中的`static`修饰的局部符号。
- `BSF_GLOBAL`：已初始化的全局符号。
- `BSF_FUNCTION`：指向函数入口点的函数符号。
- `BSF_OBJECT`：数据对象符号。

现将数据类型加入到`Symbol`类中（以下`+`号表示新增的代码，`-`表示删除了的代码）：

```c
class Symbol {
    public:
        enum SymbolType {
             SYM_TYPE_UKN = 0,
             SYM_TYPE_FUNC = 1,
+            SYM_TYPE_DATA = 2
        };
        
        Symbol(): type(SYM_TYPE_UKN), addr(0) {}

        SymbolType type;
        std::string name;
        uint64_t addr;
};
```

加入分析`ELF`文件时对数据符号类型的判断代码：

```c
static int load_symbols_bfd(bfd *bfd_h, Binary *bin) {
            ...
            if (bfd_symtab[i]->flags & BSF_FUNCTION) {
                bin->symbols.push_back(Symbol());
                sym = &bin->symbols.back();
                sym->type = Symbol::SYM_TYPE_FUNC;
                sym->name = std::string(bfd_symtab[i]->name);
                sym->addr = bfd_asymbol_value(bfd_symtab[i]);
+            } else if (bfd_symtab[i]->flags & BSF_OBJECT) {
+                bin->symbols.push_back(Symbol());
+                sym = &bin->symbols.back();
+                sym->type = Symbol::SYM_TYPE_DATA;
+                sym->name = std::string(bfd_symtab[i]->name);
+                sym->addr = bfd_asymbol_value(bfd_symtab[i]);
            }
            ...
}

static int load_dynsym_bfd(bfd *bfd_h, Binary *bin) {
            ...
            if (bfd_dynsym[i]->flags & BSF_FUNCTION) {
                bin->symbols.push_back(Symbol());
                sym = &bin->symbols.back();
                sym->type = Symbol::SYM_TYPE_FUNC;
                sym->name = std::string(bfd_dynsym[i]->name);
                sym->addr = bfd_asymbol_value(bfd_dynsym[i]);
+            } else if (bfd_dynsym[i]->flags & BSF_OBJECT) {
+                bin->symbols.push_back(Symbol());
+                sym = &bin->symbols.back();
+                sym->type = Symbol::SYM_TYPE_DATA;
+                sym->name = std::string(bfd_dynsym[i]->name);
+                sym->addr = bfd_asymbol_value(bfd_dynsym[i]);
            }
            ...
}
```

在`main`函数中打印符号表的代码中加入判断数据符号的代码：

```c
    if (!bin.symbols.empty()) {
        printf("\nsymbol table:\n");
        printf("  %-40s %-18s %s\n", "Name", "Addr", "Type");
        for (i = 0; i < bin.symbols.size(); i++) {
            sym = &bin.symbols[i];
            printf("  %-40s 0x%016jx %s\n",sym->name.c_str(), sym->addr,
-                (sym->type & Symbol::SYM_TYPE_FUNC) ? "FUNC" : "");
+                (sym->type & Symbol::SYM_TYPE_FUNC) ? "FUNC" :
+                (sym->type & Symbol::SYM_TYPE_DATA) ? "DATA" : "");
        }
    }
```



### 4.12 主函数

最终的`main`函数：

```c
int main(int argc, char* argv[])
{
    size_t i;
    size_t size;
    uint64_t vma;
    Binary bin;
    Section *sec;
    Symbol *sym;
    std::string fname;

    if (argc < 2 || argc > 3) {
        printf("Usage: %s <binary> [<section_name>]\n", argv[0]);
        return 1;
    }

    fname.assign(argv[1]);

    if (load_binary(fname, &bin, Binary::BIN_TYPE_AUTO) < 0) {
        return 1;
    };

    printf("\nloaded binary '%s' %s/%s (%ubits) entry@0x%016jx\n",
        bin.filename.c_str(), bin.type_str.c_str(), bin.arch_str.c_str(), bin.bits, bin.entry);

    if (!bin.sections.empty()) {
        printf("\nsections:\n");
        printf("  %-18s %-8s %-20s %s\n", "VMA", "Size", "Name", "Type");
        for (i = 0; i < bin.sections.size(); i++) {
            sec = &bin.sections[i];
            printf("  0x%016jx %-8ju %-20s %s\n",
                sec->vma, sec->size, sec->name.c_str(),
                sec->type == Section::SEC_TYPE_CODE ? "CODE" : "DATA");
        }
    }
    
    if (!bin.symbols.empty()) {
        printf("\nsymbol table:\n");
        printf("  %-40s %-18s %s\n", "Name", "Addr", "Type");
        for (i = 0; i < bin.symbols.size(); i++) {
            sym = &bin.symbols[i];
            printf("  %-40s 0x%016jx %s\n",sym->name.c_str(), sym->addr,
                (sym->type & Symbol::SYM_TYPE_FUNC) ? "FUNC" :
                (sym->type & Symbol::SYM_TYPE_DATA) ? "DATA" : "");
        }
    }

    if (argc == 3) {
        for (i = 0; (i < bin.sections.size()) && (bin.sections[i].name != argv[2]); i++) {}
        if (i == bin.sections.size()) {
            printf("\nThere is not a section called '%s', please check it and try again!\n", argv[2]);
            goto cleanup;
        }
        sec = &bin.sections[i];
        printf("\n%s(%dbytes):\n", argv[2], sec->size);
        for (size = 0, vma = sec->vma; size < sec->size; size += 16, vma += 16) {
            printf("  0x%016jx: ", vma);
            if (sec->size - size < 16) {
                for (i = 0; i < sec->size - size; i++) {
                    printf("%02x%s", sec->bytes[size + i], i % 2 ? " " : "");
                }
                for (; i < 16; i++) {
                    printf("  %s", i % 2 ? " " : "");
                }
                for (i = 0; i < sec->size - size; i++) {
                    if (sec->bytes[size + i] >= 0x21 && sec->bytes[size + i] <= 0x7e) {
                        printf("%c", sec->bytes[size + i]);
                    } else {
                        printf(".");
                    }
                }
            } else {
                for (i = 0; i < 16; i++) {
                    printf("%02x%s", sec->bytes[size + i], i % 2 ? " " : "");
                }
                for (i = 0; i < 16; i++) {
                    if (sec->bytes[size + i] >= 0x21 && sec->bytes[size + i] <= 0x7e) {
                        printf("%c", sec->bytes[size + i]);
                    } else {
                        printf(".");
                    }
                }
            }
            printf("\n");
        }
    }

cleanup:
    unload_binary(&bin);

    return 0;
}
```



### 4.13 测试ELF文件

编译&链接。这里是用`MinGW-W64-builds-5.0.0`在`Windows11`的`PowerShell`上编译的。

```bash
$ g++ loader_demo.cpp inc/loader.cpp -lbfd -liberty -lz -o loader_demo.exe
```

解析ELF文件。

```bash
$ ./loader_demo.exe 4_1_elf_binary.out .rodata

loaded binary '4_1_elf_binary.out' elf64-x86-64/i386:x86-64 (64bits) entry@0x0000000000001060

sections:
  VMA                Size     Name                 Type
  0x00000000000002a8 28       .interp              DATA
  0x00000000000002c4 36       .note.gnu.build-id   DATA
  0x00000000000002e8 32       .note.ABI-tag        DATA
  0x0000000000000308 36       .gnu.hash            DATA
  0x0000000000000330 192      .dynsym              DATA
  0x00000000000003f0 138      .dynstr              DATA
  0x000000000000047a 16       .gnu.version         DATA
  0x0000000000000490 32       .gnu.version_r       DATA
  0x00000000000004b0 192      .rela.dyn            DATA
  0x0000000000000570 48       .rela.plt            DATA
  0x0000000000001000 23       .init                CODE
  0x0000000000001020 48       .plt                 CODE
  0x0000000000001050 8        .plt.got             CODE
  0x0000000000001060 433      .text                CODE
  0x0000000000001214 9        .fini                CODE
  0x0000000000002000 40       .rodata              DATA
  0x0000000000002028 60       .eh_frame_hdr        DATA
  0x0000000000002068 264      .eh_frame            DATA
  0x0000000000003de8 8        .init_array          DATA
  0x0000000000003df0 8        .fini_array          DATA
  0x0000000000003df8 480      .dynamic             DATA
  0x0000000000003fd8 40       .got                 DATA
  0x0000000000004000 40       .got.plt             DATA
  0x0000000000004028 20       .data                DATA

symbol table:
  Name                                     Addr               Type
  deregister_tm_clones                     0x0000000000001090 FUNC
  register_tm_clones                       0x00000000000010c0 FUNC
  __do_global_dtors_aux                    0x0000000000001100 FUNC
  completed.0                              0x000000000000403c DATA
  __do_global_dtors_aux_fini_array_entry   0x0000000000003df0 DATA
  frame_dummy                              0x0000000000001140 FUNC
  __frame_dummy_init_array_entry           0x0000000000003de8 DATA
  __FRAME_END__                            0x000000000000216c DATA
  _DYNAMIC                                 0x0000000000003df8 DATA
  _GLOBAL_OFFSET_TABLE_                    0x0000000000004000 DATA
  _init                                    0x0000000000001000 FUNC
  __libc_csu_fini                          0x0000000000001210 FUNC
  putchar@GLIBC_2.2.5                      0x0000000000000000 FUNC
  puts@GLIBC_2.2.5                         0x0000000000000000 FUNC
  _fini                                    0x0000000000001214 FUNC
  global_var                               0x0000000000004038 DATA
  __libc_start_main@GLIBC_2.2.5            0x0000000000000000 FUNC
  __dso_handle                             0x0000000000004030 DATA
  _IO_stdin_used                           0x0000000000002000 DATA
  __libc_csu_init                          0x00000000000011b0 FUNC
  _start                                   0x0000000000001060 FUNC
  main                                     0x0000000000001145 FUNC
  __TMC_END__                              0x0000000000004040 DATA
  __cxa_finalize@GLIBC_2.2.5               0x0000000000000000 FUNC
  putchar                                  0x0000000000000000 FUNC
  puts                                     0x0000000000000000 FUNC
  __libc_start_main                        0x0000000000000000 FUNC
  __cxa_finalize                           0x0000000000000000 FUNC

.rodata(40bytes):
  0x0000000000002000: 0100 0200 0000 0000 7468 6572 6520 6172 ........there.ar
  0x0000000000002010: 6520 736f 6d65 2074 6578 7420 666f 7220 e.some.text.for.
  0x0000000000002020: 7465 7374 696e 6700                     testing.
```



# 第二部分、二进制分析基础

## 5. Linux二进制分析

二进制文件分析有以下两类手段：

- 静态分析：静态分析技术可以在不运行二进制文件的情况下对二进制文件进行分析。优点：可以一次性分析整个二进制文件，且不需要特定的CPU来运行二进制文件。缺点：静态分析不了解二进制文件运行时的状态，会使分析非常难。
- 动态分析：与静态分析相反，动态分析会运行二进制文件并在执行时对其进行分析。比静态分析简单，能完全了解运行时的状态。但仅能看到执行的代码，可能会忽略其它有用的信息。



### 5.1 使用file查看文件类型

file命令的功能是用于识别文件的类型，也可以用来辨别一些内容的编码格式。由于Linux系统并不是像Windows系统那样通过扩展名来定义文件类型，因此用户无法直接通过文件名来进行分辨。file命令则是为了解决此问题，通过分析文件头部信息中的标识来显示文件类型。

`file [参数] 文件`

| 常用参数 | 含义               |
| -------- | ------------------ |
| -i       | 显示文件的MIME类别 |

```bash
$ file payload
payload: ASCII text

$ file -i payload
payload: text/plain; charset=us-ascii
```

使用`head`以ASCII打印文件开头的字符串：

```bash
$ head payload
H4sIABzY61gAA+xaD3RTVZq/Sf+lFJIof1r+2aenKKh0klJKi4MmJaUvWrTSFlgR0jRN20iadpKX
UljXgROKjbUOKuOfWWfFnTlzZs/ZXTln9nTRcTHYERhnZ5c/R2RGV1lFTAFH/DNYoZD9vvvubd57
bcBl1ln3bL6e9Hvf9+733e/+v+/en0dqId80WYAWLVqI3LpooUXJgUpKFy6yEOsCy6KSRQtLLQsW
EExdWkIEyzceGVA4JLmDgkCaA92XTXel9/9H6ftVNcv0Ot2orCe3E5RiJhuVbUw/fH3SxkbKSS78
v47MJtkgZynS2YhNxYeZa84NLF0G/DLhV66X5XK9TcVnsXSc6xQ8S1UCm4o/M5moOCHCqB3Geny2
rD0+u1HFD7I4junVdnpmN8zshll6zglPr1eXL5P96pm+npWLcwdL51CkR6r9UGrGZ8O1zN+1NhUv
ZelKNXb3gl02+fpkZnwFyy9VvQgsfs55O3zH72sqK/2Ov3m+3xcId8/vLi+bX1ZaHOooLqExmVna
6rsbaHpejwKLeQqR+wC+n/ePA3n/duKu2kNvL175+MxD7z75W8GC76aSZLv1xgSdkGnLRV0+/KbD
7+UPnnhwadWbZ459b/Wsl/o/NZ468olxo3P9wOXK3Qe/a8fRmwhvcTVdl0J/UDe+nzMp9M4U+n9J
oX8jhT5HP77+ZIr0JWT8+NvI+OnvTpG+NoV/Qwr9Vyn0b6bQkxTl+ixF+p+m0N+qx743k+wWGlX6
```

可以猜测以上文件应该是一个Base64编码的文件。

`base64`码表：

| **索引** | **对应字符** | **索引** | **对应字符** | **索引** | **对应字符** | **索引** | **对应字符** |
| -------- | ------------ | -------- | ------------ | -------- | ------------ | -------- | ------------ |
| 0        | **A**        | 17       | **R**        | 34       | **i**        | 51       | **z**        |
| 1        | **B**        | 18       | **S**        | 35       | **j**        | 52       | **0**        |
| 2        | **C**        | 19       | **T**        | 36       | **k**        | 53       | **1**        |
| 3        | **D**        | 20       | **U**        | 37       | **l**        | 54       | **2**        |
| 4        | **E**        | 21       | **V**        | 38       | **m**        | 55       | **3**        |
| 5        | **F**        | 22       | **W**        | 39       | **n**        | 56       | **4**        |
| 6        | **G**        | 23       | **X**        | 40       | **o**        | 57       | **5**        |
| 7        | **H**        | 24       | **Y**        | 41       | **p**        | 58       | **6**        |
| 8        | **I**        | 25       | **Z**        | 42       | **q**        | 59       | **7**        |
| 9        | **J**        | 26       | **a**        | 43       | **r**        | 60       | **8**        |
| 10       | **K**        | 27       | **b**        | 44       | **s**        | 61       | **9**        |
| 11       | **L**        | 28       | **c**        | 45       | **t**        | 62       | **+**        |
| 12       | **M**        | 29       | **d**        | 46       | **u**        | 63       | **/**        |
| 13       | **N**        | 30       | **e**        | 47       | **v**        |          |              |
| 14       | **O**        | 31       | **f**        | 48       | **w**        |          |              |
| 15       | **P**        | 32       | **g**        | 49       | **x**        |          |              |
| 16       | **Q**        | 33       | **h**        | 50       | **y**        |          |              |



使用`base64`对文件解码：

```bash
$ base64 -d payload > decoded_payload
```

查看`decoded_payload`文件类型：

```bash
$ file decoded_payload
decoded_payload: gzip compressed data, last modified: Mon Apr 10 19:08:12 2017, from Unix, original size modulo 2^32 808960
```

可以看到文件使用了`gzip`进行压缩，继续使用`file -z`查看压缩文件的内容：

```bash
$ file -z decoded_payload
decoded_payload: POSIX tar archive (GNU) (gzip compressed data, last modified: Mon Apr 10 19:08:12 2017, from Unix)
```

压缩文件中还有一个用`tar`压缩的文件，使用`tar`解压缩提取`decoded_payload`的内容：

```bash
$ tar xvzf decoded_payload
ctf
67b8601
```

`tar`提取出了`ctf`和`67b8601`两个文件，使用`file`查看这两个文件的类型：

```bash
$ file ctf
ctf: ELF 64-bit LSB executable, x86-64, version 1 (SYSV), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, for GNU/Linux 2.6.32, BuildID[sha1]=29aeb60bcee44b50d1db3a56911bd1de93cd2030, stripped

$ file 67b8601
67b8601: PC bitmap, Windows 3.x format, 512 x 512 x 24, image size 786434, resolution 7872 x 7872 px/m, 1165950976 important colors, cbSize 786488, bits offset 54
```



### 5.2 使用ldd查看文件依赖

运行`ctf`文件。

```bash
$ ./ctf
./ctf: error while loading shared libraries: lib5ae9b7f.so: cannot open shared object file: No such file or directory
```

`ctf`缺少动态库`lib5ae9b7f.so`，现在需要使用`ldd`工具检查一下是否有更多的未解析的依赖项。

```bash
$ ldd ctf
        linux-vdso.so.1 (0x00007ffff70d4000)
        lib5ae9b7f.so => not found
        libstdc++.so.6 => /usr/lib/x86_64-linux-gnu/libstdc++.so.6 (0x00007f44eb790000)
        libgcc_s.so.1 => /lib/x86_64-linux-gnu/libgcc_s.so.1 (0x00007f44eb770000)
        libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f44eb5a0000)
        libm.so.6 => /lib/x86_64-linux-gnu/libm.so.6 (0x00007f44eb450000)
        /lib64/ld-linux-x86-64.so.2 (0x00007f44eb967000)
```

只有`lib5ae9b7f.so`没有被解析，使用`grep`搜索ELF文件格式的幻数`"ELF"`。

```bash
$ grep 'ELF' *
grep: 67b8601: binary file matches
grep: ctf: binary file matches
```

可以看到`67b8601`中含有ELF文件的幻数。所以猜测该文件中包含ELF文件。



### 5.3 使用xxd查看文件内容

使用`xxd`以十六进制形式打印`67b8601`文件。

```bash
$ xxd 67b8601 | head -n 15
00000000: 424d 3800 0c00 0000 0000 3600 0000 2800  BM8.......6...(.
00000010: 0000 0002 0000 0002 0000 0100 1800 0000  ................
00000020: 0000 0200 0c00 c01e 0000 c01e 0000 0000  ................
00000030: 0000 0000 7f45 4c46 0201 0100 0000 0000  .....ELF........
00000040: 0000 0000 0300 3e00 0100 0000 7009 0000  ......>.....p...
00000050: 0000 0000 4000 0000 0000 0000 7821 0000  ....@.......x!..
00000060: 0000 0000 0000 0000 4000 3800 0700 4000  ........@.8...@.
00000070: 1b00 1a00 0100 0000 0500 0000 0000 0000  ................
00000080: 0000 0000 0000 0000 0000 0000 0000 0000  ................
00000090: 0000 0000 f40e 0000 0000 0000 f40e 0000  ................
000000a0: 0000 0000 0000 2000 0000 0000 0100 0000  ...... .........
000000b0: 0600 0000 f01d 0000 0000 0000 f01d 2000  .............. .
000000c0: 0000 0000 f01d 2000 0000 0000 6802 0000  ...... .....h...
000000d0: 0000 0000 7002 0000 0000 0000 0000 2000  ....p......... .
000000e0: 0000 0000 0200 0000 0600 0000 081e 0000  ................
```

可以看到幻数ELF出现在0x00000034位置处，即偏移量为52字节。ELF文件头的大小为64字节，所以使用`dd`将ELF文件头提取出来。

```bash
$ dd skip=52 count=64 if=67b8601 of=elf_header bs=1
64+0 records in
64+0 records out
64 bytes copied, 0.0056582 s, 11.3 kB/s
```

使用`xxd`查看`elf_header`内容。

```bash
$ xxd elf_header
00000000: 7f45 4c46 0201 0100 0000 0000 0000 0000  .ELF............
00000010: 0300 3e00 0100 0000 7009 0000 0000 0000  ..>.....p.......
00000020: 4000 0000 0000 0000 7821 0000 0000 0000  @.......x!......
00000030: 0000 0000 4000 3800 0700 4000 1b00 1a00  ....@.8...@.....
```



### 5.4 使用readelf解析ELF文件

使用`readelf`读取ELF文件头，查看文件信息。

```bash
$ readelf -h elf_header
ELF Header:
  Magic:   7f 45 4c 46 02 01 01 00 00 00 00 00 00 00 00 00
  Class:                             ELF64
  Data:                              2's complement, little endian
  Version:                           1 (current)
  OS/ABI:                            UNIX - System V
  ABI Version:                       0
  Type:                              DYN (Shared object file)
  Machine:                           Advanced Micro Devices X86-64
  Version:                           0x1
  Entry point address:               0x970
  Start of program headers:          64 (bytes into file)
  Start of section headers:          8568 (bytes into file)
  Flags:                             0x0
  Size of this header:               64 (bytes)
  Size of program headers:           56 (bytes)
  Number of program headers:         7
  Size of section headers:           64 (bytes)
  Number of section headers:         27
  Section header string table index: 26
readelf: Error: Reading 1728 bytes extends past end of file for section headers
readelf: Error: Too many program headers - 0x7 - the file is not that big
```

可以得到ELF文件的节头偏移量为8568字节，节头表每项的大小为64字节，节头表中表项的数量为27。因为ELF文件的结尾是节头表，所以文件的总大小就为`节头表偏移量+节头表每项的大小*节头表表项的数量`，即8565+64*27=10296。所以该库的大小为10296字节，最终我们将它提取出来。

```bash
$ dd skip=52 count=10296 if=67b8601 of=lib5ae9b7f.so bs=1
10296+0 records in
10296+0 records out
10296 bytes (10 kB, 10 KiB) copied, 0.244036 s, 42.2 kB/s
```

使用`readelf -hs`查看文件的文件头和符号表

```bash
$ readelf -hs lib5ae9b7f.so
ELF Header:
  Magic:   7f 45 4c 46 02 01 01 00 00 00 00 00 00 00 00 00
  Class:                             ELF64
  Data:                              2's complement, little endian
  Version:                           1 (current)
  OS/ABI:                            UNIX - System V
  ABI Version:                       0
  Type:                              DYN (Shared object file)
  Machine:                           Advanced Micro Devices X86-64
  Version:                           0x1
  Entry point address:               0x970
  Start of program headers:          64 (bytes into file)
  Start of section headers:          8568 (bytes into file)
  Flags:                             0x0
  Size of this header:               64 (bytes)
  Size of program headers:           56 (bytes)
  Number of program headers:         7
  Size of section headers:           64 (bytes)
  Number of section headers:         27
  Section header string table index: 26

Symbol table '.dynsym' contains 22 entries:
   Num:    Value          Size Type    Bind   Vis      Ndx Name
     0: 0000000000000000     0 NOTYPE  LOCAL  DEFAULT  UND
     1: 00000000000008c0     0 SECTION LOCAL  DEFAULT    9
     2: 0000000000000000     0 NOTYPE  WEAK   DEFAULT  UND __gmon_start__
     3: 0000000000000000     0 NOTYPE  WEAK   DEFAULT  UND _Jv_RegisterClasses
     4: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND [...]@GLIBCXX_3.4.21 (2)
     5: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND [...]@GLIBC_2.2.5 (3)
     6: 0000000000000000     0 NOTYPE  WEAK   DEFAULT  UND _ITM_deregisterT[...]
     7: 0000000000000000     0 NOTYPE  WEAK   DEFAULT  UND _ITM_registerTMC[...]
     8: 0000000000000000     0 FUNC    WEAK   DEFAULT  UND [...]@GLIBC_2.2.5 (3)
     9: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND __[...]@GLIBC_2.4 (4)
    10: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND [...]@GLIBCXX_3.4 (5)
    11: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND memcpy@GLIBC_2.14 (6)
    12: 0000000000000bc0   149 FUNC    GLOBAL DEFAULT   12 _Z11rc4_encryptP[...]
    13: 0000000000000cb0   112 FUNC    GLOBAL DEFAULT   12 _Z8rc4_initP11rc[...]
    14: 0000000000202060     0 NOTYPE  GLOBAL DEFAULT   24 _end
    15: 0000000000202058     0 NOTYPE  GLOBAL DEFAULT   23 _edata
    16: 0000000000000b40   119 FUNC    GLOBAL DEFAULT   12 _Z11rc4_encryptP[...]
    17: 0000000000000c60     5 FUNC    GLOBAL DEFAULT   12 _Z11rc4_decryptP[...]
    18: 0000000000202058     0 NOTYPE  GLOBAL DEFAULT   24 __bss_start
    19: 00000000000008c0     0 FUNC    GLOBAL DEFAULT    9 _init
    20: 0000000000000c70    59 FUNC    GLOBAL DEFAULT   12 _Z11rc4_decryptP[...]
    21: 0000000000000d20     0 FUNC    GLOBAL DEFAULT   13 _fini
```



### 5.5 使用nm解析符号表

C++为了支持函数重载，使用了符号修饰，其实质上是原始函数名称和函数参数编码的组合，这样，函数的每个版本都会获得唯一的名称，并且链接器也不会对重载的函数产生歧义。

符号修饰对于逆向工程的作用：

- 符号修饰难以阅读
- 符号修饰实质上泄露了函数的参数

使用`nm`查看静态链接表。

```bash
$ nm lib5ae9b7f.so
nm: lib5ae9b7f.so: no symbols
```

动态库中不存在静态符号，所以用`nm -D`查看动态链接表。

```bash
$ nm -D lib5ae9b7f.so
0000000000202058 B __bss_start
                 w __cxa_finalize@GLIBC_2.2.5
0000000000202058 D _edata
0000000000202060 B _end
0000000000000d20 T _fini
                 w __gmon_start__
00000000000008c0 T _init
                 w _ITM_deregisterTMCloneTable
                 w _ITM_registerTMCloneTable
                 w _Jv_RegisterClasses
                 U malloc@GLIBC_2.2.5
                 U memcpy@GLIBC_2.14
                 U __stack_chk_fail@GLIBC_2.4
0000000000000c60 T _Z11rc4_decryptP11rc4_state_tPhi
0000000000000c70 T _Z11rc4_decryptP11rc4_state_tRNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE
0000000000000b40 T _Z11rc4_encryptP11rc4_state_tPhi
0000000000000bc0 T _Z11rc4_encryptP11rc4_state_tRNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE
0000000000000cb0 T _Z8rc4_initP11rc4_state_tPhi
                 U _ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE9_M_createERmm@GLIBCXX_3.4.21
                 U _ZSt19__throw_logic_errorPKc@GLIBCXX_3.4
```

继续使用`--demangle`解析符号修饰过的函数

```bash
$ nm -D --demangle lib5ae9b7f.so
0000000000202058 B __bss_start
                 w __cxa_finalize@GLIBC_2.2.5
0000000000202058 D _edata
0000000000202060 B _end
0000000000000d20 T _fini
                 w __gmon_start__
00000000000008c0 T _init
                 w _ITM_deregisterTMCloneTable
                 w _ITM_registerTMCloneTable
                 w _Jv_RegisterClasses
                 U malloc@GLIBC_2.2.5
                 U memcpy@GLIBC_2.14
                 U __stack_chk_fail@GLIBC_2.4
0000000000000c60 T rc4_decrypt(rc4_state_t*, unsigned char*, int)
0000000000000c70 T rc4_decrypt(rc4_state_t*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&)
0000000000000b40 T rc4_encrypt(rc4_state_t*, unsigned char*, int)
0000000000000bc0 T rc4_encrypt(rc4_state_t*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&)
0000000000000cb0 T rc4_init(rc4_state_t*, unsigned char*, int)
                 U std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_create(unsigned long&, unsigned long)@GLIBCXX_3.4.21
                 U std::__throw_logic_error(char const*)@GLIBCXX_3.4
```

还有一个叫做`c++filt`的工具支持多种修饰格式，可以自动检测并修正输入的修饰格式。

```bash
$ c++filt _Z8rc4_initP11rc4_state_tPhi
rc4_init(rc4_state_t*, unsigned char*, int)
```

设置下`GCC`的动态链接库时的环境变量并运行`ctf`。

```bash
$ export LD_LIBRARY_PATH=`pwd`
$ ./ctf
$ echo $?
1
```

链接成功，程序运行了，程序退出码为1，表示有错误。



### 5.6 使用strings查看字符串

为了弄清楚二进制文件的功能，以及程序期望的输入类型，可以检查二进制文件是否包含有用的字符串。使用`strings`工具查看文件中的字符串。

```bash
$ strings ctf
/lib64/ld-linux-x86-64.so.2
lib5ae9b7f.so
__gmon_start__
_Jv_RegisterClasses
_ITM_deregisterTMCloneTable
_ITM_registerTMCloneTable
_Z8rc4_initP11rc4_state_tPhi
...
DEBUG: argv[1] = %s
checking '%s'
show_me_the_flag
>CMb
-v@P^:
flag = %s
guess again!
It's kinda like Louisiana. Or Dagobah. Dagobah - Where Yoda lives!
;*3$"
zPLR
GCC: (Ubuntu 5.4.0-6ubuntu1~16.04.4) 5.4.0 20160609
.shstrtab
.interp
.note.ABI-tag
.note.gnu.build-id
.gnu.hash
.dynsym
.dynstr
.gnu.version
.gnu.version_r
.rela.dyn
.rela.plt
.init
.plt.got
.text
.fini
.rodata
.eh_frame_hdr
.eh_frame
.gcc_except_table
.init_array
.fini_array
.jcr
.dynamic
.got.plt
.data
.bss
.comment
```

可以看出来几个有用的字符串：

```
DEBUG: argv[1] = %s
checking '%s'
show_me_the_flag
flag = %s
guess again!
It's kinda like Louisiana. Or Dagobah. Dagobah - Where Yoda lives!
```

可以看到这个程序可以接受一个参数，尝试下给定一个参数。

```bash
$ ./ctf show_me_the_flag
checking 'show_me_the_flag'
ok
$ echo $?
1
```

参数正确，但是退出码为1，还是有问题。继续查找问题。



### 5.7 使用strace和ltrace跟踪系统调用和库调用

`strace`跟踪程序的系统调用，`ltrace`跟踪程序的库调用。

```bash
$ strace ./ctf show_me_the_flag
execve("./ctf", ["./ctf", "show_me_the_flag"], 0x7fffdd7159b8 /* 17 vars */) = 0
brk(NULL)                               = 0x1bc4000
access("/etc/ld.so.preload", R_OK)      = -1 ENOENT (No such file or directory)
openat(AT_FDCWD, "/mnt/d/Cyberspace Security/Practical Binary Analysis code/chapter5/tls/haswell/x86_64/lib5ae9b7f.so", O_RDONLY|O_CLOEXEC) = -1 ENOENT (No such file or directory)
stat("/mnt/d/Cyberspace Security/Practical Binary Analysis code/chapter5/tls/haswell/x86_64", 0x7fffe0c765a0) = -1 ENOENT (No such file or directory)
openat(AT_FDCWD, "/mnt/d/Cyberspace Security/Practical Binary Analysis code/chapter5/tls/haswell/lib5ae9b7f.so", O_RDONLY|O_CLOEXEC) = -1 ENOENT (No such file or directory)
stat("/mnt/d/Cyberspace Security/Practical Binary Analysis code/chapter5/tls/haswell", 0x7fffe0c765a0) = -1 ENOENT (No such file or directory)
openat(AT_FDCWD, "/mnt/d/Cyberspace Security/Practical Binary Analysis code/chapter5/tls/x86_64/lib5ae9b7f.so", O_RDONLY|O_CLOEXEC) = -1 ENOENT (No such file or directory)
stat("/mnt/d/Cyberspace Security/Practical Binary Analysis code/chapter5/tls/x86_64", 0x7fffe0c765a0) = -1 ENOENT (No such file or directory)
openat(AT_FDCWD, "/mnt/d/Cyberspace Security/Practical Binary Analysis code/chapter5/tls/lib5ae9b7f.so", O_RDONLY|O_CLOEXEC) = -1 ENOENT (No such file or directory)
stat("/mnt/d/Cyberspace Security/Practical Binary Analysis code/chapter5/tls", 0x7fffe0c765a0) = -1 ENOENT (No such file or directory)
openat(AT_FDCWD, "/mnt/d/Cyberspace Security/Practical Binary Analysis code/chapter5/haswell/x86_64/lib5ae9b7f.so", O_RDONLY|O_CLOEXEC) = -1 ENOENT (No such file or directory)
stat("/mnt/d/Cyberspace Security/Practical Binary Analysis code/chapter5/haswell/x86_64", 0x7fffe0c765a0) = -1 ENOENT (No such file or directory)
openat(AT_FDCWD, "/mnt/d/Cyberspace Security/Practical Binary Analysis code/chapter5/haswell/lib5ae9b7f.so", O_RDONLY|O_CLOEXEC) = -1 ENOENT (No such file or directory)
stat("/mnt/d/Cyberspace Security/Practical Binary Analysis code/chapter5/haswell", 0x7fffe0c765a0) = -1 ENOENT (No such file or directory)
openat(AT_FDCWD, "/mnt/d/Cyberspace Security/Practical Binary Analysis code/chapter5/x86_64/lib5ae9b7f.so", O_RDONLY|O_CLOEXEC) = -1 ENOENT (No such file or directory)
stat("/mnt/d/Cyberspace Security/Practical Binary Analysis code/chapter5/x86_64", 0x7fffe0c765a0) = -1 ENOENT (No such file or directory)
openat(AT_FDCWD, "/mnt/d/Cyberspace Security/Practical Binary Analysis code/chapter5/lib5ae9b7f.so", O_RDONLY|O_CLOEXEC) = 3
read(3, "\177ELF\2\1\1\0\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0p\t\0\0\0\0\0\0"..., 832) = 832
fstat(3, {st_mode=S_IFREG|0777, st_size=10296, ...}) = 0
mmap(NULL, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7facbef40000
mmap(NULL, 2105440, PROT_READ|PROT_EXEC, MAP_PRIVATE|MAP_DENYWRITE, 3, 0) = 0x7facbed00000
mprotect(0x7facbed01000, 2097152, PROT_NONE) = 0
mmap(0x7facbef01000, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x1000) = 0x7facbef01000
close(3)                                = 0
openat(AT_FDCWD, "/mnt/d/Cyberspace Security/Practical Binary Analysis code/chapter5/libstdc++.so.6", O_RDONLY|O_CLOEXEC) = -1 ENOENT (No such file or directory)
openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3
fstat(3, {st_mode=S_IFREG|0644, st_size=14796, ...}) = 0
mmap(NULL, 14796, PROT_READ, MAP_PRIVATE, 3, 0) = 0x7facbef0c000
close(3)                                = 0
openat(AT_FDCWD, "/usr/lib/x86_64-linux-gnu/libstdc++.so.6", O_RDONLY|O_CLOEXEC) = 3
read(3, "\177ELF\2\1\1\3\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0\240\240\t\0\0\0\0\0"..., 832) = 832
fstat(3, {st_mode=S_IFREG|0644, st_size=1870824, ...}) = 0
mmap(NULL, 1886208, PROT_READ, MAP_PRIVATE|MAP_DENYWRITE, 3, 0) = 0x7facbeb30000
mmap(0x7facbebc6000, 901120, PROT_READ|PROT_EXEC, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x96000) = 0x7facbebc6000
mmap(0x7facbeca2000, 303104, PROT_READ, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x172000) = 0x7facbeca2000
mmap(0x7facbecec000, 57344, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x1bb000) = 0x7facbecec000
mmap(0x7facbecfa000, 10240, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, 0) = 0x7facbecfa000
close(3)                                = 0
openat(AT_FDCWD, "/mnt/d/Cyberspace Security/Practical Binary Analysis code/chapter5/libgcc_s.so.1", O_RDONLY|O_CLOEXEC) = -1 ENOENT (No such file or directory)
openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libgcc_s.so.1", O_RDONLY|O_CLOEXEC) = 3
read(3, "\177ELF\2\1\1\0\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0\0203\0\0\0\0\0\0"..., 832) = 832
fstat(3, {st_mode=S_IFREG|0644, st_size=100736, ...}) = 0
mmap(NULL, 103496, PROT_READ, MAP_PRIVATE|MAP_DENYWRITE, 3, 0) = 0x7facbeb10000
mmap(0x7facbeb13000, 69632, PROT_READ|PROT_EXEC, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x3000) = 0x7facbeb13000
mmap(0x7facbeb24000, 16384, PROT_READ, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x14000) = 0x7facbeb24000
mmap(0x7facbeb28000, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x17000) = 0x7facbeb28000
close(3)                                = 0
openat(AT_FDCWD, "/mnt/d/Cyberspace Security/Practical Binary Analysis code/chapter5/libc.so.6", O_RDONLY|O_CLOEXEC) = -1 ENOENT (No such file or directory)
openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libc.so.6", O_RDONLY|O_CLOEXEC) = 3
read(3, "\177ELF\2\1\1\3\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0@n\2\0\0\0\0\0"..., 832) = 832
fstat(3, {st_mode=S_IFREG|0755, st_size=1839792, ...}) = 0
mmap(NULL, 1852680, PROT_READ, MAP_PRIVATE|MAP_DENYWRITE, 3, 0) = 0x7facbe940000
mprotect(0x7facbe965000, 1662976, PROT_NONE) = 0
mmap(0x7facbe965000, 1355776, PROT_READ|PROT_EXEC, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x25000) = 0x7facbe965000
mmap(0x7facbeab0000, 303104, PROT_READ, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x170000) = 0x7facbeab0000
mmap(0x7facbeafb000, 24576, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x1ba000) = 0x7facbeafb000
mmap(0x7facbeb01000, 13576, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, 0) = 0x7facbeb01000
close(3)                                = 0
openat(AT_FDCWD, "/mnt/d/Cyberspace Security/Practical Binary Analysis code/chapter5/libm.so.6", O_RDONLY|O_CLOEXEC) = -1 ENOENT (No such file or directory)
openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libm.so.6", O_RDONLY|O_CLOEXEC) = 3
read(3, "\177ELF\2\1\1\3\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0\0\362\0\0\0\0\0\0"..., 832) = 832
fstat(3, {st_mode=S_IFREG|0644, st_size=1321344, ...}) = 0
mmap(NULL, 1323280, PROT_READ, MAP_PRIVATE|MAP_DENYWRITE, 3, 0) = 0x7facbe7f0000
mmap(0x7facbe7ff000, 630784, PROT_READ|PROT_EXEC, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0xf000) = 0x7facbe7ff000
mmap(0x7facbe899000, 626688, PROT_READ, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0xa9000) = 0x7facbe899000
mmap(0x7facbe932000, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x141000) = 0x7facbe932000
close(3)                                = 0
mmap(NULL, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7facbe7e0000
mmap(NULL, 12288, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7facbe7d0000
arch_prctl(ARCH_SET_FS, 0x7facbe7d0740) = 0
mprotect(0x7facbeafb000, 12288, PROT_READ) = 0
mprotect(0x7facbe932000, 4096, PROT_READ) = 0
mprotect(0x7facbeb28000, 4096, PROT_READ) = 0
mprotect(0x7facbecec000, 45056, PROT_READ) = 0
mprotect(0x7facbef01000, 4096, PROT_READ) = 0
mprotect(0x601000, 4096, PROT_READ)     = 0
mprotect(0x7facbef3a000, 4096, PROT_READ) = 0
munmap(0x7facbef0c000, 14796)           = 0
brk(NULL)                               = 0x1bc4000
brk(0x1be5000)                          = 0x1be5000
fstat(1, {st_mode=S_IFCHR|0660, st_rdev=makedev(0x4, 0x1), ...}) = 0
ioctl(1, TCGETS, {B38400 opost isig icanon echo ...}) = 0
write(1, "checking 'show_me_the_flag'\n", 28checking 'show_me_the_flag'
) = 28
write(1, "ok\n", 3ok
)                     = 3
exit_group(1)                           = ?
+++ exited with 1 +++
```

`strace`输出的系统调用对找到flag没有帮助，但是对二进制分析有用，对调试也有帮助。接着使用`ltrace`查看库调用信息。

```bash
$ ltrace -i -C ./ctf show_me_the_flag
[0x400fe9] __libc_start_main(0x400bc0, 2, 0x7fffea4d8648, 0x4010c0 <unfinished ...>
[0x400c44] __printf_chk(1, 0x401158, 0x7fffea4d885d, 224checking 'show_me_the_flag'
) = 28
[0x400c51] strcmp("show_me_the_flag", "show_me_the_flag") = 0
[0x400cf0] puts("ok"ok
)                                    = 3
[0x400d07] rc4_init(rc4_state_t*, unsigned char*, int)(0x7fffea4d8400, 0x4011c0, 66, -3442) = 0
[0x400d14] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::assign(char const*)(0x7fffea4d8340, 0x40117b, 58, 3) = 0x7fffea4d8340
[0x400d29] rc4_decrypt(rc4_state_t*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&)(0x7fffea4d83a0, 0x7fffea4d8400, 0x7fffea4d8340, 0x7e889f91) = 0x7fffea4d83a0
[0x400d36] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_assign(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)(0x7fffea4d8340, 0x7fffea4d83a0, 0x7fffea4d83b0, 0) = 0x7fffea4d8350
[0x400d53] getenv("GUESSME")                             = nil
[0xffffffffffffffff] +++ exited (status 1) +++
```

调用了`getenv`，这个函数是查看环境变量的，`getenv("GUESSME")`是获取`GUESSME`这个环境变量的意思。所以，可以猜测还是需要设置这个环境变量才能得到flag。

```bash
$ export GUESSME='foobar'
$ ltrace -i -C ./ctf show_me_the_flag
[0x400fe9] __libc_start_main(0x400bc0, 2, 0x7ffff373ce28, 0x4010c0 <unfinished ...>
[0x400c44] __printf_chk(1, 0x401158, 0x7ffff373d045, 224checking 'show_me_the_flag'
) = 28
[0x400c51] strcmp("show_me_the_flag", "show_me_the_flag") = 0
[0x400cf0] puts("ok"ok
)                                    = 3
[0x400d07] rc4_init(rc4_state_t*, unsigned char*, int)(0x7ffff373cbe0, 0x4011c0, 66, -3442) = 0
[0x400d14] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::assign(char const*)(0x7ffff373cb20, 0x40117b, 58, 3) = 0x7ffff373cb20
[0x400d29] rc4_decrypt(rc4_state_t*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&)(0x7ffff373cb80, 0x7ffff373cbe0, 0x7ffff373cb20, 0x7e889f91) = 0x7ffff373cb80
[0x400d36] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_assign(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)(0x7ffff373cb20, 0x7ffff373cb80, 0x7ffff373cb90, 0) = 0x7ffff373cb30
[0x400d53] getenv("GUESSME")                             = "foobar"
[0x400d6e] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::assign(char const*)(0x7ffff373cb40, 0x401183, 5, 0xffffffe0) = 0x7ffff373cb40
[0x400d88] rc4_decrypt(rc4_state_t*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&)(0x7ffff373cba0, 0x7ffff373cbe0, 0x7ffff373cb40, 0x1724f00) = 0x7ffff373cba0
[0x400d9a] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_assign(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)(0x7ffff373cb40, 0x7ffff373cba0, 0x1724f30, 0) = 0x1724ee0
[0x400db4] operator delete(void*)(0x1724f30, 0x1724f30, 21, 0) = 0
[0x400dd7] puts("guess again!"guess again!
)                          = 13
[0x400c8d] operator delete(void*)(0x1724ee0, 0x1723eb0, 0, 0x7fc0b448d1c0) = 0
[0xffffffffffffffff] +++ exited (status 1) +++
```



### 5.8 使用objdump反汇编

使用`objdump`围绕`guess again`调查，有助于了解字符串的地址以找出加载该字符串的第一条指令。

```bash
$ objdump -s --section .rodata ctf

ctf:     file format elf64-x86-64

Contents of section .rodata:
 401140 01000200 44454255 473a2061 7267765b  ....DEBUG: argv[
 401150 315d203d 20257300 63686563 6b696e67  1] = %s.checking
 401160 20272573 270a0073 686f775f 6d655f74   '%s'..show_me_t
 401170 68655f66 6c616700 6f6b004f 89df919f  he_flag.ok.O....
 401180 887e009a 5b38babe 27ac0e3e 434d6285  .~..[8..'..>CMb.
 401190 55868954 3848a34d 00192d76 40505e3a  U..T8H.M..-v@P^:
 4011a0 00726200 666c6167 203d2025 730a0067  .rb.flag = %s..g
 4011b0 75657373 20616761 696e2100 00000000  uess again!.....
 4011c0 49742773 206b696e 6461206c 696b6520  It's kinda like
 4011d0 4c6f7569 7369616e 612e204f 72204461  Louisiana. Or Da
 4011e0 676f6261 682e2044 61676f62 6168202d  gobah. Dagobah -
 4011f0 20576865 72652059 6f646120 6c697665   Where Yoda live
 401200 73210000 00000000                    s!......
```

`guess again`的地址为0x4011af。反汇编`ctf`找用到0x40011af的指令。

```bash
$ objdump -d  ctf | grep 0x4011af
  400dcd:       bf af 11 40 00          mov    $0x4011af,%edi
```

查看0x400dcd附近的指令：

```bash
$ objdump -d ctf
...
  400dc0:       0f b6 14 03             movzbl (%rbx,%rax,1),%edx
  400dc4:       84 d2                   test   %dl,%dl
  400dc6:       74 05                   je     400dcd <__gmon_start__@plt+0x21d>
  400dc8:       3a 14 01                cmp    (%rcx,%rax,1),%dl
  400dcb:       74 13                   je     400de0 <__gmon_start__@plt+0x230>
  400dcd:       bf af 11 40 00          mov    $0x4011af,%edi
  400dd2:       e8 d9 fc ff ff          callq  400ab0 <puts@plt>
  400dd7:       e9 84 fe ff ff          jmpq   400c60 <__gmon_start__@plt+0xb0>
  400ddc:       0f 1f 40 00             nopl   0x0(%rax)
  400de0:       48 83 c0 01             add    $0x1,%rax
  400de4:       48 83 f8 15             cmp    $0x15,%rax
  400de8:       75 d6                   jne    400dc0 <__gmon_start__@plt+0x210>
...
```

`ctf`程序会将从GUESSME环境变量中得到的字符串与rcx的字符串进行比较。如果可以转储rcx字符串，那就可以找到GUESSME的值。而静态分析做不到，所以需要动态分析。



### 5.9 使用gdb查看动态字符串缓冲区

gdb主要用于调试，也可用于动态分析。在0x400dc8处设置断点，并查看rcx字符串。

```bash
$ gdb ./ctf
GNU gdb (Debian 10.1-1.7) 10.1.90.20210103-git
Copyright (C) 2021 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
Type "show copying" and "show warranty" for details.
This GDB was configured as "x86_64-linux-gnu".
Type "show configuration" for configuration details.
For bug reporting instructions, please see:
<https://www.gnu.org/software/gdb/bugs/>.
Find the GDB manual and other documentation resources online at:
    <http://www.gnu.org/software/gdb/documentation/>.

For help, type "help".
Type "apropos word" to search for commands related to "word"...
Reading symbols from ./ctf...
(No debugging symbols found in ./ctf)
(gdb) b *0x400dc8
Breakpoint 1 at 0x400dc8
(gdb) set env GUESSME=foobar
(gdb) run show_me_the_flag
Starting program: /mnt/d/Cyberspace Security/Practical Binary Analysis code/chapter5/ctf show_me_the_flag
checking 'show_me_the_flag'
ok

Breakpoint 1, 0x0000000000400dc8 in ?? ()
(gdb) display/i $pc
1: x/i $pc
=> 0x400dc8:    cmp    (%rcx,%rax,1),%dl
(gdb) info registers rcx
rcx            0x615ee0            6381280
(gdb) info registers rax
rax            0x0                 0
(gdb) x/s 0x615ee0
0x615ee0:       "Crackers Don't Matter"
(gdb) quit
```

可以看到rcx字符串是`Crackers Don't Matter`，到此，已经得到了环境变量`GUESSME`的值。最后，输入正确的环境变量值，得到flag。

```bash
$ export GUESSME="Crackers Don't Matter"
$ ./ctf show_me_the_flag
checking 'show_me_the_flag'
ok
flag = 84b34c124b2ba5ca224af8e33b077e9e
```



### 5.10 练习

将flag传给oracle程序。

```bash
$ ./oracle 84b34c124b2ba5ca224af8e33b077e9e
./oracle: error while loading shared libraries: libcrypto.so.1.0.0: cannot open shared object file: No such file or directory
```

链接器显示缺少`libcrypto.so.1.0.0`。使用`find`并未在我的Linux系统找到该库，所以直接下载`libcrypto.so.1.0.0`。

**libcrypto** a full-strength general purpose cryptographic library. It constitutes the basis of the TLS implementation, but can also be used independently.

下载完`libcrypto.so.1.0.0`后，继续运行程序。

```bash
$ ./oracle 84b34c124b2ba5ca224af8e33b077e9e
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
| Level 1 completed, unlocked lvl2         |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
Run oracle with -h to show a hint
```

完成了第一关。



#### level 2

现在解锁了第二关。

```bash
$ ./oracle 84b34c124b2ba5ca224af8e33b077e9e -h
Combine the parts
```

提示`Combine the parts`，即"把各部分连接起来"。

查看`lvl2`文件类型，为`ELF`类型。

```bash
$ file lvl2
lvl2: ELF 64-bit LSB executable, x86-64, version 1 (SYSV), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, for GNU/Linux 2.6.32, BuildID[sha1]=457d7940f6a73d6505db1f022071ee7368b67ce9, stripped
```

查看`lvl2`的`.rodata`段，查看是否包含有用的信息。

```bash
$ objdump -s --section .rodata lvl2

lvl2:     file format elf64-x86-64

Contents of section .rodata:
 4006c0 01000200 30330034 66006334 00663600  ....03.4f.c4.f6.
 4006d0 61350033 36006632 00626600 37340066  a5.36.f2.bf.74.f
 4006e0 38006436 00643300 38310036 63006466  8.d6.d3.81.6c.df
 4006f0 00383800                             .88.
```

可以看到`.rodata`段里有16个16进制数，连接起来一共32位，联想到过第一关的flag也是由16个16进制数连接起来的32位数，我大胆猜测`lvl2`的`.rodata`段里所有的数拼接起来即为第二关的flag。

```bash
$ ./oracle 034fc4f6a536f2bf74f8d6d3816cdf88
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
| Level 2 completed, unlocked lvl3         |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
Run oracle with -h to show a hint
```

完成了第二关。



#### level 3

现在解锁第三关。

```bash
$ ./oracle 034fc4f6a536f2bf74f8d6d3816cdf88 -h
Fix four broken things
```

提示`Fix four broken things`，即"修复四个破碎的东西"。

```bash
$ readelf -h lvl3
ELF Header:
  Magic:   7f 45 4c 46 02 01 01 0b 00 00 00 00 00 00 00 00
  Class:                             ELF64
  Data:                              2's complement, little endian
  Version:                           1 (current)
  OS/ABI:                            Novell - Modesto
  ABI Version:                       0
  Type:                              EXEC (Executable file)
  Machine:                           Motorola Coldfire
  Version:                           0x1
  Entry point address:               0x4005d0
  Start of program headers:          4022250974 (bytes into file)
  Start of section headers:          4480 (bytes into file)
  Flags:                             0x0
  Size of this header:               64 (bytes)
  Size of program headers:           56 (bytes)
  Number of program headers:         9
  Size of section headers:           64 (bytes)
  Number of section headers:         29
  Section header string table index: 28
```

大胆猜测，该文件应该能够在`x86-64`的`Linux`的操作系统上运行，所以

- 首先`OS/ABI`不正确，应为`UNIX - System V`。`Magic`中的第8字节表示`OS/ABI`，故将第8字节修改为`0x00`表示`UNIX - System V`。
- 其次`Machine`不正确，应为`Machine: Advanced Micro Devices X86-64`。故将第18、19字节修改为`0x3e00`表示`Machine: Advanced Micro Devices X86-64`。
- 然后由ELF文件结构可知，程序头紧跟在ELF头后面，而ELF头为64字节大小，所以程序头的偏移量应该为64，所以`Start of program headers: 4022250974 (bytes into file)`不正确，应该为`Start of program headers: 64(bytes into file)`。故将第32至第37字节修改为`0x4000 0000 0000 0000`。

![image-20220804032628830](assets/Practical-Binary-Analysis/image-20220804032628830.png)

使用如下命令修改二进制文件：

1. `vi-b xx.bin` ，`xx.bin` 为你要修改的文档，`-b` 以二进制方式打开修改。
2. 命令模式下 ：`%！xxd` ，切换到16进制查看，便于修改
3. 进行修改操作
4. 命令模式下 ：`%！xxd -r` ,切换到二进制模式下
5. `wq` ，保存并退出

使用`readelf -S`查看各个段的情况。

```bash
$ readelf -S lvl3
...
[14] .text             NOBITS           0000000000400550  00000550
       00000000000001f2  0000000000000000  AX       0     0     16
...
```

`.text`的段类型应该`PROGBITS`类型，所以这里的`NOBITS`不正确。先计算出`.text`段在节表中的位置，即`section headers address + index * section entry size`。从`readelf -h lvl3`中可以看出`section headers address = 4480`，`section entry size = 64`。从`readelf -S lvl3`中可以看出`index = 14`。再根据`Elf_Shdr`的结构可知，`sh_type`在`Elf_Shdr`中的偏移量为`4`，所以`.text`段的`sh_type`字段在文件`lvl3`中的位置为`4480 + 64 * 14 + 4 = 5380`。故在`5380 = 0x1504`处将`NOBITS`改成`PROGBITS`类型的二进制表示形式`0x0100 0000`。

```c
typedef struct
{
  Elf64_Word    sh_name;                /* Section name (string tbl index) */
  Elf64_Word    sh_type;                /* Section type */
  Elf64_Xword   sh_flags;               /* Section flags */
  Elf64_Addr    sh_addr;                /* Section virtual addr at execution */
  Elf64_Off     sh_offset;              /* Section file offset */
  Elf64_Xword   sh_size;                /* Section size in bytes */
  Elf64_Word    sh_link;                /* Link to another section */
  Elf64_Word    sh_info;                /* Additional section information */
  Elf64_Xword   sh_addralign;           /* Section alignment */
  Elf64_Xword   sh_entsize;             /* Entry size if section holds table */
} Elf64_Shdr;
```

将`0x1504-0x1507`处改为`0x0100 0000`。

![image-20220804043458926](assets/Practical-Binary-Analysis/image-20220804043458926.png)

```bash
$ ./oracle 3a5c381e40d2fffd95ba4452a0fb4a40
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
| Level 3 completed, unlocked lvl4         |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
Run oracle with -h to show a hint
```

完成了第三关。



## 参考

1. 《编译系统透视：图解编译原理》，第8章-预处理
2. [Debugging with Symbols - Win32 apps | Microsoft Docs](https://docs.microsoft.com/en-us/windows/win32/dxtecharts/debugging-with-symbols)
3. [Tool Interface Standard (TIS) Executable and Linking Format (ELF)  Specification Version 1.2](https://refspecs.linuxfoundation.org/elf/elf.pdf)
4. [ELF 应用程序二进制接口 - 链接程序和库指南](https://docs.oracle.com/cd/E26926_01/html/E25910/glcfv.html#scrolltoc)
5. 《程序员的自我修养---链接、装载与库》
6. [PE Format - Win32 apps | Microsoft Docs](https://docs.microsoft.com/en-us/windows/win32/debug/pe-format)
7. [LIB BFD, the Binary File Descriptor Library (gnu.org)](https://ftp.gnu.org/old-gnu/Manuals/bfd-2.9.1/html_mono/bfd.html#SEC1)

