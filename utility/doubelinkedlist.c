#include <stdlib.h>
#include "node_list.h"

node head = {-1, NULL, NULL};
node tail = {-1, NULL, NULL};

int main(){
  (&head)->next=&tail;
  (&tail)->prev=&head;
  node * n= (node *)malloc(sizeof(node));
  n->pid=100;
  insert_node(&tail, n);
  print_list(&head);
  remove_node(&head, &tail);
  print_list(&head);
}
