/*
RCU List (RCUL)
This homework deals with the C-based implementation of an RCU (Read Copy Update) list offering the following API:

int rcu_list_insert(rcu_list *l, long key) to insert into the RCU list a given key (an integer value);
int rcu_list_search(rcu_list *l, long key) to check if the key is currently present within an element of the RCU list;
int rcu_list_remove(rcu_list *l, long key) to remove a record containing the target key from the RCU list.
Atomic memory operations via gcc bult-in API like, e.g., __sync_fetch_and_add(...), can be used as building blocks. Each function should return zero upon success.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


struct rcu_node{
    int data;
    struct rcu_node *next;
};

int init_list(struct rcu_node **head);
int insert(struct rcu_node **head, int data);
int print(struct rcu_node *head);
int rcu_list_search(struct rcu_node *head, long key);

struct rcu_node *head = NULL;
int pre_count = 0;
int post_count = 0;
bool list_lock = false;

int init_list(struct rcu_node **head){
    for(int i = 1; i <= 10; i++){
        insert(head, i);
    }
    return 0;
}

int insert(struct rcu_node **head, int data){
    struct rcu_node *new_node = (struct rcu_node *)malloc(sizeof(struct rcu_node));
    if(new_node == NULL){
        printf("Error creating a new node.\n");
        exit(0);
    }
    memset(new_node, 0, sizeof(struct rcu_node));
    new_node->data = data;
    new_node->next = *head;
    *head = new_node;
    return 0;
}

int print(struct rcu_node *head){
    struct rcu_node *ptr = head;
    printf("[ ");
    while(ptr != NULL){
        if(ptr->next == NULL)
            printf("%d", ptr->data);
        else
            printf("%d, ", ptr->data);
        ptr = ptr->next;
    }
    printf(" ]\n");
    return 0;
}

int rcu_list_insert(struct rcu_node **head, long key){
    bool lock = false;
    //Try to get lock with compare and exchange
    while(!lock){
        //__atomic_compare_exchange_n (type *ptr, type *expected, type desired, bool weak, int success_memorder, int failure_memorder)
        __atomic_compare_exchange_n(&list_lock, &lock, true, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
        asm("mfence":::);
    }
    //printf("Lock acquired\n");

    //Swap counter poiners
    __atomic_exchange (&pre_count, &post_count, &post_count, __ATOMIC_SEQ_CST);
    asm("mfence":::);
    //printf("Swapped counters\n");

    //Create new node and init value
    struct rcu_node *new_node = (struct rcu_node *)malloc(sizeof(struct rcu_node));
    if(new_node == NULL){
        printf("Error creating a new node.\n");
        //Remove lock from list
        __atomic_exchange_n(&list_lock, false, __ATOMIC_SEQ_CST);
        asm("mfence":::);
        return 1;
    }
    memset(new_node, 0, sizeof(struct rcu_node));
    new_node->data = key;
    new_node->next = *head;

    //printf("New node created\n");

    //Wait grace period to update
    while(pre_count>0);
    //printf("Grace period passed\n");
    __atomic_exchange_n(head, new_node, __ATOMIC_SEQ_CST);
    asm("mfence":::);
    //Remove lock from list
    __atomic_exchange_n(&list_lock, false, __ATOMIC_SEQ_CST);
    asm("mfence":::);
    return 0;
}

//Se i lettori trovano il lock vanno ad aggiornare il post_count altrimenti il pre_count
int rcu_list_search(struct rcu_node *head, long key){
    bool lock = false;
    //If list is locked someone is updating it, so i get the updated list else i ensure to get a consistent one i end in someone grace period
    __atomic_load_n(&lock, __ATOMIC_SEQ_CST);  
    if(lock){
        __atomic_fetch_add(&post_count, 1, __ATOMIC_SEQ_CST);
        asm("mfence":::);
    }
    else{
        __atomic_fetch_add(&pre_count, 1, __ATOMIC_SEQ_CST);
        asm("mfence":::);
    }

    struct rcu_node *ptr = head;
    while(ptr != NULL){
        if(ptr->data == key){
            return 0;
        }
        ptr = ptr->next;
    }
    return 1;

}

int main(){
    head = (struct rcu_node *)malloc(sizeof(struct rcu_node));
    if(head == NULL){
        printf("Error creating a new node.\n");
        exit(0);
    }
    memset(head, 0, sizeof(struct rcu_node));
    head->data=0;
    head->next=NULL;
    init_list(&head);
    print(head);
    rcu_list_insert(&head, 11);
    print(head);
    int src = rcu_list_search(head, 12);
    printf("Search result: %d\n", src);
    return 0;
}