/* linked_list.cpp */

#include "linked_list.h"

template <typename T>
LinkedListNode<T>::LinkedListNode(const T &p_value)
	: value(p_value)
{}

template <typename T>
const T &LinkedList<T>::pop_head()
{
	const T &r_value = head->value;
	head = head->next;
	return r_value;
}

template <typename T>
const T &LinkedList<T>::pop_tail()
{
	const T &r_value = tail->value;
	tail = tail->prev;
	return r_value;
}

template <typename T>
void LinkedList<T>::append(const T &p_data)
{
	LinkedListNode<T> *new_node = new LinkedListNode<T>;
	new_node->value = p_data;

	if (tail)
	{
		tail->next = new_node;
	}

	new_node->prev = tail;
	tail = new_node;
	return;
}

template <typename T>
void LinkedList<T>::prepend(const T &p_data)
{
	LinkedListNode<T> *new_node = new LinkedListNode<T>;
	new_node->value = p_data;

	if (head)
	{
		head->prev = new_node;
	}

	new_node->next = head;
	head = new_node;
	return;
}
