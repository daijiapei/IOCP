

#ifndef __LIST_H__
#define __LIST_H__


/*
* List definitions.
*/
#define LIST_HEAD(name, type)						\
struct name {								\
	struct type *First;	/* first element */			\
}

#define LIST_HEAD_INITIALIZER(head)					\
    	{ NULL }

#define LIST_ENTRY(type)						\
struct {								\
	struct type *Next;	/* next element */			\
	struct type **Prev;	/* address of previous next element */	\
}

/*
* List access methods
*/
#define	LIST_FIRST(head)		((head)->First)
#define	LIST_END(head)			NULL
#define	LIST_EMPTY(head)		(LIST_FIRST(head) == LIST_END(head))
#define	LIST_NEXT(elm, field)		((elm)->field.Next)

#define LIST_FOREACH(var, head, field)					\
	for((var) = LIST_FIRST(head);					\
	    (var)!= LIST_END(head);					\
	    (var) = LIST_NEXT(var, field))

/*
* List functions.
*/
#define	LIST_INIT(head) do {						\
	LIST_FIRST(head) = LIST_END(head);				\
    } while (0)


#define LIST_ELEMENT_VALID(elm, field) ((elm)->field.Prev && *(elm)->field.Prev ? 1 : 0)

#define LIST_INSERT_AFTER(listelm, elm, field) do {			\
	if (((elm)->field.Next = (listelm)->field.Next) != NULL)	\
		(listelm)->field.Next->field.Prev =		\
		    &(elm)->field.Next;				\
	(listelm)->field.Next = (elm);				\
	(elm)->field.Prev = &(listelm)->field.Next;		\
} while (0)

#define	LIST_INSERT_BEFORE(listelm, elm, field) do {			\
	(elm)->field.Prev = (listelm)->field.Prev;		\
	(elm)->field.Next = (listelm);				\
	*(listelm)->field.Prev = (elm);				\
	(listelm)->field.Prev = &(elm)->field.Next;		\
} while (0)

#define LIST_INSERT_HEAD(head, elm, field) do {				\
	if (((elm)->field.Next = (head)->First) != NULL)		\
		(head)->First->field.Prev = &(elm)->field.Next;\
	(head)->First = (elm);					\
	(elm)->field.Prev = &(head)->First;			\
} while (0)

#define LIST_REMOVE(elm, field) do {					\
	if ((elm)->field.Next != NULL)				\
		(elm)->field.Next->field.Prev =			\
		    (elm)->field.Prev;				\
	*(elm)->field.Prev = (elm)->field.Next;			\
    (elm)->field.Prev = (elm)->field.Next = NULL;       \
} while (0)

#define LIST_REPLACE(elm, elm2, field) do {				\
	if (((elm2)->field.Next = (elm)->field.Next) != NULL)	\
		(elm2)->field.Next->field.Prev =			\
		    &(elm2)->field.Next;				\
	(elm2)->field.Prev = (elm)->field.Prev;			\
	*(elm2)->field.Prev = (elm2);				\
} while (0)


#endif