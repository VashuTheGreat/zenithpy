#include "zenith_builtins.h"
#include <time.h>

static ZenithValue builtin_print(size_t argc, ZenithValue* args) {
    for (size_t i = 0; i < argc; ++i) {
        zenith_value_print(args[i]);
        if (i + 1 < argc) printf(" ");
    }
    printf("\n");
    fflush(stdout);
    return zenith_val_none();
}

static ZenithValue builtin_len(size_t argc, ZenithValue* args) {
    if (argc < 1) return zenith_val_int(0);
    ZenithValue v = args[0];
    if (zenith_is_obj(v)) {
        ZenithHeader* hdr = (ZenithHeader*)zenith_as_obj(v);
        if (hdr->type == ZENITH_OBJ_LIST) {
            ZenithList* l = (ZenithList*)hdr;
            return zenith_val_int(l->count);
        } else if (hdr->type == ZENITH_OBJ_STRING) {
            ZenithString* s = (ZenithString*)hdr;
            return zenith_val_int(s->length);
        }
    }
    return zenith_val_int(0);
}

static ZenithValue builtin_time(size_t argc, ZenithValue* args) {
    (void)argc; (void)args;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    double sec = (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
    return zenith_val_float(sec);
}

static ZenithValue builtin_abs(size_t argc, ZenithValue* args) {
    if (argc < 1) return zenith_val_int(0);
    ZenithValue v = args[0];
    if (zenith_is_int(v)) {
        int64_t i = zenith_as_int(v);
        return zenith_val_int(i < 0 ? -i : i);
    } else if (zenith_is_float(v)) {
        return zenith_val_float(fabs(zenith_as_float(v)));
    }
    return v;
}

static ZenithValue builtin_sqrt(size_t argc, ZenithValue* args) {
    if (argc < 1) return zenith_val_float(0.0);
    double d = zenith_as_float(args[0]);
    return zenith_val_float(sqrt(d));
}

static ZenithValue builtin_int(size_t argc, ZenithValue* args) {
    if (argc < 1) return zenith_val_int(0);
    return zenith_val_int(zenith_as_int(args[0]));
}

static ZenithValue builtin_float(size_t argc, ZenithValue* args) {
    if (argc < 1) return zenith_val_float(0.0);
    return zenith_val_float(zenith_as_float(args[0]));
}

static ZenithValue builtin_min(size_t argc, ZenithValue* args) {
    if (argc == 0) return zenith_val_none();
    ZenithValue m = args[0];
    for (size_t i = 1; i < argc; ++i) {
        if (zenith_as_float(args[i]) < zenith_as_float(m)) {
            m = args[i];
        }
    }
    return m;
}

static ZenithValue builtin_max(size_t argc, ZenithValue* args) {
    if (argc == 0) return zenith_val_none();
    ZenithValue m = args[0];
    for (size_t i = 1; i < argc; ++i) {
        if (zenith_as_float(args[i]) > zenith_as_float(m)) {
            m = args[i];
        }
    }
    return m;
}

void zenith_register_all_builtins(ZenithVM* vm) {
    zenith_register_builtin(vm, "print", builtin_print);
    zenith_register_builtin(vm, "len", builtin_len);
    zenith_register_builtin(vm, "time", builtin_time);
    zenith_register_builtin(vm, "clock", builtin_time);
    zenith_register_builtin(vm, "abs", builtin_abs);
    zenith_register_builtin(vm, "sqrt", builtin_sqrt);
    zenith_register_builtin(vm, "int", builtin_int);
    zenith_register_builtin(vm, "float", builtin_float);
    zenith_register_builtin(vm, "min", builtin_min);
    zenith_register_builtin(vm, "max", builtin_max);
}
