#include "../inc/loader.h"

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