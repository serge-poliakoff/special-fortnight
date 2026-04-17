#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tree.h"
#include "vartable.h"

/// @brief file to which we will write assembler code
static FILE* asmb;
/// @brief name of the current function
static char* cur_func_name;

static VarTab global_vars;

/// @brief searches for a variable by its id in a given table
/// @param id id of the variable
/// @param vartable table of variables
/// @return pointer to variable's VarNode or Null
static VarNode* get_var_node(char* id, VarTab vartable){
    VarNode* var = vartable.vars;
    VarNode* variable = NULL;
    for (int i = 0 ; i < vartable.size; i ++){
        if (strcmp(var[i].id, id) == 0){
            variable = &var[i];
            break;
        }
    }

    return variable;
}

static void compile_expr(Node* expr, char* result_register){
    if (expr->label.type == INT){
        fprintf(asmb, "mov %s, %d\n",
                result_register ? result_register : "rax", expr->label.value.number);
        return;
    }
    
    if (expr->label.type == CHAR){
        fprintf(asmb, "mov %s, %c\n",
            result_register ? result_register : "rax", expr->label.value.character);
        return;
    }

    // Idexprr: function call / variable or field access
    if (expr->label.type == ID) {
        Node* child = expr->firstChild;
        // If function call: has Arguments child
        if (child != NULL){
            if (child->label.type == KEYWORD && child->label.value.label == Arguments) {
                // Function call
                
                //put arguments in rdi, rsi, rcx, r9 - 11, pushing each of them before update

                // make call

                //pop all registers, used for arguments
                return;
            }else {
                // Otherwise, field access
                // for structs, somewhat complicated adress search
                //maybe put in all struct field their relative adress (-3 for example)
                //and then just add these strings up until the last point ?
                //a.b.c   -> [{a.addr}{b.addr}{c.addr}] -> [a - 3 - 8]
            }
        }
        // no child -> simple variable access
        char* var_id = expr -> label.value.id;
        VarNode* var_node = get_var_node(var_id, global_vars);
        if (var_node && var_node->addr_type == STATIC){
            //access to a global var
            
            fprintf(asmb, "mov %s, qword [%s]\n",
                result_register ? result_register : "rax",
                var_node->id);
        }
        return;
    }

    // Operators: arithmetic, order, equality (all but boolean operators)
    // code of three :
    //  - if no specified resulting register (rdi/rsi for arguments) -> rax
    //  - principal accumulator is rax, rbx for second temp variable
    if (expr->label.type == OP) {
        Node* op1 = expr->firstChild;
        Node* op2 = expr->firstChild->nextSibling;
        if (op2 == NULL){
            //unary - or +
            
            return;            
        }else{
            //result of first operand -> r.x
            compile_expr(op2, NULL);
            fprintf(asmb, "push rax\n");
            //result of second -> rbx
            compile_expr(op1, NULL);
            fprintf(asmb, "pop rbx\n");
            switch (expr->label.value.id[0])
            {
            case '+':
                fprintf(asmb, "add rax, rbx\n");
                break;
            case '-':
                fprintf(asmb, "sub rax, rbx\n");
                break;
            case '*':
                fprintf(asmb, "imul rax, rbx\n");
                break;
            case '/':
                fprintf(asmb, "idiv rax, rbx\n");
                break;
            }
            if (result_register){
                fprintf(asmb, "mov %s, rax\n", result_register);
            }
            return;
        }
        
    }
    // AND, OR, NOT operators
    if (expr->label.type == KEYWORD) {
        
    }

    // if still don't return
    fprintf(stderr, "Compile error: no instuctions on how to treat expression\n");
    exit(3);
}

static void compile_instr(Node* instr){
    if (!instr) return;
    // Handle keyword instructions first
    if (instr->firstChild && instr->firstChild->label.type == KEYWORD) {
        Node* kw = instr->firstChild;
        switch (kw->label.value.label) {
            case Return: {
                if (strcmp(cur_func_name, "main") == 0){
                    //we know that main must return an integer value
                    //int exit_code = kw->firstChild->label.value.number;
                    compile_expr(kw->firstChild, "rdi"); // exit code value
                    fprintf(asmb, "mov rax, 60\nsyscall\n");
                }else{
                    // mov rdi, %return_value
                    // ret
                }
                break;
            }
            case If: {
                Node* cond = kw->firstChild;
                Node* thenInstr = cond ? cond->nextSibling : NULL;
                //compile if block

                // Check for else block
                Node* elseNode = kw->nextSibling;
                if (elseNode && elseNode->label.type == KEYWORD && elseNode->label.value.label == Else) {
                    //compile else
                }
                break;
            }
            case While: {
                // compile while
                break;
            }
            case SuiteInstr:
                //compile further instructions
                break;
            default:
                // todo: implement a funciton in tree.c to print a keyword
                fprintf(stderr, "Sematic analyser error: unknown keyword %d", kw->label.value.label);
                exit(2);
        }
        return;
    }
    // Function call: single child
    if (instr->firstChild && instr->firstChild->nextSibling == NULL) {
        Node* callNode = instr->firstChild;
        //call funcName
        return;
    }
    // Variable/field assignment: two children
    if (instr->firstChild && instr->firstChild->nextSibling) {
        //assign variable
        //todo: (here we do not check if it's structured)
        char* var_id = instr->firstChild->label.value.id;
        VarNode* var_node = get_var_node(var_id, global_vars);
        if (var_node && var_node->addr_type == STATIC){
            //assigment to global variable
            compile_expr(instr->firstChild->nextSibling, NULL);
            //now right hand is in rax

            fprintf(asmb, "mov qword [%s], rax\n", var_node->id);
        }
        return;
    }
    fprintf(stderr, "Semantic analyser error: cannot analyse instruction");
    exit(2);
}

static void compile_func(Node* func){
    Node* local_vars = func->firstChild->nextSibling->firstChild;
    Node* curInstr = local_vars -> nextSibling -> firstChild;
    while (curInstr != NULL)
    {
        compile_instr(curInstr);
        curInstr = curInstr ->nextSibling;
    }
}

extern void compile(Node* prog){
    if (prog == NULL){
        fprintf(stderr, "Compile error: Program tree is null");
        exit(3);
    }
    
    asmb = fopen("_anonymous.asm", "w");

    // static allocation
    global_vars = getVarTable("global");
    VarNode* vars = global_vars.vars;
    fprintf(asmb, "section .bss\n");
    for (int i = 0; i < global_vars.size; i++){
        if (vars[i].addr_type == STATIC){
            fprintf(asmb, "%s: resb %lld\n", vars[i].id, vars[i].size);
        }
        //printf("%s: resb %lld\n", vars[i].id, vars[i].size);
    }

    Node* functs = prog->firstChild->nextSibling;
    
    Node* cur_func = functs->firstChild; //current "DeclFonct" node
    while (cur_func != NULL)
    {
        cur_func_name = cur_func ->
            firstChild ->
            firstChild ->
            nextSibling ->
            label.value.id;
        if (strcmp(cur_func_name, "main") == 0){
            fprintf(asmb, "global _start\nsection .text\n_start:\n");
            compile_func(cur_func);
        }

        cur_func = cur_func -> nextSibling;
    }

    freeVarTables();
    fclose(asmb);
    
    //compile nasm
    int obj_res = system("nasm -f elf64 _anonymous.asm -o _anonymous.o");
    if (obj_res != 0){
        fprintf(stderr, "Compiler: assembler compilation error");
        exit(3);
    }
    system("ld _anonymous.o -o anon");

    return;
}