#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tree.h>

/// @brief file to which we will write assembler code
static FILE* asmb;
/// @brief name of the current function
static char* cur_func_name;

static Node* global_vars;

// todo: add new structure for saving addresses of vars and structure fields

static void compile_variables(Node* declVars/*, Node** typetable*/){
    if (!declVars) return;
    Node* cur = declVars->firstChild;
    int global = declVars == global_vars;
    int type_idx = 0;
    if (global){
        fprintf(asmb, "section .bss\n");
    }
    for (; cur; cur = cur->nextSibling) {
        if (cur->label.type == TP) {
            // todo: go through all the id's one one type label (same in semantics)
            // Check if type is standart or exists in typetable or glob_types
            char* type_name = cur->label.value.id;
            char* var_name = cur->firstChild->label.value.id;
            if (global == 1){
                if (strcmp(type_name, "char") == 0){
                    fprintf(asmb, "%s: resb 1\n", var_name);
                }else if(strcmp(type_name, "int") == 0){
                    fprintf(asmb, "%s: resb 4\n", var_name);
                }
            }
        } else if (cur->label.type == KEYWORD && cur->label.value.label == Struct) {
            
        }
    }
}

static char* get_varaible_address(char* id/*, Node* localvars*/){
    Node* cur = global_vars->firstChild;

    for (; cur; cur = cur->nextSibling) {
        if (cur->label.type == TP) {
            // Check if type is standart or exists in typetable or glob_types
            char* type_name = cur->label.value.id;
            char* var_name = cur->firstChild->label.value.id;
            if (strcmp(var_name, id) == 0){
                // as it is a global var for now we can just return it's name
                return var_name;
            }
        }
    }
    return NULL;
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
        char* addr = get_varaible_address(var_id);
        if (addr){
            //access to a global var
            
            fprintf(asmb, "mov %s, qword [%s]\n", result_register ? result_register : "rax", addr);
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
    // Variable assignment: two children
    if (instr->firstChild && instr->firstChild->nextSibling) {
        //assign variable
        //todo: (here we do not check if it's structured)
        char* var_id = instr->firstChild->label.value.id;
        char* addr = get_varaible_address(var_id);
        if (addr){
            //assigment to global variable
            compile_expr(instr->firstChild->nextSibling, NULL);
            //now right hand is in eax
            fprintf(asmb, "mov qword [%s], rax\n", addr);
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

    global_vars = prog->firstChild;
    compile_variables(global_vars);
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