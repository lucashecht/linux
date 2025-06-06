// SPDX-License-Identifier: GPL-2.0-only
/*
 * Lock-less NULL terminated single linked list
 *
 * The basic atomic operation of this list is cmpxchg on long.  On
 * architectures that don't have NMI-safe cmpxchg implementation, the
 * list can NOT be used in NMI handlers.  So code that uses the list in
 * an NMI handler should depend on CONFIG_ARCH_HAVE_NMI_SAFE_CMPXCHG.
 *
 * Copyright 2010,2011 Intel Corp.
 *   Author: Huang Ying <ying.huang@intel.com>
 */
#include <linux/kernel.h>
#include <linux/export.h>
#include <linux/llist.h>

/**
 * llist_del_first - delete the first entry of lock-less list
 * @head:	the head for your lock-less list
 *
 * If list is empty, return NULL, otherwise, return the first entry
 * deleted, this is the newest added one.
 *
 * Only one llist_del_first user can be used simultaneously with
 * multiple llist_add users without lock.  Because otherwise
 * llist_del_first, llist_add, llist_add (or llist_del_all, llist_add,
 * llist_add) sequence in another user may change @head->first->next,
 * but keep @head->first.  If multiple consumers are needed, please
 * use llist_del_all or use lock between consumers.
 */
struct llist_node *llist_del_first(struct llist_head *head)
{
	struct llist_node *entry, *next;

	entry = smp_load_acquire(&head->first);
	do {
		if (entry == NULL)
			return NULL;
		next = READ_ONCE(entry->next);
	} while (!try_cmpxchg(&head->first, &entry, next));

	return entry;
}
EXPORT_SYMBOL_GPL(llist_del_first);

/**
 * llist_del_first_this - delete given entry of lock-less list if it is first
 * @head:	the head for your lock-less list
 * @this:	a list entry.
 *
 * If head of the list is given entry, delete and return %true else
 * return %false.
 *
 * Multiple callers can safely call this concurrently with multiple
 * llist_add() callers, providing all the callers offer a different @this.
 */
bool llist_del_first_this(struct llist_head *head,
			  struct llist_node *this)
{
	struct llist_node *entry, *next;

	/* acquire ensures orderig wrt try_cmpxchg() is llist_del_first() */
	entry = smp_load_acquire(&head->first);
	do {
		if (entry != this)
			return false;
		next = READ_ONCE(entry->next);
	} while (!try_cmpxchg(&head->first, &entry, next));

	return true;
}
EXPORT_SYMBOL_GPL(llist_del_first_this);

/**
 * llist_reverse_order - reverse order of a llist chain
 * @head:	first item of the list to be reversed
 *
 * Reverse the order of a chain of llist entries and return the
 * new first entry.
 */
struct llist_node *llist_reverse_order(struct llist_node *head)
{
	struct llist_node *new_head = NULL;

	while (head) {
		struct llist_node *tmp = head;
		head = head->next;
		tmp->next = new_head;
		new_head = tmp;
	}

	return new_head;
}
EXPORT_SYMBOL_GPL(llist_reverse_order);
