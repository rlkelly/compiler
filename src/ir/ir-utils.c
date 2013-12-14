#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "../include/ir.h"
#include "../include/utilities.h"
#include "../../y.tab.h"
#include "../include/parse-tree.h"

FILE *output;

static int reg_idx = 0;
IrList *ir_list = NULL;
Boolean is_function_def_spec = FALSE;


char *current_reg(void) {
    char *buf;
    util_emalloc((void **) &buf, MAX_REG_LEN);
    snprintf(buf, MAX_REG_LEN, "$r%d", reg_idx);
    return buf;
}

char *next_reg() {
    char *buf;
    util_emalloc((void **) &buf, MAX_REG_LEN);
    snprintf(buf, MAX_REG_LEN, "$r%d", ++reg_idx);
    return buf;
}

IrNode *irn_load_from_register(int instr, int to_idx, int from_idx) {
    return create_ir_node(instr, to_idx, from_idx, NR, NULL);
}

IrNode *irn_store_from_register(int instr, int from_idx, int to_idx) {
    return create_ir_node(instr, from_idx, to_idx, NR, NULL);
}

IrNode *irn_load_from_global(int instr, int to_idx, Symbol *s) {
    return create_ir_node(instr, to_idx, NR, NR, s);
}

IrNode *irn_load_number_constant(int instr, int to_idx, int val) {
    return create_ir_node(instr, to_idx, val, NR, NULL);
}

IrNode *irn_store_number_constant(int instr, int val, int to_idx) {
    return create_ir_node(instr, val, to_idx, NR, NULL);
}

IrNode *irn_binary_expr(int instr, int to_idx, int oprnd1, int oprnd2) {
    return create_ir_node(instr, to_idx, oprnd1, oprnd2, NULL);
}

IrNode *irn_statement(int instr, int res_idx) {
    return create_ir_node(instr, res_idx, NR, NR, NULL);
}

IrNode *irn_function(int instr, Symbol *s) {
    return create_ir_node(instr, NR, NR, NR, s);
}

IrNode *create_ir_node(int instr, int n1, int n2, int n3, Symbol *s) {
    IrNode *irn;
    util_emalloc((void *) &irn, sizeof(IrNode));
    irn->instruction = instr;
    irn->n1 = n1;
    irn->n2 = n2;
    irn->n3 = n3;
    irn->s = s;
    switch (instr) {
        case RETURNED_WORD:
            break;
        default:
            break;
    }

    return irn;
}

IrList *create_ir_list(void) {
    IrList *irl;
    util_emalloc((void **) &irl, sizeof(IrList));
    irl->head = NULL;
    irl->tail = NULL;
    irl->cur = NULL;
    return irl;
}

IrNode *append_ir_node(IrNode *irn, IrList *irl) {
    if (irl->head == NULL) {
        irl->head = irn;
        irn->prev = NULL;
    } else {
        irl->tail->next = irn;
        irn->prev = irl->tail;
        irn->next = NULL;
    }
    irn->next = NULL;
    irl->tail = irn;

    return irl->tail;
}

int instruction(IrNode *irn) {
    if (irn == NULL) {
        return NO_IR_INSTRUCTION;
    }
    return irn->instruction;
}

Boolean node_is_lvalue(Node *n) {
    return n->expr->lvalue;
}

void start_ir_computation(void) {
    ir_list = create_ir_list();
}

void compute_ir(Node *n, IrList *irl) {
    IrNode *irn1, *irn2, *irn3;
    Node *child1, *child2;
    if (n == NULL) {
        return;
    }

    /* append IrNodes to the IrList  */

    /* for expressions compute_ir will update the node: */
    /* type: assume SIGNED_INT unless node is a symbol with a known type */
    /* lvalue: yes or no */
    /* location: register index of its result */

    switch (n->n_type) {
        case FUNCTION_DEF_SPEC:
            is_function_def_spec = TRUE;
            compute_ir(n->children.child1, irl);
            compute_ir(n->children.child2, irl);
            is_function_def_spec = FALSE;
            break;
        case FUNCTION_DECLARATOR:
            child1 = n->children.child1;
            child2 = n->children.child2;
            compute_ir(child1, irl);
            compute_ir(child2, irl);
            if (is_function_def_spec) {
                irn1 = irn_function(BEGIN_PROC, child1->st_entry);
                append_ir_node(irn1, irl);
            }
            break;
        case ASSIGNMENT_EXPR:
            child1 = n->children.child1;
            child2 = n->children.child2;
            compute_ir(child1, irl);
            compute_ir(child2, irl);
            irn1 = irn_store_number_constant(STORE_WORD_INDIRECT,
                            child2->expr->location, child1->expr->location);
            append_ir_node(irn1, irl);
            break;
        case BINARY_EXPR:
            child1 = n->children.child1;
            child2 = n->children.child2;
            compute_ir(child1, irl);
            compute_ir(child2, irl);

            n->expr->lvalue = FALSE;
            n->expr->location = ++reg_idx;
            irn1 = irn_binary_expr(LOG_OR, n->expr->location,
                                child2->expr->location, child1->expr->location);
            append_ir_node(irn1, irl);
            break;
        case IDENTIFIER_EXPR:
            n->expr->lvalue = TRUE;
            n->expr->location = ++reg_idx;
            /* get name from symbol */
            irn1 = irn_load_from_global(LOAD_WORD_INDIRECT,
                        n->expr->location, n->st_entry);
            append_ir_node(irn1, irl);
            break;
        case NUMBER_CONSTANT:
            n->expr->lvalue = FALSE;
            n->expr->location = ++reg_idx;
            irn1 = irn_load_number_constant(LOAD_CONSTANT,
                        n->expr->location, n->data.num);
            append_ir_node(irn1, irl);
            break;
        case EXPRESSION_STATEMENT:
        case GOTO_STATEMENT:
        case UNARY_EXPR:
        case PREFIX_EXPR:
            break;
        case RETURN_STATEMENT:
            compute_ir(n->children.child1, irl);
            if (n->children.child1 != NULL) {
                irn_statement(RETURN, n->children.child1->expr->location);
            } else {
                irn_statement(RETURN, NR);
            }
            break;
        default:
            break;
    }
}

void print_ir_list(FILE *out, IrList *irl) {
    irl->cur = irl->head;
    while (irl->cur != NULL) {
        print_ir_node(stdout, irl->cur);
        irl->cur = irl->cur->next;
    }
}

void print_ir_node(FILE *out, IrNode *irn) {
    fprintf(out, "(");
    switch(irn->instruction) {
        case BEGIN_PROC:
            fprintf(out, "beginproc, \"%s\"", get_symbol_name(irn->s));
            break;
        case RETURN:
            fprintf(out, "return, $r%d", irn->n1);
            break;
        case STORE_WORD_INDIRECT:
            fprintf(out, "storewordindirect, $r%d, $r%d", irn->n1, irn->n2);
            break;
        case LOAD_WORD_INDIRECT:
            fprintf(out, "loadwordindirect, $r%d, %s", irn->n1, get_symbol_name(irn->s));
            break;
        case LOAD_CONSTANT:
            fprintf(out, "loadconstant, $r%d, %d", irn->n1, irn->n2);
            break;
        case LOG_OR:
            fprintf(out, "logicalor, $r%d, $r%d, $r%d", irn->n1, irn->n2, irn->n3);
            break;
        default:
            break;
    }
    fprintf(out, ")\n");
}
