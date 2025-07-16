/* linked_list.h */

#ifndef LINKED_LIST_H
#define LINKED_LIST_H

template <typename T>
struct LinkedListNode
{
	LinkedListNode() = delete;
	LinkedListNode(const T &p_value);

	T value;
	LinkedListNode<T>* next = nullptr;
	LinkedListNode<T>* prev = nullptr;
};

// Double linked linked list
template <typename T>
struct LinkedList
{
	LinkedListNode<T> *head = nullptr;
	LinkedListNode<T> *tail = nullptr;

	const T &pop_head();
	const T &pop_tail();
	void prepend(const T &p_data);
	void append(const T &p_data);
};

#endif // LINKED_LIST_H
