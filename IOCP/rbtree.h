#ifndef __RED_BLACK_TREE_H__
#define __RED_BLACK_TREE_H__


typedef int(__stdcall * rbtree_compare_fnptr)(void * object1, void * object2);


// ������Ľڵ�
typedef struct __RBNODE{
    void * key;
    unsigned char color;        // ��ɫ(RED �� BLACK)
    struct __RBNODE *left;    // ����
    struct __RBNODE *right;    // �Һ���
    struct __RBNODE *parent;    // �����
    void * data;
}RBNODE;

// ������ĸ�
typedef struct __RBTREEROOT{
    RBNODE *node;
    rbtree_compare_fnptr cmpfn;
}RBTREEROOT;

#ifdef __cplusplus
extern "C" {
#endif

#define rbtree_node_data(node)  ((node) ? (node)->data : 0)
#define rbtree_set_cmpfn(root, fn) do{ (root).cmpfn = fn; } while (0)


    RBNODE * rbtree_find(RBTREEROOT *root, const void * key);


    //����Node��ʱ������init
    RBNODE * rbtree_create_node(const void * key, void * data);

    //�Լ�newһ��Node��ʱ�����ʹ�øú���
    void rbtree_init_node(RBNODE * node, const void * key, void * data);

    //��Ӧrbtree_create_node
    void rbtree_free_node(RBNODE * node);


    // ���ٺ����
    void rbtree_destroy(RBTREEROOT *root);

    RBNODE * rbtree_get_min(RBTREEROOT * root);

    RBNODE * rbtree_get_max(RBTREEROOT * root);


    // �������뵽������С�����ɹ�������0��ʧ�ܷ���-1��
    int rbtree_insert(RBTREEROOT *root, RBNODE *node);

    // ɾ�����(keyΪ�ڵ��ֵ)
    RBNODE * rbtree_remove(RBTREEROOT *root, const void * key);


#define RBTREE_ENUM_PRE  0x01//ǰ��
#define RBTREE_ENUM_MID  0x02//����
#define RBTREE_ENUM_AFT  0x04//����
    //Ĭ��Ӧ����0����Ϊ 0 �������ö��
    typedef int(__stdcall * rbtree_enum_fnptr)(RBNODE * node, void * param);
    void rbtree_enum(RBTREEROOT * root, int etype, rbtree_enum_fnptr eproc, void * eparam);

    void rbtree_print(RBTREEROOT *root);

#ifdef __cplusplus
};
#endif

#endif