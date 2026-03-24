/* tree.c */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "tree.h"
extern int linenum;       /* from lexer */

static const char *StringFromLabel[] = {
  FOREACH_TOKEN(GENERATE_STRING)
};

Node *makeNode(label_t label) {
  Node *node = malloc(sizeof(Node));
  if (!node) {
    printf("Run out of memory\n");
    exit(1);
  }
  tree_label l;
  l.type = KEYWORD;
  l.value.label = label;
  node->label = l;
  node-> firstChild = node->nextSibling = NULL;
  node->lineno=linenum;
  return node;
}

Node *makeNodeFull(tree_label label) {
  Node *node = malloc(sizeof(Node));
  if (!node) {
    printf("Run out of memory\n");
    exit(1);
  }
  node->label = label;
  node-> firstChild = node->nextSibling = NULL;
  node->lineno=linenum;
  return node;
}

void addSibling(Node *node, Node *sibling) {
  Node *curr = node;
  while (curr->nextSibling != NULL) {
    curr = curr->nextSibling;
  }
  curr->nextSibling = sibling;
}

void addChild(Node *parent, Node *child) {
  if (parent->firstChild == NULL) {
    parent->firstChild = child;
  }
  else {
    addSibling(parent->firstChild, child);
  }
}

void deleteTree(Node *node) {
  if (node->firstChild) {
    deleteTree(node->firstChild);
  }
  if (node->nextSibling) {
    deleteTree(node->nextSibling);
  }
  if (node->label.type == 3){ // id
    //printf("Free %s", node->label.value.id);
    free(node->label.value.id);
  }
  free(node);
}

void printNode(Node *node){
  if (node->label.type == KEYWORD)
    printf("%s", StringFromLabel[node->label.value.label]);
  else if (node->label.type == INT)
    printf("%d", node->label.value.number);
  else if (node->label.type == CHAR)
    printf("\'%c\'", node->label.value.character);
  else // (node->label.type == ID)
    printf("%s", node->label.value.id);
}

void printTree(Node *node) {
  static bool rightmost[128]; // tells if node is rightmost sibling
  static int depth = 0;       // depth of current node
  for (int i = 1; i < depth; i++) { // 2502 = vertical line
    printf(rightmost[i] ? "    " : "\u2502   ");
  }
  if (depth > 0) { // 2514 = L form; 2500 = horizontal line; 251c = vertical line and right horiz 
    printf(rightmost[depth] ? "\u2514\u2500\u2500 " : "\u251c\u2500\u2500 ");
  }
  //printf("%s", StringFromLabel[node->label]);
  printNode(node);
  printf("\n");
  depth++;
  for (Node *child = node->firstChild; child != NULL; child = child->nextSibling) {
    rightmost[depth] = (child->nextSibling) ? false : true;
    printTree(child);
  }
  depth--;
}
