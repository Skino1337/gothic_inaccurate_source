#ifndef ZQUEUE_H
#define ZQUEUE_H

template <class T>
class zQUEUE {
public:

    struct zTNode {
        T data;
        zTNode* next;
        zTNode* prev;
    };

    zQUEUE()
    {
        first = new zTNode;
        first->next = first;
        first->prev = first;
        last = first;
    }

    void Push(T data)
    {
        if (!freeNodes) {
            for (int i = 0; i < 11; ++i) {
                zTNode* node = new zTNode;
                node->data = NULL;
                node->next = freeNodes;
                node->prev = NULL;
                freeNodes = node;
            }
        }
        zTNode* node = freeNodes;
        freeNodes = node->next;
        node->data = data;
        node->next = last;
        node->prev = last->prev;
        last->prev->next = node;
        last->prev = node;
        if (first == last)
            first = node;
    }

    T Pop()
    {
        if (first == last)
            return NULL;

        zTNode* node = first;
        T data = node->data;
        first = node->next;
        first->prev = node->prev;
        node->next = freeNodes;
        freeNodes = node;
        return data;
    }

    zTNode* first;
    zTNode* last;
    static zTNode* freeNodes;
};

template <class T>
typename zQUEUE<T>::zTNode* zQUEUE<T>::freeNodes = NULL;

#endif
