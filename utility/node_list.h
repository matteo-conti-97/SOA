#include <stdio.h>
typedef struct _node{
        int pid;
        struct _node * next;
        struct _node * prev;
} node;

void insert_node(node *tail, node *new_node);
void remove_node(node *head, node* tail);
void print_list(node* head);

void insert_node(node *tail, node * new_node){
        new_node->next = tail;
        new_node->prev = tail->prev;
        tail->prev->next = new_node;
        tail->prev = new_node;
}

void remove_node(node *head, node* tail){
        if(head->next == tail){
                printf("Remove failed, the queue is empty\n");
                return;
        }
        head->next->next->prev = head;
        head->next = head->next->next;
}

void print_list(node* head) {
    node* current = head;

    while (current != NULL) {
        printf("%d -> ", current->pid);
        current = current->next;
    }

    printf("NULL\n");
}
