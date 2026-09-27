#include "zenith_value.h"
#include "zenith_memory.h"

ZenithString* zenith_string_new(const char* str, size_t len) {
    size_t total_size = sizeof(ZenithString) + len + 1;
    ZenithString* s = (ZenithString*)malloc(total_size);
    if (!s) return NULL;
    s->header.type = ZENITH_OBJ_STRING;
    s->header.flags = 0;
    s->header.next = NULL;
    s->length = (uint32_t)len;

    /* FNV-1a hash */
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < len; ++i) {
        hash ^= (uint8_t)str[i];
        hash *= 16777619u;
    }
    s->hash = hash;

    if (str && len > 0) {
        memcpy(s->chars, str, len);
    }
    s->chars[len] = '\0';
    return s;
}

ZenithList* zenith_list_new(size_t initial_cap) {
    if (initial_cap == 0) initial_cap = 8;
    ZenithList* list = (ZenithList*)malloc(sizeof(ZenithList));
    if (!list) return NULL;
    list->header.type = ZENITH_OBJ_LIST;
    list->header.flags = 0;
    list->header.next = NULL;
    list->count = 0;
    list->capacity = (uint32_t)initial_cap;
    list->items = (ZenithValue*)malloc(sizeof(ZenithValue) * initial_cap);
    return list;
}

void zenith_list_append(ZenithList* list, ZenithValue val) {
    if (!list) return;
    if (list->count >= list->capacity) {
        uint32_t new_cap = list->capacity * 2;
        if (new_cap < 8) new_cap = 8;
        ZenithValue* new_items = (ZenithValue*)realloc(list->items, sizeof(ZenithValue) * new_cap);
        if (!new_items) return;
        list->items = new_items;
        list->capacity = new_cap;
    }
    list->items[list->count++] = val;
}

ZenithValue zenith_list_get(ZenithList* list, int64_t index) {
    if (!list) return zenith_val_none();
    if (index < 0) index += list->count;
    if (index < 0 || (uint32_t)index >= list->count) {
        return zenith_val_none();
    }
    return list->items[index];
}

void zenith_list_set(ZenithList* list, int64_t index, ZenithValue val) {
    if (!list) return;
    if (index < 0) index += list->count;
    if (index >= 0 && (uint32_t)index < list->count) {
        list->items[index] = val;
    }
}

bool zenith_value_equal(ZenithValue a, ZenithValue b) {
    if (a == b) return true;
    if (zenith_is_number(a) && zenith_is_number(b)) {
        return zenith_as_float(a) == zenith_as_float(b);
    }
    if (zenith_is_obj(a) && zenith_is_obj(b)) {
        ZenithHeader* ha = (ZenithHeader*)zenith_as_obj(a);
        ZenithHeader* hb = (ZenithHeader*)zenith_as_obj(b);
        if (ha->type == ZENITH_OBJ_STRING && hb->type == ZENITH_OBJ_STRING) {
            ZenithString* sa = (ZenithString*)ha;
            ZenithString* sb = (ZenithString*)hb;
            if (sa->length != sb->length) return false;
            return memcmp(sa->chars, sb->chars, sa->length) == 0;
        }
    }
    return false;
}

void zenith_value_print(ZenithValue v) {
    if (zenith_is_int(v)) {
        printf("%ld", (long)zenith_as_int(v));
    } else if (zenith_is_float(v)) {
        double d = zenith_as_float(v);
        if (floor(d) == d && !isinf(d) && !isnan(d)) {
            printf("%.1f", d);
        } else {
            printf("%g", d);
        }
    } else if (zenith_is_bool(v)) {
        printf("%s", zenith_as_bool(v) ? "True" : "False");
    } else if (zenith_is_none(v)) {
        printf("None");
    } else if (zenith_is_obj(v)) {
        ZenithHeader* hdr = (ZenithHeader*)zenith_as_obj(v);
        if (!hdr) {
            printf("None");
            return;
        }
        switch (hdr->type) {
            case ZENITH_OBJ_STRING: {
                ZenithString* s = (ZenithString*)hdr;
                printf("%s", s->chars);
                break;
            }
            case ZENITH_OBJ_LIST: {
                ZenithList* l = (ZenithList*)hdr;
                printf("[");
                for (uint32_t i = 0; i < l->count; ++i) {
                    zenith_value_print(l->items[i]);
                    if (i + 1 < l->count) printf(", ");
                }
                printf("]");
                break;
            }
            default:
                printf("<object at %p>", (void*)hdr);
                break;
        }
    }
}

void zenith_value_println(ZenithValue v) {
    zenith_value_print(v);
    printf("\n");
}
