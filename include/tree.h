/* tree.h */

#define FOREACH_TOKEN(TOKEN) \
        TOKEN(character)   \
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
        TOKEN(TB)  \
        TOKEN(FB)  \
        TOKEN(M)  \
        TOKEN(E)  \
        TOKEN(T)  \
        TOKEN(F)  \
        TOKEN(Arguments)  \
        TOKEN(ListExp)

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

typedef enum {
  FOREACH_TOKEN(GENERATE_ENUM)

  /* list all other node labels, if any */
  /* The list must coincide with the string array in tree.c */
  /* To avoid listing them twice, see https://stackoverflow.com/a/10966395 */
} label_t;

typedef struct Node {
  label_t label;
  struct Node *firstChild, *nextSibling;
  int lineno;
} Node;

Node *makeNode(label_t label);
void addSibling(Node *node, Node *sibling);
void addChild(Node *parent, Node *child);
void deleteTree(Node*node);
void printTree(Node *node);

void printNode(Node *node);

#define FIRSTCHILD(node) node->firstChild
#define SECONDCHILD(node) node->firstChild->nextSibling
#define THIRDCHILD(node) node->firstChild->nextSibling->nextSibling
