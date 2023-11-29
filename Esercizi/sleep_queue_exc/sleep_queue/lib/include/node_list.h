typedef struct _node{
        struct task_struct *task;
        int pid;
        int awake;
        struct _node * next;
        struct _node * prev;
} node;

void insert_node(node *tail, node *new_node);
void remove_node(int tpid, node *head);

void insert_node(node *tail, node * new_node){
        new_node->next = tail;
        new_node->prev = tail->prev;
        tail->prev->next = new_node;
        tail->prev = new_node;
}

void remove_node(int tpid, node *head){
        node* curr = head;
        while (curr->pid != tpid)
                curr = curr->next;
        
        head->next->next->prev = head;
        head->next = head->next->next;
}