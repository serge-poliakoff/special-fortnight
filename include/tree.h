/* tree.h */
#ifndef TREE
#define TREE 1

#define FOREACH_TOKEN(TOKEN) \
        TOKEN(Character)   \
        TOKEN(Num)  \
        TOKEN(Ident)   \
        TOKEN(Type)  \
        TOKEN(Eq)  \
        TOKEN(Order)  \
        TOKEN(Addsub)  \
        TOKEN(Divstar)  \
        TOKEN(Or)  \
        TOKEN(And)  \
        TOKEN(Void)  \
        TOKEN(If)  \
        TOKEN(While)  \
        TOKEN(Return)  \
        TOKEN(Else)  \
        TOKEN(Struct)  \
        TOKEN(Prog)  \
        TOKEN(DeclVars)  \
        TOKEN(DeclFoncts)  \
        TOKEN(DeclStructVars)  \
        TOKEN(Declarateurs)  \
        TOKEN(DeclFonct)  \
        TOKEN(EnTeteFonct)  \
        TOKEN(Parametres)  \
        TOKEN(ListTypVar)  \
        TOKEN(Corps)  \
        TOKEN(SuiteInstr)  \
        TOKEN(Instr)  \
        TOKEN(IdExpr)  \
        TOKEN(Exp)  \
        TOKEN(Tb)  \
        TOKEN(Fb)  \
        TOKEN(M)  \
        TOKEN(E)  \
        TOKEN(T)  \
        TOKEN(F)  \
        TOKEN(Arguments)  \
        TOKEN(ListExp)  \
        TOKEN(Not)

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

typedef enum {
  FOREACH_TOKEN(GENERATE_ENUM)

  /* list all other node labels, if any */
  /* The list must coincide with the string array in tree.c */
  /* To avoid listing them twice, see https://stackoverflow.com/a/10966395 */
} label_t;

/// @brief KEYWORD stands for language keyword (struc for declaring a struct),
/// INT for integer value
/// CHAR for character value
/// ID for variable or function identifier
/// TP for type, if the type name would be then contained in tree_label.value.id
/// OP for operator, if the operator signature would be then contained in tree_label.value.id
typedef enum { KEYWORD, INT, CHAR, ID, TP, OP } tree_label_type;

typedef struct  {
  tree_label_type type;
  union {
    label_t label;
    int number;
    char character;
    char* id;
  } value;
} tree_label;

typedef struct Node {
  tree_label label;
  struct Node *firstChild, *nextSibling;
  int lineno;
} Node;

char* strdup(const char* source);
char* getTreeLabelName(int ind);

/// @brief makes KEYWORD tree node from just a label
/// @param label - the keyword of tcp language
/// @return pointer to created node
Node *makeNode(label_t label);

Node *makeNodeFull(tree_label label);

void addSibling(Node *node, Node *sibling);

void addChild(Node *parent, Node *child);

Node* copyTree(Node* tree);

void deleteTree(Node*node);

void printTree(Node *node);

void printNode(Node *node);


#define FIRSTCHILD(node) node->firstChild
#define SECONDCHILD(node) node->firstChild->nextSibling
#define THIRDCHILD(node) node->firstChild->nextSibling->nextSibling


#endif