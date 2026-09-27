#include "zenith.h"
#include "zenith_ast.h"
#include "zenith_vm.h"
#include <time.h>

static char* read_file(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Error: Could not open file '%s'\n", path);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }

    size_t read = fread(buffer, 1, size, file);
    buffer[read] = '\0';
    fclose(file);
    return buffer;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("ZenithPy %s (High-Performance Python Native Runtime)\n", ZENITH_VERSION);
        printf("Usage: zenithpy [options] <script.py>\n");
        printf("Options:\n");
        printf("  -c <code>       Execute inline Python code\n");
        printf("  -b, --bench     Print execution time benchmark\n");
        printf("  -v, --version   Display version\n");
        return 0;
    }

    bool benchmark = false;
    const char* filepath = NULL;
    const char* inline_code = NULL;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--bench") == 0) {
            benchmark = true;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("ZenithPy %s (x86_64 JIT & Native Assembly Engine)\n", ZENITH_VERSION);
            return 0;
        } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            inline_code = argv[++i];
        } else if (argv[i][0] != '-') {
            filepath = argv[i];
        }
    }

    char* source = NULL;
    if (inline_code) {
        source = strdup(inline_code);
    } else if (filepath) {
        source = read_file(filepath);
        if (!source) return 1;
    } else {
        fprintf(stderr, "Error: No input file or code specified.\n");
        return 1;
    }

    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    ZenithVM* vm = zenith_vm_new();
    if (!vm) {
        fprintf(stderr, "Error: Failed to initialize Zenith VM.\n");
        free(source);
        return 1;
    }

    ASTNode* root = zenith_parse(source, vm->arena);
    if (!root) {
        fprintf(stderr, "SyntaxError: Failed to parse Python source.\n");
        zenith_vm_free(vm);
        free(source);
        return 1;
    }

    ZenithResult res = zenith_run(vm, root);

    clock_gettime(CLOCK_MONOTONIC, &t_end);
    double elapsed_ms = (t_end.tv_sec - t_start.tv_sec) * 1000.0 + (t_end.tv_nsec - t_start.tv_nsec) * 1e-6;

    if (benchmark) {
        printf("\n[ZenithPy] Execution time: %.3f ms (%.4f s)\n", elapsed_ms, elapsed_ms / 1000.0);
    }

    zenith_vm_free(vm);
    free(source);
    return (res == ZENITH_OK) ? 0 : 1;
}
