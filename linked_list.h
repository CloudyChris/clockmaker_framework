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
};

// Double linked linked list
template <typename T>
struct LinkedList
{
	LinkedListNode<T> *head = nullptr;

	const T &pop_head();
	void prepend(const T &p_data);
};

template <typename T>
struct DoubleLinkedListNode
{
	DoubleLinkedListNode() = delete;
	DoubleLinkedListNode(const T &p_value);

	T value;
	DoubleLinkedListNode<T>* next = nullptr;
	DoubleLinkedListNode<T>* prev = nullptr;
};

// Double linked linked list
template <typename T>
struct DoubleLinkedList
{
	DoubleLinkedListNode<T> *head = nullptr;
	DoubleLinkedListNode<T> *tail = nullptr;

	const T &pop_head();
	const T &pop_tail();
	void prepend(const T &p_data);
	void append(const T &p_data);
};

#endif // LINKED_LIST_H
