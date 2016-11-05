#pragma once
#include "natConfig.h"
#include <type_traits>

namespace NatsuLib
{
	template <typename Concept, typename Enable = void>
	struct Models
		: std::false_type
	{
	};

	template <typename Concept, typename... Ts>
	struct Models<Concept(Ts...), std::void_t<decltype(std::declval<Concept>().requires_(std::declval<Ts>()...))>>
		: std::true_type
	{
	};
}

#define REQUIRES(OriginalReturnType, ...) std::enable_if_t<(__VA_ARGS__), OriginalReturnType>

#define HasMemberTrait(membername) template <typename T, typename Test = void>\
struct HasMemberNamed##membername : std::false_type {};\
template <typename T>\
struct HasMemberNamed##membername<T, std::void_t<decltype(T::membername)>> : std::true_type {}
