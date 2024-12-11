#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"

void freeObject(Object* obj);
void freeScope(Scope* scope);
void freeObjectList(ObjectNode *objList);
void freeConstantValue(ConstantValue* value);

SymTab* symtab;
Type* intType;
Type* charType;

/******************* Type utilities ******************************/

Type* makeIntType(void) {
  Type* type = (Type*) malloc(sizeof(Type));
  type->typeClass = TP_INT;
  return type;
}

Type* makeCharType(void) {
  Type* type = (Type*) malloc(sizeof(Type));
  type->typeClass = TP_CHAR;
  return type;
}

Type* makeArrayType(int arraySize, Type* elementType) {
  Type* type = (Type*) malloc(sizeof(Type));
  type->typeClass = TP_ARRAY;
  type->arraySize = arraySize;
  type->elementType = elementType;
  return type;
}

Type* duplicateType(Type* type) {
  if (type == NULL) return NULL;
  Type* newType = (Type*) malloc(sizeof(Type));
  newType->typeClass = type->typeClass;
  if (type->typeClass == TP_ARRAY) {
    newType->arraySize = type->arraySize;
    newType->elementType = duplicateType(type->elementType);
  } else {
    newType->arraySize = 0;
    newType->elementType = NULL;
  }
  return newType;
}

int compareType(Type* type1, Type* type2) {
  if (type1 == NULL || type2 == NULL)
    return 0;
  if (type1->typeClass != type2->typeClass)
    return 0;
  if (type1->typeClass == TP_ARRAY) {
    return (type1->arraySize == type2->arraySize) &&
           compareType(type1->elementType, type2->elementType);
  }
  return 1;  // For TP_INT and TP_CHAR
}

void freeType(Type* type) {
  if (type == NULL) return;
  if (type->typeClass == TP_ARRAY) {
    freeType(type->elementType);
  }
  free(type);
}

/******************* Constant utility ******************************/

ConstantValue* makeIntConstant(int i) {
  ConstantValue* constVal = (ConstantValue*) malloc(sizeof(ConstantValue));
  constVal->type = TP_INT;
  constVal->intValue = i;
  return constVal;
}

ConstantValue* makeCharConstant(char ch) {
  ConstantValue* constVal = (ConstantValue*) malloc(sizeof(ConstantValue));
  constVal->type = TP_CHAR;
  constVal->charValue = ch;
  return constVal;
}

ConstantValue* duplicateConstantValue(ConstantValue* v) {
  if (v == NULL) return NULL;
  ConstantValue* newVal = (ConstantValue*) malloc(sizeof(ConstantValue));
  newVal->type = v->type;
  if (v->type == TP_INT)
    newVal->intValue = v->intValue;
  else if (v->type == TP_CHAR)
    newVal->charValue = v->charValue;
  return newVal;
}

/******************* Object utilities ******************************/

Scope* createScope(Object* owner, Scope* outer) {
  Scope* scope = (Scope*) malloc(sizeof(Scope));
  scope->objList = NULL;
  scope->owner = owner;
  scope->outer = outer;
  return scope;
}

Object* createProgramObject(char *programName) {
  Object* program = (Object*) malloc(sizeof(Object));
  strcpy(program->name, programName);
  program->kind = OBJ_PROGRAM;
  program->progAttrs = (ProgramAttributes*) malloc(sizeof(ProgramAttributes));
  program->progAttrs->scope = createScope(program, NULL);
  symtab->program = program;

  return program;
}

Object* createConstantObject(char *name) {
  Object* obj = (Object*) malloc(sizeof(Object));
  strcpy(obj->name, name);
  obj->kind = OBJ_CONSTANT;
  obj->constAttrs = (ConstantAttributes*) malloc(sizeof(ConstantAttributes));
  obj->constAttrs->value = NULL;  // Value to be set later
  return obj;
}

Object* createTypeObject(char *name) {
  Object* obj = (Object*) malloc(sizeof(Object));
  strcpy(obj->name, name);
  obj->kind = OBJ_TYPE;
  obj->typeAttrs = (TypeAttributes*) malloc(sizeof(TypeAttributes));
  obj->typeAttrs->actualType = NULL;  // To be set later
  return obj;
}

Object* createVariableObject(char *name) {
  Object* obj = (Object*) malloc(sizeof(Object));
  strcpy(obj->name, name);
  obj->kind = OBJ_VARIABLE;
  obj->varAttrs = (VariableAttributes*) malloc(sizeof(VariableAttributes));
  obj->varAttrs->type = NULL;  // To be set later
  obj->varAttrs->scope = symtab->currentScope;
  return obj;
}

Object* createFunctionObject(char *name) {
  Object* obj = (Object*) malloc(sizeof(Object));
  strcpy(obj->name, name);
  obj->kind = OBJ_FUNCTION;
  obj->funcAttrs = (FunctionAttributes*) malloc(sizeof(FunctionAttributes));
  obj->funcAttrs->paramList = NULL;
  obj->funcAttrs->returnType = NULL;  // To be set later
  obj->funcAttrs->scope = createScope(obj, symtab->currentScope);
  return obj;
}

Object* createProcedureObject(char *name) {
  Object* obj = (Object*) malloc(sizeof(Object));
  strcpy(obj->name, name);
  obj->kind = OBJ_PROCEDURE;
  obj->procAttrs = (ProcedureAttributes*) malloc(sizeof(ProcedureAttributes));
  obj->procAttrs->paramList = NULL;
  obj->procAttrs->scope = createScope(obj, symtab->currentScope);
  return obj;
}

Object* createParameterObject(char *name, enum ParamKind kind, Object* owner) {
  Object* obj = (Object*) malloc(sizeof(Object));
  strcpy(obj->name, name);
  obj->kind = OBJ_PARAMETER;
  obj->paramAttrs = (ParameterAttributes*) malloc(sizeof(ParameterAttributes));
  obj->paramAttrs->kind = kind;
  obj->paramAttrs->type = NULL;  // To be set later
  obj->paramAttrs->function = owner;
  return obj;
}

void freeObject(Object* obj) {
  if (obj == NULL) return;
  switch(obj->kind) {
    case OBJ_CONSTANT:
      freeConstantValue(obj->constAttrs->value);
      free(obj->constAttrs);
      break;
    case OBJ_TYPE:
      freeType(obj->typeAttrs->actualType);
      free(obj->typeAttrs);
      break;
    case OBJ_VARIABLE:
      freeType(obj->varAttrs->type);
      free(obj->varAttrs);
      break;
    case OBJ_FUNCTION:
      freeObjectList(obj->funcAttrs->paramList); // Frees parameters
      freeScope(obj->funcAttrs->scope);
      freeType(obj->funcAttrs->returnType);
      free(obj->funcAttrs);
      break;
    case OBJ_PROCEDURE:
      freeObjectList(obj->procAttrs->paramList); // Frees parameters
      freeScope(obj->procAttrs->scope);
      free(obj->procAttrs);
      break;
    case OBJ_PROGRAM:
      freeScope(obj->progAttrs->scope);
      free(obj->progAttrs);
      break;
    case OBJ_PARAMETER:
      freeType(obj->paramAttrs->type);
      free(obj->paramAttrs);
      break;
  }
  free(obj);
  obj = NULL;
}

void freeScope(Scope* scope) {
  if (scope == NULL) return;
  freeObjectList(scope->objList);
  // Owner is managed elsewhere
  free(scope);
}

void freeObjectList(ObjectNode *objList) {
  ObjectNode *node = objList;
  while (node != NULL) {
    ObjectNode *next = node->next;
    freeObject(node->object);
    free(node);
    node = next;
  }
}

void freeConstantValue(ConstantValue* value) {
  if (value != NULL) {
    free(value);
  }
}

void addObject(ObjectNode **objList, Object* obj) {
  ObjectNode* node = (ObjectNode*) malloc(sizeof(ObjectNode));
  node->object = obj;
  node->next = NULL;
  if ((*objList) == NULL)
    *objList = node;
  else {
    ObjectNode *n = *objList;
    while (n->next != NULL)
      n = n->next;
    n->next = node;
  }
}

Object* findObject(ObjectNode *objList, char *name) {
  ObjectNode *node = objList;
  while (node != NULL) {
    if (strcmp(node->object->name, name) == 0)
      return node->object;
    node = node->next;
  }
  return NULL;
}

/******************* others ******************************/

void initSymTab(void) {
  Object* obj;
  Object* param;

  symtab = (SymTab*) malloc(sizeof(SymTab));
  symtab->globalObjectList = NULL;

  obj = createFunctionObject("READC");
  obj->funcAttrs->returnType = makeCharType();
  addObject(&(symtab->globalObjectList), obj);

  obj = createFunctionObject("READI");
  obj->funcAttrs->returnType = makeIntType();
  addObject(&(symtab->globalObjectList), obj);

  obj = createProcedureObject("WRITEI");
  param = createParameterObject("i", PARAM_VALUE, obj);
  param->paramAttrs->type = makeIntType();
  addObject(&(obj->procAttrs->paramList), param);
  addObject(&(symtab->globalObjectList), obj);

  obj = createProcedureObject("WRITEC");
  param = createParameterObject("ch", PARAM_VALUE, obj);
  param->paramAttrs->type = makeCharType();
  addObject(&(obj->procAttrs->paramList), param);
  addObject(&(symtab->globalObjectList), obj);

  obj = createProcedureObject("WRITELN");
  addObject(&(symtab->globalObjectList), obj);

  intType = makeIntType();
  charType = makeCharType();
}

void cleanSymTab(void) {
  freeObject(symtab->program);
  freeObjectList(symtab->globalObjectList);
  free(symtab);
  freeType(intType);
  freeType(charType);
}

void enterBlock(Scope* scope) {
  symtab->currentScope = scope;
}

void exitBlock(void) {
  symtab->currentScope = symtab->currentScope->outer;
}

void declareObject(Object* obj) {
  if (obj->kind == OBJ_PARAMETER) {
    Object* owner = symtab->currentScope->owner;
    switch (owner->kind) {
      case OBJ_FUNCTION:
        addObject(&(owner->funcAttrs->paramList), obj);
        break;
      case OBJ_PROCEDURE:
        addObject(&(owner->procAttrs->paramList), obj);
        break;
      default:
        break;
    }
    // Do NOT add parameters to currentScope->objList to prevent double-free
  } else {
    // Add other objects to the current scope's object list
    addObject(&(symtab->currentScope->objList), obj);
  }
}