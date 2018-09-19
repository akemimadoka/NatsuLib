#pragma once

#include "natConfig.h"
#include "natMisc.h"
#include <atomic>

namespace NatsuLib::Concurrent
{
	namespace Detail
	{
		template <typename T, typename TagType = unsigned>
		class TaggedPointer
		{
		public:
			constexpr TaggedPointer() = default;
			constexpr TaggedPointer(T* pointer, TagType tag)
				: m_Pointer{ pointer }, m_Tag{ tag }
			{
			}

			constexpr T* GetPointer() const noexcept
			{
				return m_Pointer;
			}

			constexpr void SetPointer(T* pointer) noexcept
			{
				m_Pointer = pointer;
			}

			constexpr TagType GetTag() const noexcept
			{
				return m_Tag;
			}

			constexpr void SetTag(TagType tag) noexcept
			{
				m_Tag = tag;
			}

			constexpr explicit operator bool() const noexcept
			{
				return static_cast<bool>(m_Pointer);
			}

			constexpr T* operator->() const noexcept
			{
				return m_Pointer;
			}

			constexpr T& operator*() const noexcept
			{
				return *m_Pointer;
			}

		private:
			T* m_Pointer;
			TagType m_Tag;
		};

		template <typename T>
		struct StackNode
		{
			T Data;
			StackNode* Next;

			template <typename... Args>
			explicit StackNode(StackNode* next, Args&&... args)
				: Data(std::forward<Args>(args)...), Next(next)
			{
			}
		};
	}

	template <typename T, typename Allocator = std::allocator<T>>
	class Stack
		: CompressedPair<std::atomic<Detail::TaggedPointer<Detail::StackNode<T>>>, typename std::allocator_traits<Allocator>::template rebind_alloc<Detail::StackNode<T>>>
	{
		using NodePtr = Detail::TaggedPointer<Detail::StackNode<T>>;
		using NodeAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<Detail::StackNode<T>>;
		using BasePair = CompressedPair<std::atomic<NodePtr>, NodeAllocator>;

	public:
		explicit Stack(Allocator const& allocator = Allocator())
			: BasePair{ NodePtr{ nullptr, 0 }, allocator }
		{
		}

		Stack(Stack const&) = delete;
		Stack& operator=(Stack const&) = delete;

		~Stack()
		{
			auto head = BasePair::GetFirst().load(std::memory_order_relaxed).GetPointer();
			while (head)
			{
				const auto current = head;
				head = current->Next;
				std::allocator_traits<NodeAllocator>::destroy(BasePair::GetSecond(), current);
				std::allocator_traits<NodeAllocator>::deallocate(BasePair::GetSecond(), current, 1);
			}
		}

		bool IsLockFree() const noexcept
		{
			return BasePair::GetFirst().is_lock_free();
		}

		template <typename... Args>
		void Push(Args&&... args)
		{
			const auto node = std::allocator_traits<NodeAllocator>::allocate(BasePair::GetSecond(), 1);
			std::allocator_traits<NodeAllocator>::construct(BasePair::GetSecond(), node, nullptr, std::forward<Args>(args)...);

			auto head = BasePair::GetFirst().load(std::memory_order_relaxed);
			NodePtr newHead;
			do
			{
				node->Next = head.GetPointer();
				newHead.SetPointer(node);
				newHead.SetTag(head.GetTag() + 1);
			} while (!BasePair::GetFirst().compare_exchange_weak(head, newHead, std::memory_order_release, std::memory_order_relaxed));
		}

		template <typename... Args>
		void UnsynchronizedPush(Args&&... args)
		{
			const auto head = BasePair::GetFirst().load(std::memory_order_relaxed);
			const auto node = std::allocator_traits<NodeAllocator>::allocate(BasePair::GetSecond(), 1);
			std::allocator_traits<NodeAllocator>::construct(BasePair::GetSecond(), node, head.GetPointer(), std::forward<Args>(args)...);
			BasePair::GetFirst().store(NodePtr{ node, head.GetTag() + 1 });
		}

		void Pop()
		{
			auto head = BasePair::GetFirst().load(std::memory_order_relaxed);
			NodePtr newHead;
			do
			{
				if (!head)
				{
					return;
				}
				newHead.SetPointer(head->Next);
				newHead.SetTag(head.GetTag() + 1);
			} while (!BasePair::GetFirst().compare_exchange_weak(head, newHead, std::memory_order_release, std::memory_order_relaxed));
			std::allocator_traits<NodeAllocator>::destroy(BasePair::GetSecond(), head.GetPointer());
			std::allocator_traits<NodeAllocator>::deallocate(BasePair::GetSecond(), head.GetPointer(), 1);
		}

		void UnsynchronizedPop()
		{
			const auto head = BasePair::GetFirst().load(std::memory_order_relaxed);
			BasePair::GetFirst().store(NodePtr{ head->Next, head.GetTag() + 1 });
			std::allocator_traits<NodeAllocator>::destroy(BasePair::GetSecond(), head.GetPointer());
			std::allocator_traits<NodeAllocator>::deallocate(BasePair::GetSecond(), head.GetPointer(), 1);
		}

		[[carries_dependency]] bool IsEmpty() const noexcept
		{
			return static_cast<bool>(BasePair::GetFirst().load(std::memory_order_consume));
		}

		[[carries_dependency]] T& GetTop() noexcept
		{
			return BasePair::GetFirst().load(std::memory_order_consume)->Data;
		}

		[[carries_dependency]] T const& GetTop() const noexcept
		{
			return BasePair::GetFirst().load(std::memory_order_consume)->Data;
		}
	};
}
