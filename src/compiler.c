#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tree.h"
#include "vartable.h"

#define LABEL_NAME_SIZE 6

//todo: move part of checking result register on operands to the end of comiple_expr
//  when finished with all types of expressions

typedef struct {
    char* memory_call;
    size_t size;
} MemoryCall;

/// @brief file to which we will write assembler code
static FILE* asmb;
/// @brief name of the current function
static char* cur_func_name;

static VarTab global_vars;

/// @brief writes a random name consisting of latters of LABEL_NAME_SIZE length (+1 for '\0')
/// @param destination pointer to the first latter of name
void get_random_label_name(char* destination){
    int i;
    for(i = 0; i < LABEL_NAME_SIZE; i++){
        destination[i] = 'a' + rand() % 26;
    }
    destination[i] = '\0';
    return;
}


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

/// @brief gives name a part-register of given size by the name of 64-bit register (ex. rax 4 -> eax) 
/// @param r64name name of 64bit register
/// @param size size of operand to fit (4 or 1)
static char* transform_register(char* r64name, size_t size){
    int i; char* result;
    switch (size)
    {
    case 4:
        result = (char*)malloc(4);
        if (result == NULL){
            fprintf(stderr, "Allocation error\n");
            exit(-1);
        }
        result[0] = 'e';
        
        for (i = 1; i < strlen(r64name); i++)
            result[i] = r64name[i];
        result[i] = '\0';
        printf("%s\n", result);
        break;
    case 1:
        if (!strcmp(r64name, "rsi") || !strcmp(r64name, "rdi")){
            //here byte registers are named "sil" and "dil" for whatever reason
            result = malloc(4);
            if (result == NULL){
                fprintf(stderr, "Allocation error\n");
                exit(-1);
            }
            result[0] = r64name[1];
            result[1] = 'i'; result[2] = 'l'; result[3] = '\0';
        }else if(result[1] >= '0' && result[2] <= '9'){
            //r8-r15
            result = malloc(strlen(r64name) + 2);
            if (result == NULL){
                fprintf(stderr, "Allocation error\n");
                exit(-1);
            }
            for (i = 0; i < strlen(r64name); i++)
                result[i] = r64name[i]; 
            result[i++] = 'b'; result[i] = '\0';
        }else{
            //rax, rbx, rcx...
            result = malloc(3);
            if (result == NULL){
                fprintf(stderr, "Allocation error\n");
                exit(-1);
            }
            result[0] = r64name[1];
            result[1] = 'l'; result[2] = '\0';
        }  
        break;
    case 8:
        result = malloc(strlen(r64name) + 1);
        if (result == NULL){
            fprintf(stderr, "Allocation error\n");
            exit(-1);
        }
        for(i = 0; i < strlen(r64name); i++)
            result[i] = r64name[i];
        result[++i] = '\0';
        break;;
    default:
        fprintf(stderr, "Error converting register name: function works only with 1 and 4 byte registers, got : %ld\n", size);
        exit(-1);
    }
    return result;
}

/// @brief returns a Memory Call for given idExpr (simple variable access or field)
/// @param idExpr 
/// @return char* memory_call - an adress of given field/variable (to be surrounded with []), size - size of variable in bytes
/// @attention char* memory_call is to be freed manually
static MemoryCall getMemoryCall(Node* idExpr/*, VarTab localvars*/){
    char* var_id = idExpr -> label.value.id;
    
    VarNode* var_node = get_var_node(var_id, global_vars);
    if (var_node == NULL){
        fprintf(stderr, "Variables %s cannot be found\n", var_id);
        exit(-1);
    }
    int mem_call_size = (strlen(var_id) > 3 ? strlen(var_id) : 3) + 5; //max: var_id+999\0 / rbp-999\0
    char* mem_call = malloc(mem_call_size); 
    // just variable access, no fields
    if (idExpr->firstChild == NULL){
        if (var_node -> addr_type == STATIC){
            sprintf(mem_call, "%s", var_id);
        }else{
            sprintf(mem_call, "rbp%d", -1*var_node->addr);
        }
        MemoryCall res = {mem_call, var_node->size};
        return res;
    }
    //field access
    int offset_mult = var_node -> addr_type == STATIC ? 1 : -1;
    int offset  = 0;
    Node* field = idExpr->firstChild;
    VarTab* parentFieldsTab = var_node->fields;
    VarNode* field_node;
    while (field)
    {
        field_node = get_var_node(field->label.value.id, *parentFieldsTab);
        if (field_node == NULL){
            fprintf(stderr, "Field %s cannot be found on variable%s\n",
                field->label.value.id, var_id);
            exit(-1);
        }
        offset += field_node->addr;

        field = field->firstChild;
        parentFieldsTab = field_node->fields;
    }

    if (var_node -> addr_type == STATIC){
        sprintf(mem_call, "%s+%d", var_id, offset);
    }else{
        sprintf(mem_call, "rbp-%d", offset);
    }
    MemoryCall res = {mem_call, field_node->size};
    return res;
}

//rax, rbx and rdx can be suppressed by any instruction
static void compile_expr(Node* expr, char* result_register){
    if (expr->label.type == INT){
        fprintf(asmb, "mov %s, %d\n",
                result_register ? result_register : "rax", expr->label.value.number);
        return;
    }
    
    if (expr->label.type == CHAR){
        fprintf(asmb, "mov %s, \'%c\'\n",
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
                fprintf(asmb, "call %s\n", expr->label.value.id);
                // make call

                //pop all registers, used for arguments
                return;
            }
            // structs are treated in getMemoryCall
        }
        // variable / struct field access
        MemoryCall mem_c = getMemoryCall(expr);
        //later consider checking size for r10-r15 registers
        char* register_name;
        char* reg = result_register ? result_register : "rax";
        register_name = transform_register(reg, mem_c.size);
        fprintf(asmb, "mov %s, %s [%s]\n",
            register_name,
            mem_c.size == 4 ? "dword" : "byte", //for now only char and int
            mem_c.memory_call);
        free(mem_c.memory_call);
        free(register_name);
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
            //result of first operand -> rax
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
                fprintf(asmb, "idiv rbx\n");
                break;
            case '%':
                fprintf(asmb, "idiv rbx\n");
                fprintf(asmb, "mov rax, rdx\n");
                break;
            case '!': {  //!=
                char neLabel[LABEL_NAME_SIZE + 1], endCompLabel[LABEL_NAME_SIZE + 1];
                get_random_label_name(neLabel); get_random_label_name(endCompLabel);
                fprintf(asmb,
                    "cmp rax, rbx\n"
                    "jne %s\n"
                    "mov rax, 0\n"
                    "jmp %s\n"
                    "%s:\n"
                    "mov rax, 1\n"
                    "%s:\n", neLabel, endCompLabel, neLabel, endCompLabel);
                break;
            }
            case '=': {  //==
                char eqLabel[LABEL_NAME_SIZE + 1], endCompLabel[LABEL_NAME_SIZE + 1];
                get_random_label_name(eqLabel); get_random_label_name(endCompLabel);
                fprintf(asmb,
                    "cmp rax, rbx\n"
                    "je %s\n"
                    "mov rax, 0\n"
                    "jmp %s\n"
                    "%s:\n"
                    "mov rax, 1\n"
                    "%s:\n", eqLabel, endCompLabel, eqLabel, endCompLabel);
                break;
            }
            case '>': {
                // Check for >= (second char is '=')
                if (expr->label.value.id[1] == '=') {
                    char geLabel[LABEL_NAME_SIZE + 1], endCompLabel[LABEL_NAME_SIZE + 1];
                    get_random_label_name(geLabel); get_random_label_name(endCompLabel);
                    fprintf(asmb,
                        "cmp rax, rbx\n"
                        "jge %s\n"
                        "mov rax, 0\n"
                        "jmp %s\n"
                        "%s:\n"
                        "mov rax, 1\n"
                        "%s:\n", geLabel, endCompLabel, geLabel, endCompLabel);
                } else {
                    char gtLabel[LABEL_NAME_SIZE + 1], endCompLabel[LABEL_NAME_SIZE + 1];
                    get_random_label_name(gtLabel); get_random_label_name(endCompLabel);
                    fprintf(asmb,
                        "cmp rax, rbx\n"
                        "jg %s\n"
                        "mov rax, 0\n"
                        "jmp %s\n"
                        "%s:\n"
                        "mov rax, 1\n"
                        "%s:\n", gtLabel, endCompLabel, gtLabel, endCompLabel);
                }
                break;
            }
            case '<': {
                // Check for <= (second char is '=')
                if (expr->label.value.id[1] == '=') {
                    char leLabel[LABEL_NAME_SIZE + 1], endCompLabel[LABEL_NAME_SIZE + 1];
                    get_random_label_name(leLabel); get_random_label_name(endCompLabel);
                    fprintf(asmb,
                        "cmp rax, rbx\n"
                        "jle %s\n"
                        "mov rax, 0\n"
                        "jmp %s\n"
                        "%s:\n"
                        "mov rax, 1\n"
                        "%s:\n", leLabel, endCompLabel, leLabel, endCompLabel);
                } else {
                    char ltLabel[LABEL_NAME_SIZE + 1], endCompLabel[LABEL_NAME_SIZE + 1];
                    get_random_label_name(ltLabel); get_random_label_name(endCompLabel);
                    fprintf(asmb,
                        "cmp rax, rbx\n"
                        "jl %s\n"
                        "mov rax, 0\n"
                        "jmp %s\n"
                        "%s:\n"
                        "mov rax, 1\n"
                        "%s:\n", ltLabel, endCompLabel, ltLabel, endCompLabel);
                }
                break;
            }
            }
            if (result_register){
                fprintf(asmb, "mov %s, rax\n", result_register);
            }
            return;
        }
        
    }
    // AND, OR, NOT operators
    if (expr->label.type == KEYWORD) {
        Node* op1 = expr->firstChild;
        
        if (expr->label.value.label == Not){
            compile_expr(op1, NULL);
            fprintf(asmb,"not rax\n");
        }else{
            Node* op2 = expr->firstChild->nextSibling;
            //result of first operand -> rax
            compile_expr(op2, NULL);
            fprintf(asmb, "push rax\n");
            //result of second -> rbx
            compile_expr(op1, NULL);
            fprintf(asmb, "pop rbx\n");
            switch (expr->label.value.label)
            {
                case And:
                    fprintf(asmb, "and rax, rbx\n");
                    break;
                case Or:
                    fprintf(asmb, "or rax, rbx\n");
                    break;
                default:
                    fprintf(stderr, "Unknown boolean operator: %s", getTreeLabelName(expr->label.value.label));
                    exit(-1);
            }
            
            
        }
        //restrict results to one and zero
        char labelName[LABEL_NAME_SIZE + 1];
        get_random_label_name(labelName);
        fprintf(asmb, 
                "cmp rax, 0\n"
                "je %s\n"
                "mov rax, 1\n"
                "%s:\n", labelName, labelName);
        if (result_register){
            fprintf(asmb, "mov %s, rax\n", result_register);
        }
        return;
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
                    // mov rax, %return_value
                    // ret
                }
                break;
            }
            case If: {
                Node* cond = kw->firstChild;
                Node* thenInstr = cond->nextSibling;
                Node* elseNode = kw->nextSibling;
                //compile condition evaluation
                compile_expr(cond, NULL);
                if (elseNode && elseNode->label.type == KEYWORD && elseNode->label.value.label == Else) {
                    //if - else
                    char endIfLabel[LABEL_NAME_SIZE + 1], elseLabel[LABEL_NAME_SIZE + 1];
                    get_random_label_name(endIfLabel); get_random_label_name(elseLabel);
                    fprintf(asmb, "cmp rax, 0\n"
                        "je %s\n", elseLabel);
                    compile_instr(thenInstr);
                    fprintf(asmb, "jmp %s\n"
                        "%s:\n", endIfLabel, elseLabel);
                    compile_instr(elseNode->firstChild);
                    fprintf(asmb, "%s:\n", endIfLabel);
                }else{
                    //no else block
                    char endIfLabel[LABEL_NAME_SIZE + 1];
                    get_random_label_name(endIfLabel);
                    fprintf(asmb, "cmp rax, 0\n"
                        "je %s\n", endIfLabel);
                    compile_instr(thenInstr);
                    fprintf(asmb, "%s:\n", endIfLabel);
                }
                break;
            }
            case While: {
                Node* cond = kw->firstChild;
                Node* bodyInstr = cond ? cond->nextSibling : NULL;
                char conditionLabel[LABEL_NAME_SIZE + 1],
                     endWhileLabel[LABEL_NAME_SIZE + 1];
                get_random_label_name(conditionLabel); get_random_label_name(endWhileLabel);
                fprintf(asmb, "%s:\n", conditionLabel);
                compile_expr(cond, NULL);
                fprintf(asmb, "cmp rax, 0\n"
                    "je %s\n", endWhileLabel);
                compile_instr(bodyInstr);
                fprintf(asmb, "jmp %s\n"
                    "%s:\n", conditionLabel, endWhileLabel);
                break;
            }
            case SuiteInstr:
                Node* curInstr = kw -> firstChild;
                while (curInstr != NULL)
                {
                    compile_instr(curInstr);
                    curInstr = curInstr ->nextSibling;
                }
                break;
            default:
                fprintf(stderr, "Compile error: unknown keyword %s\n",
                    getTreeLabelName(kw->label.value.label));
                exit(-1);
        }
        return;
    }
    // Function call: single child
    if (instr->firstChild && instr->firstChild->nextSibling == NULL) {
        Node* callNode = instr->firstChild;
        Node* cur_arg = callNode->firstChild->firstChild;
        while (cur_arg != NULL)
        {
            compile_expr(cur_arg, "rdi");
            cur_arg = cur_arg -> nextSibling;
        }
        
        fprintf(asmb, "call %s\n", callNode->label.value.id);
        
        //call funcName
        return;
    }
    // Variable/field assignment: two children
    if (instr->firstChild && instr->firstChild->nextSibling) {
        compile_expr(instr->firstChild->nextSibling, NULL);

        //now right hand is in rax
        MemoryCall mem_c = getMemoryCall(instr->firstChild);

        char* register_name;
        char* reg = "rax";
        register_name = transform_register(reg, mem_c.size);
        printTree(instr->firstChild);
        printf("Mem_size: %ld, register: %s\n", mem_c.size, register_name);

        fprintf(asmb, "mov %s [%s], %s\n",
            mem_c.size == 4 ? "dword" : "byte",
            mem_c.memory_call,
            register_name);
        free(mem_c.memory_call);
        free(register_name);
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
        exit(-1);
    }
    
    printTree(prog);

    asmb = fopen("_anonymous.asm", "w");

    // static allocation
    global_vars = getVarTable("global");
    VarNode* vars = global_vars.vars;
    fprintf(asmb, "section .bss\n");
    for (int i = 0; i < global_vars.size; i++){
        if (vars[i].addr_type == STATIC){
            fprintf(asmb, "%s: resb %lld\n", vars[i].id, vars[i].size);
        }
    }
    // Built-in function buffers
    fprintf(asmb, "getint_buf: resb 32\nputint_buf: resb 24\n");
    fprintf(asmb,
        "global getchar\n"
        "global getint\n"
        "global putchar\n"
        "global putint\n");
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

    // Write built-in functions to output assembly
    fprintf(asmb,
        "; --- getchar ---\n"
        "getchar:\n"
        "    mov rax, 0\n"
        "    mov rdi, 0\n"
        "    lea rsi, [rsp-8]\n"
        "    mov rdx, 1\n"
        "    syscall\n"
        "    cmp rax, 1\n"
        "    jne getchar_eof\n"
        "    mov al, byte [rsp-8]\n"
        "    movzx rax, al\n"
        "    ret\n"
        "getchar_eof:\n"
        "    mov rax, -1\n"
        "    ret\n"

        "; --- getint ---\n"
        "getint:\n"
        "    mov rax, 0\n"
        "    mov rdi, 0\n"
        "    mov rsi, getint_buf\n"
        "    mov rdx, 32\n"
        "    syscall\n"
        "    cmp rax, 0\n"
        "    jle getint_io_error\n"
        "    mov rcx, getint_buf\n"
        "    mov rbx, 0\n"
        "    mov rdx, 0\n"
        "    mov al, [rcx]\n"
        "    cmp al, '-'\n"
        "    jne getint_check_plus\n"
        "    inc rcx\n"
        "    mov rdx, 1\n"
        "    jmp getint_parse\n"
        "getint_check_plus:\n"
        "    cmp al, '+'\n"
        "    jne getint_parse\n"
        "    inc rcx\n"
        "getint_parse:\n"
        "    mov al, [rcx]\n"
        "    cmp al, 10\n"
        "    je getint_done\n"
        "    cmp al, '0'\n"
        "    jb getint_io_error\n"
        "    cmp al, '9'\n"
        "    ja getint_io_error\n"
        "    imul rbx, rbx, 10\n"
        "    sub al, '0'\n"
        "    add rbx, rax\n"
        "    inc rcx\n"
        "    jmp getint_parse\n"
        "getint_done:\n"
        "    cmp rdx, 0\n"
        "    je getint_ret\n"
        "    neg rbx\n"
        "getint_ret:\n"
        "    mov rax, rbx\n"
        "    ret\n"
        "getint_io_error:\n"
        "    mov rax, 60\n"
        "    mov rdi, 5\n"
        "    syscall\n"

        "; --- putchar ---\n"
        "putchar:\n"
        "    mov rax, 1\n"
        "    push 0\n"
        "    lea rsi, [rsp-8]\n"
        "    mov [rsp-8], dil\n"
        "    mov rdx, 1\n"
        "    mov rdi, 1\n"
        "    syscall\n"
        "    pop rax\n"
        "    ret\n"

        "; --- putint ---\n"
        "putint:\n"
        "    mov rcx, putint_buf\n"
        "    mov rax, rdi\n"
        "    mov rbx, 10\n"
        "    mov rdx, 0\n"
        "    mov r8, 0\n"
        "    cmp rax, 0\n"
        "    jge putint_loop\n"
        "    mov byte [rcx], '-'\n"
        "    inc rcx\n"
        "    neg rax\n"
        "putint_loop:\n"
        "    xor rdx, rdx\n"
        "    div rbx\n"
        "    add dl, '0'\n"
        "    push rdx\n"
        "    inc r8\n"
        "    test rax, rax\n"
        "    jnz putint_loop\n"
        "putint_print_digits:\n"
        "    pop rdx\n"
        "    mov [rcx], dl\n"
        "    inc rcx\n"
        "    dec r8\n"
        "    jnz putint_print_digits\n"
        "    mov byte [rcx], 10\n"
        "    inc rcx\n"
        "    sub rcx, putint_buf\n"
        "    mov rax, 1\n"
        "    mov rdi, 1\n"
        "    mov rsi, putint_buf\n"
        "    mov rdx, rcx\n"
        "    syscall\n"
        "    ret\n"
    );

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