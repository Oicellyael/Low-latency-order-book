#pragma once
#include "Order.h"


struct OrderPool
{
	std::vector<Order> stack;
	std::vector<Order*> free_stack;
	int top = 65536;

	OrderPool() : stack(65536), free_stack(65536) {
		stack.reserve(65536);
   		free_stack.reserve(65536);
		for (int i = 0; i < 65536; ++i) {
			free_stack[i] = &stack[i];
		}
	}

	Order* acquire() {
		if (top == 0) return nullptr;
		--top;
		Order* ptr = free_stack[top];
		ptr->prev = nullptr;
		ptr->next = nullptr;
		return ptr;
	}
	
	void release(Order* ptr) {
		if (top >= 65536 || ptr == nullptr) return; 
		free_stack[top++] = ptr;
	}
};

