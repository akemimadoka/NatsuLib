#pragma once

#include "natConfig.h"
#include "natMisc.h"
#include <atomic>

#if NATSULIB_USE_TAGGED_POINTER
#include <memory>
#endif

namespace NatsuLib::Concurrent
{
	namespace Detail
	{
#if NATSULIB_USE_TAGGED_POINTER
		// T 需要是对齐大于 1 且是 2 的倍数的对象，这样才有可利用的位，在特定的系统上可能有额外未利用的位，暂时不考虑
		template <typename T, std::size_t Alignment = alignof(T)>
		class ModCountedPointer
		{
		public:
			static_assert(Alignment > 1 && !(Alignment & (Alignment - 1)), "Tagged pointer required alignment of T should be bigger than 1 and be power of 2.");

			static constexpr std::uintptr_t PointerMask = std::numeric_limits<std::uintptr_t>::max() << (Alignment - 1);
			static constexpr std::uintptr_t ModCountMask = ~PointerMask;

			constexpr ModCountedPointer() = default;

			ModCountedPointer(T* pointer, std::uintptr_t modCount)
			{
#if NATSULIB_RESPECT_GC_SUPPORT
				std::declare_reachable(pointer);
#endif
				const auto pointerValue = reinterpret_cast<std::uintptr_t>(pointer);
				assert(!(pointerValue & ModCountMask));
				m_Pointer = pointerValue | (modCount & ModCountMask);
			}

			~ModCountedPointer()
			{
				std::undeclare_reachable(GetPointer());
			}


			T* GetPointer() const noexcept
			{
				return reinterpret_cast<T*>(m_Pointer & PointerMask);
			}

			void SetPointer(T* pointer)
			{
#if NATSULIB_RESPECT_GC_SUPPORT
				std::declare_reachable(pointer);
#endif
				const auto pointerValue = reinterpret_cast<std::uintptr_t>(pointer);
				assert(!(pointerValue & ModCountMask));
				const auto oldPointerValue = GetPointer();
				m_Pointer = pointerValue | GetModCount();
#if NATSULIB_RESPECT_GC_SUPPORT
				std::undeclare_reachable(oldPointerValue);
#endif
			}

			constexpr std::uintptr_t GetModCount() const noexcept
			{
				return m_Pointer & ModCountMask;
			}

			constexpr void SetModCount(std::uintptr_t modCount) noexcept
			{
				m_Pointer = GetPointer() | (modCount & ModCountMask);
			}

			constexpr explicit operator bool() const noexcept
			{
				return static_cast<bool>(GetPointer());
			}

			constexpr T* operator->() const noexcept
			{
				return GetPointer();
			}

			constexpr T& operator*() const noexcept
			{
				return *GetPointer();
			}

		private:
			std::uintptr_t m_Pointer;
		};
#else
		template <typename T, typename TagType = unsigned>
		class ModCountedPointer
		{
		public:
			constexpr ModCountedPointer() = default;
			constexpr ModCountedPointer(T* pointer, TagType tag)
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
#endif
		template <typename T>
		struct ForwardNode
		{
			T Data;
			ForwardNode* Next;

			template <typename... Args>
			constexpr explicit ForwardNode(ForwardNode* next, Args&&... args)
				: Data(std::forward<Args>(args)...), Next(next)
			{
			}
		};

		template <typename T>
		struct QueueNode
		{
			UncheckedLazyInit<T> Data;
			std::atomic<ModCountedPointer<QueueNode>> Next;

			constexpr explicit QueueNode(QueueNode* next)
				: Next{ next }
			{
			}

			template <typename... Args>
			constexpr explicit QueueNode(QueueNode* next, Args&&... args)
				: Data{ std::in_place, std::forward<Args>(args)... }, Next{ next }
			{
			}

			constexpr bool IsDummyNode() const noexcept
			{
				return !Next;
			}
		};
	}

	template <typename T, typename Allocator = std::allocator<T>>
	class Stack
		: CompressedPair<std::atomic<Detail::ModCountedPointer<Detail::ForwardNode<T>>>, typename std::allocator_traits<Allocator>::template rebind_alloc<Detail::ForwardNode<T>>>
	{
		using NodePtr = Detail::ModCountedPointer<Detail::ForwardNode<T>>;
		using NodeAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<Detail::ForwardNode<T>>;
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
