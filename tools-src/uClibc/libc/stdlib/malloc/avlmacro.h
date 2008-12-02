/* MACRO-CODED FAST FIXED AVL TREES IMPLEMENTATION IN C              */
/* COPYRIGHT (C) 1998 VALERY SHCHEDRIN                               */
/* IT IS DISTRIBUTED UNDER GLPL (GNU GENERAL LIBRARY PUBLIC LICENSE) */

/*
 * Manuel Novoa III       Jan 2001
 *
 * Modified to decrease object size.
 *   Tree balancing is now done in a fuction rather than inline.
 *   Removed common code in balance by use of a goto.
 *   Added macro Avl_Tree_no_replace since ptrs_replace was not used.
 *   Prepended may symbols with "__" for possible conversion to extern.
 */

#define __Avl_balance_proto(objname, pr, root) \
  static int __Avl_##objname##pr##balance(objname **root) \
  { \
    objname *p; \
    int ht_changed; \
    p = *root; \
    if (p->bal_##pr < -1) \
    { \
      if (p->l_##pr->bal_##pr == 1) \
      { \
	objname *pp; \
	pp=p->l_##pr; *root=p->l_##pr->r_##pr; p->l_##pr = (*root)->r_##pr; \
	(*root)->r_##pr = p; pp->r_##pr = (*root)->l_##pr; \
	p = *root; p->l_##pr = pp; \
    goto pr_common_ht_changed; \
      } \
      else \
      { \
	ht_changed = (p->l_##pr ->bal_##pr)?1:0; \
	*root = p->l_##pr ; \
	p->l_##pr = (*root)->r_##pr ; (*root)->r_##pr = p; \
	p->bal_##pr = - (++((*root)->bal_##pr )); \
      } \
    } \
    else if (p->bal_##pr > 1) \
    { \
      if (p->r_##pr->bal_##pr == -1) \
      { \
	objname *pp; \
	pp=p->r_##pr ; *root=p->r_##pr ->l_##pr ; p->r_##pr =(*root)->l_##pr ; \
	(*root)->l_##pr = p; pp->l_##pr = (*root)->r_##pr ; \
	p = *root; p->r_##pr = pp; \
    pr_common_ht_changed: \
	if (p->bal_##pr > 0) p->l_##pr ->bal_##pr = -p->bal_##pr ; \
	else p->l_##pr ->bal_##pr = 0; \
	if (p->bal_##pr < 0) p->r_##pr ->bal_##pr = -p->bal_##pr ; \
	else p->r_##pr ->bal_##pr = 0; \
	p->bal_##pr = 0; \
	ht_changed = 1; \
      } \
      else \
      { \
	ht_changed = (p->r_##pr ->bal_##pr)?1:0; \
	*root = p->r_##pr ; \
	p->r_##pr = (*root)->l_##pr ; (*root)->l_##pr = p; \
	p->bal_##pr = - (--((*root)->bal_##pr )); \
      } \
    } else ht_changed = 0; \
   return ht_changed; \
  }

#define balance(objname, pr, root) \
  __Avl_##objname##pr##balance(root)

#define __Avl_r_insert_proto(objname, pr, COMPARE) \
  static int __Avl_##objname##pr##_r_insert(objname **root) \
  { \
    int i; /* height increase */ \
    if (!*root) \
    { \
      *root = __Avl_##objname##pr##_new_node; \
      __Avl_##objname##pr##_new_node = NULL; \
      return 1; \
    } \
    COMPARE(i, __Avl_##objname##pr##_new_node, *root); \
    \
    if (i < 0) \
    { /* insert into the left subtree */ \
      i = -__Avl_##objname##pr##_r_insert(&((*root)->l_##pr)); \
      if (__Avl_##objname##pr##_new_node != NULL) return 0; /* already there */ \
    } \
    else if (i > 0) \
    { /* insert into the right subtree */ \
      i = __Avl_##objname##pr##_r_insert(&((*root)->r_##pr)); \
      if (__Avl_##objname##pr##_new_node != NULL) return 0; /* already there */ \
    } \
    else \
    { /* found */ \
      __Avl_##objname##pr##_new_node = *root; \
      return 0; \
    } \
    if (!i) return 0; \
    (*root)->bal_##pr += i; /* update balance factor */ \
    if ((*root)->bal_##pr) \
    { \
      return 1 - balance(objname,pr,root); \
    } \
    else return 0; \
  }

#define __Avl_r_delete_proto(objname,pr,COMPARE) \
  static int __Avl_##objname##pr##_r_delete(objname **root) \
  { \
    int i; /* height decrease */ \
    \
    if (!*root) return 0; /* not found */ \
    \
    COMPARE(i, __Avl_##objname##pr##_new_node, *root); \
    \
    if (i < 0) \
      i = -__Avl_##objname##pr##_r_delete(&((*root)->l_##pr)); \
    else if (i > 0) \
      i =  __Avl_##objname##pr##_r_delete(&((*root)->r_##pr)); \
    else \
    { \
      if (!(*root)->l_##pr) \
      { \
	*root = (*root)->r_##pr; \
	return 1; \
      } \
      else if (!(*root)->r_##pr) \
      { \
	*root = (*root)->l_##pr; \
	return 1; \
      } \
      else \
      { \
	i = __Avl_##objname##pr##_r_delfix(&((*root)->r_##pr)); \
	__Avl_##objname##pr##_new_node->l_##pr = (*root)->l_##pr; \
	__Avl_##objname##pr##_new_node->r_##pr = (*root)->r_##pr; \
	__Avl_##objname##pr##_new_node->bal_##pr = (*root)->bal_##pr; \
	*root = __Avl_##objname##pr##_new_node; \
      } \
    } \
    if (!i) return 0; \
    (*root)->bal_##pr -= i; \
    if ((*root)->bal_##pr) \
    { \
      return balance(objname,pr,root); \
    } \
    return 1; \
  }

#define __Avl_r_delfix_proto(objname,pr) \
  static int __Avl_##objname##pr##_r_delfix(objname **root) \
  { \
    int i; /* height decrease */ \
    \
    if (!(*root)->l_##pr) \
    { \
      __Avl_##objname##pr##_new_node = *root; \
      *root = (*root)->r_##pr; \
      return 1; \
    } \
    i = -__Avl_##objname##pr##_r_delfix(&((*root)->l_##pr)); \
    if (!i) return 0; \
    (*root)->bal_##pr -= i; \
    if ((*root)->bal_##pr) \
    { \
      return balance(objname,pr,root); \
    } \
    return 1; \
  }

#define __Avl_ins_proto(alias,objname,pr) \
  objname *__##alias##_ins(objname *data) \
  { \
    __Avl_##objname##pr##_new_node = data; \
    (data)->l_##pr = NULL; \
    (data)->r_##pr = NULL; \
    (data)->bal_##pr = 0; \
    __Avl_##objname##pr##_r_insert(&__Avl_##objname##pr##_tree); \
    if (__Avl_##objname##pr##_new_node) \
      return __Avl_##objname##pr##_new_node; \
    return NULL; \
  }

#define __Avl_del_proto(alias,objname,pr) \
  void __##alias##_del(objname *data) \
  { \
    __Avl_##objname##pr##_new_node = data; \
    __Avl_##objname##pr##_r_delete(&__Avl_##objname##pr##_tree); \
  }

#define __Avl_replace_proto(alias,objname,pr,COMPARE) \
  void __##alias##_replace(objname *data) \
  { \
    objname **p = &__Avl_##objname##pr##_tree; \
    int cmp; \
    while (*p) \
    { \
      COMPARE(cmp, data, *p); \
      if (cmp < 0) \
	p = &((*p)->l_##pr); \
      else if (cmp > 0) \
	p = &((*p)->r_##pr); \
      else \
      { \
	(data)->l_##pr = (*p)->l_##pr; \
	(data)->r_##pr = (*p)->r_##pr; \
	(data)->bal_##pr = (*p)->bal_##pr; \
	*p = data; \
	return; \
      } \
    } \
  }

#define Avl_Root(objname,pr) __Avl_##objname##pr##_tree

#define Avl_Tree_no_replace(alias,objname,pr,COMPARE) \
objname *__Avl_##objname##pr##_tree = NULL; \
static objname *__Avl_##objname##pr##_new_node; \
__Avl_balance_proto(objname, pr, root) \
__Avl_r_insert_proto(objname,pr,COMPARE) \
__Avl_r_delfix_proto(objname,pr) \
__Avl_r_delete_proto(objname,pr,COMPARE) \
__Avl_ins_proto(alias,objname,pr) \
__Avl_del_proto(alias,objname,pr)

#define Avl_Tree(alias,objname,pr,COMPARE) \
Avl_Tree_no_replace(alias,objname,pr,COMPARE) \
__Avl_replace_proto(alias,objname,pr,COMPARE)
