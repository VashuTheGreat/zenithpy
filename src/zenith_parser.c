#include "zenith_ast.h"
#include <ctype.h>

typedef enum {
    TOK_EOF,
    TOK_NEWLINE,
    TOK_INDENT,
    TOK_DEDENT,
    TOK_IDENT,
    TOK_INT,
    TOK_FLOAT,
    TOK_STRING,
    
    /* Keywords */
    TOK_DEF,
    TOK_RETURN,
    TOK_IF,
    TOK_ELIF,
    TOK_ELSE,
    TOK_WHILE,
    TOK_FOR,
    TOK_IN,
    TOK_BREAK,
    TOK_CONTINUE,
    TOK_PASS,
    TOK_AND,
    TOK_OR,
    TOK_NOT,
    TOK_TRUE,
    TOK_FALSE,
    TOK_NONE,

    /* Operators & Delimiters */
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_SLASHSLASH,
    TOK_PERCENT,
    TOK_STARSTAR,
    TOK_AMP,
    TOK_PIPE,
    TOK_CARET,
    TOK_LSHIFT,
    TOK_RSHIFT,
    TOK_TILDE,

    TOK_EQ,
    TOK_EQEQ,
    TOK_BANGEQ,
    TOK_LT,
    TOK_LTE,
    TOK_GT,
    TOK_GTE,

    TOK_PLUSEQ,
    TOK_MINUSEQ,
    TOK_STAREQ,
    TOK_SLASHEQ,

    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_COLON,
    TOK_COMMA
} TokenType;

typedef struct {
    TokenType type;
    int line;
    const char* start;
    size_t length;
    ZenithValue literal;
} Token;

typedef struct {
    const char* src;
    size_t pos;
    int line;
    ZenithArena* arena;

    int indent_stack[128];
    int indent_depth;
    int pending_dedents;
    bool at_line_start;

    Token previous;
    Token current;
    Token peek;
} Parser;


ASTNode* ast_new_node(ZenithArena* arena, ASTNodeType type, int line) {
    ASTNode* n = (ASTNode*)zenith_arena_alloc(arena, sizeof(ASTNode));
    if (!n) return NULL;
    memset(n, 0, sizeof(ASTNode));
    n->type = type;
    n->line = line;
    return n;
}


ASTNodeList* ast_node_list_new(ZenithArena* arena, size_t cap) {
    if (cap == 0) cap = 4;
    ASTNodeList* l = (ASTNodeList*)zenith_arena_alloc(arena, sizeof(ASTNodeList));
    l->count = 0;
    l->capacity = cap;
    l->items = (ASTNode**)zenith_arena_alloc(arena, sizeof(ASTNode*) * cap);
    return l;
}

void ast_node_list_add(ZenithArena* arena, ASTNodeList* list, ASTNode* node) {
    if (list->count >= list->capacity) {
        size_t new_cap = list->capacity * 2;
        ASTNode** new_items = (ASTNode**)zenith_arena_alloc(arena, sizeof(ASTNode*) * new_cap);
        memcpy(new_items, list->items, sizeof(ASTNode*) * list->count);
        list->items = new_items;
        list->capacity = new_cap;
    }
    list->items[list->count++] = node;
}

static char* arena_strdup_len(ZenithArena* arena, const char* str, size_t len) {
    char* copy = (char*)zenith_arena_alloc(arena, len + 1);
    memcpy(copy, str, len);
    copy[len] = '\0';
    return copy;
}

#define ZENITH_MAX_INTERNED 1024
static const char* intern_table[ZENITH_MAX_INTERNED];
static size_t intern_count = 0;

static const char* arena_intern_string(ZenithArena* arena, const char* str, size_t len) {
    for (size_t i = 0; i < intern_count; ++i) {
        if (strlen(intern_table[i]) == len && memcmp(intern_table[i], str, len) == 0) {
            return intern_table[i];
        }
    }
    char* copy = arena_strdup_len(arena, str, len);
    if (intern_count < ZENITH_MAX_INTERNED) {
        intern_table[intern_count++] = copy;
    }
    return copy;
}


/* Lexer */
static char lexer_peek(Parser* p) {
    return p->src[p->pos];
}

static char lexer_advance(Parser* p) {
    char c = p->src[p->pos];
    if (c != '\0') p->pos++;
    return c;
}

static void skip_whitespace_and_comments(Parser* p) {
    while (1) {
        char c = lexer_peek(p);
        if (c == ' ' || c == '\t' || c == '\r') {
            lexer_advance(p);
        } else if (c == '#') {
            while (lexer_peek(p) != '\n' && lexer_peek(p) != '\0') {
                lexer_advance(p);
            }
        } else {
            break;
        }
    }
}

static Token scan_token(Parser* p) {
    Token tok;
    tok.literal = zenith_val_none();

    if (p->pending_dedents > 0) {
        p->pending_dedents--;
        tok.type = TOK_DEDENT;
        tok.line = p->line;
        tok.start = "";
        tok.length = 0;
        return tok;
    }

    if (p->at_line_start) {
        int indent = 0;
        while (lexer_peek(p) == ' ' || lexer_peek(p) == '\t') {
            indent += (lexer_peek(p) == '\t') ? 4 : 1;
            lexer_advance(p);
        }

        char c = lexer_peek(p);
        if (c == '\n') {
            lexer_advance(p);
            p->line++;
            return scan_token(p); /* empty line */
        }
        if (c == '#') {
            while (lexer_peek(p) != '\n' && lexer_peek(p) != '\0') lexer_advance(p);
            if (lexer_peek(p) == '\n') {
                lexer_advance(p);
                p->line++;
                return scan_token(p);
            }
        }
        if (c == '\0') {
            p->at_line_start = false;
            tok.type = TOK_EOF;
            tok.line = p->line;
            return tok;
        }

        p->at_line_start = false;
        int current_indent = p->indent_stack[p->indent_depth];
        if (indent > current_indent) {
            p->indent_depth++;
            p->indent_stack[p->indent_depth] = indent;
            tok.type = TOK_INDENT;
            tok.line = p->line;
            tok.start = "";
            tok.length = 0;
            return tok;
        } else if (indent < current_indent) {
            while (p->indent_depth > 0 && p->indent_stack[p->indent_depth] > indent) {
                p->indent_depth--;
                p->pending_dedents++;
            }
            p->pending_dedents--;
            tok.type = TOK_DEDENT;
            tok.line = p->line;
            tok.start = "";
            tok.length = 0;
            return tok;
        }
    }

    skip_whitespace_and_comments(p);

    char c = lexer_peek(p);
    tok.line = p->line;
    tok.start = &p->src[p->pos];

    if (c == '\0') {
        if (p->indent_depth > 0) {
            p->indent_depth--;
            tok.type = TOK_DEDENT;
            tok.length = 0;
            return tok;
        }
        tok.type = TOK_EOF;
        tok.length = 0;
        return tok;
    }

    if (c == '\n') {
        lexer_advance(p);
        p->line++;
        p->at_line_start = true;
        tok.type = TOK_NEWLINE;
        tok.length = 1;
        return tok;
    }

    /* Numbers: int or float */
    if (isdigit((unsigned char)c)) {
        bool is_float = false;
        size_t start = p->pos;
        while (isdigit((unsigned char)lexer_peek(p))) {
            lexer_advance(p);
        }
        if (lexer_peek(p) == '.' && isdigit((unsigned char)p->src[p->pos + 1])) {
            is_float = true;
            lexer_advance(p); /* '.' */
            while (isdigit((unsigned char)lexer_peek(p))) lexer_advance(p);
        }
        if (lexer_peek(p) == 'e' || lexer_peek(p) == 'E') {
            is_float = true;
            lexer_advance(p);
            if (lexer_peek(p) == '+' || lexer_peek(p) == '-') lexer_advance(p);
            while (isdigit((unsigned char)lexer_peek(p))) lexer_advance(p);
        }

        tok.length = p->pos - start;
        char buf[64];
        if (tok.length < sizeof(buf)) {
            memcpy(buf, tok.start, tok.length);
            buf[tok.length] = '\0';
            if (is_float) {
                tok.type = TOK_FLOAT;
                tok.literal = zenith_val_float(atof(buf));
            } else {
                tok.type = TOK_INT;
                tok.literal = zenith_val_int(strtoll(buf, NULL, 10));
            }
        }
        return tok;
    }

    /* Strings */
    if (c == '"' || c == '\'') {
        char quote = lexer_advance(p);
        size_t start = p->pos;
        while (lexer_peek(p) != quote && lexer_peek(p) != '\0') {
            if (lexer_peek(p) == '\\') lexer_advance(p);
            lexer_advance(p);
        }
        size_t len = p->pos - start;
        if (lexer_peek(p) == quote) lexer_advance(p);

        char* str_data = arena_strdup_len(p->arena, &p->src[start], len);
        ZenithString* zstr = zenith_string_new(str_data, len);
        tok.type = TOK_STRING;
        tok.literal = zenith_val_obj(zstr);
        tok.length = len;
        return tok;
    }

    /* Identifiers / Keywords */
    if (isalpha((unsigned char)c) || c == '_') {
        size_t start = p->pos;
        while (isalnum((unsigned char)lexer_peek(p)) || lexer_peek(p) == '_') {
            lexer_advance(p);
        }
        tok.length = p->pos - start;
        
        #define CHECK_KEYWORD(kw, t) \
            if (tok.length == strlen(kw) && strncmp(tok.start, kw, tok.length) == 0) { \
                tok.type = t; return tok; \
            }

        CHECK_KEYWORD("def", TOK_DEF);
        CHECK_KEYWORD("return", TOK_RETURN);
        CHECK_KEYWORD("if", TOK_IF);
        CHECK_KEYWORD("elif", TOK_ELIF);
        CHECK_KEYWORD("else", TOK_ELSE);
        CHECK_KEYWORD("while", TOK_WHILE);
        CHECK_KEYWORD("for", TOK_FOR);
        CHECK_KEYWORD("in", TOK_IN);
        CHECK_KEYWORD("break", TOK_BREAK);
        CHECK_KEYWORD("continue", TOK_CONTINUE);
        CHECK_KEYWORD("pass", TOK_PASS);
        CHECK_KEYWORD("and", TOK_AND);
        CHECK_KEYWORD("or", TOK_OR);
        CHECK_KEYWORD("not", TOK_NOT);
        CHECK_KEYWORD("True", TOK_TRUE);
        CHECK_KEYWORD("False", TOK_FALSE);
        CHECK_KEYWORD("None", TOK_NONE);

        tok.type = TOK_IDENT;
        return tok;
    }

    /* Operators and punctuation */
    lexer_advance(p);
    switch (c) {
        case '+':
            if (lexer_peek(p) == '=') { lexer_advance(p); tok.type = TOK_PLUSEQ; tok.length = 2; }
            else { tok.type = TOK_PLUS; tok.length = 1; }
            break;
        case '-':
            if (lexer_peek(p) == '=') { lexer_advance(p); tok.type = TOK_MINUSEQ; tok.length = 2; }
            else { tok.type = TOK_MINUS; tok.length = 1; }
            break;
        case '*':
            if (lexer_peek(p) == '*') {
                lexer_advance(p);
                tok.type = TOK_STARSTAR; tok.length = 2;
            } else if (lexer_peek(p) == '=') {
                lexer_advance(p);
                tok.type = TOK_STAREQ; tok.length = 2;
            } else {
                tok.type = TOK_STAR; tok.length = 1;
            }
            break;
        case '/':
            if (lexer_peek(p) == '/') {
                lexer_advance(p);
                tok.type = TOK_SLASHSLASH; tok.length = 2;
            } else if (lexer_peek(p) == '=') {
                lexer_advance(p);
                tok.type = TOK_SLASHEQ; tok.length = 2;
            } else {
                tok.type = TOK_SLASH; tok.length = 1;
            }
            break;
        case '%': tok.type = TOK_PERCENT; tok.length = 1; break;
        case '&': tok.type = TOK_AMP; tok.length = 1; break;
        case '|': tok.type = TOK_PIPE; tok.length = 1; break;
        case '^': tok.type = TOK_CARET; tok.length = 1; break;
        case '~': tok.type = TOK_TILDE; tok.length = 1; break;
        case '=':
            if (lexer_peek(p) == '=') { lexer_advance(p); tok.type = TOK_EQEQ; tok.length = 2; }
            else { tok.type = TOK_EQ; tok.length = 1; }
            break;
        case '!':
            if (lexer_peek(p) == '=') { lexer_advance(p); tok.type = TOK_BANGEQ; tok.length = 2; }
            else { tok.type = TOK_EOF; }
            break;
        case '<':
            if (lexer_peek(p) == '=') { lexer_advance(p); tok.type = TOK_LTE; tok.length = 2; }
            else if (lexer_peek(p) == '<') { lexer_advance(p); tok.type = TOK_LSHIFT; tok.length = 2; }
            else { tok.type = TOK_LT; tok.length = 1; }
            break;
        case '>':
            if (lexer_peek(p) == '=') { lexer_advance(p); tok.type = TOK_GTE; tok.length = 2; }
            else if (lexer_peek(p) == '>') { lexer_advance(p); tok.type = TOK_RSHIFT; tok.length = 2; }
            else { tok.type = TOK_GT; tok.length = 1; }
            break;
        case '(': tok.type = TOK_LPAREN; tok.length = 1; break;
        case ')': tok.type = TOK_RPAREN; tok.length = 1; break;
        case '[': tok.type = TOK_LBRACKET; tok.length = 1; break;
        case ']': tok.type = TOK_RBRACKET; tok.length = 1; break;
        case ':': tok.type = TOK_COLON; tok.length = 1; break;
        case ',': tok.type = TOK_COMMA; tok.length = 1; break;
        default:  tok.type = TOK_EOF; tok.length = 1; break;
    }
    return tok;
}

static void next_token(Parser* p) {
    p->previous = p->current;
    p->current = p->peek;
    p->peek = scan_token(p);
}

static bool check(Parser* p, TokenType type) {
    return p->current.type == type;
}

static bool match(Parser* p, TokenType type) {
    if (check(p, type)) {
        next_token(p);
        return true;
    }
    return false;
}

static void match_newlines(Parser* p) {
    while (match(p, TOK_NEWLINE));
}

/* Forward declarations */
static ASTNode* parse_statement(Parser* p);
static ASTNode* parse_expression(Parser* p);
static ASTNode* parse_block(Parser* p);

/* Expression Parsing with Precedence */
static ASTNode* parse_primary(Parser* p) {
    int line = p->current.line;

    if (match(p, TOK_INT)) {
        ASTNode* n = ast_new_node(p->arena, AST_LITERAL, line);
        n->literal.value = p->previous.literal;
        return n;
    }
    if (match(p, TOK_FLOAT)) {
        ASTNode* n = ast_new_node(p->arena, AST_LITERAL, line);
        n->literal.value = p->previous.literal;
        return n;
    }
    if (match(p, TOK_STRING)) {
        ASTNode* n = ast_new_node(p->arena, AST_LITERAL, line);
        n->literal.value = p->previous.literal;
        return n;
    }
    if (match(p, TOK_TRUE)) {
        ASTNode* n = ast_new_node(p->arena, AST_LITERAL, line);
        n->literal.value = zenith_val_bool(true);
        return n;
    }
    if (match(p, TOK_FALSE)) {
        ASTNode* n = ast_new_node(p->arena, AST_LITERAL, line);
        n->literal.value = zenith_val_bool(false);
        return n;
    }
    if (match(p, TOK_NONE)) {
        ASTNode* n = ast_new_node(p->arena, AST_LITERAL, line);
        n->literal.value = zenith_val_none();
        return n;
    }
    if (match(p, TOK_IDENT)) {
        char* name = (char*)arena_intern_string(p->arena, p->previous.start, p->previous.length);
        ASTNode* n = ast_new_node(p->arena, AST_VARIABLE, line);
        n->variable.name = name;
        n->variable.local_index = -1;
        return n;
    }

    if (match(p, TOK_LPAREN)) {
        ASTNode* expr = parse_expression(p);
        match(p, TOK_RPAREN);
        return expr;
    }

    if (match(p, TOK_LBRACKET)) {
        ASTNode* n = ast_new_node(p->arena, AST_LIST_LITERAL, line);
        n->list_literal.elements = ast_node_list_new(p->arena, 4);
        if (!check(p, TOK_RBRACKET)) {
            do {
                ast_node_list_add(p->arena, n->list_literal.elements, parse_expression(p));
            } while (match(p, TOK_COMMA) && !check(p, TOK_RBRACKET));
        }
        match(p, TOK_RBRACKET);
        return n;
    }

    return NULL;
}

static ASTNode* parse_postfix(Parser* p) {
    ASTNode* expr = parse_primary(p);
    if (!expr) return NULL;

    while (1) {
        if (match(p, TOK_LPAREN)) {
            /* Function call */
            ASTNode* call_node = ast_new_node(p->arena, AST_CALL, p->current.line);
            call_node->call.callee = expr;
            if (expr->type == AST_VARIABLE) {
                call_node->call.func_name = expr->variable.name;
            } else {
                call_node->call.func_name = NULL;
            }
            call_node->call.args = ast_node_list_new(p->arena, 4);
            if (!check(p, TOK_RPAREN)) {
                do {
                    ast_node_list_add(p->arena, call_node->call.args, parse_expression(p));
                } while (match(p, TOK_COMMA) && !check(p, TOK_RPAREN));
            }
            match(p, TOK_RPAREN);
            expr = call_node;
        } else if (match(p, TOK_LBRACKET)) {
            /* Indexing */
            ASTNode* idx_node = ast_new_node(p->arena, AST_INDEX, p->current.line);
            idx_node->index.target = expr;
            idx_node->index.index = parse_expression(p);
            match(p, TOK_RBRACKET);
            expr = idx_node;
        } else {
            break;
        }
    }
    return expr;
}

static ASTNode* parse_unary(Parser* p) {
    if (match(p, TOK_MINUS)) {
        ASTNode* n = ast_new_node(p->arena, AST_UNARYOP, p->current.line);
        n->unaryop.op = UNARY_NEG;
        n->unaryop.operand = parse_unary(p);
        return n;
    }
    if (match(p, TOK_NOT)) {
        ASTNode* n = ast_new_node(p->arena, AST_UNARYOP, p->current.line);
        n->unaryop.op = UNARY_NOT;
        n->unaryop.operand = parse_unary(p);
        return n;
    }
    if (match(p, TOK_TILDE)) {
        ASTNode* n = ast_new_node(p->arena, AST_UNARYOP, p->current.line);
        n->unaryop.op = UNARY_BIT_NOT;
        n->unaryop.operand = parse_unary(p);
        return n;
    }
    return parse_postfix(p);
}

static ASTNode* parse_power(Parser* p) {
    ASTNode* left = parse_unary(p);
    if (match(p, TOK_STARSTAR)) {
        ASTNode* n = ast_new_node(p->arena, AST_BINOP, p->current.line);
        n->binop.op = BINOP_POW;
        n->binop.left = left;
        n->binop.right = parse_power(p); /* right-associative */
        return n;
    }
    return left;
}

static ASTNode* parse_multiplicative(Parser* p) {
    ASTNode* left = parse_power(p);
    while (check(p, TOK_STAR) || check(p, TOK_SLASH) || check(p, TOK_SLASHSLASH) || check(p, TOK_PERCENT)) {
        TokenType op_tok = p->current.type;
        next_token(p);
        BinOpType op = BINOP_MUL;
        if (op_tok == TOK_SLASH) op = BINOP_DIV;
        else if (op_tok == TOK_SLASHSLASH) op = BINOP_FLOORDIV;
        else if (op_tok == TOK_PERCENT) op = BINOP_MOD;

        ASTNode* n = ast_new_node(p->arena, AST_BINOP, p->current.line);
        n->binop.op = op;
        n->binop.left = left;
        n->binop.right = parse_power(p);
        left = n;
    }
    return left;
}

static ASTNode* parse_additive(Parser* p) {
    ASTNode* left = parse_multiplicative(p);
    while (check(p, TOK_PLUS) || check(p, TOK_MINUS)) {
        TokenType op_tok = p->current.type;
        next_token(p);
        BinOpType op = (op_tok == TOK_PLUS) ? BINOP_ADD : BINOP_SUB;

        ASTNode* n = ast_new_node(p->arena, AST_BINOP, p->current.line);
        n->binop.op = op;
        n->binop.left = left;
        n->binop.right = parse_multiplicative(p);
        left = n;
    }
    return left;
}

static ASTNode* parse_shift(Parser* p) {
    ASTNode* left = parse_additive(p);
    while (check(p, TOK_LSHIFT) || check(p, TOK_RSHIFT)) {
        TokenType op_tok = p->current.type;
        next_token(p);
        BinOpType op = (op_tok == TOK_LSHIFT) ? BINOP_SHL : BINOP_SHR;
        ASTNode* n = ast_new_node(p->arena, AST_BINOP, p->current.line);
        n->binop.op = op;
        n->binop.left = left;
        n->binop.right = parse_additive(p);
        left = n;
    }
    return left;
}

static ASTNode* parse_bitwise_and(Parser* p) {
    ASTNode* left = parse_shift(p);
    while (match(p, TOK_AMP)) {
        ASTNode* n = ast_new_node(p->arena, AST_BINOP, p->current.line);
        n->binop.op = BINOP_BIT_AND;
        n->binop.left = left;
        n->binop.right = parse_shift(p);
        left = n;
    }
    return left;
}

static ASTNode* parse_bitwise_xor(Parser* p) {
    ASTNode* left = parse_bitwise_and(p);
    while (match(p, TOK_CARET)) {
        ASTNode* n = ast_new_node(p->arena, AST_BINOP, p->current.line);
        n->binop.op = BINOP_BIT_XOR;
        n->binop.left = left;
        n->binop.right = parse_bitwise_and(p);
        left = n;
    }
    return left;
}

static ASTNode* parse_bitwise_or(Parser* p) {
    ASTNode* left = parse_bitwise_xor(p);
    while (match(p, TOK_PIPE)) {
        ASTNode* n = ast_new_node(p->arena, AST_BINOP, p->current.line);
        n->binop.op = BINOP_BIT_OR;
        n->binop.left = left;
        n->binop.right = parse_bitwise_xor(p);
        left = n;
    }
    return left;
}

static ASTNode* parse_comparison(Parser* p) {
    ASTNode* left = parse_bitwise_or(p);
    while (check(p, TOK_EQEQ) || check(p, TOK_BANGEQ) || check(p, TOK_LT) ||
           check(p, TOK_LTE) || check(p, TOK_GT) || check(p, TOK_GTE)) {
        TokenType op_tok = p->current.type;
        next_token(p);
        CmpOpType op = CMP_EQ;
        if (op_tok == TOK_BANGEQ) op = CMP_NE;
        else if (op_tok == TOK_LT) op = CMP_LT;
        else if (op_tok == TOK_LTE) op = CMP_LE;
        else if (op_tok == TOK_GT) op = CMP_GT;
        else if (op_tok == TOK_GTE) op = CMP_GE;

        ASTNode* n = ast_new_node(p->arena, AST_COMPARE, p->current.line);
        n->compare.op = op;
        n->compare.left = left;
        n->compare.right = parse_bitwise_or(p);
        left = n;
    }
    return left;
}

static ASTNode* parse_logical_and(Parser* p) {
    ASTNode* left = parse_comparison(p);
    while (match(p, TOK_AND)) {
        ASTNode* n = ast_new_node(p->arena, AST_LOGICAL, p->current.line);
        n->logical.op = LOGIC_AND;
        n->logical.left = left;
        n->logical.right = parse_comparison(p);
        left = n;
    }
    return left;
}

static ASTNode* parse_logical_or(Parser* p) {
    ASTNode* left = parse_logical_and(p);
    while (match(p, TOK_OR)) {
        ASTNode* n = ast_new_node(p->arena, AST_LOGICAL, p->current.line);
        n->logical.op = LOGIC_OR;
        n->logical.left = left;
        n->logical.right = parse_logical_and(p);
        left = n;
    }
    return left;
}

static ASTNode* parse_expression(Parser* p) {
    return parse_logical_or(p);
}

/* Statement Parsing */
static ASTNode* parse_block(Parser* p) {
    match(p, TOK_NEWLINE);
    match(p, TOK_INDENT);

    ASTNode* block = ast_new_node(p->arena, AST_BLOCK, p->current.line);
    block->block.stmts = ast_node_list_new(p->arena, 8);

    while (!check(p, TOK_DEDENT) && !check(p, TOK_EOF)) {
        while (match(p, TOK_NEWLINE));
        if (check(p, TOK_DEDENT) || check(p, TOK_EOF)) break;
        ASTNode* stmt = parse_statement(p);
        if (stmt) {
            ast_node_list_add(p->arena, block->block.stmts, stmt);
        }
    }
    match(p, TOK_DEDENT);
    return block;
}

static ASTNode* parse_statement(Parser* p) {
    int line = p->current.line;

    if (match(p, TOK_PASS)) {
        match(p, TOK_NEWLINE);
        return ast_new_node(p->arena, AST_PASS, line);
    }
    if (match(p, TOK_BREAK)) {
        match(p, TOK_NEWLINE);
        return ast_new_node(p->arena, AST_BREAK, line);
    }
    if (match(p, TOK_CONTINUE)) {
        match(p, TOK_NEWLINE);
        return ast_new_node(p->arena, AST_CONTINUE, line);
    }
    if (match(p, TOK_RETURN)) {
        ASTNode* n = ast_new_node(p->arena, AST_RETURN, line);
        if (!check(p, TOK_NEWLINE) && !check(p, TOK_EOF)) {
            n->return_stmt.value = parse_expression(p);
        } else {
            n->return_stmt.value = NULL;
        }
        match(p, TOK_NEWLINE);
        return n;
    }

    if (match(p, TOK_IF)) {
        ASTNode* n = ast_new_node(p->arena, AST_IF, line);
        n->if_stmt.condition = parse_expression(p);
        match(p, TOK_COLON);
        n->if_stmt.then_block = parse_block(p);

        if (match(p, TOK_ELIF)) {
            /* Desugar elif to else: if ... */
            ASTNode* elif_block = ast_new_node(p->arena, AST_BLOCK, p->current.line);
            elif_block->block.stmts = ast_node_list_new(p->arena, 1);
            
            ASTNode* nested_if = ast_new_node(p->arena, AST_IF, p->current.line);
            nested_if->if_stmt.condition = parse_expression(p);
            match(p, TOK_COLON);
            nested_if->if_stmt.then_block = parse_block(p);
            nested_if->if_stmt.else_block = NULL; /* can recurse if needed */
            ast_node_list_add(p->arena, elif_block->block.stmts, nested_if);
            n->if_stmt.else_block = elif_block;
        } else if (match(p, TOK_ELSE)) {
            match(p, TOK_COLON);
            n->if_stmt.else_block = parse_block(p);
        } else {
            n->if_stmt.else_block = NULL;
        }
        return n;
    }

    if (match(p, TOK_WHILE)) {
        ASTNode* n = ast_new_node(p->arena, AST_WHILE, line);
        n->while_stmt.condition = parse_expression(p);
        match(p, TOK_COLON);
        n->while_stmt.body = parse_block(p);
        return n;
    }

    if (match(p, TOK_FOR)) {
        /* for var in range(...) */
        if (!match(p, TOK_IDENT)) return NULL;
        char* var_name = (char*)arena_intern_string(p->arena, p->previous.start, p->previous.length);
        match(p, TOK_IN);
        
        ASTNode* n = ast_new_node(p->arena, AST_FOR_RANGE, line);
        n->for_range.var_name = var_name;
        n->for_range.local_index = -1;

        if (match(p, TOK_IDENT)) {
            /* Expecting range */
            if (match(p, TOK_LPAREN)) {
                ASTNode* a1 = parse_expression(p);
                if (match(p, TOK_COMMA)) {
                    ASTNode* a2 = parse_expression(p);
                    if (match(p, TOK_COMMA)) {
                        ASTNode* a3 = parse_expression(p);
                        n->for_range.start = a1;
                        n->for_range.end = a2;
                        n->for_range.step = a3;
                    } else {
                        n->for_range.start = a1;
                        n->for_range.end = a2;
                        n->for_range.step = NULL;
                    }
                } else {
                    /* single arg: range(N) -> start=0, end=N */
                    ASTNode* zero = ast_new_node(p->arena, AST_LITERAL, line);
                    zero->literal.value = zenith_val_int(0);
                    n->for_range.start = zero;
                    n->for_range.end = a1;
                    n->for_range.step = NULL;
                }
                match(p, TOK_RPAREN);
            }
        }
        match(p, TOK_COLON);
        n->for_range.body = parse_block(p);
        return n;
    }

    if (match(p, TOK_DEF)) {
        if (!match(p, TOK_IDENT)) return NULL;
        char* fn_name = (char*)arena_intern_string(p->arena, p->previous.start, p->previous.length);
        match(p, TOK_LPAREN);

        ASTNode* n = ast_new_node(p->arena, AST_FUNC_DEF, line);
        n->func_def.name = fn_name;
        n->func_def.param_names = (char**)zenith_arena_alloc(p->arena, sizeof(char*) * 16);
        n->func_def.param_count = 0;
        n->func_def.local_count = 0;
        n->func_def.jit_native_func = NULL;

        if (!check(p, TOK_RPAREN)) {
            do {
                if (match(p, TOK_IDENT)) {
                    char* param = (char*)arena_intern_string(p->arena, p->previous.start, p->previous.length);
                    n->func_def.param_names[n->func_def.param_count++] = param;
                }
            } while (match(p, TOK_COMMA) && !check(p, TOK_RPAREN));
        }
        match(p, TOK_RPAREN);
        match(p, TOK_COLON);
        n->func_def.body = parse_block(p);
        return n;
    }


    /* Assignment or Expression statement */
    ASTNode* expr = parse_expression(p);
    if (!expr) return NULL;

    /* Check if assignment: var = expr */
    if (expr->type == AST_VARIABLE && match(p, TOK_EQ)) {
        ASTNode* assign = ast_new_node(p->arena, AST_ASSIGN, line);
        assign->assign.target_name = expr->variable.name;
        assign->assign.local_index = -1;
        assign->assign.value = parse_expression(p);
        match_newlines(p);
        return assign;
    }
    /* Augmented assignment: var += expr */
    if (expr->type == AST_VARIABLE && (check(p, TOK_PLUSEQ) || check(p, TOK_MINUSEQ) || check(p, TOK_STAREQ) || check(p, TOK_SLASHEQ))) {
        TokenType op_tok = p->current.type;
        next_token(p);
        BinOpType op = BINOP_ADD;
        if (op_tok == TOK_MINUSEQ) op = BINOP_SUB;
        else if (op_tok == TOK_STAREQ) op = BINOP_MUL;
        else if (op_tok == TOK_SLASHEQ) op = BINOP_DIV;

        ASTNode* aug = ast_new_node(p->arena, AST_AUG_ASSIGN, line);
        aug->aug_assign.target_name = expr->variable.name;
        aug->aug_assign.local_index = -1;
        aug->aug_assign.op = op;
        aug->aug_assign.value = parse_expression(p);
        match_newlines(p);
        return aug;
    }
    /* Index assignment: a[i] = expr */
    if (expr->type == AST_INDEX && match(p, TOK_EQ)) {
        ASTNode* idx_assign = ast_new_node(p->arena, AST_INDEX_ASSIGN, line);
        idx_assign->index_assign.target = expr->index.target;
        idx_assign->index_assign.index = expr->index.index;
        idx_assign->index_assign.value = parse_expression(p);
        match_newlines(p);
        return idx_assign;
    }

    match_newlines(p);
    ASTNode* stmt = ast_new_node(p->arena, AST_EXPR_STMT, line);

    stmt->expr_stmt.expr = expr;
    return stmt;
}

ASTNode* zenith_parse(const char* source, ZenithArena* arena) {
    Parser parser;
    memset(&parser, 0, sizeof(Parser));
    parser.src = source;
    parser.pos = 0;
    parser.line = 1;
    parser.arena = arena;
    parser.indent_stack[0] = 0;
    parser.indent_depth = 0;
    parser.pending_dedents = 0;
    parser.at_line_start = true;

    /* prime the pump */
    parser.peek = scan_token(&parser);
    next_token(&parser);

    ASTNode* root = ast_new_node(arena, AST_BLOCK, 1);
    root->block.stmts = ast_node_list_new(arena, 16);

    while (!check(&parser, TOK_EOF)) {
        while (match(&parser, TOK_NEWLINE));
        if (check(&parser, TOK_EOF)) break;
        ASTNode* stmt = parse_statement(&parser);
        if (stmt) {
            ast_node_list_add(arena, root->block.stmts, stmt);
        } else {
            next_token(&parser); /* recovery */
        }
    }

    return root;
}
