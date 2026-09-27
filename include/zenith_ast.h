#ifndef ZENITH_AST_H
#define ZENITH_AST_H

#include "zenith.h"
#include "zenith_value.h"
#include "zenith_memory.h"

typedef enum {
    /* Literals & Variables */
    AST_LITERAL,
    AST_VARIABLE,
    AST_LIST_LITERAL,

    /* Binary & Unary */
    AST_BINOP,
    AST_UNARYOP,
    AST_COMPARE,
    AST_LOGICAL,

    /* Calls & Access */
    AST_CALL,
    AST_INDEX,

    /* Statements */
    AST_ASSIGN,
    AST_AUG_ASSIGN,
    AST_INDEX_ASSIGN,
    AST_IF,
    AST_WHILE,
    AST_FOR_RANGE,
    AST_RETURN,
    AST_BREAK,
    AST_CONTINUE,
    AST_PASS,
    AST_FUNC_DEF,
    AST_BLOCK,
    AST_EXPR_STMT
} ASTNodeType;

typedef enum {
    BINOP_ADD,
    BINOP_SUB,
    BINOP_MUL,
    BINOP_DIV,
    BINOP_FLOORDIV,
    BINOP_MOD,
    BINOP_POW,
    BINOP_BIT_AND,
    BINOP_BIT_OR,
    BINOP_BIT_XOR,
    BINOP_SHL,
    BINOP_SHR
} BinOpType;

typedef enum {
    UNARY_NEG,
    UNARY_NOT,
    UNARY_BIT_NOT
} UnaryOpType;

typedef enum {
    CMP_EQ,
    CMP_NE,
    CMP_LT,
    CMP_LE,
    CMP_GT,
    CMP_GE
} CmpOpType;

typedef enum {
    LOGIC_AND,
    LOGIC_OR
} LogicOpType;

typedef struct ASTNode ASTNode;

typedef struct ASTNodeList {
    ASTNode** items;
    size_t count;
    size_t capacity;
} ASTNodeList;

struct ASTNode {
    ASTNodeType type;
    int line;
    union {
        struct {
            ZenithValue value;
        } literal;

        struct {
            char* name;
            int local_index; /* resolved slot index */
        } variable;

        struct {
            ASTNodeList* elements;
        } list_literal;

        struct {
            BinOpType op;
            ASTNode* left;
            ASTNode* right;
        } binop;

        struct {
            UnaryOpType op;
            ASTNode* operand;
        } unaryop;

        struct {
            CmpOpType op;
            ASTNode* left;
            ASTNode* right;
        } compare;

        struct {
            LogicOpType op;
            ASTNode* left;
            ASTNode* right;
        } logical;

        struct {
            char* func_name;
            ASTNode* callee;
            ASTNodeList* args;
        } call;

        struct {
            ASTNode* target;
            ASTNode* index;
        } index;

        struct {
            char* target_name;
            int local_index;
            ASTNode* value;
        } assign;

        struct {
            char* target_name;
            int local_index;
            BinOpType op;
            ASTNode* value;
        } aug_assign;

        struct {
            ASTNode* target;
            ASTNode* index;
            ASTNode* value;
        } index_assign;

        struct {
            ASTNode* condition;
            ASTNode* then_block;
            ASTNode* else_block;
        } if_stmt;

        struct {
            ASTNode* condition;
            ASTNode* body;
        } while_stmt;

        struct {
            char* var_name;
            int local_index;
            ASTNode* start;
            ASTNode* end;
            ASTNode* step;
            ASTNode* body;
        } for_range;

        struct {
            ASTNode* value; /* NULL for empty return */
        } return_stmt;

        struct {
            char* name;
            char** param_names;
            size_t param_count;
            size_t local_count;
            ASTNode* body;
            void* jit_native_func; /* compiled native x86-64 code if JITted */
        } func_def;

        struct {
            ASTNodeList* stmts;
        } block;

        struct {
            ASTNode* expr;
        } expr_stmt;
    };
};

ASTNode* ast_new_node(ZenithArena* arena, ASTNodeType type, int line);
ASTNodeList* ast_node_list_new(ZenithArena* arena, size_t cap);
void ast_node_list_add(ZenithArena* arena, ASTNodeList* list, ASTNode* node);

/* Parse Python source code into AST */
ASTNode* zenith_parse(const char* source, ZenithArena* arena);

#endif /* ZENITH_AST_H */
