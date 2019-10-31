/**
* C����ʵ�ֵĺ����(Red Black Tree)
*
* @author skywang
* @date 2013/11/18
*/


#include "rbtree.h"
#include "iocpdef.h"

#define RED        0    // ��ɫ�ڵ�
#define BLACK      1    // ��ɫ�ڵ�

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
    //��Ȼ�Ѿ���ǰ���˲�Ϊ�յ��ж�, ������Ҫ���ж�һ�²Ž��к�������, 
    //��Ϊ�������õĿ����ܴ�, ������Ч�ж�, ������ĩ�һ����������ν
    //��������, ���¿�������
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
* �����������������
*
* ���������в���ڵ�֮��(ʧȥƽ��)���ٵ��øú�����
* Ŀ���ǽ������������һ�ź������
*
* ����˵����
*     root ������ĸ�
*     node ����Ľ��        // ��Ӧ���㷨���ۡ��е�z
*/
static void rbtree_insert_fixup(RBTREEROOT *root, RBNODE *node)
{
    RBNODE *parent = NULL, *grandpa = NULL;

    // �������ڵ���ڣ����Ҹ��ڵ����ɫ�Ǻ�ɫ��
    while ((parent = rb_parent(node)) && rb_is_red(parent))
    {
        grandpa = rb_parent(parent);

        //�������ڵ㡱�ǡ��游�ڵ�����ӡ�
        if (parent == grandpa->left)
        {
            // Case 1����������ڵ��Ǻ�ɫ
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

            // Case 2�����������Ǻ�ɫ���ҵ�ǰ�ڵ����Һ���
            if (parent->right == node)
            {
                RBNODE *tmp;
                rbtree_left_rotate(root, parent);
                tmp = parent;
                parent = node;
                node = tmp;
            }

            // Case 3�����������Ǻ�ɫ���ҵ�ǰ�ڵ������ӡ�
            if (3)
            {
                rb_set_black(parent);
                rb_set_red(grandpa);
                rbtree_right_rotate(root, grandpa);
            }
        }
        else//����z�ĸ��ڵ㡱�ǡ�z���游�ڵ���Һ��ӡ�
        {
            // Case 1����������ڵ��Ǻ�ɫ
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

            // Case 2�����������Ǻ�ɫ���ҵ�ǰ�ڵ�������
            if (parent->left == node)
            {
                RBNODE *tmp;
                rbtree_right_rotate(root, parent);
                tmp = parent;
                parent = node;
                node = tmp;
            }

            // Case 3�����������Ǻ�ɫ���ҵ�ǰ�ڵ����Һ��ӡ�
            if (3)
            {
                rb_set_black(parent);
                rb_set_red(grandpa);
                rbtree_left_rotate(root, grandpa);
            }
        }
    }

    // �����ڵ���Ϊ��ɫ
    rb_set_black(root->node);
}

/*
* ��ӽڵ㣺���ڵ�(node)���뵽�������
*
* ����˵����
*     root ������ĸ�
*     node ����Ľ��        // ��Ӧ���㷨���ۡ��е�z
*/
int rbtree_insert(RBTREEROOT *root, RBNODE *node)
{
    RBNODE * parent = NULL;
    RBNODE * son = root->node;

    // �����������ͬ��ֵ�Ľڵ㡣
    // (�������������ͬ��ֵ�Ľڵ㣬ע�͵��������仰���ɣ�)
    if (NULL != rbtree_find(root->node, node->key)) return -1;

    // 1. �����������һ�Ŷ�������������ڵ���ӵ�����������С�
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
            parent->left = node;                // ���2������node��������ֵ�� < ��parent��������ֵ������node��Ϊ��parent�����ӡ�
        else
            parent->right = node;            // ���3��(��node��������ֵ�� >= ��parent��������ֵ��)��node��Ϊ��parent���Һ��ӡ� 
    }
    else
    {
        root->node = node;                // ���1����parent�ǿսڵ㣬��node��Ϊ��
    }

    // 2. ���ýڵ����ɫΪ��ɫ
    node->color = RED;

    // 3. ������������Ϊһ�Ŷ��������
    rbtree_insert_fixup(root, node);
    return 0;
}


/*
* �����ɾ����������
*
* �ڴӺ������ɾ������ڵ�֮��(�����ʧȥƽ��)���ٵ��øú�����
* Ŀ���ǽ������������һ�ź������
*
* ����˵����
*     root ������ĸ�
*     node �������Ľڵ�
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
                // Case 1: x���ֵ�w�Ǻ�ɫ��  
                rb_set_black(other);
                rb_set_red(parent);
                rbtree_left_rotate(root, parent);
                other = parent->right;
            }
            if ((!other->left || rb_is_black(other->left)) &&
                (!other->right || rb_is_black(other->right)))
            {
                // Case 2: x���ֵ�w�Ǻ�ɫ����w����������Ҳ���Ǻ�ɫ��  
                rb_set_red(other);
                node = parent;
                parent = rb_parent(node);
            }
            else
            {
                if (!other->right || rb_is_black(other->right))
                {
                    // Case 3: x���ֵ�w�Ǻ�ɫ�ģ�����w�������Ǻ�ɫ���Һ���Ϊ��ɫ��  
                    rb_set_black(other->left);
                    rb_set_red(other);
                    rbtree_right_rotate(root, other);
                    other = parent->right;
                }
                // Case 4: x���ֵ�w�Ǻ�ɫ�ģ�����w���Һ����Ǻ�ɫ�ģ�����������ɫ��
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
                // Case 1: x���ֵ�w�Ǻ�ɫ��  
                rb_set_black(other);
                rb_set_red(parent);
                rbtree_right_rotate(root, parent);
                other = parent->left;
            }
            if ((!other->left || rb_is_black(other->left)) &&
                (!other->right || rb_is_black(other->right)))
            {
                // Case 2: x���ֵ�w�Ǻ�ɫ����w����������Ҳ���Ǻ�ɫ��  
                rb_set_red(other);
                node = parent;
                parent = rb_parent(node);
            }
            else
            {
                if (!other->left || rb_is_black(other->left))
                {
                    // Case 3: x���ֵ�w�Ǻ�ɫ�ģ�����w�������Ǻ�ɫ���Һ���Ϊ��ɫ��  
                    rb_set_black(other->right);
                    rb_set_red(other);
                    rbtree_left_rotate(root, other);
                    other = parent->left;
                }
                // Case 4: x���ֵ�w�Ǻ�ɫ�ģ�����w���Һ����Ǻ�ɫ�ģ�����������ɫ��
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
* ɾ�����
*
* ����˵����
*     tree ������ĸ����
*     node ɾ���Ľ��
*/
static void rbtree_remove_(RBTREEROOT *root, RBNODE *node)
{
    RBNODE *child, *parent;
    int color;

    // ��ɾ���ڵ��"���Һ��Ӷ���Ϊ��"�������
    if ((node->left != NULL) && (node->right != NULL))
    {
        // ��ɾ�ڵ�ĺ�̽ڵ㡣(��Ϊ"ȡ���ڵ�")
        // ������ȡ��"��ɾ�ڵ�"��λ�ã�Ȼ���ٽ�"��ɾ�ڵ�"ȥ����
        RBNODE *replace = node;

        // ��ȡ��̽ڵ�
        replace = replace->right;
        while (replace->left != NULL)
            replace = replace->left;

        // "node�ڵ�"���Ǹ��ڵ�(ֻ�и��ڵ㲻���ڸ��ڵ�)
        if (rb_parent(node))
        {
            if (rb_parent(node)->left == node)
                rb_parent(node)->left = replace;
            else
                rb_parent(node)->right = replace;
        }
        else
            // "node�ڵ�"�Ǹ��ڵ㣬���¸��ڵ㡣
            root->node = replace;

        // child��"ȡ���ڵ�"���Һ��ӣ�Ҳ����Ҫ"�����Ľڵ�"��
        // "ȡ���ڵ�"�϶����������ӣ���Ϊ����һ����̽ڵ㡣
        child = replace->right;
        parent = rb_parent(replace);
        // ����"ȡ���ڵ�"����ɫ
        color = rb_color(replace);

        // "��ɾ���ڵ�"��"���ĺ�̽ڵ�ĸ��ڵ�"
        if (parent == node)
        {
            parent = replace;
        }
        else
        {
            // child��Ϊ��
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
    // ����"ȡ���ڵ�"����ɫ
    color = node->color;

    if (child)
        child->parent = parent;

    // "node�ڵ�"���Ǹ��ڵ�
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
* ɾ����ֵΪkey�Ľ��
*
* ����˵����
*     root ������ĸ����
*     key ��ֵ
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
        if (direction == 0)    // tree�Ǹ��ڵ�
            printf("%4d(B) is root\n", (int)node->key.LowPart);
        else                // tree�Ƿ�֧�ڵ�
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