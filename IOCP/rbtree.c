/**
* C语言实现的红黑树(Red Black Tree)
*
* @author skywang
* @date 2013/11/18
*/


#include "rbtree.h"
#include "iocpdef.h"

#define RED        0    // 红色节点
#define BLACK      1    // 黑色节点

#define rb_parent(r)   ((r)->parent)
#define rb_color(r) ((r)->color)
#define rb_is_red(r)   ((r)->color == RED)
#define rb_is_black(r)  ((r)->color==BLACK)
#define rb_set_black(r)  do { (r)->color = BLACK; } while (0)
#define rb_set_red(r)  do { (r)->color = RED; } while (0)
#define rb_set_parent(r,p)  do { (r)->parent = (p); } while (0)
#define rb_set_color(r,c)  do { (r)->color = (c); } while (0)


#define RBMALLOC   IOCP_MALLOC
#define RBREALLOC  IOCP_REALLOC
#define RBFREE     IOCP_FREE

RBNODE * rbtree_find(RBTREEROOT *root, const void * key)
{
    int ret = 0;
    RBNODE *node = root->node;
    while ((0 != node) && 0 != (ret = root->cmpfn(node->key, key)))
    {
        if (ret > 0)
        {
            node = node->left;
        }
        else
        {
            node = node->right;
        }
    }

    return node;
}

RBNODE * rbtree_create_node(const void * key, void * data)
{
    RBNODE * node = (RBNODE *)RBMALLOC(sizeof(RBNODE));

    if (node)
    {
        rbtree_init_node(node, key, data);
    }

    return node;
}


void rbtree_init_node(RBNODE * node, const void * key, void * data)
{
    memset(node, 0, sizeof(RBNODE));
    node->key = key;
    node->data = data;
}


void rbtree_free_node(RBNODE * node)
{
    RBFREE(node);
}


static void rbtree_destroy_(RBNODE * node)
{
    if (0 == node) return;
    //虽然已经提前做了不为空的判断, 但还是要再判断一下才进行函数调用, 
    //因为函数调用的开销很大, 不做有效判断, 到了树末梢会产生大量无谓
    //函数调用, 导致开销增加
    if (node->left)
    {
        rbtree_destroy_(node->left);
    }

    if (node->right)
    {
        rbtree_destroy_(node->right);
    }

    rbtree_free_node(node);
}

void rbtree_destroy(RBTREEROOT *root)
{
    if (root != NULL)
    {
        rbtree_destroy_(root->node);
        root->node = NULL;
    }
}


static RBNODE * rbtree_get_min_(RBNODE * node)
{
    if (0 == node) return 0;
    while (0 != node->left)
        node = node->left;

    return node;
}

RBNODE * rbtree_get_min(RBTREEROOT * root)
{
    return rbtree_get_min_(root->node);
}


static RBNODE * rbtree_get_max_(RBNODE * node)
{
    if (0 == node) return 0;
    while (0 != node->right)
        node = node->right;

    return node;
}

RBNODE * rbtree_get_max(RBTREEROOT * root)
{
    return rbtree_get_max_(root->node);
}


static void rbtree_left_rotate(RBTREEROOT *root, RBNODE * node)
{
    RBNODE *node2 = node->right;

    node->right = node2->left;
    if (node2->left != NULL)
        node2->left->parent = node;

    node2->parent = node->parent;

    if (node->parent == NULL)
    {
        root->node = node2;
    }
    else
    {
        if (node->parent->left == node)
            node->parent->left = node2;
        else
            node->parent->right = node2;
    }

    node2->left = node;
    node->parent = node2;
}


static void rbtree_right_rotate(RBTREEROOT *root, RBNODE * node)
{
    RBNODE *node2 = node->left;

    node->left = node2->right;
    if (node2->right != NULL)
        node2->right->parent = node;

    node2->parent = node->parent;

    if (node->parent == NULL)
    {
        root->node = node2;
    }
    else
    {
        if (node == node->parent->right)
            node->parent->right = node2;
        else
            node->parent->left = node2;
    }

    node2->right = node;

    node->parent = node2;
}

/*
* 红黑树插入修正函数
*
* 在向红黑树中插入节点之后(失去平衡)，再调用该函数；
* 目的是将它重新塑造成一颗红黑树。
*
* 参数说明：
*     root 红黑树的根
*     node 插入的结点        // 对应《算法导论》中的z
*/
static void rbtree_insert_fixup(RBTREEROOT *root, RBNODE *node)
{
    RBNODE *parent = NULL, *grandpa = NULL;

    // 若“父节点存在，并且父节点的颜色是红色”
    while ((parent = rb_parent(node)) && rb_is_red(parent))
    {
        grandpa = rb_parent(parent);

        //若“父节点”是“祖父节点的左孩子”
        if (parent == grandpa->left)
        {
            // Case 1条件：叔叔节点是红色
            if (1)
            {
                RBNODE *uncle = grandpa->right;
                if (uncle && rb_is_red(uncle))
                {
                    rb_set_black(uncle);
                    rb_set_black(parent);
                    rb_set_red(grandpa);
                    node = grandpa;
                    continue;
                }
            }

            // Case 2条件：叔叔是黑色，且当前节点是右孩子
            if (parent->right == node)
            {
                RBNODE *tmp;
                rbtree_left_rotate(root, parent);
                tmp = parent;
                parent = node;
                node = tmp;
            }

            // Case 3条件：叔叔是黑色，且当前节点是左孩子。
            if (3)
            {
                rb_set_black(parent);
                rb_set_red(grandpa);
                rbtree_right_rotate(root, grandpa);
            }
        }
        else//若“z的父节点”是“z的祖父节点的右孩子”
        {
            // Case 1条件：叔叔节点是红色
            if (1)
            {
                RBNODE *uncle = grandpa->left;
                if (uncle && rb_is_red(uncle))
                {
                    rb_set_black(uncle);
                    rb_set_black(parent);
                    rb_set_red(grandpa);
                    node = grandpa;
                    continue;
                }
            }

            // Case 2条件：叔叔是黑色，且当前节点是左孩子
            if (parent->left == node)
            {
                RBNODE *tmp;
                rbtree_right_rotate(root, parent);
                tmp = parent;
                parent = node;
                node = tmp;
            }

            // Case 3条件：叔叔是黑色，且当前节点是右孩子。
            if (3)
            {
                rb_set_black(parent);
                rb_set_red(grandpa);
                rbtree_left_rotate(root, grandpa);
            }
        }
    }

    // 将根节点设为黑色
    rb_set_black(root->node);
}

/*
* 添加节点：将节点(node)插入到红黑树中
*
* 参数说明：
*     root 红黑树的根
*     node 插入的结点        // 对应《算法导论》中的z
*/
int rbtree_insert(RBTREEROOT *root, RBNODE *node)
{
    RBNODE * parent = NULL;
    RBNODE * son = root->node;

    // 不允许插入相同键值的节点。
    // (若想允许插入相同键值的节点，注释掉下面两句话即可！)
    if (NULL != rbtree_find(root->node, node->key)) return -1;

    // 1. 将红黑树当作一颗二叉查找树，将节点添加到二叉查找树中。
    while (son != NULL)
    {
        parent = son;
        if (root->cmpfn(son->key, node->key) > 0)
            son = son->left;
        else
            son = son->right;
    }
    rb_parent(node) = parent;

    if (parent != NULL)
    {
        if (root->cmpfn(parent->key, node->key) > 0)
            parent->left = node;                // 情况2：若“node所包含的值” < “parent所包含的值”，则将node设为“parent的左孩子”
        else
            parent->right = node;            // 情况3：(“node所包含的值” >= “parent所包含的值”)将node设为“parent的右孩子” 
    }
    else
    {
        root->node = node;                // 情况1：若parent是空节点，则将node设为根
    }

    // 2. 设置节点的颜色为红色
    node->color = RED;

    // 3. 将它重新修正为一颗二叉查找树
    rbtree_insert_fixup(root, node);
    return 0;
}


/*
* 红黑树删除修正函数
*
* 在从红黑树中删除插入节点之后(红黑树失去平衡)，再调用该函数；
* 目的是将它重新塑造成一颗红黑树。
*
* 参数说明：
*     root 红黑树的根
*     node 待修正的节点
*/
static void rbtree_remove_fixup(RBTREEROOT *root, RBNODE *node, RBNODE *parent)
{
    RBNODE *other;

    while ((!node || rb_is_black(node)) && node != root->node)
    {
        if (parent->left == node)
        {
            other = parent->right;
            if (rb_is_red(other))
            {
                // Case 1: x的兄弟w是红色的  
                rb_set_black(other);
                rb_set_red(parent);
                rbtree_left_rotate(root, parent);
                other = parent->right;
            }
            if ((!other->left || rb_is_black(other->left)) &&
                (!other->right || rb_is_black(other->right)))
            {
                // Case 2: x的兄弟w是黑色，且w的俩个孩子也都是黑色的  
                rb_set_red(other);
                node = parent;
                parent = rb_parent(node);
            }
            else
            {
                if (!other->right || rb_is_black(other->right))
                {
                    // Case 3: x的兄弟w是黑色的，并且w的左孩子是红色，右孩子为黑色。  
                    rb_set_black(other->left);
                    rb_set_red(other);
                    rbtree_right_rotate(root, other);
                    other = parent->right;
                }
                // Case 4: x的兄弟w是黑色的；并且w的右孩子是红色的，左孩子任意颜色。
                rb_set_color(other, rb_color(parent));
                rb_set_black(parent);
                rb_set_black(other->right);
                rbtree_left_rotate(root, parent);
                node = root->node;
                break;
            }
        }
        else
        {
            other = parent->left;
            if (rb_is_red(other))
            {
                // Case 1: x的兄弟w是红色的  
                rb_set_black(other);
                rb_set_red(parent);
                rbtree_right_rotate(root, parent);
                other = parent->left;
            }
            if ((!other->left || rb_is_black(other->left)) &&
                (!other->right || rb_is_black(other->right)))
            {
                // Case 2: x的兄弟w是黑色，且w的俩个孩子也都是黑色的  
                rb_set_red(other);
                node = parent;
                parent = rb_parent(node);
            }
            else
            {
                if (!other->left || rb_is_black(other->left))
                {
                    // Case 3: x的兄弟w是黑色的，并且w的左孩子是红色，右孩子为黑色。  
                    rb_set_black(other->right);
                    rb_set_red(other);
                    rbtree_left_rotate(root, other);
                    other = parent->left;
                }
                // Case 4: x的兄弟w是黑色的；并且w的右孩子是红色的，左孩子任意颜色。
                rb_set_color(other, rb_color(parent));
                rb_set_black(parent);
                rb_set_black(other->left);
                rbtree_right_rotate(root, parent);
                node = root->node;
                break;
            }
        }
    }
    if (node)
        rb_set_black(node);
}

/*
* 删除结点
*
* 参数说明：
*     tree 红黑树的根结点
*     node 删除的结点
*/
static void rbtree_remove_(RBTREEROOT *root, RBNODE *node)
{
    RBNODE *child, *parent;
    int color;

    // 被删除节点的"左右孩子都不为空"的情况。
    if ((node->left != NULL) && (node->right != NULL))
    {
        // 被删节点的后继节点。(称为"取代节点")
        // 用它来取代"被删节点"的位置，然后再将"被删节点"去掉。
        RBNODE *replace = node;

        // 获取后继节点
        replace = replace->right;
        while (replace->left != NULL)
            replace = replace->left;

        // "node节点"不是根节点(只有根节点不存在父节点)
        if (rb_parent(node))
        {
            if (rb_parent(node)->left == node)
                rb_parent(node)->left = replace;
            else
                rb_parent(node)->right = replace;
        }
        else
            // "node节点"是根节点，更新根节点。
            root->node = replace;

        // child是"取代节点"的右孩子，也是需要"调整的节点"。
        // "取代节点"肯定不存在左孩子！因为它是一个后继节点。
        child = replace->right;
        parent = rb_parent(replace);
        // 保存"取代节点"的颜色
        color = rb_color(replace);

        // "被删除节点"是"它的后继节点的父节点"
        if (parent == node)
        {
            parent = replace;
        }
        else
        {
            // child不为空
            if (child)
                rb_set_parent(child, parent);
            parent->left = child;

            replace->right = node->right;
            rb_set_parent(node->right, replace);
        }

        replace->parent = node->parent;
        replace->color = node->color;
        replace->left = node->left;
        node->left->parent = replace;

        if (color == BLACK)
            rbtree_remove_fixup(root, child, parent);
        free(node);

        return;
    }

    if (node->left != NULL)
        child = node->left;
    else
        child = node->right;

    parent = node->parent;
    // 保存"取代节点"的颜色
    color = node->color;

    if (child)
        child->parent = parent;

    // "node节点"不是根节点
    if (parent)
    {
        if (parent->left == node)
            parent->left = child;
        else
            parent->right = child;
    }
    else
        root->node = child;

    if (color == BLACK)
        rbtree_remove_fixup(root, child, parent);
}

/*
* 删除键值为key的结点
*
* 参数说明：
*     root 红黑树的根结点
*     key 键值
*/
RBNODE * rbtree_remove(RBTREEROOT *root, const void * key)
{
    RBNODE * node = rbtree_find(root, key);

    if (NULL == node) return NULL;

    rbtree_remove_(root, node);

    return node;
}


static int rbtree_enum_(RBNODE * node, int etype, rbtree_enum_fnptr eproc, void * eparam)
{
    if (0 == node) return 0;

    if (RBTREE_ENUM_PRE == etype && 0 != eproc(node, eparam))
    {
        return 1;
    }

    if (0 != rbtree_enum_(node->left, etype, eproc, eparam))
    {
        return 1;
    }

    if (RBTREE_ENUM_MID == etype && 0 != eproc(node, eparam))
    {
        return 1;
    }

    if (0 != rbtree_enum_(node->right, etype, eproc, eparam))
    {
        return 1;
    }

    if (RBTREE_ENUM_AFT == etype && 0 != eproc(node, eparam))
    {
        return 1;
    }

    return 1;
}

void rbtree_enum(RBTREEROOT * root, int etype, rbtree_enum_fnptr eproc, void * eparam)
{
    RBNODE * node = root->node;
    rbtree_enum_(node, etype, eproc, eparam);
}



static void rbtree_print_(RBNODE * node, int direction)
{
#if 0
    if (node)
    {
        if (direction == 0)    // tree是根节点
            printf("%4d(B) is root\n", (int)node->key.LowPart);
        else                // tree是分支节点
            printf("%4d(%s) is %2d's %6s child\n", (int)node->key.LowPart, rb_is_red(node) ? "R" : "B", node,
            direction == 1 ? "right" : "left");

        rbtree_print_(node->left, -1);
        rbtree_print_(node->right, 1);
    }
#endif
}

void rbtree_print(RBTREEROOT *root)
{
    if (root != NULL && root->node != NULL)
        rbtree_print_(root->node, 0);
}