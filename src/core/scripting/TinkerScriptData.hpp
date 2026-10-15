// Shared C++ values keep the original menu's tables and callback state together.
// Feature code is compiled C++; this layer does not parse or execute scripts.
#pragma once
#include <cmath>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>
#include <type_traits>
#include <initializer_list>
#include <utility>

namespace YimMenu::TinkerScript
{
	struct Table;
	struct Function;
	struct Pointer
	{
		std::uintptr_t address = 0;
		bool operator==(const Pointer&) const = default;
	};

	struct Value
	{
		using Storage = std::variant<std::monostate, bool, double, std::string,
		    std::shared_ptr<Table>, std::shared_ptr<Function>, Pointer>;
		Storage data;
		Value() = default;
		Value(bool value) :
		    data(value)
		{
		}
		template<class T>
		    requires(std::is_arithmetic_v<T> && !std::is_same_v<T, bool>)
		Value(T value) :
		    data(static_cast<double>(value))
		{
		}
		// ImGui flags and indices are numbers; converting them to bool loses their value.
		template<class T>
		    requires std::is_enum_v<T>
		Value(T value) :
		    data(static_cast<double>(static_cast<std::underlying_type_t<T>>(value)))
		{
		}
		Value(const char* value) :
		    data(value ? Storage(std::string(value)) : Storage{})
		{
		}
		Value(std::string value) :
		    data(std::move(value))
		{
		}
		Value(std::shared_ptr<Table> value) :
		    data(std::move(value))
		{
		}
		Value(std::shared_ptr<Function> value) :
		    data(std::move(value))
		{
		}
		Value(Pointer value) :
		    data(value)
		{
		}
		bool IsNil() const
		{
			return std::holds_alternative<std::monostate>(data);
		}
	};

	using Values = std::vector<Value>;
	using Key = std::variant<bool, double, std::string>;
	struct Table
	{
		std::map<Key, Value> entries;
	};
	struct Function
	{
		std::function<Values(const Values&)> call;
	};

	// Captured cells outlive the frame that created a UI or game callback.
	inline auto MakeCell(Value value = {})
	{
		return std::make_shared<Value>(std::move(value));
	}
	inline Value At(const Values& values, std::size_t index)
	{
		return index < values.size() ? values[index] : Value{};
	}
	inline Value First(const Values& values)
	{
		return At(values, 0);
	}
	inline Values Append(Values first, const Values& last)
	{
		first.insert(first.end(), last.begin(), last.end());
		return first;
	}
	inline Value MakeFunction(std::function<Values(const Values&)> function)
	{
		return std::make_shared<Function>(Function{std::move(function)});
	}

	bool Truthy(const Value& value);
	double Number(const Value& value);
	std::uint32_t UnsignedBits(const Value& value);
	std::int32_t Integer(const Value& value);
	std::string Text(const Value& value);
	std::uint32_t Hash(const Value& value);
	Value& Member(const Value& table, const Value& key);
	Value MakeTable(std::initializer_list<std::pair<Value, Value>> values = {});
	Values Call(const Value& function, const Values& args = {});
	Values Invoke(const Value& object, std::string method, const Values& args = {});
	Value Length(const Value& value);
	Value Add(const Value& lhs, const Value& rhs);
	Value Subtract(const Value& lhs, const Value& rhs);
	Value Multiply(const Value& lhs, const Value& rhs);
	Value Divide(const Value& lhs, const Value& rhs);
	Value Power(const Value& lhs, const Value& rhs);
	Value Modulo(const Value& lhs, const Value& rhs);
	Value Negate(const Value& value);
	Value Equal(const Value& lhs, const Value& rhs);
	Value NotEqual(const Value& lhs, const Value& rhs);
	Value Less(const Value& lhs, const Value& rhs);
	Value LessEqual(const Value& lhs, const Value& rhs);
	Value Greater(const Value& lhs, const Value& rhs);
	Value GreaterEqual(const Value& lhs, const Value& rhs);
	Value Concat(const Value& lhs, const Value& rhs);
	template<class F>
	Value And(const Value& lhs, F&& rhs)
	{
		return Truthy(lhs) ? rhs() : lhs;
	}
	template<class F>
	Value Or(const Value& lhs, F&& rhs)
	{
		return Truthy(lhs) ? lhs : rhs();
	}

	class Iterator
	{
		Value function, state, last;

	public:
		explicit Iterator(const Values& iterator) :
		    function(At(iterator, 0)),
		    state(At(iterator, 1)),
		    last(At(iterator, 2))
		{
		}
		Values Next()
		{
			auto values = Call(function, {state, last});
			last = First(values);
			return values;
		}
	};

	struct Context
	{
		std::map<std::string, Value> globals;
		std::recursive_mutex mutex;
		Value& Global(const std::string& name)
		{
			return globals[name];
		}
		void Bind(std::string library, std::string name, std::function<Values(const Values&)> callback);
	};

	void InitializeStandardLibrary(Context& context);
	void SaveNativeSettings(Context& context, const Value& top, const Value& configuration);
	Values InitializeBlips(Context& context);
	Values InitializeUnlocks(Context& context);
	Values InitializeTunables(Context& context);
	Values InitializeTattoos(Context& context);
	Values InitializeSettings(Context& context);
	Values InitializeFeatures(Context& context);
}
