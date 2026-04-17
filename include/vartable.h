#ifndef VARTABLE
    #define VARTABLE 1

    #include <stdlib.h>
    

    /// @brief type of variable address
    /// in VarNode if STATIC -> a global variable, id is the address,
    /// if relative -> local variable, address in bytes offset from framestack (rbp)
    typedef enum  {
        STATIC,
        RELATIVE
    } AddrType;

    typedef struct var_node {
        char* id;
        /// @brief type of address. if STATIC, use "id" as address, if RELATIVE - addr field gives you offset from framestack
        AddrType addr_type;
        /// @brief offset from framestack for local variables. For global use "id"
        int addr;
        /// @brief pointer to a list of fields of structure with their relative addresses. NULL for int or char. Do not free when cleaning vartables
        struct var_node** fields;
        /// @brief size of variable / type in bytes
        size_t size;
    } VarNode;


    /// @brief table of vartables - used by both structDict & varsDict as their element
    typedef struct {
        char* key;
        VarNode* vars;
        int size;
    } VarTab;
    

    extern VarTab globalTable;

    void addStuctVars(char* struct_name, VarNode* fields, int tab_size);

    void addFunctionVars(char* func_name, VarNode* vars, int tab_size);

    /// @brief returns variable table of given funciton
    /// @param func_name name of function or global
    /// @return shallow copy of VarTab object
    VarTab getVarTable(char* func_name);

    /// @brief retrieve global or local variable info
    /// @param func_name name of function or "global"
    /// @param id id of a variable
    /// @return variable information - address, size, fields
    VarNode* getVariable(char* func_name, char* id);

    void freeVarTables();
#endif