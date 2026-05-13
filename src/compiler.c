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
    size_t offset;
} MemoryCall;

/// @brief file to which we will write assembler code
static FILE* asmb;
/// @brief name of the current function
static char* cur_func_name;

static VarTab global_vars;
static Node* functs;

static char* params_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

static void compile_expr(Node* expr, char* result_register);
static void compile_instr(Node* instr);

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
        }else if(r64name[1] >= '0' && r64name[1] <= '9'){
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
        result[i] = '\0';
        break;;
    default:
        fprintf(stderr, "Error converting register name: function works only with 1 and 4 byte registers, got : %ld\n", size);
        exit(-1);
    }
    return result;
}

/// @brief returns a Memory Call for given idExpr (simple variable access or field)
/// @param idExpr
/// @param split_offset if non-zero, in mem_call contains base adress without offset. Helpfull if passing address as variable
/// @return char* memory_call - an adress of given field/variable (to be surrounded with []), size - size of variable in bytes
/// @attention char* memory_call is to be freed manually
static MemoryCall getMemoryCall(Node* idExpr, int split_offset){
    char* var_id = idExpr -> label.value.id;
    
    //prefer local variable over global
    VarNode* var_node = get_var_node(var_id, getVarTable(cur_func_name));
    var_node = var_node ? var_node : get_var_node(var_id, global_vars);
    if (var_node == NULL){
        fprintf(stderr, "Variables %s cannot be found\n", var_id);
        exit(-1);
    }
    int mem_call_size = (strlen(var_id) > 3 ? strlen(var_id) : 3) + 5; //max: var_id+999\0 / rbp-999\0
    char* mem_call = malloc(mem_call_size); 
    // just variable access, no fields
    if (idExpr->firstChild == NULL){
        MemoryCall res = {mem_call, var_node->size, 0};
        if (var_node -> addr_type == STATIC){
            sprintf(mem_call, "%s", var_id);
            //offset is 0
        }else{
            if (!split_offset){
                if (var_node->addr >= 0)
                    sprintf(mem_call, "rbp - %d", var_node->addr + var_node -> size);
                else
                    sprintf(mem_call, "rbp + %d", -var_node->addr);
            }else{
                sprintf(mem_call, "rbp");
                res.offset = var_node->addr >= 0 ?
                    -var_node->addr - var_node -> size :
                    -var_node->addr;
            }
        }
        
        return res;
    }

    //field access
    int is_param_on_stack = var_node -> addr >= 0 ? 0 : 1;
    printTree(idExpr);
    printf("COMPILE LOGS: %s stack\n", is_param_on_stack ? "On" : "Not on");
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

    MemoryCall res = {mem_call, field_node->size, 0};
    if (var_node -> addr_type == STATIC){
        if (split_offset)
            sprintf(mem_call, "%s", var_id);
        else
            sprintf(mem_call, "%s-%d", var_id, offset - field_node->size);
        res.offset = offset - field_node->size;
    }else{
        if (split_offset){
            sprintf(mem_call, "rbp");
            res.offset = is_param_on_stack ?
                (offset + -var_node->addr)
                : -offset;
        }else{
            //negative offset for arguments pushed on caller's stack
            sprintf(mem_call, "rbp %c %d", is_param_on_stack ? '+' : '-',
                is_param_on_stack ? (offset + -var_node->addr) : 
                    (var_node->addr + offset + field_node->size));
            
        }
    }
    
    return res;
}

static void put_struct_on_stack(VarTab* fields, size_t baseoffset, Node* baseIdExpr){
    for(int i = 0; i < fields->size; i++){
        VarNode cur = fields->vars[i];

        Node* last_child = baseIdExpr;
        while (last_child->firstChild != NULL) last_child = last_child->firstChild;
        Node nextChild;
        nextChild.label.type = ID; nextChild.label.value.id = cur.id;
        nextChild.firstChild = nextChild.nextSibling = NULL;
        addChild(last_child, &nextChild);

        if (cur.fields != NULL){

            put_struct_on_stack(cur.fields, baseoffset + cur.addr, baseIdExpr);

            last_child->firstChild = NULL;
        }else{
            switch (cur.size)
            {
            case 1:
                compile_expr(baseIdExpr, NULL);
                fprintf(asmb, "mov [rsp + %ld], al\n", baseoffset + cur.addr);
                
                last_child->firstChild = NULL;
                break;
            case 4:
                compile_expr(baseIdExpr, NULL);
                fprintf(asmb, "mov [rsp + %ld], eax\n", baseoffset + cur.addr);

                last_child->firstChild = NULL;
                break;
            default:
                fprintf(stderr, "Compile error: "
                    "%s field have unknown base type with size %ld",
                    cur.id, cur.size);
                exit(-1);
            }
        }
    }
}

/// @brief copies values of struct from a struct adress of which is in rax
/// @param fields fields of current struct
/// @param baseoffset 0, argument for recursive calls
/// @param baseIdExpr node of destination struct
static void copy_struct_values(VarTab* fields, size_t baseoffset, Node* baseIdExpr){
    for(int i = 0; i < fields->size; i++){
        VarNode cur = fields->vars[i];

        Node* last_child = baseIdExpr;
        while (last_child->firstChild != NULL) last_child = last_child->firstChild;
        Node nextChild;
        nextChild.label.type = ID; nextChild.label.value.id = cur.id;
        nextChild.firstChild = nextChild.nextSibling = NULL;
        addChild(last_child, &nextChild);

        if (cur.fields != NULL){

            copy_struct_values(cur.fields, baseoffset + cur.addr, baseIdExpr);

            last_child->firstChild = NULL;
        }else{
            //memory call to current field
            MemoryCall mem_c = getMemoryCall(baseIdExpr, 0);
            
            switch (cur.size)
            {
            case 1:
                fprintf(asmb, "mov bl, [rax - %ld]\n", baseoffset + cur.addr + 1);
                fprintf(asmb, "mov [%s], bl\n", mem_c.memory_call);
                break;
            case 4:
                fprintf(asmb, "mov ebx, [rax - %ld]\n", baseoffset + cur.addr + 4);
                fprintf(asmb, "mov [%s], ebx\n", mem_c.memory_call);
                break;
            default:
                fprintf(stderr, "Compile error: "
                    "%s field have unknown base type with size %ld",
                    cur.id, cur.size);
                exit(-1);
            }

            free(mem_c.memory_call);
            last_child->firstChild = NULL;
        }
    }
}

/// @brief compiles a function call to a given function, pushing and popping volatile registers
/// @param callNode node of function call (one with Arguments as first child)
/// @param result_register 64bit register to put return value, NULL for rax
static void compile_function_call(Node* callNode, char* result_register){
    char* callee_name = callNode->label.value.id;
    Node* cur_arg = callNode->firstChild->firstChild;
        
    size_t stack_total_size = 0;
    int arg_count = 0;

    // find function signature
    Node* cur = functs->firstChild;
    Node* found_func = NULL;
    while (cur) {
        // Each DeclFonct: firstChild is EnTeteFonct
        Node* header = cur->firstChild;
        if (header && header->firstChild && header->firstChild->nextSibling) {
            Node* nameNode = header->firstChild->nextSibling;
            if (nameNode->label.type == ID && strcmp(nameNode->label.value.id, callee_name) == 0) {
                found_func = cur;
                break;
            }
        }
        cur = cur->nextSibling;
    }

    if (found_func == NULL){
        //printf("COMPILE LOG: function %s not found between declared (considering built-in)\n", callee_name);
        //one of built-ins, put argument in rdi if exists
        if (strcmp(callee_name, "getint") == 0 || strcmp(callee_name, "getchar") == 0){
            //no arguments
        }else{
            compile_expr(cur_arg, "rdi");
        }
        fprintf(asmb, "call %s\n", callee_name);
        return;
    }
    // First, count arguments and their size
        
    Node* tmp_arg = found_func->firstChild->firstChild->nextSibling->nextSibling->firstChild;
        
    while (tmp_arg) {
        if (tmp_arg->label.type == KEYWORD){
            //printf("COMIPLER LOG: funciton call with no arguments\n");
            break;
        } //VOID
        char* type_arg = tmp_arg->label.value.id;
        if (strcmp(type_arg, "int")== 0){
            stack_total_size += 4;
        }else if (strcmp(type_arg, "char") == 0){
            stack_total_size += 1;
        }else{
            StructListNode* struct_type = getStructType("global", type_arg);
            stack_total_size += struct_type->size;
        }
        arg_count++;
        tmp_arg = tmp_arg->nextSibling;
    }
    //here we will just simply move all the arguments on the stuck in the order of their appearence
    //implementing this approcach is like moving all the calculations of arguments inside the called function
    //so if we got to compile foo(bar(), 1)
    //it will be something similar to
    //void foo(){ arg1 = bar(); arg2 = 1; ...}
    //so no problems with nested calls
    //todo: aline stack on 16 bytes
    
    cur_arg = arg_count > 0 ? callNode->firstChild->firstChild : NULL;
    
    tmp_arg = found_func->firstChild->firstChild->nextSibling->nextSibling->firstChild;
    
    //todo: hidden pointer if function returns struct
    char* returning_type = found_func->firstChild->firstChild->label.value.id;
    size_t return_size_aligned = 0;
    if (strcmp(returning_type, "int") == 0 | strcmp(returning_type, "char") == 0){
        //pass
    }else{
        //allocate memory for return struct
        StructListNode *ret_snode = getStructType("global", returning_type);
        return_size_aligned = (ret_snode->size + 15) / 16 * 16;
        fprintf(asmb, "sub rsp, %ld\n", return_size_aligned);
        //adress rsp - retrun_size_aligned will be passed to funciton via rdi
    }

    size_t offset = 0;
    while (cur_arg) {
        char* type_arg = tmp_arg->label.value.id;
        if (strcmp(type_arg, "int")== 0){
              
            compile_expr(cur_arg, NULL);
            fprintf(asmb, "mov [rsp + %ld], eax\n", offset);
            offset += 4;
    
        }else if (strcmp(type_arg, "char") == 0){
            
            compile_expr(cur_arg, NULL);
            fprintf(asmb, "mov [rsp + %ld], al\n", offset);
            offset += 1;
        }else{
            StructListNode* struct_type = getStructType("global", type_arg);
            //copy struct on stack recursively
            put_struct_on_stack(struct_type->fields,
                offset, cur_arg);
            offset += struct_type->size;
        }
         
        cur_arg = cur_arg->nextSibling;
        tmp_arg = tmp_arg->nextSibling;
    }
    if (return_size_aligned > 0){
        //returning struct -> save address in rdi
        fprintf(asmb, "mov rdi, rsp\n"
            "add rdi, %ld", return_size_aligned);
    }
    fprintf(asmb, "sub rsp, %d\n", offset);
    fprintf(asmb, "call %s\n", callee_name);

    // Clean up stack arguments after call
    if (arg_count > 0) {
        fprintf(asmb, "add rsp, %d\n", stack_total_size);
    }
    
    if (result_register){
        if (strcmp(returning_type, "int") == 0){
            char* register_name = transform_register(result_register, 4);
            fprintf(asmb, "mov %s, eax\n", register_name);
            free(register_name);
        }else if (strcmp(returning_type, "char") == 0){
            char* register_name = transform_register(result_register, 1);
            fprintf(asmb, "mov %s, al\n", register_name);
            free(register_name);
        }else{
            //address of struct
            fprintf(asmb, "mov %s, rax\n", result_register);
        }
    }

    // If function returns oversized struct, copy from returned pointer if needed
    // (User should implement this logic as needed)
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
                compile_function_call(expr, result_register);
                return;
            }
            // structs are treated in getMemoryCall
        }
        // variable / struct field access
        MemoryCall mem_c = getMemoryCall(expr, 0);

        if (mem_c.size != 1 && mem_c.size != 4){
            //split mem_c on register/static_name and offset
            free(mem_c.memory_call);
            mem_c = getMemoryCall(expr, 1);
            //stuct type -> copy address
            fprintf(asmb, "mov rax, %s\n",
                mem_c.memory_call);
            fprintf(asmb, "add rax, %ld\n",
                mem_c.offset + mem_c.size);

            free(mem_c.memory_call);
            return;
        }

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
                    compile_expr(kw->firstChild, NULL);

                    //todo: if struct
                    //fprintf(asmb, "pop rdi\n");
                    //copy to rdi
                    //fprintf(asmb, "mov rax, rdi\n");
                    //ret and all stack reestablishing is in compile_func
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
        compile_function_call(instr->firstChild, NULL);
        return;
    }
    // Variable/field assignment: two children
    if (instr->firstChild && instr->firstChild->nextSibling) {

        //check if it is a structure copying
        // Descend all children of the first assignment operand to get the deepest VarNode
        Node* lhs_node = instr->firstChild;
        VarNode* dest_var_node = NULL;
        VarTab cur_tab = getVarTable(cur_func_name);
        VarTab global_tab = getVarTable("global");
        dest_var_node = get_var_node(lhs_node->label.value.id, cur_tab);
        dest_var_node = dest_var_node ? dest_var_node : get_var_node(lhs_node->label.value.id, global_tab);
        Node* descend = lhs_node->firstChild;
        while (descend && dest_var_node && dest_var_node->fields) {
            dest_var_node = get_var_node(descend->label.value.id, *dest_var_node->fields);
            descend = descend->firstChild;
        }

        if (dest_var_node && dest_var_node->fields != NULL){
            //struct copying

            //copy address of source struct to rax
            compile_expr(instr->firstChild->nextSibling, NULL);
            //copy field by field recursively
            copy_struct_values(dest_var_node->fields, 0, lhs_node);
            return;
        }

        compile_expr(instr->firstChild->nextSibling, NULL);

        //now right hand is in rax
        MemoryCall mem_c = getMemoryCall(instr->firstChild, 0);

        char* register_name;
        char* reg = "rax";
        register_name = transform_register(reg, mem_c.size);

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
    //arranging stack pointer, giving space for local variables
    if (strcmp(cur_func_name, "main") != 0){
        fprintf(asmb, "%s:\n"
            "push rbp\n", cur_func_name);
    }
    fprintf(asmb, "mov rbp, rsp\n");

    Node* local_vars = func->firstChild->nextSibling->firstChild;
    VarTab local_vartab = getVarTable(cur_func_name);
    size_t local_vars_stack_offset = 0;
    
    //check if returns a struct -> additional param on rdi
    char* return_type = func->firstChild->firstChild->label.value.id;


    Node* cur_param = func->firstChild->firstChild->nextSibling->nextSibling->firstChild;
    //as parameters are already on stack, we don't need to allocate space for them
    int real_local_vars_offset = 0;
    if (cur_param -> label.type != KEYWORD){
        //have parameters
        
        for (;cur_param != NULL; cur_param = cur_param->nextSibling) real_local_vars_offset++;
    }

    //allocating stack for local variables
    for (int i = real_local_vars_offset; i < local_vartab.size; i ++)
        local_vars_stack_offset += local_vartab.vars[i].size;
    if (local_vars_stack_offset > 0){
        local_vars_stack_offset = local_vars_stack_offset / 8 + 1;
        for (int i = 0; i < local_vars_stack_offset; i++)
            fprintf(asmb, "push 0\n");
    }
    if (strcmp(return_type, "int") != 0 && strcmp(return_type, "char") != 0){
        fprintf(asmb, "push rdi\n");
    }

    printf("COMPILE LOGS: %s - compiling instructions\n", cur_func_name);

    Node* curInstr = local_vars -> nextSibling -> firstChild;
    while (curInstr != NULL)
    {
        compile_instr(curInstr);
        curInstr = curInstr ->nextSibling;
    }

    if (local_vars_stack_offset > 0){
        for (int i = 0; i < local_vars_stack_offset; i++)
            fprintf(asmb, "pop r8\n");
    }
    if (strcmp(cur_func_name, "main") != 0){
        fprintf(asmb,
            "pop rbp\n"
            "ret\n");
    }
}

extern void compile(Node* prog){
    if (prog == NULL){
        fprintf(stderr, "Compile error: Program tree is null");
        exit(-1);
    }
    
    //printTree(prog);

    functs = prog->firstChild->nextSibling;

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
    
    printf("COMPILE LOGS: static allocation done, starting compiling functions\n");

    Node* cur_func = functs->firstChild; //current "DeclFonct" node
    while (cur_func != NULL)
    {
        cur_func_name = cur_func ->
            firstChild ->
            firstChild ->
            nextSibling ->
            label.value.id;
        if (strcmp(cur_func_name, "main") == 0){
            printf("COMPILE LOGS: start compiling function %s\n", cur_func_name);

            fprintf(asmb, "global _start\nsection .text\n_start:\n");
            compile_func(cur_func);
        }

        cur_func = cur_func -> nextSibling;
    }

    cur_func = functs->firstChild;
    while (cur_func != NULL)
    {
        cur_func_name = cur_func ->
            firstChild ->
            firstChild ->
            nextSibling ->
            label.value.id;
        if (strcmp(cur_func_name, "main") != 0){
            printf("COMPILE LOGS: start compiling function %s\n", cur_func_name);

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