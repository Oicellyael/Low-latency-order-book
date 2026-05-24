#pragma once
#include "Order.h"

// Pre-allocated pool eliminates malloc/free on hot path (order submission)
// Reduces GC pauses and improves cache locality with contiguous memory
struct OrderPool
{
	std::vector<Order> stack;       // Contiguous storage minimizes cache misses
	std::vector<Order*> free_stack; // Stack of pointers enables O(1) acquire/release
	int top = 200000;               // Top index tracks next available slot

	// Initialize 200k Order objects once at startup, not per-order
	OrderPool() : stack(200000), free_stack(200000) {
		stack.reserve(200000);
		free_stack.reserve(200000);
		for (int i = 0; i < 200000; ++i) {
			free_stack[i] = &stack[i];
		}
	}

	// O(1) allocation by popping from free_stack instead of malloc
	Order* acquire() {
		if (top == 0) return nullptr;  // Signal exhaustion instead of exception
		--top;
		Order* ptr = free_stack[top];
		// Reset state to prevent stale linked list pointers from previous orders
		ptr->prev = nullptr;
		ptr->next = nullptr;
		return ptr;
	}

	// O(1) deallocation by pushing to free_stack instead of free()
	void release(Order* ptr) {
		if (top >= 200000 || ptr == nullptr) return;  //  Prevent double-free and overflow
		free_stack[top++] = ptr;  //  Stack pattern ensures LIFO, improving cache reuse
	}
};

