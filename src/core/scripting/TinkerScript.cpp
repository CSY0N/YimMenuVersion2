// Native TinkerScripts feature logic and menu integration.
// Copyright (C) 2026 ImagineNothing; GPL-3.0-or-later.
#include "common.hpp"
#include "TinkerScriptData.hpp"

// Shared values retain table keys, multiple results and mutable callback state.
// Table, numeric and callback operations shared by the compiled TinkerScripts features.
#include "TinkerScriptData.hpp"
#include <algorithm>
#include <iomanip>
#include <random>
#include <regex>
#include <sstream>
#include <cstdio>

namespace YimMenu::TinkerScript
{
	static Key GetKey(const Value& value)
	{
		if (auto key = std::get_if<std::string>(&value.data))
			return *key;
		if (auto key = std::get_if<double>(&value.data))
			return *key;
		if (auto key = std::get_if<bool>(&value.data))
			return *key;
		throw std::runtime_error("Invalid TinkerScripts table key");
	}

	bool Truthy(const Value& value)
	{
		if (value.IsNil())
			return false;
		if (auto flag = std::get_if<bool>(&value.data))
			return *flag;
		return true;
	}

	double Number(const Value& value)
	{
		if (auto number = std::get_if<double>(&value.data))
			return *number;
		if (auto flag = std::get_if<bool>(&value.data))
			return *flag ? 1 : 0;
		if (auto string = std::get_if<std::string>(&value.data))
			return std::stod(*string);
		throw std::runtime_error("Expected a numeric TinkerScripts value");
	}

	// Wrap through an integer first so negative masks never rely on a floating-point cast to unsigned.
	std::uint32_t UnsignedBits(const Value& value)
	{
		return static_cast<std::uint32_t>(static_cast<std::int64_t>(Number(value)));
	}

	std::int32_t Integer(const Value& value)
	{
		return static_cast<std::int32_t>(UnsignedBits(value));
	}

	std::string Text(const Value& value)
	{
		if (auto string = std::get_if<std::string>(&value.data))
			return *string;
		if (auto flag = std::get_if<bool>(&value.data))
			return *flag ? "true" : "false";
		if (auto number = std::get_if<double>(&value.data))
		{
			std::ostringstream out;
			out << std::setprecision(14) << *number;
			return out.str();
		}
		if (value.IsNil())
			return "nil";
		throw std::runtime_error("Expected text in TinkerScripts");
	}

	Value& Member(const Value& value, const Value& key)
	{
		auto table = std::get_if<std::shared_ptr<Table>>(&value.data);
		if (!table)
			throw std::runtime_error("Expected a TinkerScripts table");
		return (*table)->entries[GetKey(key)];
	}

	Value MakeTable(std::initializer_list<std::pair<Value, Value>> values)
	{
		Value table = std::make_shared<Table>();
		for (const auto& [key, value] : values)
			Member(table, key) = value;
		return table;
	}

	Values Call(const Value& value, const Values& args)
	{
		auto function = std::get_if<std::shared_ptr<Function>>(&value.data);
		if (!function)
			throw std::runtime_error("TinkerScripts callback is not registered");
		return (*function)->call(args);
	}

	static Values StringOperation(std::string_view method, const Values& args);

	Values Invoke(const Value& object, std::string method, const Values& args)
	{
		Values all{object};
		all.insert(all.end(), args.begin(), args.end());
		if (std::holds_alternative<std::string>(object.data))
			return StringOperation(method, all);
		return Call(Member(object, method), all);
	}

	Value Length(const Value& value)
	{
		if (auto string = std::get_if<std::string>(&value.data))
			return string->size();
		auto table = std::get_if<std::shared_ptr<Table>>(&value.data);
		if (!table)
			throw std::runtime_error("TinkerScripts length requires a table or string");
		int length = 0;
		while (true)
		{
			auto entry = (*table)->entries.find(Key(static_cast<double>(length + 1)));
			if (entry == (*table)->entries.end() || entry->second.IsNil())
				break;
			++length;
		}
		return length;
	}
	Value Add(const Value& a, const Value& b)
	{
		return Number(a) + Number(b);
	}
	Value Subtract(const Value& a, const Value& b)
	{
		return Number(a) - Number(b);
	}
	Value Multiply(const Value& a, const Value& b)
	{
		return Number(a) * Number(b);
	}
	Value Divide(const Value& a, const Value& b)
	{
		return Number(a) / Number(b);
	}
	Value Power(const Value& a, const Value& b)
	{
		return std::pow(Number(a), Number(b));
	}
	Value Modulo(const Value& a, const Value& b)
	{
		return Number(a) - std::floor(Number(a) / Number(b)) * Number(b);
	}
	Value Negate(const Value& a)
	{
		return -Number(a);
	}
	Value Equal(const Value& a, const Value& b)
	{
		return a.data == b.data;
	}
	Value NotEqual(const Value& a, const Value& b)
	{
		return a.data != b.data;
	}
	Value Less(const Value& a, const Value& b)
	{
		return a.data.index() == 3 && b.data.index() == 3 ? Text(a) < Text(b) : Number(a) < Number(b);
	}
	Value LessEqual(const Value& a, const Value& b)
	{
		return Truthy(Less(a, b)) || Truthy(Equal(a, b));
	}
	Value Greater(const Value& a, const Value& b)
	{
		return Less(b, a);
	}
	Value GreaterEqual(const Value& a, const Value& b)
	{
		return LessEqual(b, a);
	}
	Value Concat(const Value& a, const Value& b)
	{
		return Text(a) + Text(b);
	}

	void Context::Bind(std::string library, std::string name, std::function<Values(const Values&)> callback)
	{
		auto& value = Global(library);
		if (value.IsNil())
			value = MakeTable();
		Member(value, name) = MakeFunction(std::move(callback));
	}

	// Preserve the release's one-based substring positions and tattoo import patterns.
	static std::string Pattern(std::string_view input)
	{
		std::string output;
		bool insideClass = false;
		for (std::size_t i = 0; i < input.size(); ++i)
		{
			char c = input[i];
			if (c == '%' && i + 1 < input.size())
			{
				c = input[++i];
				switch (c)
				{
				case 'd': output += insideClass ? "0-9" : "[0-9]"; break;
				case 's': output += insideClass ? "\\s" : "[\\s]"; break;
				case 'w': output += insideClass ? "A-Za-z0-9" : "[A-Za-z0-9]"; break;
				case 'a': output += insideClass ? "A-Za-z" : "[A-Za-z]"; break;
				case 'l': output += insideClass ? "a-z" : "[a-z]"; break;
				case 'u': output += insideClass ? "A-Z" : "[A-Z]"; break;
				case 'x': output += insideClass ? "0-9A-Fa-f" : "[0-9A-Fa-f]"; break;
				default:
					output += '\\';
					output += c;
					break;
				}
			}
			else if (c == '[')
			{
				insideClass = true;
				output += c;
			}
			else if (c == ']')
			{
				insideClass = false;
				output += c;
			}
			else if (c == '-' && !insideClass)
				output += "*?";
			else if (c == '\\')
				output += "\\\\";
			else if ((c == '{' || c == '}' || c == '|') && !insideClass)
			{
				output += '\\';
				output += c;
			}
			else
				output += c;
		}
		return output;
	}

	static Values Captures(const std::smatch& match)
	{
		Values values;
		if (match.size() == 1)
			return {match.str()};
		for (std::size_t i = 1; i < match.size(); ++i)
			values.push_back(match[i].str());
		return values;
	}

	static Values StringOperation(std::string_view method, const Values& args)
	{
		auto text = Text(At(args, 0));
		if (method == "lower")
		{
			for (auto& c : text)
				c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
			return {text};
		}
		if (method == "sub")
		{
			int size = static_cast<int>(text.size());
			auto position = [size](double value) {
				int pos = static_cast<int>(value);
				return pos < 0 ? size + pos + 1 : pos;
			};
			int first = std::max(1, position(Number(At(args, 1))));
			int last = args.size() > 2 ? std::min(size, position(Number(args[2]))) : size;
			return {last < first ? std::string{} : text.substr(first - 1, last - first + 1)};
		}
		if (method == "format")
		{
			std::string result;
			std::size_t argument = 1;
			for (std::size_t i = 0; i < text.size(); ++i)
			{
				if (text[i] != '%')
				{
					result += text[i];
					continue;
				}
				if (i + 1 < text.size() && text[i + 1] == '%')
				{
					result += '%';
					++i;
					continue;
				}
				auto start = i++;
				while (i < text.size() && std::string_view("-+ #0.123456789").find(text[i]) != std::string_view::npos)
					++i;
				if (i >= text.size())
					throw std::runtime_error("Incomplete TinkerScripts format");
				char type = text[i];
				auto spec = text.substr(start, i - start + 1);
				const auto& value = At(args, argument++);
				auto format = [&](auto item) {
					int count = std::snprintf(nullptr, 0, spec.c_str(), item);
					if (count < 0 || count > 1048576)
						throw std::runtime_error("Invalid TinkerScripts format");
					std::vector<char> buffer(count + 1);
					std::snprintf(buffer.data(), buffer.size(), spec.c_str(), item);
					result.append(buffer.data(), count);
				};
				switch (type)
				{
				case 's':
				{
					auto string = Text(value);
					format(string.c_str());
					break;
				}
				case 'd':
				case 'i':
				case 'c': format(Integer(value)); break;
				case 'x':
				case 'X':
				case 'u':
				case 'o': format(static_cast<unsigned int>(Number(value))); break;
				case 'f':
				case 'e':
				case 'E':
				case 'g':
				case 'G': format(Number(value)); break;
				default: throw std::runtime_error("Unsupported TinkerScripts format");
				}
			}
			return {result};
		}
		auto expression = std::make_shared<std::regex>(Pattern(Text(At(args, 1))));
		if (method == "gmatch")
		{
			auto saved = std::make_shared<std::string>(std::move(text));
			auto position = std::make_shared<std::size_t>(0);
			return {MakeFunction([saved, expression, position](const Values&) -> Values {
				if (*position > saved->size())
					return {};
				auto remaining = saved->substr(*position);
				std::smatch match;
				if (!std::regex_search(remaining, match, *expression))
					return {};
				*position += match.position() + std::max<std::size_t>(1, match.length());
				return Captures(match);
			})};
		}
		int begin = args.size() > 2 ? Integer(args[2]) : 1;
		if (begin < 0)
			begin = static_cast<int>(text.size()) + begin + 1;
		auto offset = static_cast<std::size_t>(std::max(1, begin) - 1);
		if (offset > text.size())
			return {};
		auto remaining = text.substr(offset);
		std::smatch match;
		if (!std::regex_search(remaining, match, *expression))
			return {};
		if (method == "match")
			return Captures(match);
		if (method == "find")
			return Append({static_cast<int>(offset + match.position() + 1), static_cast<int>(offset + match.position() + match.length())}, match.size() > 1 ? Captures(match) : Values{});
		throw std::runtime_error("Unknown TinkerScripts string operation");
	}

	// The iteration helpers retain one-based indices and stop at the first missing element.
	void InitializeStandardLibrary(Context& context)
	{
		for (auto name : {"lower", "sub", "format", "match", "find", "gmatch"})
			context.Bind("string", name, [name](const Values& args) {
				return StringOperation(name, args);
			});
		context.Global("ipairs") = MakeFunction([](const Values& args) -> Values {
			return {MakeFunction([](const Values& step) -> Values {
				        auto next = Number(At(step, 1)) + 1;
				        auto value = Member(At(step, 0), next);
				        return value.IsNil() ? Values{} : Values{next, value};
			        }),
			    At(args, 0),
			    0};
		});
		auto next = MakeFunction([](const Values& args) -> Values {
			auto table = std::get<std::shared_ptr<Table>>(At(args, 0).data);
			auto iterator = At(args, 1).IsNil() ? table->entries.begin() : table->entries.upper_bound(GetKey(At(args, 1)));
			while (iterator != table->entries.end() && iterator->second.IsNil())
				++iterator;
			if (iterator == table->entries.end())
				return {};
			return {std::visit([](const auto& key) {
				        return Value(key);
			        },
			            iterator->first),
			    iterator->second};
		});
		context.Global("next") = next;
		context.Global("pairs") = MakeFunction([next](const Values& args) -> Values {
			return {next, At(args, 0), Value{}};
		});
		context.Global("type") = MakeFunction([](const Values& args) -> Values {
			const char* names[] = {"nil", "boolean", "number", "string", "table", "function", "userdata"};
			return {names[At(args, 0).data.index()]};
		});
		context.Global("tostring") = MakeFunction([](const Values& args) -> Values {
			return {Text(At(args, 0))};
		});
		context.Global("tonumber") = MakeFunction([](const Values& args) -> Values {
			try
			{
				return {Number(At(args, 0))};
			}
			catch (...)
			{
				return {};
			}
		});
		context.Bind("math", "floor", [](const Values& a) -> Values {
			return {std::floor(Number(At(a, 0)))};
		});
		context.Bind("math", "sin", [](const Values& a) -> Values {
			return {std::sin(Number(At(a, 0)))};
		});
		context.Bind("math", "min", [](const Values& a) -> Values {
			auto result = Number(At(a, 0));
			for (auto& value : a)
				result = std::min(result, Number(value));
			return {result};
		});
		context.Bind("math", "random", [](const Values& a) -> Values {
			static std::mt19937 engine(std::random_device{}());
			if (a.empty())
				return {std::generate_canonical<double, 53>(engine)};
			int lower = a.size() == 1 ? 1 : Integer(a[0]);
			int upper = Integer(a.back());
			return {std::uniform_int_distribution<int>(lower, upper)(engine)};
		});
		context.Bind("bit", "band", [](const Values& a) -> Values {
			std::uint32_t result = ~0u;
			for (auto& v : a)
				result &= UnsignedBits(v);
			return {static_cast<std::int32_t>(result)};
		});
		context.Bind("bit", "bor", [](const Values& a) -> Values {
			std::uint32_t result = 0;
			for (auto& v : a)
				result |= UnsignedBits(v);
			return {static_cast<std::int32_t>(result)};
		});
		context.Bind("bit", "bnot", [](const Values& a) -> Values {
			return {static_cast<std::int32_t>(~UnsignedBits(At(a, 0)))};
		});
		context.Bind("bit", "lshift", [](const Values& a) -> Values {
			return {static_cast<std::int32_t>(UnsignedBits(At(a, 0)) << (Integer(At(a, 1)) & 31))};
		});
		context.Bind("table", "insert", [](const Values& a) -> Values {
			int length = Integer(Length(At(a, 0)));
			int position = a.size() == 2 ? length + 1 : Integer(a[1]);
			for (int i = length; i >= position; --i)
				Member(a[0], i + 1) = Member(a[0], i);
			Member(a[0], position) = a.back();
			return {};
		});
		context.Bind("table", "remove", [](const Values& a) -> Values {
			int length = Integer(Length(a[0]));
			int position = a.size() == 1 ? length : Integer(a[1]);
			auto result = Member(a[0], position);
			for (int i = position; i < length; ++i)
				Member(a[0], i) = Member(a[0], i + 1);
			Member(a[0], length) = {};
			return {result};
		});
		context.Bind("table", "concat", [](const Values& a) -> Values {
			std::string result, separator = a.size() > 1 ? Text(a[1]) : "";
			int first = a.size() > 2 ? Integer(a[2]) : 1;
			int last = a.size() > 3 ? Integer(a[3]) : Integer(Length(a[0]));
			for (int i = first; i <= last; ++i)
			{
				if (i > first)
					result += separator;
				result += Text(Member(a[0], i));
			}
			return {result};
		});
	}
}

// Widgets use the same defaults and return values as the original submenu.
// Render bindings retain the original widget defaults and returned values.
#include "common.hpp"
#include "TinkerScriptData.hpp"
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
namespace YimMenu::TinkerScript
{
	static const char* ArgText(const Values& args, int index)
	{
		return std::get<std::string>(args.at(index - 1).data).c_str();
	}
	static double ArgNumber(const Values& args, int index, double fallback = 0)
	{
		auto value = At(args, index - 1);
		return value.IsNil() ? fallback : Number(value);
	}
	static int ArgType(const Values& args, int index)
	{
		return static_cast<int>(At(args, index - 1).data.index());
	}
	constexpr int ValueBool = 1, ValueString = 3;
	static void ReadFloatArray(const Values& args, int index, float* output, int count)
	{
		for (int i = 0; i < count; ++i)
			output[i] = static_cast<float>(Number(Member(args.at(index - 1), i + 1)));
	}
	static Values RenderAlignTextToFramePadding(const Values& args)
	{
		Values values;
		ImGui::AlignTextToFramePadding();
		return values;
	}
	static Values RenderBegin(const Values& args)
	{
		Values values;
		const char* name = ArgText(args, 1);
		int top = args.size();

		if (top >= 2 && ArgType(args, 2) == ValueBool)
		{
			bool open = Truthy(At(args, 2 - 1));
			int flags = (int)ArgNumber(args, 3, 0);
			bool draw = ImGui::Begin(name, &open, flags);
			values.push_back(open);
			values.push_back(draw);
			return values;
		}

		int flags = (int)ArgNumber(args, 3, 0);
		bool draw = ImGui::Begin(name, nullptr, flags);
		values.push_back(draw);
		return values;
	}
	static Values RenderBeginChild(const Values& args)
	{
		Values values;
		const char* name = ArgText(args, 1);
		float sx = (float)ArgNumber(args, 2, 0.0);
		float sy = (float)ArgNumber(args, 3, 0.0);
		bool border = At(args, 4 - 1).IsNil() ? false : Truthy(At(args, 4 - 1));
		int flags = (int)ArgNumber(args, 5, 0);
		ImGuiChildFlags child_flags = border ? ImGuiChildFlags_Borders : ImGuiChildFlags_None;
		values.push_back(ImGui::BeginChild(name, ImVec2(sx, sy), child_flags, flags));
		return values;
	}
	static Values RenderBeginCombo(const Values& args)
	{
		Values values;
		const char* label = ArgText(args, 1);
		const char* preview = ArgText(args, 2);
		int flags = (int)ArgNumber(args, 3, 0);
		values.push_back(ImGui::BeginCombo(label, preview, flags));
		return values;
	}
	static Values RenderBeginDisabled(const Values& args)
	{
		Values values;
		bool disabled = At(args, 1 - 1).IsNil() ? true : Truthy(At(args, 1 - 1));
		ImGui::BeginDisabled(disabled);
		return values;
	}
	static Values RenderBeginGroup(const Values& args)
	{
		Values values;
		ImGui::BeginGroup();
		return values;
	}
	static Values RenderBeginListBox(const Values& args)
	{
		Values values;
		const char* label = ArgText(args, 1);
		int top = args.size();
		if (top >= 3)
		{
			float sx = (float)ArgNumber(args, 2);
			float sy = (float)ArgNumber(args, 3);
			values.push_back(ImGui::BeginListBox(label, ImVec2(sx, sy)));
			return values;
		}

		int items_count = (int)ArgNumber(args, 2);
		int height_in_items = (int)ArgNumber(args, 3, -1);
		if (height_in_items < 0)
			height_in_items = items_count < 7 ? items_count : 7;
		float h = ImGui::GetTextLineHeightWithSpacing() * (height_in_items + 0.25f) + ImGui::GetStyle().FramePadding.y * 2.0f;
		values.push_back(ImGui::BeginListBox(label, ImVec2(0.0f, h)));
		return values;
	}
	static Values RenderBeginPopup(const Values& args)
	{
		Values values;
		const char* id = ArgText(args, 1);
		int flags = (int)ArgNumber(args, 2, 0);
		values.push_back(ImGui::BeginPopup(id, flags));
		return values;
	}
	static Values RenderBeginPopupModal(const Values& args)
	{
		Values values;
		const char* name = ArgText(args, 1);
		int top = args.size();
		if (top >= 2 && ArgType(args, 2) == ValueBool)
		{
			bool open = Truthy(At(args, 2 - 1));
			int flags = (int)ArgNumber(args, 3, 0);
			bool draw = ImGui::BeginPopupModal(name, &open, flags);
			values.push_back(open);
			values.push_back(draw);
			return values;
		}
		int flags = (int)ArgNumber(args, 3, 0);
		values.push_back(ImGui::BeginPopupModal(name, nullptr, flags));
		return values;
	}
	static Values RenderBeginTabBar(const Values& args)
	{
		Values values;
		const char* id = ArgText(args, 1);
		int flags = (int)ArgNumber(args, 2, 0);
		values.push_back(ImGui::BeginTabBar(id, flags));
		return values;
	}
	static Values RenderBeginTabItem(const Values& args)
	{
		Values values;
		const char* label = ArgText(args, 1);
		if (ArgType(args, 2) == ValueBool)
		{
			bool open = Truthy(At(args, 2 - 1));
			int flags = (int)ArgNumber(args, 3, 0);
			bool selected = ImGui::BeginTabItem(label, &open, flags);
			values.push_back(open);
			values.push_back(selected);
			return values;
		}
		values.push_back(ImGui::BeginTabItem(label));
		return values;
	}
	static Values RenderButton(const Values& args)
	{
		Values values;
		const char* label = ArgText(args, 1);
		float sx = (float)ArgNumber(args, 2, 0.0);
		float sy = (float)ArgNumber(args, 3, 0.0);
		values.push_back(ImGui::Button(label, ImVec2(sx, sy)));
		return values;
	}
	static Values RenderCalcTextSize(const Values& args)
	{
		Values values;
		const char* text = ArgText(args, 1);
		const char* tend = At(args, 2 - 1).IsNil() ? nullptr : ArgText(args, 2);
		bool hide_after_dh = At(args, 3 - 1).IsNil() ? false : Truthy(At(args, 3 - 1));
		float wrap = (float)ArgNumber(args, 4, -1.0);
		ImVec2 sz = ImGui::CalcTextSize(text, tend, hide_after_dh, wrap);
		values.push_back(sz.x);
		values.push_back(sz.y);
		return values;
	}
	static Values RenderCheckbox(const Values& args)
	{
		Values values;
		const char* label = ArgText(args, 1);
		bool value = Truthy(At(args, 2 - 1));
		bool pressed = ImGui::Checkbox(label, &value);
		values.push_back(value);
		values.push_back(pressed);
		return values;
	}
	static Values RenderCloseCurrentPopup(const Values& args)
	{
		Values values;
		ImGui::CloseCurrentPopup();
		return values;
	}
	static Values RenderColorButton(const Values& args)
	{
		Values values;
		const char* desc_id = ArgText(args, 1);
		float c[4];
		ReadFloatArray(args, 2, c, 4);
		int flags = (int)ArgNumber(args, 3, 0);
		float sx = (float)ArgNumber(args, 4, 0.0);
		float sy = (float)ArgNumber(args, 5, 0.0);
		values.push_back(ImGui::ColorButton(desc_id, ImVec4(c[0], c[1], c[2], c[3]), flags, ImVec2(sx, sy)));
		return values;
	}
	static Values RenderDummy(const Values& args)
	{
		Values values;
		float x = (float)ArgNumber(args, 1);
		float y = (float)ArgNumber(args, 2);
		ImGui::Dummy(ImVec2(x, y));
		return values;
	}
	static Values RenderEnd(const Values& args)
	{
		Values values;
		ImGui::End();
		return values;
	}
	static Values RenderEndChild(const Values& args)
	{
		Values values;
		ImGui::EndChild();
		return values;
	}
	static Values RenderEndDisabled(const Values& args)
	{
		Values values;
		ImGui::EndDisabled();
		return values;
	}
	static Values RenderEndGroup(const Values& args)
	{
		Values values;
		ImGui::EndGroup();
		return values;
	}
	static Values RenderEndListBox(const Values& args)
	{
		Values values;
		ImGui::EndListBox();
		return values;
	}
	static Values RenderEndPopup(const Values& args)
	{
		Values values;
		ImGui::EndPopup();
		return values;
	}
	static Values RenderEndTabBar(const Values& args)
	{
		Values values;
		ImGui::EndTabBar();
		return values;
	}
	static Values RenderEndTabItem(const Values& args)
	{
		Values values;
		ImGui::EndTabItem();
		return values;
	}
	static Values RenderGetClipboardText(const Values& args)
	{
		Values values;
		values.push_back(ImGui::GetClipboardText());
		return values;
	}
	static Values RenderGetDisplaySize(const Values& args)
	{
		Values values;
		ImVec2 sz = ImGui::GetIO().DisplaySize;
		values.push_back(sz.x);
		values.push_back(sz.y);
		return values;
	}
	static Values RenderGetWindowWidth(const Values& args)
	{
		Values values;
		values.push_back(ImGui::GetWindowWidth());
		return values;
	}
	static Values RenderIndent(const Values& args)
	{
		Values values;
		ImGui::Indent((float)ArgNumber(args, 1, 0.0));
		return values;
	}
	static Values RenderInputInt(const Values& args)
	{
		Values values;
		const char* label = ArgText(args, 1);
		int value = (int)ArgNumber(args, 2);
		int step = (int)ArgNumber(args, 3, 1);
		int step_fast = (int)ArgNumber(args, 4, 100);
		int flags = (int)ArgNumber(args, 5, 0);
		bool used = ImGui::InputInt(label, &value, step, step_fast, flags);
		values.push_back(value);
		values.push_back(used);
		return values;
	}
	static Values RenderInputText(const Values& args)
	{
		Values values;
		const char* label = ArgText(args, 1);
		std::string buf = ArgText(args, 2);
		int flags = (int)ArgNumber(args, 3, 0);
		bool changed = ImGui::InputText(label, &buf, flags);
		values.push_back(buf.c_str());
		values.push_back(changed);
		return values;
	}
	static Values RenderInputTextMultiline(const Values& args)
	{
		Values values;
		const char* label = ArgText(args, 1);
		std::string buf = ArgText(args, 2);
		float sx = (float)ArgNumber(args, 3, 0.0);
		float sy = (float)ArgNumber(args, 4, 0.0);
		int flags = (int)ArgNumber(args, 5, 0);
		bool changed = ImGui::InputTextMultiline(label, &buf, ImVec2(sx, sy), flags);
		values.push_back(buf.c_str());
		values.push_back(changed);
		return values;
	}
	static Values RenderInputTextWithHint(const Values& args)
	{
		Values values;
		const char* label = ArgText(args, 1);
		const char* hint = ArgText(args, 2);
		std::string buf = ArgText(args, 3);
		int flags = (int)ArgNumber(args, 4, 0);
		bool changed = ImGui::InputTextWithHint(label, hint, &buf, flags);
		values.push_back(buf.c_str());
		values.push_back(changed);
		return values;
	}
	static Values RenderIsItemActive(const Values& args)
	{
		Values values;
		values.push_back(ImGui::IsItemActive());
		return values;
	}
	static Values RenderIsItemHovered(const Values& args)
	{
		Values values;
		int flags = (int)ArgNumber(args, 1, 0);
		values.push_back(ImGui::IsItemHovered(flags));
		return values;
	}
	static Values RenderIsPopupOpen(const Values& args)
	{
		Values values;
		const char* id = ArgText(args, 1);
		int flags = (int)ArgNumber(args, 2, 0);
		values.push_back(ImGui::IsPopupOpen(id, flags));
		return values;
	}
	static Values RenderIsWindowHovered(const Values& args)
	{
		Values values;
		int flags = (int)ArgNumber(args, 1, 0);
		values.push_back(ImGui::IsWindowHovered(flags));
		return values;
	}
	static Values RenderNewLine(const Values& args)
	{
		Values values;
		ImGui::NewLine();
		return values;
	}
	static Values RenderOpenPopup(const Values& args)
	{
		Values values;
		const char* id = ArgText(args, 1);
		int flags = (int)ArgNumber(args, 2, 0);
		ImGui::OpenPopup(id, flags);
		return values;
	}
	static Values RenderPopItemWidth(const Values& args)
	{
		Values values;
		ImGui::PopItemWidth();
		return values;
	}
	static Values RenderPopStyleColor(const Values& args)
	{
		Values values;
		int count = (int)ArgNumber(args, 1, 1);
		ImGui::PopStyleColor(count);
		return values;
	}
	static Values RenderPopStyleVar(const Values& args)
	{
		Values values;
		int count = (int)ArgNumber(args, 1, 1);
		ImGui::PopStyleVar(count);
		return values;
	}
	static Values RenderPushItemWidth(const Values& args)
	{
		Values values;
		ImGui::PushItemWidth((float)ArgNumber(args, 1));
		return values;
	}
	static Values RenderPushStyleColor(const Values& args)
	{
		Values values;
		int idx = (int)ArgNumber(args, 1);
		float r = (float)ArgNumber(args, 2);
		float g = (float)ArgNumber(args, 3);
		float b = (float)ArgNumber(args, 4);
		float a = (float)ArgNumber(args, 5);
		ImGui::PushStyleColor(idx, ImVec4(r, g, b, a));
		return values;
	}
	static Values RenderPushStyleVar(const Values& args)
	{
		Values values;
		int idx = (int)ArgNumber(args, 1);
		if (args.size() >= 3)
		{
			float x = (float)ArgNumber(args, 2);
			float y = (float)ArgNumber(args, 3);
			ImGui::PushStyleVar(idx, ImVec2(x, y));
		}
		else
		{
			ImGui::PushStyleVar(idx, (float)ArgNumber(args, 2));
		}
		return values;
	}
	static Values RenderSameLine(const Values& args)
	{
		Values values;
		float off = (float)ArgNumber(args, 1, 0.0);
		float sp = (float)ArgNumber(args, 2, -1.0);
		ImGui::SameLine(off, sp);
		return values;
	}
	static Values RenderSelectable(const Values& args)
	{
		Values values;
		const char* label = ArgText(args, 1);
		bool selected = At(args, 2 - 1).IsNil() ? false : Truthy(At(args, 2 - 1));
		int flags = (int)ArgNumber(args, 3, 0);
		float sx = (float)ArgNumber(args, 4, 0.0);
		float sy = (float)ArgNumber(args, 5, 0.0);
		bool clicked = ImGui::Selectable(label, &selected, flags, ImVec2(sx, sy));
		(void)clicked;
		values.push_back(selected);
		return values;
	}
	static Values RenderSeparator(const Values& args)
	{
		Values values;
		ImGui::Separator();
		return values;
	}
	static Values RenderSeparatorText(const Values& args)
	{
		Values values;
		ImGui::SeparatorText(ArgText(args, 1));
		return values;
	}
	static Values RenderSetClipboardText(const Values& args)
	{
		Values values;
		ImGui::SetClipboardText(ArgText(args, 1));
		return values;
	}
	static Values RenderSetCursorPosX(const Values& args)
	{
		Values values;
		ImGui::SetCursorPosX((float)ArgNumber(args, 1));
		return values;
	}
	static Values RenderSetNextItemWidth(const Values& args)
	{
		Values values;
		ImGui::SetNextItemWidth((float)ArgNumber(args, 1));
		return values;
	}
	static Values RenderSetNextWindowPos(const Values& args)
	{
		Values values;
		float x = (float)ArgNumber(args, 1);
		float y = (float)ArgNumber(args, 2);
		int cond = (int)ArgNumber(args, 3, 0);
		float px = (float)ArgNumber(args, 4, 0.0);
		float py = (float)ArgNumber(args, 5, 0.0);
		ImGui::SetNextWindowPos(ImVec2(x, y), cond, ImVec2(px, py));
		return values;
	}
	static Values RenderSetNextWindowSize(const Values& args)
	{
		Values values;
		float x = (float)ArgNumber(args, 1);
		float y = (float)ArgNumber(args, 2);
		int cond = (int)ArgNumber(args, 3, 0);
		ImGui::SetNextWindowSize(ImVec2(x, y), cond);
		return values;
	}
	static Values RenderSetTooltip(const Values& args)
	{
		Values values;
		ImGui::SetTooltip("%s", ArgText(args, 1));
		return values;
	}
	static Values RenderSetWindowFontScale(const Values& args)
	{
		Values values;
		ImGui::SetWindowFontScale((float)ArgNumber(args, 1));
		return values;
	}
	static Values RenderSliderFloat(const Values& args)
	{
		Values values;
		const char* label = ArgText(args, 1);
		float value = (float)ArgNumber(args, 2);
		float vmin = (float)ArgNumber(args, 3);
		float vmax = (float)ArgNumber(args, 4);
		const char* fmt = At(args, 5 - 1).IsNil() ? "%.3f" : ArgText(args, 5);
		bool used = ImGui::SliderFloat(label, &value, vmin, vmax, fmt, 0);
		values.push_back(value);
		values.push_back(used);
		return values;
	}
	static Values RenderSliderInt(const Values& args)
	{
		Values values;
		const char* label = ArgText(args, 1);
		int value = (int)ArgNumber(args, 2);
		int vmin = (int)ArgNumber(args, 3);
		int vmax = (int)ArgNumber(args, 4);
		const char* fmt = At(args, 5 - 1).IsNil() ? "%d" : ArgText(args, 5);
		bool used = ImGui::SliderInt(label, &value, vmin, vmax, fmt, 0);
		values.push_back(value);
		values.push_back(used);
		return values;
	}
	static Values RenderSpacing(const Values& args)
	{
		Values values;
		ImGui::Spacing();
		return values;
	}
	static Values RenderText(const Values& args)
	{
		Values values;
		ImGui::TextUnformatted(ArgText(args, 1));
		return values;
	}
	static Values RenderTextColored(const Values& args)
	{
		Values values;
		float r = (float)ArgNumber(args, 1);
		float g = (float)ArgNumber(args, 2);
		float b = (float)ArgNumber(args, 3);
		float a = (float)ArgNumber(args, 4);
		const char* text = ArgText(args, 5);
		ImGui::TextColored(ImVec4(r, g, b, a), "%s", text);
		return values;
	}
	static Values RenderTextDisabled(const Values& args)
	{
		Values values;
		ImGui::TextDisabled("%s", ArgText(args, 1));
		return values;
	}
	static Values RenderTreeNodeEx(const Values& args)
	{
		Values values;
		const char* label = ArgText(args, 1);
		int flags = (int)ArgNumber(args, 2, 0);
		if (At(args, 3 - 1).IsNil())
			values.push_back(ImGui::TreeNodeEx(label, flags));
		else
			values.push_back(ImGui::TreeNodeEx(label, flags, "%s", ArgText(args, 3)));
		return values;
	}
	static Values RenderUnindent(const Values& args)
	{
		Values values;
		ImGui::Unindent((float)ArgNumber(args, 1, 0.0));
		return values;
	}

	static Values RenderCombo(const Values& args)
	{
		int selected = Integer(args.at(1));
		bool changed;
		if (std::holds_alternative<std::string>(args.at(2).data))
			changed = ImGui::Combo(ArgText(args, 1), &selected, ArgText(args, 3), static_cast<int>(ArgNumber(args, 4, -1)));
		else
		{
			int count = Integer(args.at(3));
			std::vector<std::string> labels(count);
			std::vector<const char*> pointers(count);
			for (int i = 0; i < count; ++i)
			{
				labels[i] = Text(Member(args[2], i + 1));
				pointers[i] = labels[i].c_str();
			}
			changed = ImGui::Combo(ArgText(args, 1), &selected, pointers.data(), count, static_cast<int>(ArgNumber(args, 5, -1)));
		}
		return {selected, changed};
	}
	void BindRenderApi(Context& context)
	{
		context.Bind("ImGui", "AlignTextToFramePadding", RenderAlignTextToFramePadding);
		context.Bind("ImGui", "Begin", RenderBegin);
		context.Bind("ImGui", "BeginChild", RenderBeginChild);
		context.Bind("ImGui", "BeginCombo", RenderBeginCombo);
		context.Bind("ImGui", "BeginDisabled", RenderBeginDisabled);
		context.Bind("ImGui", "BeginGroup", RenderBeginGroup);
		context.Bind("ImGui", "BeginListBox", RenderBeginListBox);
		context.Bind("ImGui", "BeginPopup", RenderBeginPopup);
		context.Bind("ImGui", "BeginPopupModal", RenderBeginPopupModal);
		context.Bind("ImGui", "BeginTabBar", RenderBeginTabBar);
		context.Bind("ImGui", "BeginTabItem", RenderBeginTabItem);
		context.Bind("ImGui", "Button", RenderButton);
		context.Bind("ImGui", "CalcTextSize", RenderCalcTextSize);
		context.Bind("ImGui", "Checkbox", RenderCheckbox);
		context.Bind("ImGui", "CloseCurrentPopup", RenderCloseCurrentPopup);
		context.Bind("ImGui", "ColorButton", RenderColorButton);
		context.Bind("ImGui", "Combo", RenderCombo);
		context.Bind("ImGui", "Dummy", RenderDummy);
		context.Bind("ImGui", "End", RenderEnd);
		context.Bind("ImGui", "EndChild", RenderEndChild);
		context.Bind("ImGui", "EndDisabled", RenderEndDisabled);
		context.Bind("ImGui", "EndGroup", RenderEndGroup);
		context.Bind("ImGui", "EndListBox", RenderEndListBox);
		context.Bind("ImGui", "EndPopup", RenderEndPopup);
		context.Bind("ImGui", "EndTabBar", RenderEndTabBar);
		context.Bind("ImGui", "EndTabItem", RenderEndTabItem);
		context.Bind("ImGui", "GetClipboardText", RenderGetClipboardText);
		context.Bind("ImGui", "GetDisplaySize", RenderGetDisplaySize);
		context.Bind("ImGui", "GetWindowWidth", RenderGetWindowWidth);
		context.Bind("ImGui", "Indent", RenderIndent);
		context.Bind("ImGui", "InputInt", RenderInputInt);
		context.Bind("ImGui", "InputText", RenderInputText);
		context.Bind("ImGui", "InputTextMultiline", RenderInputTextMultiline);
		context.Bind("ImGui", "InputTextWithHint", RenderInputTextWithHint);
		context.Bind("ImGui", "IsItemActive", RenderIsItemActive);
		context.Bind("ImGui", "IsItemHovered", RenderIsItemHovered);
		context.Bind("ImGui", "IsPopupOpen", RenderIsPopupOpen);
		context.Bind("ImGui", "IsWindowHovered", RenderIsWindowHovered);
		context.Bind("ImGui", "NewLine", RenderNewLine);
		context.Bind("ImGui", "OpenPopup", RenderOpenPopup);
		context.Bind("ImGui", "PopItemWidth", RenderPopItemWidth);
		context.Bind("ImGui", "PopStyleColor", RenderPopStyleColor);
		context.Bind("ImGui", "PopStyleVar", RenderPopStyleVar);
		context.Bind("ImGui", "PushItemWidth", RenderPushItemWidth);
		context.Bind("ImGui", "PushStyleColor", RenderPushStyleColor);
		context.Bind("ImGui", "PushStyleVar", RenderPushStyleVar);
		context.Bind("ImGui", "SameLine", RenderSameLine);
		context.Bind("ImGui", "Selectable", RenderSelectable);
		context.Bind("ImGui", "Separator", RenderSeparator);
		context.Bind("ImGui", "SeparatorText", RenderSeparatorText);
		context.Bind("ImGui", "SetClipboardText", RenderSetClipboardText);
		context.Bind("ImGui", "SetCursorPosX", RenderSetCursorPosX);
		context.Bind("ImGui", "SetNextItemWidth", RenderSetNextItemWidth);
		context.Bind("ImGui", "SetNextWindowPos", RenderSetNextWindowPos);
		context.Bind("ImGui", "SetNextWindowSize", RenderSetNextWindowSize);
		context.Bind("ImGui", "SetTooltip", RenderSetTooltip);
		context.Bind("ImGui", "SetWindowFontScale", RenderSetWindowFontScale);
		context.Bind("ImGui", "SliderFloat", RenderSliderFloat);
		context.Bind("ImGui", "SliderInt", RenderSliderInt);
		context.Bind("ImGui", "Spacing", RenderSpacing);
		context.Bind("ImGui", "Text", RenderText);
		context.Bind("ImGui", "TextColored", RenderTextColored);
		context.Bind("ImGui", "TextDisabled", RenderTextDisabled);
		context.Bind("ImGui", "TreeNodeEx", RenderTreeNodeEx);
		context.Bind("ImGui", "Unindent", RenderUnindent);
		context.Global("ImGuiStyleVar") = MakeTable();
		Member(context.Global("ImGuiStyleVar"), "WindowTitleAlign") = ImGuiStyleVar_WindowTitleAlign;
		context.Global("ImGuiInputTextFlags") = MakeTable();
		Member(context.Global("ImGuiInputTextFlags"), "ReadOnly") = ImGuiInputTextFlags_ReadOnly;
		context.Global("ImGuiCol") = MakeTable();
		Member(context.Global("ImGuiCol"), "Button") = ImGuiCol_Button;
		Member(context.Global("ImGuiCol"), "ButtonActive") = ImGuiCol_ButtonActive;
		Member(context.Global("ImGuiCol"), "ButtonHovered") = ImGuiCol_ButtonHovered;
		context.Global("ImGuiTabBarFlags") = MakeTable();
		Member(context.Global("ImGuiTabBarFlags"), "FittingPolicyScroll") = ImGuiTabBarFlags_FittingPolicyScroll;
		Member(context.Global("ImGuiTabBarFlags"), "NoTabListScrollingButtons") = ImGuiTabBarFlags_NoTabListScrollingButtons;
		context.Global("ImGuiWindowFlags") = MakeTable();
		Member(context.Global("ImGuiWindowFlags"), "NoMove") = ImGuiWindowFlags_NoMove;
		Member(context.Global("ImGuiWindowFlags"), "NoResize") = ImGuiWindowFlags_NoResize;
	}
}
// Settings are JSON data; tattoo exports keep their original text format.
// Settings and tattoo exports are user data; no executable script files are installed.
#include "common.hpp"
#include "TinkerScriptData.hpp"
#include "core/filemgr/FileMgr.hpp"
#include <regex>

namespace YimMenu::TinkerScript
{
	void RequestCloudSave(std::string command);
	void StartCloudSaveTool();
	static std::filesystem::path DataFolder()
	{
		return FileMgr::GetProjectFolder("./TinkerScripts").Path();
	}

	// Read earlier settings as data only. Never execute the previous Lua settings file.
	static void LoadNativeSettings(Context& context)
	{
		auto path = DataFolder() / "Settings.json";
		if (std::filesystem::exists(path))
		{
			std::ifstream input(path);
			auto settings = json::parse(input);
			auto& table = context.Global("TS_Settings");
			for (auto& [name, value] : settings.items())
			{
				if (value.is_boolean())
					Member(table, name) = value.get<bool>();
				else if (value.is_number())
					Member(table, name) = value.get<double>();
			}
			return;
		}
		auto old = FileMgr::GetProjectFolder("./scripts").Path() / "TinkerScripts/include/TS_Settings.lua";
		if (!std::filesystem::exists(old))
			old = old.parent_path() / "Settings/TS_Settings.lua";
		std::ifstream input(old);
		std::string line;
		const std::regex assignment(R"(^\s*TS_Settings\.([A-Za-z_0-9]+)\s*=\s*(true|false|-?[0-9]+)\s*$)");
		while (std::getline(input, line))
		{
			std::smatch match;
			if (!std::regex_match(line, match, assignment))
				continue;
			auto value = match[2].str();
			Member(context.Global("TS_Settings"), match[1].str()) = value == "true" ? Value(true) : value == "false" ? Value(false) :
			                                                                                                           Value(std::stod(value));
		}
	}

	void SaveNativeSettings(Context& context, const Value& top, const Value& configuration)
	{
		const std::pair<const char*, const char*> entries[] = {
		    {"HGWHrestock", "HGWH_restock"},
		    {"HGWHmaxgoods", "HGWH_maxgoods"},
		    {"HGsetgood", "HG_setgood"},
		    {"HGgoodtype", "HG_goodtype"},
		    {"WHsetgood", "WH_setgood"},
		    {"WHgoodtype", "WH_goodtype"},
		    {"MCresup", "MC_resup"},
		    {"MCrestock", "MC_restock"},
		    {"NCgoods", "NC_goods"},
		    {"NoClipState", "noclip_onscreen"}};
		json settings;
		auto& table = context.Global("TS_Settings");
		for (auto [key, name] : entries)
			Member(table, key) = First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), {name})), "get_value", {}));
		Member(table, "NoClipWindowTop") = top;
		Member(table, "LoadSettings") = Member(configuration, "LoadSettings");
		Member(table, "CloseBCSonUnload") = Member(configuration, "CloseBCSonUnload");
		for (auto& [key, value] : std::get<std::shared_ptr<Table>>(table.data)->entries)
		{
			auto* name = std::get_if<std::string>(&key);
			if (!name)
				continue;
			if (auto* flag = std::get_if<bool>(&value.data))
				settings[*name] = *flag;
			else if (auto* number = std::get_if<double>(&value.data))
				settings[*name] = *number;
		}
		auto path = DataFolder() / "Settings.json";
		auto temporary = path;
		temporary += ".tmp";
		std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
		output << settings.dump(2) << '\n';
		output.close();
		if (!output)
			throw std::runtime_error("Unable to save TinkerScripts settings");
		if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
			throw std::runtime_error("Unable to replace TinkerScripts settings");
	}

	static std::filesystem::path RuntimeDataPath(const Value& value)
	{
		auto name = std::filesystem::path(Text(value)).filename();
		if (name != "NoSave.txt" && name != "Exported_TattooCombo.txt")
			throw std::runtime_error("Unknown TinkerScripts data file");
		return DataFolder() / name;
	}

	void BindCloudSaves(Context& context)
	{
		context.Global("require") = MakeFunction([&context](const Values& args) -> Values {
			if (Text(args[0]) == "TS_Settings")
				LoadNativeSettings(context);
			return {};
		});
		context.Bind("FileMgr", "GetMenuRootPath", [](const Values&) -> Values {
			return {FileMgr::GetProjectFolder("./scripts").Path().generic_string()};
		});
		context.Bind("FileMgr", "ReadFileContent", [](const Values& args) -> Values {
			auto path = RuntimeDataPath(args[0]);
			// Migrate tattoo exports only when a current saved copy does not exist.
			if (!std::filesystem::exists(path) && path.filename() == "Exported_TattooCombo.txt")
			{
				auto previous = FileMgr::GetProjectFolder("./scripts").Path() / "TinkerScripts/Exported_TattooCombo.txt";
				if (!std::filesystem::exists(previous))
					previous = previous.parent_path() / "include/HumanCanvas/Exported_TattooCombo.txt";
				if (std::filesystem::exists(previous))
					std::filesystem::copy_file(previous, path);
			}
			std::ifstream input(path, std::ios::binary);
			return {std::string(std::istreambuf_iterator<char>(input), {})};
		});
		context.Bind("FileMgr", "WriteFileContent", [](const Values& args) -> Values {
			auto path = RuntimeDataPath(args[0]);
			if (path.filename() == "NoSave.txt")
			{
				RequestCloudSave(Text(args[1]));
				return {};
			}
			std::ofstream output(path, std::ios::binary | (Truthy(At(args, 2)) ? std::ios::app : std::ios::trunc));
			output << Text(args[1]);
			if (!output)
				throw std::runtime_error("Unable to write TinkerScripts data");
			return {};
		});
		context.Bind("CloudSave", "Start", [](const Values&) -> Values {
			StartCloudSaveTool();
			return {};
		});
	}
}

// All feature callbacks below are compiled C++ functions.
// TinkerScripts feature logic, converted from ImagineNothing's v1.0.1 release.
// Copyright (C) 2026 ImagineNothing; GPL-3.0-or-later.
#include "TinkerScriptData.hpp"

namespace YimMenu::TinkerScript
{
	Values InitializeFeatures(Context& context)
	{
		Call(Member(context.Global("natives"), "load_natives"), Values{});
		auto cls = MakeCell(Value("                                            \r"));
		auto lual = MakeCell(Value("\r\033[9C]\033[94m[INFO/LuaScript]\033[m "));
		auto lual_s = MakeCell(Value(": \033[92mInitialized successfully\033[m                           "));
		// IsOnline preserves the feature callback and its captured state.
		auto IsOnline = MakeCell();
		*IsOnline = MakeFunction([=, &context](const Values& args) -> Values {
			return Values{And(And(And(First(Call(Member(context.Global("NETWORK"), "NETWORK_IS_SESSION_STARTED"), Values{})), [&] {
				return Value(!Truthy(First(Call(Member(context.Global("NETWORK"), "NETWORK_IS_IN_TRANSITION"), Values{}))));
			}),
			                      [&] {
				                      return Value(!Truthy(First(Call(Member(context.Global("STREAMING"), "IS_PLAYER_SWITCH_IN_PROGRESS"), Values{}))));
			                      }),
			    [&] {
				    return Value(!Truthy(First(Call(Member(context.Global("NETWORK"), "NETWORK_IS_ACTIVITY_SESSION"), Values{}))));
			    })};
			return {};
		});
		auto TS_BUILD = MakeCell(Value("1158.16"));
		auto values_1 = Append(Values{}, Call(Member(context.Global("util"), "get_game_version"), Values{}));
		auto OnlineVersion = MakeCell(At(values_1, 0));
		auto GameVersion = MakeCell(At(values_1, 1));
		if (Truthy(NotEqual(*GameVersion, *TS_BUILD)))
		{
			Call(Member(context.Global("notify"), "error"), Append(Values{Value("TinkeScripts - WRONG GAME BUILD")}, Call(Member(context.Global("string"), "format"), Values{Value("TinkeScripts: %s\nCurrent Game Build: %s"), *TS_BUILD, *GameVersion})));
			Call(Member(context.Global("log"), "error"), Append(Values{}, Call(Member(context.Global("string"), "format"), Values{Value("%s\r\033[18C\033[31m[ERROR/LuaScript]\033[m TinkerScripts: WRONG GAME BUILD: %s | TinkeScripts: %s %s\033[B]                %s   "), *cls, *GameVersion, *TS_BUILD, *cls, *cls})));
			return Values{};
		}
		auto TinkerPath = MakeCell(First(Call(Member(context.Global("FileMgr"), "GetMenuRootPath"), Values{})));
		Call(context.Global("require"), Values{Value("Blips")});
		Call(context.Global("require"), Values{Value("Unlocks")});
		Call(context.Global("require"), Values{Value("Tunables")});
		Call(context.Global("require"), Values{Value("FMCharTats")});
		Call(context.Global("require"), Values{Value("TS_Settings")});
		// TS_PATH_FMCharTatCombo preserves the feature callback and its captured state.
		auto TS_PATH_FMCharTatCombo = MakeCell();
		*TS_PATH_FMCharTatCombo = MakeFunction([=, &context](const Values& args) -> Values {
			return Append(Values{}, Call(Member(context.Global("FileMgr"), "ReadFileContent"), Values{Concat(*TinkerPath, Value("/TinkerScripts/include/HumanCanvas/Exported_TattooCombo.txt"))}));
			return {};
		});
		// TS_PATH_NoCloudSave preserves the feature callback and its captured state.
		auto TS_PATH_NoCloudSave = MakeCell();
		*TS_PATH_NoCloudSave = MakeFunction([=, &context](const Values& args) -> Values {
			return Append(Values{}, Call(Member(context.Global("FileMgr"), "ReadFileContent"), Values{Concat(*TinkerPath, Value("/TinkerScripts/include/Settings/BlockCloudSaves/NoSave.txt"))}));
			return {};
		});
		Call(Member(context.Global("menu"), "set_menu_icon"), Values{Value("ï®ï®")});
		auto Tinker = MakeCell(First(Call(Member(context.Global("menu"), "get_submenu"), Values{Value("TinkerScripts")})));
		auto TS_Main = MakeCell(First(Invoke(*Tinker, "add_category", Values{Value("Main")})));
		auto TS_Bizteroids = MakeCell(First(Invoke(*Tinker, "add_category", Values{Value("Biz-Teroids")})));
		auto TS_PhoneMaster = MakeCell(First(Invoke(*Tinker, "add_category", Values{Value("Phone Master")})));
		auto TS_GoonVan = MakeCell(First(Invoke(*Tinker, "add_category", Values{Value("Gun Van Halen")})));
		auto TS_RainbowVeh = MakeCell(First(Invoke(*Tinker, "add_category", Values{Value("Rainbow Vehicles")})));
		auto TS_RecoveryPlus = MakeCell(First(Invoke(*Tinker, "add_category", Values{Value("Recovery +")})));
		auto TS_SettingsPlus = MakeCell(First(Invoke(*Tinker, "add_category", Values{Value("Settings +")})));
		// Native implementation of the FindNewSession callback.
		context.Global("FindNewSession") = MakeFunction([=, &context](const Values& args) -> Values {
			Call(Member(context.Global("network"), "join_session"), Values{Member(context.Global("session_types"), "invite_only")});
			return {};
		});
		// Native implementation of the CurPlayerCoords callback.
		context.Global("CurPlayerCoords") = MakeFunction([=, &context](const Values& args) -> Values {
			context.Global("PlaCoords") = First(Call(Member(context.Global("ENTITY"), "GET_ENTITY_COORDS"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value(0)}));
			context.Global("PlaHeading") = First(Call(Member(context.Global("ENTITY"), "GET_ENTITY_HEADING"), Append(Values{}, Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{}))));
			context.Global("PlaFwdVec") = First(Call(Member(context.Global("ENTITY"), "GET_ENTITY_FORWARD_VECTOR"), Append(Values{}, Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{}))));
			return {};
		});
		// Native implementation of the scripts.start_new_script callback.
		Member(context.Global("scripts"), "start_new_script") = MakeFunction([=, &context](const Values& args) -> Values {
			auto ScriptName = MakeCell(At(args, 0));
			auto StackSize = MakeCell(At(args, 1));
			Call(Member(context.Global("script"), "run_in_callback"), Values{MakeFunction([=, &context](const Values& args) -> Values {
				if (Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{*ScriptName}))))
				{
					Call(Member(context.Global("scripts"), "run_as_script"), Values{*ScriptName, Member(context.Global("SCRIPT"), "TERMINATE_THIS_THREAD")});
				}
				Call(Member(context.Global("SCRIPT"), "REQUEST_SCRIPT"), Values{*ScriptName});
				Call(Member(context.Global("script"), "yield"), Values{Value(50)});
				if (Truthy(First(Call(Member(context.Global("SCRIPT"), "HAS_SCRIPT_LOADED"), Values{*ScriptName}))))
				{
					Call(Member(context.Global("ScriptNatives"), "START_NEW_SCRIPT"), Values{*ScriptName, *StackSize});
					Call(Member(context.Global("SCRIPT"), "SET_SCRIPT_AS_NO_LONGER_NEEDED"), Values{*ScriptName});
				}
				return {};
			})});
			return {};
		});
		auto values_2 = Append(Values{}, Call(Member(context.Global("ImGui"), "GetDisplaySize"), Values{}));
		auto screenX = MakeCell(At(values_2, 0));
		auto screenY = MakeCell(At(values_2, 1));
		auto nomoretreeplease = MakeCell(First(Call(Member(context.Global("util"), "time"), Values{})));
		auto value_INT_MAX = MakeCell(Value(2147483647));
		auto value_INT_MIN = MakeCell(Negate(Value(2147483648)));
		// CenteredText preserves the feature callback and its captured state.
		auto CenteredText = MakeCell();
		*CenteredText = MakeFunction([=, &context](const Values& args) -> Values {
			auto Text = MakeCell(At(args, 0));
			auto NewSeparator = MakeCell(At(args, 1));
			auto FakeSeparatorText = MakeCell(At(args, 2));
			if (Truthy(Value(!Truthy(*FakeSeparatorText))))
			{
				Call(Member(context.Global("ImGui"), "SetCursorPosX"), Values{Divide(Subtract(First(Call(Member(context.Global("ImGui"), "GetWindowWidth"), Values{})), First(Call(Member(context.Global("ImGui"), "CalcTextSize"), Values{*Text}))), Value(2))});
				Call(Member(context.Global("ImGui"), "TextDisabled"), Values{*Text});
				if (Truthy(*NewSeparator))
				{
					Call(Member(context.Global("ImGui"), "Separator"), Values{});
				}
			}
			else
			{
				Call(Member(context.Global("ImGui"), "SeparatorText"), Values{Value("")});
				Call(Member(context.Global("ImGui"), "SameLine"), Values{});
				Call(Member(context.Global("ImGui"), "SetCursorPosX"), Values{Divide(Subtract(First(Call(Member(context.Global("ImGui"), "GetWindowWidth"), Values{})), First(Call(Member(context.Global("ImGui"), "CalcTextSize"), Values{*Text}))), Value(2))});
				Call(Member(context.Global("ImGui"), "Text"), Values{*Text});
			}
			return {};
		});
		// CustomChild preserves the feature callback and its captured state.
		auto CustomChild = MakeCell();
		*CustomChild = MakeFunction([=, &context](const Values& args) -> Values {
			auto title = MakeCell(At(args, 0));
			auto childname = MakeCell(At(args, 1));
			auto width = MakeCell(At(args, 2));
			auto height = MakeCell(At(args, 3));
			auto flags = MakeCell(At(args, 4));
			auto func = MakeCell(At(args, 5));
			Call(Member(context.Global("ImGui"), "BeginGroup"), Values{});
			Call(Member(context.Global("ImGui"), "BeginChild"), Values{*childname, *width, *height, Add(Value(1), *flags)});
			if (Truthy(Equal(First(Call(context.Global("type"), Values{*title})), Value("string"))))
			{
				Call(*CenteredText, Values{*title, Value(false), Value(true)});
			}
			if (Truthy(Equal(First(Call(context.Global("type"), Values{*func})), Value("function"))))
			{
				Call(*func, Values{});
			}
			Call(Member(context.Global("ImGui"), "EndChild"), Values{});
			Call(Member(context.Global("ImGui"), "EndGroup"), Values{});
			return {};
		});
		// SetRageTooltip preserves the feature callback and its captured state.
		auto SetRageTooltip = MakeCell();
		*SetRageTooltip = MakeFunction([=, &context](const Values& args) -> Values {
			auto tooltip = MakeCell(At(args, 0));
			Call(Member(context.Global("script"), "run_in_callback"), Values{MakeFunction([=, &context](const Values& args) -> Values {
				Call(Member(context.Global("HUD"), "BEGIN_TEXT_COMMAND_DISPLAY_HELP"), Values{Value("STRING")});
				Call(Member(context.Global("HUD"), "ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME"), Values{*tooltip});
				Call(Member(context.Global("HUD"), "END_TEXT_COMMAND_DISPLAY_HELP"), Values{Value(0), Value(0), Value(0), Value(200)});
				return {};
			})});
			return {};
		});
		// TpToBlip preserves the feature callback and its captured state.
		auto TpToBlip = MakeCell();
		*TpToBlip = MakeFunction([=, &context](const Values& args) -> Values {
			auto KillDelPed = MakeCell(At(args, 0));
			auto Blip = MakeCell(At(args, 1));
			auto BEntity = MakeCell(At(args, 2));
			if (Truthy(*KillDelPed))
			{
				Call(Member(context.Global("script"), "run_in_callback"), Values{MakeFunction([=, &context](const Values& args) -> Values {
					{
						Iterator iterator_3(Append(Values{}, Call(context.Global("ipairs"), Append(Values{}, Call(Member(context.Global("entities"), "get_all_peds_as_handles"), Values{})))));
						for (auto row_4 = iterator_3.Next(); !row_4.empty() && !row_4[0].IsNil(); row_4 = iterator_3.Next())
						{
							auto _ = MakeCell(At(row_4, 0));
							auto MissionEnemy = MakeCell(At(row_4, 1));
							if (Truthy(First(Invoke(First(Call(context.Global("Ped"), Values{*MissionEnemy})), "is_enemy", Values{}))))
							{
								Invoke(First(Call(context.Global("Entity"), Values{*MissionEnemy})), "kill", Values{});
							}
						}
					}
					Call(Member(context.Global("script"), "yield"), Values{Value(300)});
					{
						Iterator iterator_5(Append(Values{}, Call(context.Global("ipairs"), Append(Values{}, Call(Member(context.Global("entities"), "get_all_peds_as_handles"), Values{})))));
						for (auto row_6 = iterator_5.Next(); !row_6.empty() && !row_6[0].IsNil(); row_6 = iterator_5.Next())
						{
							auto __7 = MakeCell(At(row_6, 0));
							auto MissionEnemy_8 = MakeCell(At(row_6, 1));
							if (Truthy(First(Invoke(First(Call(context.Global("Ped"), Values{*MissionEnemy_8})), "is_dead", Values{}))))
							{
								Invoke(First(Call(context.Global("Entity"), Values{*MissionEnemy_8})), "delete", Values{});
							}
						}
					}
					return {};
				})});
			}
			auto BlipID = MakeCell(First(Call(Member(context.Global("HUD"), "GET_FIRST_BLIP_INFO_ID"), Values{*Blip})));
			if (Truthy(*BEntity))
			{
				auto BlipH = MakeCell(First(Call(Member(context.Global("HUD"), "GET_BLIP_INFO_ID_ENTITY_INDEX"), Values{*BlipID})));
				if (Truthy(First(Invoke(First(Call(context.Global("Entity"), Values{*BlipH})), "is_valid", Values{}))))
				{
					auto BEntityPos = MakeCell(First(Invoke(First(Call(context.Global("Entity"), Values{*BlipH})), "get_position", Values{})));
					Call(Member(context.Global("PED"), "SET_PED_COORDS_KEEP_VEHICLE"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Member(*BEntityPos, "x"), Member(*BEntityPos, "y"), Add(Member(*BEntityPos, "z"), Value(0.5))});
				}
			}
			else
			{
				if (Truthy(First(Call(Member(context.Global("HUD"), "DOES_BLIP_EXIST"), Values{*BlipID}))))
				{
					auto blip_coords = MakeCell(First(Call(Member(context.Global("HUD"), "GET_BLIP_INFO_ID_COORD"), Values{*BlipID})));
					Call(Member(context.Global("PED"), "SET_PED_COORDS_KEEP_VEHICLE"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Member(*blip_coords, "x"), Member(*blip_coords, "y"), Add(Member(*blip_coords, "z"), Value(0.5))});
				}
			}
			return {};
		});
		// TpToEntity preserves the feature callback and its captured state.
		auto TpToEntity = MakeCell();
		*TpToEntity = MakeFunction([=, &context](const Values& args) -> Values {
			auto EntityModel = MakeCell(At(args, 0));
			Call(Member(context.Global("script"), "run_in_callback"), Values{MakeFunction([=, &context](const Values& args) -> Values {
				{
					Iterator iterator_9(Append(Values{}, Call(context.Global("ipairs"), Append(Values{}, Call(Member(context.Global("entities"), "get_all_objects_as_handles"), Values{})))));
					for (auto row_10 = iterator_9.Next(); !row_10.empty() && !row_10[0].IsNil(); row_10 = iterator_9.Next())
					{
						auto __11 = MakeCell(At(row_10, 0));
						auto h = MakeCell(At(row_10, 1));
						if (Truthy(Equal(First(Invoke(First(Call(context.Global("Entity"), Values{*h})), "get_model", Values{})), *EntityModel)))
						{
							auto EntityCoords = MakeCell(First(Invoke(First(Call(context.Global("Entity"), Values{*h})), "get_position", Values{})));
							Call(Member(context.Global("PED"), "SET_PED_COORDS_KEEP_VEHICLE"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Member(*EntityCoords, "x"), Member(*EntityCoords, "y"), Add(Member(*EntityCoords, "z"), Value(0.5))});
							break;
						}
					}
				}
				return {};
			})});
			return {};
		});
		auto LocationCoords = MakeCell(MakeTable({{Value(1), MakeTable({{"LocName", Value("Franklin's Aunt's House")}, {"LocDesc", Value("Teleport to Franklin's Aunt's House")}, {"LocCoords", MakeTable({{Value(1), Negate(Value(17.95))}, {Value(2), Negate(Value(1440.61))}, {Value(3), Value(31.1)}})}})}, {Value(2), MakeTable({{"LocName", Value("Franklin's House")}, {"LocDesc", Value("Teleport to Franklin's House")}, {"LocCoords", MakeTable({{Value(1), Value(7.61)}, {Value(2), Value(538.3)}, {Value(3), Value(176.03)}})}})}, {Value(3), MakeTable({{"LocName", Value("Michael's House")}, {"LocDesc", Value("Teleport to Michael's House")}, {"LocCoords", MakeTable({{Value(1), Negate(Value(815.44))}, {Value(2), Value(178.77)}, {Value(3), Value(72.15)}})}})}, {Value(4), MakeTable({{"LocName", Value("Trevor's Trailer")}, {"LocDesc", Value("Teleport to Trevor's Trailer")}, {"LocCoords", MakeTable({{Value(1), Value(1972.9)}, {Value(2), Value(3816.28)}, {Value(3), Value(33.42)}})}})}, {Value(5), MakeTable({{"LocName", Value("Match Winner Room")}, {"LocDesc", Value("Teleport to Match \"WINNER\" Room")}, {"LocCoords", MakeTable({{Value(1), Value(414.4)}, {Value(2), Negate(Value(977.6))}, {Value(3), Negate(Value(100.0042))}})}})}, {Value(6), MakeTable({{"LocName", Value("Character Creation Room")}, {"LocDesc", Value("Teleport to Character Creation/Selection Room")}, {"LocCoords", MakeTable({{Value(1), Value(409.13)}, {Value(2), Negate(Value(999.44))}, {Value(3), Negate(Value(99.0))}})}})}, {Value(7), MakeTable({{"LocName", Value("Height Messure")}, {"LocDesc", Value("Teleport to Character Height Messure")}, {"LocCoords", MakeTable({{Value(1), Value(402.91)}, {Value(2), Negate(Value(996.9))}, {Value(3), Negate(Value(99.0))}})}})}, {Value(8), MakeTable({{"LocName", Value("Omega's Garage")}, {"LocDesc", Value("Teleport to Omega's Garage")}, {"LocCoords", MakeTable({{Value(1), Value(2331.344)}, {Value(2), Value(2574.073)}, {Value(3), Value(46.68137)}})}})}, {Value(9), MakeTable({{"LocName", Value("Lester's House")}, {"LocDesc", Value("Teleport to Lester's House")}, {"LocCoords", MakeTable({{Value(1), Value(1273.9)}, {Value(2), Negate(Value(1719.305))}, {Value(3), Value(54.77141)}})}})}, {Value(10), MakeTable({{"LocName", Value("Torture Room")}, {"LocDesc", Value("Teleport to Torture Room")}, {"LocCoords", MakeTable({{Value(1), Value(136.5146)}, {Value(2), Negate(Value(2203.149))}, {Value(3), Value(7.30914)}})}})}, {Value(11), MakeTable({{"LocName", Value("Split-Sides Comedy Club")}, {"LocDesc", Value("Teleport to Split-Sides Comedy Club")}, {"LocCoords", MakeTable({{Value(1), Value(382.71)}, {Value(2), Negate(Value(1001.45))}, {Value(3), Negate(Value(99.0))}})}})}, {Value(12), MakeTable({{"LocName", Value("Motel")}, {"LocDesc", Value("Teleport to Motel")}, {"LocCoords", MakeTable({{Value(1), Value(152.2605)}, {Value(2), Negate(Value(1004.471))}, {Value(3), Negate(Value(98.99999))}})}})}, {Value(13), MakeTable({{"LocName", Value("FIB Building")}, {"LocDesc", Value("Teleport to FIB Building")}, {"LocCoords", MakeTable({{Value(1), Value(134.5835)}, {Value(2), Negate(Value(749.339))}, {Value(3), Value(258.152)}})}})}, {Value(14), MakeTable({{"LocName", Value("IAA Office")}, {"LocDesc", Value("Teleport to IAA Office")}, {"LocCoords", MakeTable({{Value(1), Value(117.22)}, {Value(2), Negate(Value(620.938))}, {Value(3), Value(206.1398)}})}})}}));
		// FastRespawn preserves the feature callback and its captured state.
		auto FastRespawn = MakeCell();
		*FastRespawn = MakeFunction([=, &context](const Values& args) -> Values {
			auto logresp = MakeCell(MakeTable({{Value(1), Value("Wasted Screen Skipped!")}, {Value(2), Value("Player Respawned a thousand times!")}, {Value(3), Value("Fast-Respawn!")}, {Value(4), Value("mp_forcerespawnpla... oops, wrong game.")}, {Value(5), Value("Had to make the script longer somehow.")}, {Value(6), Value("I Need Healing!")}, {Value(7), Value("I should be playing Warframe...")}, {Value(8), Value("Wanna play RnG?")}, {Value(9), Value("I will not die; not yet, amigo!")}, {Value(10), Value("Kifflom!")}}));
			if (Truthy(Equal(First(Invoke(First(Invoke(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Value(2658296)})), "at", Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_ID"), Values{})), Value(468)})), "at", Values{Value(236)})), "get_int", Values{})), Negate(Value(1)))))
			{
				Call(context.Global("CurPlayerCoords"), Values{});
				Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Value(2635562), Value(2924))})), "set_int", Values{Value(1)});
				Call(Member(context.Global("script"), "yield"), Values{Value(1500)});
				Call(Member(context.Global("PED"), "SET_PED_COORDS_KEEP_VEHICLE"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Member(context.Global("PlaCoords"), "x"), Member(context.Global("PlaCoords"), "y"), Member(context.Global("PlaCoords"), "z")});
				Call(Member(context.Global("ENTITY"), "SET_ENTITY_HEADING"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), context.Global("PlaHeading")});
			}
			return {};
		});
		// FastReload preserves the feature callback and its captured state.
		auto FastReload = MakeCell();
		*FastReload = MakeFunction([=, &context](const Values& args) -> Values {
			if (Truthy(First(Call(Member(context.Global("PAD"), "IS_CONTROL_JUST_PRESSED"), Values{Value(0), Value(24)}))))
			{
				auto cur_weap = MakeCell(First(Call(Member(context.Global("WEAPON"), "GET_SELECTED_PED_WEAPON"), Append(Values{}, Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})))));
				Call(Member(context.Global("script"), "yield"), Values{Value(20)});
				Call(Member(context.Global("WEAPON"), "SET_CURRENT_PED_WEAPON"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value("WEAPON_STICKYBOMB"), Value(0)});
				Call(Member(context.Global("script"), "yield"), Values{Value(200)});
				Call(Member(context.Global("WEAPON"), "SET_CURRENT_PED_WEAPON"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), *cur_weap, Value(1)});
			}
			return {};
		});
		// ClearWorld preserves the feature callback and its captured state.
		auto ClearWorld = MakeCell();
		*ClearWorld = MakeFunction([=, &context](const Values& args) -> Values {
			auto CwToggle = MakeCell(At(args, 0));
			Invoke(First(Call(context.Global("ScriptGlobal"), Values{Value(1690468)})), "set_int", Values{*CwToggle});
			Invoke(First(Call(context.Global("ScriptGlobal"), Values{Value(1690469)})), "set_int", Values{*CwToggle});
			Invoke(First(Call(context.Global("ScriptGlobal"), Values{Value(1690470)})), "set_int", Values{*CwToggle});
			return {};
		});
		// GlobalSetBit preserves the feature callback and its captured state.
		auto GlobalSetBit = MakeCell();
		*GlobalSetBit = MakeFunction([=, &context](const Values& args) -> Values {
			auto global = MakeCell(At(args, 0));
			auto bitonoff = MakeCell(At(args, 1));
			auto bits = MakeCell(At(args, 2));
			auto bitvalue = MakeCell(First(Invoke(*global, "get_int", Values{})));
			auto togglebit = MakeCell(*bitonoff);
			if (Truthy(Equal(*togglebit, Value(1))))
			{
				*bitvalue = First(Call(Member(context.Global("bit"), "bor"), Append(Values{*bitvalue}, Call(Member(context.Global("bit"), "lshift"), Values{Value(1), *bits}))));
			}
			else if (Truthy(Equal(*togglebit, Value(0))))
			{
				*bitvalue = First(Call(Member(context.Global("bit"), "band"), Append(Values{*bitvalue}, Call(Member(context.Global("bit"), "bnot"), Append(Values{}, Call(Member(context.Global("bit"), "lshift"), Values{Value(1), *bits}))))));
			}
			Invoke(*global, "set_int", Values{*bitvalue});
			return {};
		});
		// LocalSetBit preserves the feature callback and its captured state.
		auto LocalSetBit = MakeCell();
		*LocalSetBit = MakeFunction([=, &context](const Values& args) -> Values {
			auto sLocal = MakeCell(At(args, 0));
			auto bitonoff_12 = MakeCell(At(args, 1));
			auto bits_13 = MakeCell(At(args, 2));
			auto bitvalue_14 = MakeCell(First(Invoke(*sLocal, "get_int", Values{})));
			auto togglebit_15 = MakeCell(*bitonoff_12);
			if (Truthy(Equal(*togglebit_15, Value(1))))
			{
				*bitvalue_14 = First(Call(Member(context.Global("bit"), "bor"), Append(Values{*bitvalue_14}, Call(Member(context.Global("bit"), "lshift"), Values{Value(1), *bits_13}))));
			}
			else if (Truthy(Equal(*togglebit_15, Value(0))))
			{
				*bitvalue_14 = First(Call(Member(context.Global("bit"), "band"), Append(Values{*bitvalue_14}, Call(Member(context.Global("bit"), "bnot"), Append(Values{}, Call(Member(context.Global("bit"), "lshift"), Values{Value(1), *bits_13}))))));
			}
			Invoke(*sLocal, "set_int", Values{*bitvalue_14});
			return {};
		});
		// RagdollPlayer preserves the feature callback and its captured state.
		auto RagdollPlayer = MakeCell();
		*RagdollPlayer = MakeFunction([=, &context](const Values& args) -> Values {
			if (Truthy(First(Call(Member(context.Global("PAD"), "IS_CONTROL_PRESSED"), Values{Value(0), Value(19)}))))
			{
				Call(Member(context.Global("PED"), "SET_PED_TO_RAGDOLL"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value(2000), Value(2000), Value(0), Value(0), Value(0), Value(0)});
			}
			return {};
		});
		Call(Member(context.Global("commandmgr"), "add_looped_command"), Values{Value("fastrespawn"), Value("Fast Respawn"), Value("Skips the WASTED screen."), *FastRespawn, MakeFunction([=, &context](const Values& args) -> Values {
			                                                                        Invoke(First(Call(context.Global("ScriptGlobal"), Values{Value(2709139)})), "set_int", Values{Value(1)});
			                                                                        Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Rejack"), Value("Fast Respawn Enabled!"), Value(3000)});
			                                                                        return {};
		                                                                        }),
		                                                                     MakeFunction([=, &context](const Values& args) -> Values {
			                                                                     Invoke(First(Call(context.Global("ScriptGlobal"), Values{Value(2709139)})), "set_int", Values{Value(0)});
			                                                                     Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Rejack"), Value("Fast Respawn Disabled."), Value(3000)});
			                                                                     return {};
		                                                                     })});
		Call(Member(context.Global("commandmgr"), "add_looped_command"), Values{Value("fastreload"), Value("Fast Reload"), Value("Similar to Fast Reload macros. (FIRST PERSON ONLY)"), *FastReload, MakeFunction([=, &context](const Values& args) -> Values {
			                                                                        Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Fast Reload"), Value("Fast Reload Enabled!"), Value(3000)});
			                                                                        return {};
		                                                                        }),
		                                                                     MakeFunction([=, &context](const Values& args) -> Values {
			                                                                     Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Fast Reload"), Value("Fast Reload Disabled."), Value(3000)});
			                                                                     return {};
		                                                                     })});
		Call(Member(context.Global("commandmgr"), "add_bool_command"), Values{Value("notm25_bst"), Value("Bullshark Testosterone"), Value("Enables BST"), Value(false), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                      Call(*GlobalSetBit, Values{First(Call(context.Global("ScriptGlobal"), Values{Add(Value(2673276), Value(3768))})), Value(1), Value(0)});
			                                                                      Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - BST"), Value("BST Enabled!"), Value(3000)});
			                                                                      return {};
		                                                                      }),
		                                                                   MakeFunction([=, &context](const Values& args) -> Values {
			                                                                   Call(*GlobalSetBit, Values{First(Call(context.Global("ScriptGlobal"), Values{Add(Value(2673276), Value(3768))})), Value(1), Value(2)});
			                                                                   Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - BST"), Value("BST Disabled."), Value(3000)});
			                                                                   return {};
		                                                                   })});
		auto m25_yogaRmulti = MakeCell(Value(1.15));
		auto m25_yogaSmulti = MakeCell(Value(2.0));
		Call(Member(context.Global("commandmgr"), "add_bool_command"), Values{Value("m25_runboost"), Value("Run/Swim Boost"), Value("Enables Run/Swim Boost.\n*This is essentially the same as \"Super Run\" from Self submenu, but using a global (Not to be confused with \"Fast Run\" stat)."), Value(false), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                      Call(*GlobalSetBit, Values{First(Call(context.Global("ScriptGlobal"), Values{Add(Value(2673276), Value(3768))})), Value(1), Value(10)});
			                                                                      Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Run Boost"), Value("Run Boost Enabled!"), Value(3000)});
			                                                                      return {};
		                                                                      }),
		                                                                   MakeFunction([=, &context](const Values& args) -> Values {
			                                                                   Call(*GlobalSetBit, Values{First(Call(context.Global("ScriptGlobal"), Values{Add(Value(2673276), Value(3768))})), Value(0), Value(10)});
			                                                                   Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Run Boost"), Value("Run Boost Disabled."), Value(3000)});
			                                                                   return {};
		                                                                   })});
		Call(Member(context.Global("commandmgr"), "add_bool_command"), Values{Value("m25_strengthboost"), Value("Strength Boost"), Value("Enables Strength Boost"), Value(false), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                      Call(*GlobalSetBit, Values{First(Call(context.Global("ScriptGlobal"), Values{Add(Value(2673276), Value(3768))})), Value(1), Value(6)});
			                                                                      Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Strength Boost"), Value("Strength Boost Enabled!"), Value(3000)});
			                                                                      return {};
		                                                                      }),
		                                                                   MakeFunction([=, &context](const Values& args) -> Values {
			                                                                   Call(*GlobalSetBit, Values{First(Call(context.Global("ScriptGlobal"), Values{Add(Value(2673276), Value(3768))})), Value(0), Value(6)});
			                                                                   Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Strength Boost"), Value("Strength Boost Disabled."), Value(3000)});
			                                                                   return {};
		                                                                   })});
		Call(Member(context.Global("commandmgr"), "add_bool_command"), Values{Value("clearworld"), Value("Clear World"), Value("Remove all peds and vehicles."), Value(false), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                      Call(*ClearWorld, Values{Value(1)});
			                                                                      Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Clear World"), Value("Clear World Enabled!"), Value(3000)});
			                                                                      return {};
		                                                                      }),
		                                                                   MakeFunction([=, &context](const Values& args) -> Values {
			                                                                   Call(*ClearWorld, Values{Value(0)});
			                                                                   Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Clear World"), Value("Clear World Disabled."), Value(3000)});
			                                                                   return {};
		                                                                   })});
		Call(Member(context.Global("commandmgr"), "add_looped_command"), Values{Value("ragdoll"), Value("Ragdoll"), Value("Ragdoll on demand by pressing [L-ALT]"), *RagdollPlayer, MakeFunction([=, &context](const Values& args) -> Values {
			                                                                        Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Ragdoll"), Value("Manual Ragdoll Enabled!"), Value(3000)});
			                                                                        return {};
		                                                                        }),
		                                                                     MakeFunction([=, &context](const Values& args) -> Values {
			                                                                     Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Ragdoll"), Value("Manual Ragdoll Disabled."), Value(3000)});
			                                                                     return {};
		                                                                     })});
		Call(Member(context.Global("commandmgr"), "add_looped_command"), Values{Value("infcombatroll"), Value("Infinite Combat Roll"), Value("& No-Recoil/Spread ;)"), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                        Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_SHOOTING_ABILITY"), Value(500)});
			                                                                        return {};
		                                                                        }),
		                                                                     MakeFunction([=, &context](const Values& args) -> Values {
			                                                                     Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Infinite Combat Roll"), Value("Infinite Combat Roll Enabled!"), Value(3000)});
			                                                                     return {};
		                                                                     }),
		                                                                     MakeFunction([=, &context](const Values& args) -> Values {
			                                                                     Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Infinite Combat Roll"), Value("Infinite Combat Roll Disabled."), Value(3000)});
			                                                                     return {};
		                                                                     })});
		Call(Member(context.Global("commandmgr"), "add_looped_command"), Values{Value("noidlecam"), Value("Disable Idle Cam"), Value("Disables Idle Cam."), Member(context.Global("CAMERA"), "INVALIDATE_IDLE_CAM")});
		auto VehAutoDrive = MakeCell(MakeTable({{"Toggle", Value(false)}, {"Mode", Value(0)}, {"Label", MakeTable({{Value(1), Value("Normal")}, {Value(2), Value("Rushed")}, {Value(3), Value("GTA")}})}, {"Flag", MakeTable({{Value(1), Value(443)}, {Value(2), Value(787118)}, {Value(3), Value(1076625980)}})}}));
		Member(*VehAutoDrive, "SkipRedLight") = MakeFunction([=, &context](const Values& args) -> Values {
			auto PlaVeh = MakeCell(At(args, 0));
			if (Truthy(And(Equal(Member(Member(*VehAutoDrive, "Flag"), Add(Member(*VehAutoDrive, "Mode"), Value(1))), Value(787118)), [&] {
				    return First(Call(Member(context.Global("VEHICLE"), "IS_VEHICLE_STOPPED_AT_TRAFFIC_LIGHTS"), Values{*PlaVeh}));
			    })))
			{
				Call(Member(context.Global("script"), "yield"), Values{Value(4000)});
				Call(Member(context.Global("TASK"), "SET_DRIVE_TASK_DRIVING_STYLE"), Append(Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{}))}, Call(context.Global("tonumber"), Append(Values{}, Call(Member(context.Global("bit"), "band"), Append(Values{Value(787118)}, Call(Member(context.Global("bit"), "bnot"), Append(Values{}, Call(Member(context.Global("bit"), "lshift"), Values{Value(1), Value(7)})))))))));
				Call(Member(context.Global("script"), "yield"), Values{Value(2000)});
				Call(Member(context.Global("TASK"), "SET_DRIVE_TASK_DRIVING_STYLE"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value(787118)});
			}
			return {};
		});
		auto AutoDriveSpeed = MakeCell(Value(40));
		// SelfTaxi preserves the feature callback and its captured state.
		auto SelfTaxi = MakeCell();
		*SelfTaxi = MakeFunction([=, &context](const Values& args) -> Values {
			auto PlaVeh_16 = MakeCell(First(Call(Member(context.Global("PED"), "GET_VEHICLE_PED_IS_IN"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value(false)})));
			if (Truthy(First(Call(Member(context.Global("PED"), "IS_PED_SITTING_IN_ANY_VEHICLE"), Append(Values{}, Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{}))))))
			{
				auto wp_blip = MakeCell(First(Call(Member(context.Global("HUD"), "GET_FIRST_BLIP_INFO_ID"), Values{Value(8)})));
				if (Truthy(Value(!Truthy(First(Call(Member(context.Global("HUD"), "DOES_BLIP_EXIST"), Values{*wp_blip}))))))
				{
					Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Self Taxi"), Value("Please set a waypoint first."), Value(3000)});
					return Values{};
				}
				auto wp_coords = MakeCell(First(Call(Member(context.Global("HUD"), "GET_BLIP_INFO_ID_COORD"), Values{*wp_blip})));
				Call(Member(context.Global("TASK"), "TASK_VEHICLE_DRIVE_TO_COORD"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), *PlaVeh_16, Member(*wp_coords, "x"), Member(*wp_coords, "y"), Member(*wp_coords, "z"), *AutoDriveSpeed, Value(0), Value(0), Member(Member(*VehAutoDrive, "Flag"), Add(Member(*VehAutoDrive, "Mode"), Value(1))), Value(10.0), Value(0)});
				Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Self Taxi"), Value("Auto-Drive to Waypoint has started."), Value(3000)});
				while (Truthy(Value(true)))
				{
					Call(Member(context.Global("VEHICLE"), "SET_VEHICLE_MAX_SPEED"), Values{*PlaVeh_16, *AutoDriveSpeed});
					Call(context.Global("CurPlayerCoords"), Values{});
					auto dist = MakeCell(First(Call(Member(context.Global("ScriptNatives"), "VDIST"), Values{Member(context.Global("PlaCoords"), "x"), Member(context.Global("PlaCoords"), "y"), Member(context.Global("PlaCoords"), "z"), Member(*wp_coords, "x"), Member(*wp_coords, "y"), Member(*wp_coords, "z")})));
					if (Truthy(context.Global("drivemodechanged")))
					{
						Call(Member(context.Global("TASK"), "SET_DRIVE_TASK_DRIVING_STYLE"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Member(Member(*VehAutoDrive, "Flag"), Add(Member(*VehAutoDrive, "Mode"), Value(1)))});
					}
					Call(Member(*VehAutoDrive, "SkipRedLight"), Values{*PlaVeh_16});
					if (Truthy(Less(*dist, Value(70.0))))
					{
						Call(Member(context.Global("TASK"), "CLEAR_PED_TASKS"), Append(Values{}, Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})));
						Call(Member(context.Global("VEHICLE"), "BRING_VEHICLE_TO_HALT"), Values{*PlaVeh_16, Value(3.0), Value(1), Value(0)});
						Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Self Taxi"), Value("Destination reached."), Value(3000)});
						return Values{};
					}
					if (Truthy(First(Call(Member(context.Global("PAD"), "IS_CONTROL_JUST_PRESSED"), Values{Value(0), Value(22)}))))
					{
						Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Self Taxi"), Value("Auto-Drive to Waypoint has been canceled."), Value(3000)});
						Call(Member(context.Global("TASK"), "CLEAR_PED_TASKS"), Append(Values{}, Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})));
						return Values{};
					}
					Call(Member(context.Global("script"), "yield"), Values{Value(0)});
				}
			}
			else
			{
				Call(Member(context.Global("notify"), "warn"), Values{Value("TinkerScripts - Self Taxi"), Value("You are not in a vehicle."), Value(3000)});
			}
			return {};
		});
		// Wander preserves the feature callback and its captured state.
		auto Wander = MakeCell();
		*Wander = MakeFunction([=, &context](const Values& args) -> Values {
			auto PlaVeh_17 = MakeCell(First(Call(Member(context.Global("PED"), "GET_VEHICLE_PED_IS_IN"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value(false)})));
			if (Truthy(First(Call(Member(context.Global("PED"), "IS_PED_SITTING_IN_ANY_VEHICLE"), Append(Values{}, Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{}))))))
			{
				Call(Member(context.Global("TASK"), "TASK_VEHICLE_DRIVE_WANDER"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), *PlaVeh_17, *AutoDriveSpeed, Member(Member(*VehAutoDrive, "Flag"), Add(Member(*VehAutoDrive, "Mode"), Value(1)))});
				Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Auto-Wander"), Value("Auto-Wander has started."), Value(3000)});
				while (Truthy(Value(true)))
				{
					Call(Member(context.Global("VEHICLE"), "SET_VEHICLE_MAX_SPEED"), Values{*PlaVeh_17, *AutoDriveSpeed});
					if (Truthy(context.Global("drivemodechanged")))
					{
						Call(Member(context.Global("TASK"), "SET_DRIVE_TASK_DRIVING_STYLE"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Member(Member(*VehAutoDrive, "Flag"), Add(Member(*VehAutoDrive, "Mode"), Value(1)))});
					}
					Call(Member(*VehAutoDrive, "SkipRedLight"), Values{*PlaVeh_17});
					if (Truthy(First(Call(Member(context.Global("PAD"), "IS_CONTROL_JUST_PRESSED"), Values{Value(0), Value(22)}))))
					{
						Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Auto-Wander"), Value("You took the wheel."), Value(3000)});
						Call(Member(context.Global("TASK"), "CLEAR_PED_TASKS"), Append(Values{}, Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})));
						return Values{};
					}
					Call(Member(context.Global("script"), "yield"), Values{Value(0)});
				}
			}
			else
			{
				Call(Member(context.Global("notify"), "warn"), Values{Value("TinkerScripts - Auto-Wander"), Value("You are not in a vehicle."), Value(3000)});
			}
			return {};
		});
		auto jr_radius = MakeCell(Value(10));
		// Joyrider preserves the feature callback and its captured state.
		auto Joyrider = MakeCell();
		*Joyrider = MakeFunction([=, &context](const Values& args) -> Values {
			auto jr_radius_18 = MakeCell(At(args, 0));
			Call(context.Global("CurPlayerCoords"), Values{});
			auto vehicle = MakeCell(First(Call(Member(context.Global("VEHICLE"), "GET_RANDOM_VEHICLE_IN_SPHERE"), Values{Member(context.Global("PlaCoords"), "x"), Member(context.Global("PlaCoords"), "y"), Member(context.Global("PlaCoords"), "z"), *jr_radius_18, Value(0), Value(101247)})));
			auto drivernomore = MakeCell(First(Call(Member(context.Global("VEHICLE"), "GET_PED_IN_VEHICLE_SEAT"), Values{*vehicle, Negate(Value(1)), Value(0)})));
			if (Truthy(And(NotEqual(*vehicle, Value(0)), [&] {
				    return Value(!Truthy(First(Invoke(First(Call(context.Global("Vehicle"), Values{*vehicle})), "is_seat_free", Values{Negate(Value(1))}))));
			    })))
			{
				Invoke(First(Call(context.Global("Entity"), Values{*drivernomore})), "delete", Values{});
				Call(Member(context.Global("PED"), "SET_PED_INTO_VEHICLE"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), *vehicle, Negate(Value(1))});
			}
			else if (Truthy(And(NotEqual(*vehicle, Value(0)), [&] {
				         return First(Invoke(First(Call(context.Global("Vehicle"), Values{*vehicle})), "is_seat_free", Values{Negate(Value(1))}));
			         })))
			{
				Call(Member(context.Global("PED"), "SET_PED_INTO_VEHICLE"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), *vehicle, Negate(Value(1))});
				Call(Member(context.Global("VEHICLE"), "SET_VEHICLE_UNDRIVEABLE"), Values{*vehicle, Value(0)});
				Call(Member(context.Global("VEHICLE"), "SET_VEHICLE_ENGINE_ON"), Values{*vehicle, Value(1), Value(1), Value(0)});
			}
			else
			{
				Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Joyrider"), Concat(Value("No vehicles in radius ("), Concat(*jr_radius_18, Value("m) detected.\nIncrease your Teleport Radius."))), Value(3000)});
			}
			return {};
		});
		auto DuffelBagSavePatch = MakeCell(MakeTable({}));
		Call(Member(context.Global("commandmgr"), "add_bool_command"), Values{Value("DuffelBagSave"), Value("Enable Duffel Bag Save"), Value("Bypasses Duffel Bag save restrictions at Clothing Stores and Wardrobe."), Value(false), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                      if (Truthy(Value(!Truthy(Member(*DuffelBagSavePatch, "Shop")))))
			                                                                      {
				                                                                      Member(*DuffelBagSavePatch, "Shop") = First(Call(context.Global("ScriptPatch"), Values{Value("clothes_shop_mp"), Value("DuffelBagSavePatch1"), Value("2C 0D ? ? 56 ? ? 72 2E 01 01 71 2E 01 01 2D 02 04"), Value(7), MakeTable({{Value(1), Value(0)}})}));
			                                                                      }
			                                                                      if (Truthy(Value(!Truthy(Member(*DuffelBagSavePatch, "Wardrobe")))))
			                                                                      {
				                                                                      Member(*DuffelBagSavePatch, "Wardrobe") = First(Call(context.Global("ScriptPatch"), Values{Value("wardrobe_mp"), Value("DuffelBagSavePatch2"), Value("2C 0D ? ? 56 ? ? 72 2E 01 01 71 2E 01 01 2D 01 03"), Value(7), MakeTable({{Value(1), Value(0)}})}));
			                                                                      }
			                                                                      Invoke(Member(*DuffelBagSavePatch, "Shop"), "enable", Values{});
			                                                                      Invoke(Member(*DuffelBagSavePatch, "Wardrobe"), "enable", Values{});
			                                                                      return {};
		                                                                      }),
		                                                                   MakeFunction([=, &context](const Values& args) -> Values {
			                                                                   if (Truthy(Member(*DuffelBagSavePatch, "Shop")))
			                                                                   {
				                                                                   Invoke(Member(*DuffelBagSavePatch, "Shop"), "disable", Values{});
			                                                                   }
			                                                                   if (Truthy(Member(*DuffelBagSavePatch, "Wardrobe")))
			                                                                   {
				                                                                   Invoke(Member(*DuffelBagSavePatch, "Wardrobe"), "disable", Values{});
			                                                                   }
			                                                                   return {};
		                                                                   })});
		auto OR_offsets = MakeCell(MakeTable({{Value(1), Value(1320)}, {Value(2), Value(1384)}, {Value(3), Value(1448)}, {Value(4), Value(1512)}, {Value(5), Value(1576)}, {Value(6), Value(1640)}, {Value(7), Value(1704)}, {Value(8), Value(1768)}, {Value(9), Value(1832)}, {Value(10), Value(1896)}, {Value(11), Value(1960)}, {Value(12), Value(2024)}, {Value(13), Value(2088)}, {Value(14), Value(2152)}, {Value(15), Value(2216)}, {Value(16), Value(2280)}, {Value(17), Value(2344)}, {Value(18), Value(2408)}, {Value(19), Value(2472)}, {Value(20), Value(2536)}}));
		auto values_19 = Values{};
		auto OR_ptr = MakeCell(At(values_19, 0));
		auto OR_selected = MakeCell(Value(0));
		auto New_Name = MakeCell(Value(""));
		auto renamemode = MakeCell(Value(false));
		auto canrename = MakeCell(Value(false));
		auto ibox_checkbox = MakeCell(Value(false));
		// ReNameEditor preserves the feature callback and its captured state.
		auto ReNameEditor = MakeCell();
		*ReNameEditor = MakeFunction([=, &context](const Values& args) -> Values {
			if (Truthy(Value(!Truthy(*canrename))))
			{
				if (Truthy(Equal(First(Call(Member(context.Global("MISC"), "UPDATE_ONSCREEN_KEYBOARD"), Values{})), Value(0))))
				{
					Call(Member(context.Global("log"), "info"), Values{Concat(*cls, Concat(*lual, Concat(Value("Waiting for \"New Name\""), *cls)))});
					Call(Member(context.Global("MISC"), "DISPLAY_ONSCREEN_KEYBOARD"), Values{Value(0), Value("CELL_7000"), Value(""), *New_Name, Value{}, Value{}, Value{}, Value(64)});
					*canrename = Value(true);
				}
				else if (Truthy(NotEqual(First(Call(Member(context.Global("MISC"), "UPDATE_ONSCREEN_KEYBOARD"), Values{})), Value(0))))
				{
					Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Re-Name Editor"), Value("Open an \"Input Box\" first."), Value(3000)});
					Call(Member(context.Global("log"), "info"), Values{Concat(*cls, Concat(*lual, Concat(Value("Re-Name Editor: \033[33mOpen an \"Input Box\" first.\033[m"), Concat(*cls, Value("\n")))))});
					return Values{};
				}
			}
			while (Truthy(*canrename))
			{
				auto nn = MakeCell(First(Call(Member(context.Global("MISC"), "GET_ONSCREEN_KEYBOARD_RESULT"), Values{})));
				if (Truthy(Equal(First(Call(Member(context.Global("MISC"), "UPDATE_ONSCREEN_KEYBOARD"), Values{})), Value(1))))
				{
					Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Re-Name Editor"), Concat(Value("New name is:\n"), *nn), Value(3000)});
					Call(Member(context.Global("log"), "info"), Append(Values{}, Call(Member(context.Global("string"), "format"), Values{Value("%s%s\033[ARe-Name Editor: \033[32mNew name is:\033[m%s %s%s"), *cls, *lual, *nn, *cls, *cls})));
					*New_Name = Value("");
					*canrename = Value(false);
					return Values{};
				}
				else if (Truthy(Equal(First(Call(Member(context.Global("MISC"), "UPDATE_ONSCREEN_KEYBOARD"), Values{})), Value(2))))
				{
					Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Re-Name Editor"), Value("Logic canceled."), Value(3000)});
					Call(Member(context.Global("log"), "info"), Values{Concat(*cls, Concat(*lual, Concat(Value("\033[ARe-Name Editor: Logic canceled.                             \033[B\r                                      "), *cls)))});
					*canrename = Value(false);
					return Values{};
				}
				Call(Member(context.Global("script"), "yield"), Values{Value(0)});
			}
			return {};
		});
		auto ReN_Style = MakeCell(MakeTable({{Value(1), MakeTable({{"tbLabel", Value("R* Logo")}, {"tStyle", Value("∑")}})}, {Value(2), MakeTable({{"tbLabel", Value("Wanted Star")}, {"tStyle", Value("~ws~")}})}, {Value(3), MakeTable({{"tbLabel", Value("R* Verified")}, {"tStyle", Value("¦")}})}, {Value(4), MakeTable({{"tbLabel", Value("Lock Icon")}, {"tStyle", Value("Ω")}})}, {Value(5), MakeTable({{"tbLabel", Value("R* Created")}, {"tStyle", Value("‹")}})}, {Value(6), MakeTable({{"tbLabel", Value("Blank Icon")}, {"tStyle", Value("›")}})}, {Value(7), MakeTable({{"tbLabel", Value("Bold")}, {"tStyle", Value("~h~")}})}, {Value(8), MakeTable({{"tbLabel", Value("Italic")}, {"tStyle", Value("~italic~")}})}, {Value(9), MakeTable({{"tbLabel", Value("New line")}, {"tStyle", Value("~n~")}})}, {Value(10), MakeTable({{"tbLabel", Value("Reset")}, {"tStyle", Value("~s~")}})}}));
		auto ReN_Color = MakeCell(MakeTable({{Value(1), MakeTable({{"bName", Value("##Rednnf")}, {"bTip", Value("Red")}, {"tColf", Value("~r~")}, {"bCol", MakeTable({{Value(1), Value(0.87)}, {Value(2), Value(0.196)}, {Value(3), Value(0.196)}, {Value(4), Value(1.0)}})}})}, {Value(2), MakeTable({{"bName", Value("##Yellownnf")}, {"bTip", Value("Yellow")}, {"tColf", Value("~y~")}, {"bCol", MakeTable({{Value(1), Value(0.933)}, {Value(2), Value(0.776)}, {Value(3), Value(0.313)}, {Value(4), Value(1.0)}})}})}, {Value(3), MakeTable({{"bName", Value("##Bluennf")}, {"bTip", Value("Blue")}, {"tColf", Value("~b~")}, {"bCol", MakeTable({{Value(1), Value(0.36)}, {Value(2), Value(0.705)}, {Value(3), Value(0.89)}, {Value(4), Value(1.0)}})}})}, {Value(4), MakeTable({{"bName", Value("##DarkBluennf")}, {"bTip", Value("Dark Blue")}, {"tColf", Value("~d~")}, {"bCol", MakeTable({{Value(1), Value(0.184)}, {Value(2), Value(0.361)}, {Value(3), Value(0.451)}, {Value(4), Value(1.0)}})}})}, {Value(5), MakeTable({{"bName", Value("##Orangennf")}, {"bTip", Value("Orange")}, {"tColf", Value("~o~")}, {"bCol", MakeTable({{Value(1), Value(0.992)}, {Value(2), Value(0.517)}, {Value(3), Value(0.333)}, {Value(4), Value(1.0)}})}})}, {Value(6), MakeTable({{"bName", Value("##Greennnf")}, {"bTip", Value("Green")}, {"tColf", Value("~g~")}, {"bCol", MakeTable({{Value(1), Value(0.443)}, {Value(2), Value(0.792)}, {Value(3), Value(0.443)}, {Value(4), Value(1.0)}})}})}, {Value(7), MakeTable({{"bName", Value("##Pinknnf")}, {"bTip", Value("Pink")}, {"tColf", Value("~q~")}, {"bCol", MakeTable({{Value(1), Value(0.886)}, {Value(2), Value(0.309)}, {Value(3), Value(0.502)}, {Value(4), Value(1.0)}})}})}, {Value(8), MakeTable({{"bName", Value("##Purplennf")}, {"bTip", Value("Purple")}, {"tColf", Value("~p~")}, {"bCol", MakeTable({{Value(1), Value(0.513)}, {Value(2), Value(0.396)}, {Value(3), Value(0.878)}, {Value(4), Value(1.0)}})}})}, {Value(9), MakeTable({{"bName", Value("##Whitennf")}, {"bTip", Value("White")}, {"tColf", Value("~w~")}, {"bCol", MakeTable({{Value(1), Value(1)}, {Value(2), Value(1)}, {Value(3), Value(1)}, {Value(4), Value(1.0)}})}})}, {Value(10), MakeTable({{"bName", Value("##Greynnf")}, {"bTip", Value("Grey")}, {"tColf", Value("~c~")}, {"bCol", MakeTable({{Value(1), Value(0.545)}, {Value(2), Value(0.545)}, {Value(3), Value(0.545)}, {Value(4), Value(1.0)}})}})}, {Value(11), MakeTable({{"bName", Value("##DarkGreynnf")}, {"bTip", Value("Dark Grey")}, {"tColf", Value("~m~")}, {"bCol", MakeTable({{Value(1), Value(0.388)}, {Value(2), Value(0.388)}, {Value(3), Value(0.388)}, {Value(4), Value(1.0)}})}})}, {Value(12), MakeTable({{"bName", Value("##Blacknnf")}, {"bTip", Value("Black")}, {"tColf", Value("~u~")}, {"bCol", MakeTable({{Value(1), Value(0.0)}, {Value(2), Value(0.0)}, {Value(3), Value(0.0)}, {Value(4), Value(1.0)}})}})}}));
		auto values_20 = Values{};
		auto ReN_selectedprop = MakeCell(At(values_20, 0));
		auto ReN_Properties = MakeCell(MakeTable({{Value(1), MakeTable({{"renProperty", Value("Office")}, {"renPropStat", Value("MPX_GB_OFFICE_NAME")}, {"renPropStat2", Value("MPX_GB_OFFICE_NAME2")}})}, {Value(2), MakeTable({{"renProperty", Value("Organization")}, {"renPropStat", Value("MPX_GB_GANG_NAME")}, {"renPropStat2", Value("MPX_GB_GANG_NAME2")}})}, {Value(3), MakeTable({{"renProperty", Value("MC Clubhouse")}, {"renPropStat", Value("MPX_MC_CLBHOSE_NAME")}, {"renPropStat2", Value("MPX_MC_CLBHOSE_NAME2")}})}, {Value(4), MakeTable({{"renProperty", Value("MC Club")}, {"renPropStat", Value("MPX_MC_GANG_NAME")}, {"renPropStat2", Value("MPX_MC_GANG_NAME2")}})}, {Value(5), MakeTable({{"renProperty", Value("Yacht")}, {"renPropStat", Value("MPX_YACHT_NAME")}, {"renPropStat2", Value("MPX_YACHT_NAME2")}})}, {Value(6), MakeTable({{"renProperty", Value("Nightclub")}, {"renPropStat", Value("MPX_NIGHTCLUB_NAME")}, {"renPropStat2", Value("MPX_NIGHTCLUB_NAME2")}})}, {Value(7), MakeTable({{"renProperty", Value("Acid Lab")}, {"renPropStat", Value("MPX_ACID_LAB_NAME")}, {"renPropStat2", Value("MPX_ACID_LAB_NAME2")}})}, {Value(8), MakeTable({{"renProperty", Value("Acid Lab Product")}, {"renPropStat", Value("MPX_ACIDLAB_PRODUCT_NAME")}, {"renPropStat2", Value("MPX_ACIDLAB_PRODUCT_NAME2")}})}, {Value(9), MakeTable({{"renProperty", Value("Mansion Dog")}, {"renPropStat", Value("MPX_MANSION_DOG_NAME1")}, {"renPropStat2", Value("MPX_MANSION_DOG_NAME2")}})}, {Value(10), MakeTable({{"renProperty", Value("Mansion Cat")}, {"renPropStat", Value("MPX_MANSION_CAT_NAME1")}, {"renPropStat2", Value("MPX_MANSION_CAT_NAME2")}})}}));
		auto JBvJG = MakeCell(MakeTable({{"SoV", Value(false)}, {"Phoenix", Value(false)}, {"SoVLoop", Value(0)}, {"PhoenixLoop", Value(0)}, {"PhoenixReactToNoclip", Value(false)}}));
		// RapidOxidation preserves the feature callback and its captured state.
		auto RapidOxidation = MakeCell();
		*RapidOxidation = MakeFunction([=, &context](const Values& args) -> Values {
			if (Truthy(First(Call(Member(context.Global("PAD"), "IS_CONTROL_JUST_PRESSED"), Values{Value(0), Value(183)}))))
			{
				Member(*JBvJG, "SoV") = Value(!Truthy(Member(*JBvJG, "SoV")));
			}
			if (Truthy(Member(*JBvJG, "SoV")))
			{
				if (Truthy(Greater(Subtract(First(Call(Member(context.Global("MISC"), "GET_GAME_TIMER"), Values{})), Member(*JBvJG, "SoVLoop")), Value(2200))))
				{
					Call(Member(context.Global("STREAMING"), "REQUEST_NAMED_PTFX_ASSET"), Values{Value("core")});
					{
						Iterator iterator_21(Append(Values{}, Call(context.Global("ipairs"), Values{MakeTable({{Value(1), MakeTable({{Value(1), Value(31086)}, {Value(2), Value(0.7)}})}, {Value(2), MakeTable({{Value(1), Value(14201)}, {Value(2), Value(0.3)}})}, {Value(3), MakeTable({{Value(1), Value(52301)}, {Value(2), Value(0.3)}})}, {Value(4), MakeTable({{Value(1), Value(57005)}, {Value(2), Value(0.3)}})}, {Value(5), MakeTable({{Value(1), Value(18905)}, {Value(2), Value(0.3)}})}})})));
						for (auto row_22 = iterator_21.Next(); !row_22.empty() && !row_22[0].IsNil(); row_22 = iterator_21.Next())
						{
							auto __23 = MakeCell(At(row_22, 0));
							auto i = MakeCell(At(row_22, 1));
							Call(Member(context.Global("GRAPHICS"), "USE_PARTICLE_FX_ASSET"), Values{Value("core")});
							Call(Member(context.Global("GRAPHICS"), "START_NETWORKED_PARTICLE_FX_NON_LOOPED_ON_PED_BONE"), Values{Value("ent_sht_petrol_fire"), First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value(0), Value(0), Value(0.0), Value(90.0), Negate(Value(100.0)), Value(90.0), Member(*i, Value(1)), Member(*i, Value(2)), Value(1), Value(1), Value(1)});
							Call(Member(context.Global("STREAMING"), "REMOVE_PTFX_ASSET"), Values{});
						}
					}
					Member(*JBvJG, "SoVLoop") = First(Call(Member(context.Global("MISC"), "GET_GAME_TIMER"), Values{}));
				}
			}
			if (Truthy(First(Call(Member(context.Global("PAD"), "IS_CONTROL_JUST_PRESSED"), Values{Value(0), Value(29)}))))
			{
				Member(*JBvJG, "Phoenix") = Value(!Truthy(Member(*JBvJG, "Phoenix")));
			}
			if (Truthy(Member(*JBvJG, "Phoenix")))
			{
				if (Truthy(Greater(Subtract(First(Call(Member(context.Global("MISC"), "GET_GAME_TIMER"), Values{})), Member(*JBvJG, "PhoenixLoop")), Value(6200))))
				{
					Call(Member(context.Global("STREAMING"), "REQUEST_NAMED_PTFX_ASSET"), Values{Value("core")});
					{
						Iterator iterator_24(Append(Values{}, Call(context.Global("ipairs"), Values{MakeTable({{Value(1), MakeTable({{Value(1), Negate(Value(0.3))}, {Value(2), Value(0)}})}, {Value(2), MakeTable({{Value(1), Negate(Value(0.35))}, {Value(2), Value(10.0)}})}, {Value(3), MakeTable({{Value(1), Negate(Value(0.4))}, {Value(2), Value(20.0)}})}, {Value(4), MakeTable({{Value(1), Negate(Value(0.45))}, {Value(2), Value(30.0)}})}})})));
						for (auto row_25 = iterator_24.Next(); !row_25.empty() && !row_25[0].IsNil(); row_25 = iterator_24.Next())
						{
							auto __26 = MakeCell(At(row_25, 0));
							auto i_27 = MakeCell(At(row_25, 1));
							if (Truthy(Member(*JBvJG, "PhoenixReactToNoclip")))
							{
								if (Truthy(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("noclip")})), "get_value", Values{}))))
								{
									Call(Member(context.Global("GRAPHICS"), "USE_PARTICLE_FX_ASSET"), Values{Value("core")});
									Call(Member(context.Global("GRAPHICS"), "START_NETWORKED_PARTICLE_FX_NON_LOOPED_ON_PED_BONE"), Values{Value("ent_sht_flame"), First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Add(Member(*i_27, Value(1)), Value(0.5)), Negate(Value(0.15)), Value(0.03), Negate(Value(25.0)), Subtract(Member(*i_27, Value(2)), Value(28)), Value(0), Value(24818), Value(1), Value(1), Value(1), Value(1)});
									Call(Member(context.Global("STREAMING"), "REMOVE_PTFX_ASSET"), Values{});
									Call(Member(context.Global("GRAPHICS"), "USE_PARTICLE_FX_ASSET"), Values{Value("core")});
									Call(Member(context.Global("GRAPHICS"), "START_NETWORKED_PARTICLE_FX_NON_LOOPED_ON_PED_BONE"), Values{Value("ent_sht_flame"), First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Add(Member(*i_27, Value(1)), Value(0.5)), Negate(Value(0.15)), Value(0.03), Value(205.0), Subtract(Member(*i_27, Value(2)), Value(28)), Value(0), Value(24818), Value(1), Value(1), Value(1), Value(1)});
									Call(Member(context.Global("STREAMING"), "REMOVE_PTFX_ASSET"), Values{});
								}
								else
								{
									Call(Member(context.Global("GRAPHICS"), "USE_PARTICLE_FX_ASSET"), Values{Value("core")});
									Call(Member(context.Global("GRAPHICS"), "START_NETWORKED_PARTICLE_FX_NON_LOOPED_ON_PED_BONE"), Values{Value("ent_sht_flame"), First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Add(Member(*i_27, Value(1)), Value(0.5)), Negate(Value(0.15)), Value(0.03), Negate(Value(25.0)), Add(Member(*i_27, Value(2)), Value(50)), Value(0), Value(24818), Value(1), Value(1), Value(1), Value(1)});
									Call(Member(context.Global("STREAMING"), "REMOVE_PTFX_ASSET"), Values{});
									Call(Member(context.Global("GRAPHICS"), "USE_PARTICLE_FX_ASSET"), Values{Value("core")});
									Call(Member(context.Global("GRAPHICS"), "START_NETWORKED_PARTICLE_FX_NON_LOOPED_ON_PED_BONE"), Values{Value("ent_sht_flame"), First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Add(Member(*i_27, Value(1)), Value(0.5)), Negate(Value(0.15)), Value(0.03), Value(205.0), Add(Member(*i_27, Value(2)), Value(50)), Value(0), Value(24818), Value(1), Value(1), Value(1), Value(1)});
									Call(Member(context.Global("STREAMING"), "REMOVE_PTFX_ASSET"), Values{});
								}
							}
							else
							{
								Call(Member(context.Global("GRAPHICS"), "USE_PARTICLE_FX_ASSET"), Values{Value("core")});
								Call(Member(context.Global("GRAPHICS"), "START_NETWORKED_PARTICLE_FX_NON_LOOPED_ON_PED_BONE"), Values{Value("ent_sht_flame"), First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Member(*i_27, Value(1)), Negate(Value(0.15)), Value(0.03), Negate(Value(25.0)), Member(*i_27, Value(2)), Value(0), Value(11816), Value(1), Value(1), Value(1), Value(1)});
								Call(Member(context.Global("STREAMING"), "REMOVE_PTFX_ASSET"), Values{});
								Call(Member(context.Global("GRAPHICS"), "USE_PARTICLE_FX_ASSET"), Values{Value("core")});
								Call(Member(context.Global("GRAPHICS"), "START_NETWORKED_PARTICLE_FX_NON_LOOPED_ON_PED_BONE"), Values{Value("ent_sht_flame"), First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Member(*i_27, Value(1)), Negate(Value(0.15)), Value(0.03), Value(205.0), Member(*i_27, Value(2)), Value(0), Value(11816), Value(1), Value(1), Value(1), Value(1)});
								Call(Member(context.Global("STREAMING"), "REMOVE_PTFX_ASSET"), Values{});
							}
						}
					}
					Member(*JBvJG, "PhoenixLoop") = First(Call(Member(context.Global("MISC"), "GET_GAME_TIMER"), Values{}));
				}
			}
			return {};
		});
		Call(Member(context.Global("commandmgr"), "add_looped_command"), Values{Value("rapidoxidation"), Value("Rapid Oxidation"), Value("Johnny Blaze meets Jean Grey...\n\nPress [G] to become Ghost Rider.\nPress [B] to unleash The Phoenix!"), *RapidOxidation, MakeFunction([=, &context](const Values& args) -> Values {
			                                                                        Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Rapid Oxidation"), Value("Rapid Oxidation enabled\nPress [G] to become Ghost Rider.\nPress [B] to unleash The Phoenix!"), Value(3000)});
			                                                                        Invoke(First(Invoke(First(Call(Member(context.Global("players"), "get_local"), Values{})), "get_ped", Values{})), "set_invincible", Values{Value(true)});
			                                                                        return {};
		                                                                        }),
		                                                                     MakeFunction([=, &context](const Values& args) -> Values {
			                                                                     Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Rapid Oxidation"), Value("Rapid Oxidation disabled."), Value(3000)});
			                                                                     Invoke(First(Invoke(First(Call(Member(context.Global("players"), "get_local"), Values{})), "get_ped", Values{})), "set_invincible", Values{Value(false)});
			                                                                     return {};
		                                                                     })});
		Call(Member(context.Global("commandmgr"), "add_bool_command"), Values{Value("phoenixnoclip"), Value("Phoenix Wings react to No clip"), Value("Folds or Spreads Phoenix Wings based on No Clip state."), Value(false), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                      Member(*JBvJG, "PhoenixReactToNoclip") = Value(true);
			                                                                      return {};
		                                                                      }),
		                                                                   MakeFunction([=, &context](const Values& args) -> Values {
			                                                                   Member(*JBvJG, "PhoenixReactToNoclip") = Value(false);
			                                                                   return {};
		                                                                   })});
		Call(Member(context.Global("commandmgr"), "add_bool_command"), Values{Value("noclip_onscreen"), Value("No Clip State On-Screen"), Value("Displays a small window when No Clip is enabled."), Value(false), Value{}});
		auto NoClipWindowTop = MakeCell(Value(false));
		Call(Member(context.Global("menu"), "add_always_draw_imgui"), Values{MakeFunction([=, &context](const Values& args) -> Values {
			if (Truthy(And(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("noclip_onscreen")})), "get_value", Values{})), [&] {
				    return First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("noclip")})), "get_value", Values{}));
			    })))
			{
				if (Truthy(Value(!Truthy(*NoClipWindowTop))))
				{
					Call(Member(context.Global("ImGui"), "SetNextWindowPos"), Values{Divide(*screenX, Value(2)), *screenY, Value(0), Value(0.5), Value(1.0)});
				}
				else
				{
					Call(Member(context.Global("ImGui"), "SetNextWindowPos"), Values{Divide(*screenX, Value(2)), Value(0), Value(0), Value(0.5), Value(0)});
				}
				Call(Member(context.Global("ImGui"), "SetNextWindowSize"), Values{Value(150), Value(70)});
				Call(Member(context.Global("ImGui"), "PushStyleVar"), Values{Member(context.Global("ImGuiStyleVar"), "WindowTitleAlign"), Value(0.5), Value(0.5)});
				Call(Member(context.Global("ImGui"), "Begin"), Values{Value("No Clip"), Value{}, Add(Add(Add(Add(Value(2), Value(4)), Value(32)), Value(256)), Value(512))});
				Call(Member(context.Global("ImGui"), "SetCursorPosX"), Values{Divide(Subtract(First(Call(Member(context.Global("ImGui"), "GetWindowWidth"), Values{})), First(Call(Member(context.Global("ImGui"), "CalcTextSize"), Values{Value("Enabled")}))), Value(2))});
				Call(Member(context.Global("ImGui"), "TextColored"), Values{Value(0.0), Value(1.0), Value(0.0), Value(1.0), Value("Enabled")});
				Call(Member(context.Global("ImGui"), "End"), Values{});
				Call(Member(context.Global("ImGui"), "PopStyleVar"), Values{});
			}
			return {};
		})});
		auto playing_anim = MakeCell(Value(false));
		Call(Member(context.Global("commandmgr"), "add_looped_command"), Values{Value("noclip_anim"), Value("No Clip Animation"), Value("Makes your character play a Flying Animation when No Clip is enabled."), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                        if (Truthy(And(And(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("noclip")})), "get_value", Values{})), [&] {
				                                                                            return Greater(First(Call(Member(context.Global("ENTITY"), "GET_ENTITY_HEIGHT_ABOVE_GROUND"), Append(Values{}, Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})))), Value(3));
			                                                                            }),
			                                                                                [&] {
				                                                                                return Value(!Truthy(First(Call(Member(context.Global("PED"), "IS_PED_IN_ANY_VEHICLE"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value(false)}))));
			                                                                                })))
			                                                                        {
				                                                                        if (Truthy(And(And(First(Call(Member(context.Global("STREAMING"), "HAS_ANIM_DICT_LOADED"), Values{Value("skydive@freefall")})), [&] {
					                                                                            return First(Call(Member(context.Global("STREAMING"), "HAS_ANIM_DICT_LOADED"), Values{Value("missfam5_yoga")}));
				                                                                            }),
				                                                                                [&] {
					                                                                                return Value(!Truthy(*playing_anim));
				                                                                                })))
				                                                                        {
					                                                                        Call(Member(context.Global("TASK"), "CLEAR_PED_TASKS_IMMEDIATELY"), Append(Values{}, Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})));
					                                                                        *playing_anim = Value(true);
					                                                                        Call(Member(context.Global("TASK"), "TASK_PLAY_ANIM"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value("skydive@parachute@first_person"), Value("chute_idle_alt_lookright"), Value(4), Value(1), Negate(Value(1)), Add(Value(1), Value(1048576)), Value(0), Value(false), Value(false), Value(false)});
					                                                                        Call(Member(context.Global("TASK"), "TASK_PLAY_ANIM"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value("missfam5_yoga"), Value("c8_to_start"), Value(4), Value(1), Negate(Value(1)), Add(Add(Add(Add(Value(2), Value(16)), Value(32)), Value(131072)), Value(1048576)), Value(0), Value(false), Value(false), Value(false)});
				                                                                        }
				                                                                        else
				                                                                        {
					                                                                        Call(Member(context.Global("STREAMING"), "REQUEST_ANIM_DICT"), Values{Value("skydive@freefall")});
					                                                                        Call(Member(context.Global("STREAMING"), "REQUEST_ANIM_DICT"), Values{Value("skydive@parachute@first_person")});
					                                                                        Call(Member(context.Global("STREAMING"), "REQUEST_ANIM_DICT"), Values{Value("missfam5_yoga")});
				                                                                        }
				                                                                        if (Truthy(And(*playing_anim, [&] {
					                                                                            return First(Call(Member(context.Global("PAD"), "IS_CONTROL_JUST_PRESSED"), Values{Value(0), Value(150)}));
				                                                                            })))
				                                                                        {
					                                                                        Call(Member(context.Global("TASK"), "TASK_PLAY_ANIM"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value("skydive@freefall"), Value("free_forward"), Value(4.0), Value(1.0), Negate(Value(1)), Add(Value(1), Value(64)), Value(0), Value(false), Value(false), Value(false)});
					                                                                        Call(Member(context.Global("TASK"), "STOP_ANIM_TASK"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value("skydive@parachute@first_person"), Value("chute_idle_alt_lookright"), Value(6.0)});
					                                                                        Call(Member(context.Global("TASK"), "STOP_ANIM_TASK"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value("missfam5_yoga"), Value("c8_to_start"), Value(6.0)});
				                                                                        }
				                                                                        if (Truthy(And(*playing_anim, [&] {
					                                                                            return First(Call(Member(context.Global("PAD"), "IS_CONTROL_JUST_PRESSED"), Values{Value(0), Value(151)}));
				                                                                            })))
				                                                                        {
					                                                                        Call(Member(context.Global("TASK"), "TASK_PLAY_ANIM"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value("skydive@freefall"), Value("free_back"), Value(4.0), Value(1.0), Negate(Value(1)), Add(Value(1), Value(64)), Value(0), Value(false), Value(false), Value(false)});
					                                                                        Call(Member(context.Global("TASK"), "STOP_ANIM_TASK"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value("skydive@parachute@first_person"), Value("chute_idle_alt_lookright"), Value(6.0)});
					                                                                        Call(Member(context.Global("TASK"), "STOP_ANIM_TASK"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value("missfam5_yoga"), Value("c8_to_start"), Value(6.0)});
				                                                                        }
				                                                                        if (Truthy(And(*playing_anim, [&] {
					                                                                            return First(Call(Member(context.Global("PAD"), "IS_CONTROL_JUST_PRESSED"), Values{Value(0), Value(147)}));
				                                                                            })))
				                                                                        {
					                                                                        Call(Member(context.Global("TASK"), "TASK_PLAY_ANIM"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value("skydive@freefall"), Value("free_left"), Value(4.0), Value(1.0), Negate(Value(1)), Add(Value(1), Value(64)), Value(0), Value(false), Value(false), Value(false)});
					                                                                        Call(Member(context.Global("TASK"), "STOP_ANIM_TASK"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value("skydive@parachute@first_person"), Value("chute_idle_alt_lookright"), Value(6.0)});
					                                                                        Call(Member(context.Global("TASK"), "STOP_ANIM_TASK"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value("missfam5_yoga"), Value("c8_to_start"), Value(6.0)});
				                                                                        }
				                                                                        if (Truthy(And(*playing_anim, [&] {
					                                                                            return First(Call(Member(context.Global("PAD"), "IS_CONTROL_JUST_PRESSED"), Values{Value(0), Value(148)}));
				                                                                            })))
				                                                                        {
					                                                                        Call(Member(context.Global("TASK"), "TASK_PLAY_ANIM"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value("skydive@freefall"), Value("free_right"), Value(4.0), Value(1.0), Negate(Value(1)), Add(Value(1), Value(64)), Value(0), Value(false), Value(false), Value(false)});
					                                                                        Call(Member(context.Global("TASK"), "STOP_ANIM_TASK"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value("skydive@parachute@first_person"), Value("chute_idle_alt_lookright"), Value(6.0)});
					                                                                        Call(Member(context.Global("TASK"), "STOP_ANIM_TASK"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value("missfam5_yoga"), Value("c8_to_start"), Value(6.0)});
				                                                                        }
				                                                                        if (Truthy(Or(And(*playing_anim, [&] {
					                                                                            return Or(Or(Or(First(Call(Member(context.Global("PAD"), "IS_CONTROL_JUST_RELEASED"), Values{Value(0), Value(150)})), [&] {
						                                                                            return First(Call(Member(context.Global("PAD"), "IS_CONTROL_JUST_RELEASED"), Values{Value(0), Value(151)}));
					                                                                            }),
					                                                                                          [&] {
						                                                                                          return First(Call(Member(context.Global("PAD"), "IS_CONTROL_JUST_RELEASED"), Values{Value(0), Value(147)}));
					                                                                                          }),
					                                                                                [&] {
						                                                                                return First(Call(Member(context.Global("PAD"), "IS_CONTROL_JUST_RELEASED"), Values{Value(0), Value(148)}));
					                                                                                });
				                                                                            }),
				                                                                                [&] {
					                                                                                return First(Call(Member(context.Global("PAD"), "IS_DISABLED_CONTROL_PRESSED"), Values{Value(0), Value(36)}));
				                                                                                })))
				                                                                        {
					                                                                        Call(Member(context.Global("TASK"), "TASK_PLAY_ANIM"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value("skydive@parachute@first_person"), Value("chute_idle_alt_lookright"), Value(4), Value(1), Negate(Value(1)), Add(Value(1), Value(1048576)), Value(0), Value(false), Value(false), Value(false)});
					                                                                        Call(Member(context.Global("TASK"), "TASK_PLAY_ANIM"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value("missfam5_yoga"), Value("c8_to_start"), Value(4), Value(1), Negate(Value(1)), Add(Add(Add(Add(Value(2), Value(16)), Value(32)), Value(131072)), Value(1048576)), Value(0), Value(false), Value(false), Value(false)});
				                                                                        }
			                                                                        }
			                                                                        else
			                                                                        {
				                                                                        if (Truthy(*playing_anim))
				                                                                        {
					                                                                        Call(Member(context.Global("TASK"), "CLEAR_PED_TASKS"), Append(Values{}, Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})));
					                                                                        *playing_anim = Value(false);
				                                                                        }
			                                                                        }
			                                                                        return {};
		                                                                        })});
		auto PTFXt = MakeCell(MakeTable({{Value(1), MakeTable({{Value(1), Value(1)}, {Value(2), Value("Explosion")}})}, {Value(2), MakeTable({{Value(1), Value(2)}, {Value(2), Value("Electricity")}})}, {Value(3), MakeTable({{Value(1), Value(3)}, {Value(2), Value("Sparkles")}})}, {Value(4), MakeTable({{Value(1), Value(4)}, {Value(2), Value("Blink")}})}, {Value(5), MakeTable({{Value(1), Value(5)}, {Value(2), Value("Poof1")}})}, {Value(6), MakeTable({{Value(1), Value(6)}, {Value(2), Value("Poof2")}})}}));
		auto Particles = MakeCell(Value(false));
		// TransformFX preserves the feature callback and its captured state.
		auto TransformFX = MakeCell();
		*TransformFX = MakeFunction([=, &context](const Values& args) -> Values {
			if (Truthy(*Particles))
			{
				if (Truthy(Equal(context.Global("PTFX"), Value(1))))
				{
					Call(Member(context.Global("GRAPHICS"), "USE_PARTICLE_FX_ASSET"), Values{Value("core")});
					Call(Member(context.Global("GRAPHICS"), "START_PARTICLE_FX_NON_LOOPED_ON_PED_BONE"), Values{Value("exp_grd_grenade_lod"), First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value(0), Value(0), Value(0), Value(0), Value(0), Value(0), Value(11816), Value(1), Value(1), Value(1), Value(1)});
					Call(Member(context.Global("STREAMING"), "REMOVE_PTFX_ASSET"), Values{});
					Call(Member(context.Global("script"), "yield"), Values{Value(100)});
				}
				else if (Truthy(Equal(context.Global("PTFX"), Value(2))))
				{
					Call(Member(context.Global("GRAPHICS"), "USE_PARTICLE_FX_ASSET"), Values{Value("scr_fm_mp_missioncreator")});
					Call(Member(context.Global("GRAPHICS"), "START_PARTICLE_FX_NON_LOOPED_ON_PED_BONE"), Values{Value("scr_mp_elec_dst"), First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value(0), Value(0), Value(0), Value(0), Value(0), Value(0), Value(11816), Value(5), Value(1), Value(1), Value(1)});
					Call(Member(context.Global("STREAMING"), "REMOVE_PTFX_ASSET"), Values{});
					Call(Member(context.Global("script"), "yield"), Values{Value(100)});
				}
				else if (Truthy(Equal(context.Global("PTFX"), Value(3))))
				{
					Call(Member(context.Global("GRAPHICS"), "USE_PARTICLE_FX_ASSET"), Values{Value("proj_indep_firework_v2")});
					Call(Member(context.Global("GRAPHICS"), "START_PARTICLE_FX_NON_LOOPED_ON_PED_BONE"), Values{Value("scr_xmas_firework_sparkle_spawn"), First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value(0), Value(0), Value(0), Value(0), Value(0), Value(0), Value(11816), Value(15), Value(1), Value(1), Value(1)});
					Call(Member(context.Global("STREAMING"), "REMOVE_PTFX_ASSET"), Values{});
					Call(Member(context.Global("script"), "yield"), Values{Value(100)});
				}
				else if (Truthy(Equal(context.Global("PTFX"), Value(4))))
				{
					Call(Member(context.Global("GRAPHICS"), "USE_PARTICLE_FX_ASSET"), Values{Value("scr_indep_fireworks")});
					Call(Member(context.Global("GRAPHICS"), "START_PARTICLE_FX_NON_LOOPED_ON_PED_BONE"), Values{Value("scr_indep_firework_trailburst_spawn"), First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value(0), Value(0), Value(0), Value(0), Value(0), Value(0), Value(11816), Value(10), Value(1), Value(1), Value(1)});
					Call(Member(context.Global("STREAMING"), "REMOVE_PTFX_ASSET"), Values{});
					Call(Member(context.Global("script"), "yield"), Values{Value(100)});
				}
				else if (Truthy(Equal(context.Global("PTFX"), Value(5))))
				{
					Call(Member(context.Global("GRAPHICS"), "USE_PARTICLE_FX_ASSET"), Values{Value("scr_rcbarry2")});
					Call(Member(context.Global("GRAPHICS"), "START_PARTICLE_FX_NON_LOOPED_ON_PED_BONE"), Values{Value("scr_clown_appears"), First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value(0.3), Value(0), Value(0), Value(0), Value(0), Value(0), Value(11816), Value(1), Value(1), Value(1), Value(1)});
					Call(Member(context.Global("STREAMING"), "REMOVE_PTFX_ASSET"), Values{});
					Call(Member(context.Global("script"), "yield"), Values{Value(300)});
				}
				else if (Truthy(Equal(context.Global("PTFX"), Value(6))))
				{
					Call(Member(context.Global("GRAPHICS"), "USE_PARTICLE_FX_ASSET"), Values{Value("scr_powerplay")});
					Call(Member(context.Global("GRAPHICS"), "START_PARTICLE_FX_NON_LOOPED_ON_PED_BONE"), Values{Value("scr_powerplay_beast_appear"), First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value(0.3), Value(0), Value(0), Value(0), Value(0), Value(0), Value(11816), Value(1.3), Value(1), Value(1), Value(1)});
					Call(Member(context.Global("STREAMING"), "REMOVE_PTFX_ASSET"), Values{});
					Call(Member(context.Global("script"), "yield"), Values{Value(300)});
				}
			}
			return {};
		});
		// TinyPlayer preserves the feature callback and its captured state.
		auto TinyPlayer = MakeCell();
		*TinyPlayer = MakeFunction([=, &context](const Values& args) -> Values {
			if (Truthy(Value(!Truthy(First(Call(Member(context.Global("PED"), "GET_PED_CONFIG_FLAG"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value(223), Value(1)}))))))
			{
				if (Truthy(First(Call(Member(context.Global("PAD"), "IS_CONTROL_JUST_PRESSED"), Values{Value(0), Value(73)}))))
				{
					Call(*TransformFX, Values{});
					Call(Member(context.Global("PED"), "SET_PED_CONFIG_FLAG"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value(223), Value(1)});
					Call(Member(context.Global("STREAMING"), "REMOVE_PTFX_ASSET"), Values{});
				}
			}
			else
			{
				if (Truthy(First(Call(Member(context.Global("PAD"), "IS_CONTROL_JUST_PRESSED"), Values{Value(0), Value(73)}))))
				{
					Call(*TransformFX, Values{});
					Call(Member(context.Global("PED"), "SET_PED_CONFIG_FLAG"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value(223), Value(0)});
					Call(Member(context.Global("STREAMING"), "REMOVE_PTFX_ASSET"), Values{});
				}
			}
			return {};
		});
		Call(Member(context.Global("commandmgr"), "add_looped_command"), Values{Value("tinyplayer"), Value("TinyPlayer"), Value("Become Ant-Man?\n[X]"), *TinyPlayer, MakeFunction([=, &context](const Values& args) -> Values {
			                                                                        {
				                                                                        Iterator iterator_28(Append(Values{}, Call(context.Global("ipairs"), Values{MakeTable({{Value(1), Value("core")}, {Value(2), Value("scr_fm_mp_missioncreator")}, {Value(3), Value("proj_indep_firework_v2")}, {Value(4), Value("scr_indep_fireworks")}, {Value(5), Value("scr_rcbarry2")}, {Value(6), Value("scr_powerplay")}})})));
				                                                                        for (auto row_29 = iterator_28.Next(); !row_29.empty() && !row_29[0].IsNil(); row_29 = iterator_28.Next())
				                                                                        {
					                                                                        auto __30 = MakeCell(At(row_29, 0));
					                                                                        auto i_31 = MakeCell(At(row_29, 1));
					                                                                        Call(Member(context.Global("STREAMING"), "REQUEST_NAMED_PTFX_ASSET"), Values{*i_31});
				                                                                        }
			                                                                        }
			                                                                        return {};
		                                                                        }),
		                                                                     MakeFunction([=, &context](const Values& args) -> Values {
			                                                                     {
				                                                                     Iterator iterator_32(Append(Values{}, Call(context.Global("ipairs"), Values{MakeTable({{Value(1), Value("core")}, {Value(2), Value("scr_fm_mp_missioncreator")}, {Value(3), Value("proj_indep_firework_v2")}, {Value(4), Value("scr_indep_fireworks")}, {Value(5), Value("scr_rcbarry2")}, {Value(6), Value("scr_powerplay")}})})));
				                                                                     for (auto row_33 = iterator_32.Next(); !row_33.empty() && !row_33[0].IsNil(); row_33 = iterator_32.Next())
				                                                                     {
					                                                                     auto __34 = MakeCell(At(row_33, 0));
					                                                                     auto i_35 = MakeCell(At(row_33, 1));
					                                                                     Call(Member(context.Global("STREAMING"), "REMOVE_NAMED_PTFX_ASSET"), Values{*i_35});
				                                                                     }
			                                                                     }
			                                                                     return {};
		                                                                     })});
		Call(Member(context.Global("commandmgr"), "add_bool_command"), Values{Value("tinyp_ptfxtoggle"), Value("Particles"), Value("Enables Particle Effects"), Value(false), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                      *Particles = Value(true);
			                                                                      return {};
		                                                                      }),
		                                                                   MakeFunction([=, &context](const Values& args) -> Values {
			                                                                   *Particles = Value(false);
			                                                                   return {};
		                                                                   })});
		Call(Member(context.Global("commandmgr"), "add_list_command"), Values{Value("tinyp_ptfxlist"), Value("Particle effects"), Value(""), *PTFXt, Value(1), Value{}});
		Call(Member(context.Global("commandmgr"), "add_bool_command"), Values{Value("vehicles_blackout"), Value("Vehicles"), Value("Vehicle Lights"), Value(true), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                      Call(Member(context.Global("GRAPHICS"), "SET_ARTIFICIAL_VEHICLE_LIGHTS_STATE"), Values{Value(false)});
			                                                                      return {};
		                                                                      }),
		                                                                   MakeFunction([=, &context](const Values& args) -> Values {
			                                                                   Call(Member(context.Global("GRAPHICS"), "SET_ARTIFICIAL_VEHICLE_LIGHTS_STATE"), Values{Value(true)});
			                                                                   return {};
		                                                                   })});
		Call(Member(context.Global("commandmgr"), "add_bool_command"), Values{Value("citylights_blackout"), Value("Blackout"), Value("Turns the lights off."), Value(false), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                      Call(Member(context.Global("GRAPHICS"), "SET_ARTIFICIAL_LIGHTS_STATE"), Values{Value(true)});
			                                                                      return {};
		                                                                      }),
		                                                                   MakeFunction([=, &context](const Values& args) -> Values {
			                                                                   Call(Member(context.Global("GRAPHICS"), "SET_ARTIFICIAL_LIGHTS_STATE"), Values{Value(false)});
			                                                                   return {};
		                                                                   })});
		Call(Member(context.Global("commandmgr"), "add_bool_command"), Values{Value("hiddenlocs"), Value("Hidden Locations"), Value("Teleport to Hidden/Random Locations."), Value(false), Value{}});
		auto BoxWidth = MakeCell(Value(0.208));
		auto BoxHeight = MakeCell(Value(0.492));
		auto xPos = MakeCell(Value(0.8545));
		auto yPos = MakeCell(Value(0.312));
		auto MM_msg = MakeCell(Value("\n\n~y~      Welcome to\nKiddion's Modest Menu!~s~\n\nThis mod is offered free of charge\nand wihout warranty!\n\nFor any help, please visit the\nModest Menu thread at\n~HC_64~https://unknowncheats.me~s~ and\ncarefully read the FAQ and Known\nIssues.\n\n~r~This is for EDUCATIONAL AND\nEVALUATION PURPOSES ONLY.\nNO responsability is held or\naccepted for misuse.\n~h~Use at your own risk!~s~\n\n"));
		// ModestMenu preserves the feature callback and its captured state.
		auto ModestMenu = MakeCell();
		*ModestMenu = MakeFunction([=, &context](const Values& args) -> Values {
			auto msg = MakeCell(At(args, 0));
			auto x = MakeCell(At(args, 1));
			auto y = MakeCell(At(args, 2));
			Call(Member(context.Global("HUD"), "SET_TEXT_SCALE"), Values{Value(0.0), Value(0.375)});
			Call(Member(context.Global("HUD"), "SET_TEXT_LEADING"), Values{Value(3.0)});
			Call(Member(context.Global("HUD"), "BEGIN_TEXT_COMMAND_DISPLAY_TEXT"), Values{Value("CELL_EMAIL_BCON")});
			{
				const auto start_36 = Number(Value(1));
				const auto end_37 = Number(Length(*msg));
				const auto step_38 = Number(Value(99));
				for (double index_39 = start_36; step_38 > 0 ? index_39 <= end_37 : index_39 >= end_37; index_39 += step_38)
				{
					auto i_40 = MakeCell(Value(index_39));
					Call(Member(context.Global("HUD"), "ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME"), Append(Values{}, Call(Member(context.Global("string"), "sub"), Values{*msg, *i_40, Add(*i_40, Value(98))})));
				}
			}
			Call(Member(context.Global("HUD"), "END_TEXT_COMMAND_DISPLAY_TEXT"), Values{*x, *y, Value(0)});
			Call(Member(context.Global("GRAPHICS"), "REQUEST_STREAMED_TEXTURE_DICT"), Values{Value("shared"), Value(true)});
			return {};
		});
		Invoke(*TS_Main, "imgui", Values{MakeFunction([=, &context](const Values& args) -> Values {
			if (Truthy(Value(!Truthy(First(Call(*IsOnline, Values{}))))))
			{
				Call(Member(context.Global("ImGui"), "TextDisabled"), Values{Value("Please join a freemode session.")});
				return Values{};
			}
			Call(Member(context.Global("ImGui"), "BeginTabBar"), Values{Value("MainCatTab")});
			if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginTabItem"), Values{Value("General")}))))
			{
				if (Truthy(And(Value(!Truthy(context.Global("windowcheck"))), [&] {
					    return First(Call(Member(context.Global("ImGui"), "IsWindowHovered"), Values{}));
				    })))
				{
					*nomoretreeplease = First(Call(Member(context.Global("util"), "time"), Values{}));
					context.Global("windowcheck") = Value(true);
				}
				else if (Truthy(context.Global("windowcheck")))
				{
					if (Truthy(Less(First(Call(Member(context.Global("util"), "time"), Values{})), Add(*nomoretreeplease, Value(5000)))))
					{
						Call(*CenteredText, Values{Value("Original TinkerScripts by ImagineNothing"), Value(true), Value(false)});
					}
				}
				Call(Member(context.Global("ImGui"), "Text"), Values{Value("Self")});
				Call(Member(context.Global("ImGui"), "Separator"), Values{});
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "BeginGroup"), Values{});
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("fastrespawn")})), "draw", Values{});
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("fastreload")})), "draw", Values{});
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("infcombatroll")})), "draw", Values{});
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("ragdoll")})), "draw", Values{});
				Call(Member(context.Global("ImGui"), "EndGroup"), Values{});
				Call(Member(context.Global("ImGui"), "SameLine"), Values{});
				Call(Member(context.Global("ImGui"), "BeginGroup"), Values{});
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("notm25_bst")})), "draw", Values{});
				Call(Member(context.Global("ImGui"), "SameLine"), Values{});
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("noidlecam")})), "draw", Values{});
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("m25_runboost")})), "draw", Values{});
				if (Truthy(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("m25_runboost")})), "get_value", Values{}))))
				{
					Call(Member(context.Global("ImGui"), "PushItemWidth"), Values{Value(80)});
					Call(Member(context.Global("ImGui"), "SameLine"), Values{});
					*m25_yogaRmulti = First(Call(Member(context.Global("ImGui"), "SliderFloat"), Values{Value("Run Mult."), *m25_yogaRmulti, Value(1.15), Value(3.0)}));
					Call(Member(context.Global("ImGui"), "SameLine"), Values{});
					*m25_yogaSmulti = First(Call(Member(context.Global("ImGui"), "SliderFloat"), Values{Value("Swim Mult."), *m25_yogaSmulti, Value(2.0), Value(3.0)}));
					Call(Member(context.Global("ImGui"), "PopItemWidth"), Values{});
					Call(Member(context.Global("tunables"), "set_float"), Values{Negate(Value(1585694365)), *m25_yogaRmulti});
					Call(Member(context.Global("tunables"), "set_float"), Values{Negate(Value(1986470272)), *m25_yogaSmulti});
				}
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("m25_strengthboost")})), "draw", Values{});
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("clearworld")})), "draw", Values{});
				Call(Member(context.Global("ImGui"), "EndGroup"), Values{});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Force Cloud Save")}))))
				{
					Call(Member(context.Global("STATS"), "STAT_SAVE"), Values{Value(0), Value(0), Value(3), Value(0)});
				}
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Teleport into Personal Vehicle")}))))
				{
					Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Value(2640101), Value(8))})), "set_int", Values{Value(1)});
				}
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Teleport into Closest Vehicle")}))))
				{
					Call(*Joyrider, Values{*jr_radius});
				}
				Call(Member(context.Global("ImGui"), "SameLine"), Values{});
				Call(Member(context.Global("ImGui"), "SetNextItemWidth"), Values{Value(100)});
				*jr_radius = First(Call(Member(context.Global("ImGui"), "InputInt"), Values{Value("Teleport Radius"), *jr_radius}));
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "Text"), Values{Value("Auto-Drive")});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
				{
					Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Enables KnoWay Autonomous Vehicles System.")});
				}
				Call(Member(context.Global("ImGui"), "Separator"), Values{});
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "SetNextItemWidth"), Values{Value(130)});
				auto values_41 = Append(Values{}, Call(Member(context.Global("ImGui"), "Combo"), Values{Value("Driving Style"), Member(*VehAutoDrive, "Mode"), Member(*VehAutoDrive, "Label"), Value(3)}));
				Member(*VehAutoDrive, "Mode") = At(values_41, 0);
				context.Global("drivemodechanged") = At(values_41, 1);
				Call(Member(context.Global("ImGui"), "SameLine"), Values{});
				Call(Member(context.Global("ImGui"), "SetNextItemWidth"), Values{Value(100)});
				*AutoDriveSpeed = First(Call(Member(context.Global("ImGui"), "InputInt"), Values{Value("Speed##wander"), *AutoDriveSpeed}));
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Auto-Drive to Waypoint")}))))
				{
					Call(Member(context.Global("script"), "run_in_callback"), Values{*SelfTaxi});
				}
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Auto-Wander")}))))
				{
					Call(Member(context.Global("script"), "run_in_callback"), Values{*Wander});
				}
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "Text"), Values{Value("Re-Name Editor")});
				Call(Member(context.Global("ImGui"), "Separator"), Values{});
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				if (Truthy(Equal(*OR_ptr, Value{})))
				{
					*OR_ptr = First(Invoke(First(Call(Member(context.Global("memory"), "scan_pattern"), Values{Value("40 5D ? ? ? ? 00 00 90 5D ? ? ? ? 00 00 00 00 00 00 00 00 00 00 00")})), "deref", Values{}));
				}
				Call(Member(context.Global("ImGui"), "AlignTextToFramePadding"), Values{});
				if (Truthy(*renamemode))
				{
					Call(Member(context.Global("ImGui"), "Text"), Values{Value("Mode: Business Renamer")});
				}
				else
				{
					Call(Member(context.Global("ImGui"), "Text"), Values{Value("Mode: Outfit Renamer")});
				}
				Call(Member(context.Global("ImGui"), "SameLine"), Values{});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Switch Mode")}))))
				{
					*renamemode = Value(!Truthy(*renamemode));
					*New_Name = Value("");
				}
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				if (Truthy(*renamemode))
				{
					if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginListBox"), Values{Concat(Value("##renprops"), *nomoretreeplease), Value(400), Value(232)}))))
					{
						{
							Iterator iterator_42(Append(Values{}, Call(context.Global("ipairs"), Values{*ReN_Properties})));
							for (auto row_43 = iterator_42.Next(); !row_43.empty() && !row_43[0].IsNil(); row_43 = iterator_42.Next())
							{
								auto i_44 = MakeCell(At(row_43, 0));
								auto item = MakeCell(At(row_43, 1));
								auto is_selected = MakeCell(Equal(context.Global("selected_prop"), *i_44));
								if (Truthy(And(First(Call(Member(context.Global("ImGui"), "Selectable"), Values{Concat(Member(*item, "renProperty"), Concat(Value(": "), Concat(First(Call(Member(context.Global("stats"), "get_string"), Values{Member(*item, "renPropStat")})), First(Call(Member(context.Global("stats"), "get_string"), Values{Member(*item, "renPropStat2")}))))), *is_selected})), [&] {
									    return NotEqual(context.Global("selected_prop"), *i_44);
								    })))
								{
									context.Global("selected_prop") = *i_44;
									*New_Name = Concat(First(Call(Member(context.Global("stats"), "get_string"), Values{Member(*item, "renPropStat")})), First(Call(Member(context.Global("stats"), "get_string"), Values{Member(*item, "renPropStat2")})));
									*ReN_selectedprop = *item;
								}
							}
						}
						Call(Member(context.Global("ImGui"), "EndListBox"), Values{});
						Call(Member(context.Global("ImGui"), "SetNextItemWidth"), Values{Value(400)});
						*New_Name = First(Call(Member(context.Global("ImGui"), "InputTextWithHint"), Values{Value("##newrename"), Value("New Organization/Acid Lab/Pet Name"), *New_Name}));
						Call(Member(context.Global("ImGui"), "SameLine"), Values{});
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Re-Name")}))))
						{
							Call(Member(context.Global("stats"), "set_string"), Append(Values{Member(*ReN_selectedprop, "renPropStat")}, Invoke(*New_Name, "sub", Values{Value(1), Value(10)})));
							Call(Member(context.Global("stats"), "set_string"), Append(Values{Member(*ReN_selectedprop, "renPropStat2")}, Invoke(*New_Name, "sub", Values{Value(11), Value(20)})));
						}
					}
					Call(Member(context.Global("ImGui"), "SameLine"), Values{});
					*ibox_checkbox = First(Call(Member(context.Global("ImGui"), "Checkbox"), Values{Value("Input Box Override"), *ibox_checkbox}));
					if (Truthy(*ibox_checkbox))
					{
						Call(Member(context.Global("ImGui"), "SetNextItemWidth"), Values{Value(400)});
						*New_Name = First(Call(Member(context.Global("ImGui"), "InputTextWithHint"), Values{Value("##iboxoverride"), Value("New Input Box Text"), *New_Name}));
						Call(Member(context.Global("ImGui"), "SameLine"), Values{});
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Override")}))))
						{
							Call(Member(context.Global("script"), "run_in_callback"), Values{*ReNameEditor});
						}
						if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
						{
							Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("R* fixed input box filters. Consider this DEPRECATED.")});
						}
					}
				}
				else
				{
					if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginListBox"), Values{Concat(Value("##outfitslots"), *nomoretreeplease), Value(400), Value(232)}))))
					{
						{
							const auto start_45 = Number(Value(1));
							const auto end_46 = Number(Value(20));
							const auto step_47 = Number(Value(1));
							for (double index_48 = start_45; step_47 > 0 ? index_48 <= end_46 : index_48 >= end_46; index_48 += step_47)
							{
								auto i_49 = MakeCell(Value(index_48));
								auto outfitname = MakeCell(First(Invoke(First(Invoke(*OR_ptr, "add", Values{Member(*OR_offsets, *i_49)})), "get_string", Values{})));
								auto is_selected_50 = MakeCell(Equal(*OR_selected, *i_49));
								if (Truthy(And(First(Call(Member(context.Global("ImGui"), "Selectable"), Values{Concat(*i_49, Concat(Value(". "), *outfitname)), *is_selected_50})), [&] {
									    return NotEqual(*OR_selected, *i_49);
								    })))
								{
									*OR_selected = *i_49;
									*New_Name = *outfitname;
								}
							}
						}
						Call(Member(context.Global("ImGui"), "EndListBox"), Values{});
					}
					Call(Member(context.Global("ImGui"), "SetNextItemWidth"), Values{Value(400)});
					*New_Name = First(Call(Member(context.Global("ImGui"), "InputTextWithHint"), Values{Value("##outfitname"), Value("~ws~~r~¦~s~New Name~r~¦~ws~"), *New_Name}));
					Call(Member(context.Global("ImGui"), "SameLine"), Values{});
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Apply##OR")}))))
					{
						if (Truthy(And(NotEqual(*New_Name, Value("")), [&] {
							    return Greater(*OR_selected, Value(0));
						    })))
						{
							Invoke(First(Invoke(*OR_ptr, "add", Values{Member(*OR_offsets, *OR_selected)})), "set_string", Append(Values{}, Invoke(*New_Name, "sub", Values{Value(1), Value(31)})));
							Call(Member(context.Global("notify"), "success"), Values{Value("Outfit Renamer"), Concat(Value("Slot "), Concat(*OR_selected, Value(" renamed!"))), Value(3000)});
							*New_Name = Value("");
						}
						else if (Truthy(And(Or(NotEqual(*New_Name, Value("")), [&] {
							         return Equal(*New_Name, Value(""));
						         }),
						             [&] {
							             return LessEqual(*OR_selected, Value(0));
						             })))
						{
							Call(Member(context.Global("notify"), "error"), Values{Value("TinkerScripts - Re-Name"), Value("Select something first!"), Value(3000)});
						}
						else if (Truthy(And(Equal(*New_Name, Value("")), [&] {
							         return Greater(*OR_selected, Value(0));
						         })))
						{
							Call(Member(context.Global("notify"), "error"), Values{Value("TinkerScripts - Re-Name"), Value("Name is empty!\nPlease enter a name."), Value(3000)});
						}
					}
					Call(Member(context.Global("ImGui"), "SameLine"), Values{});
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Copy")}))))
					{
						if (Truthy(Greater(*OR_selected, Value(0))))
						{
							auto copyoutfitname = MakeCell(First(Invoke(First(Invoke(*OR_ptr, "add", Values{Member(*OR_offsets, *OR_selected)})), "get_string", Values{})));
							Call(Member(context.Global("ImGui"), "SetClipboardText"), Values{*copyoutfitname});
							Call(Member(context.Global("notify"), "info"), Values{Value("Outfit Renamer"), Concat(Value("Copied: "), *copyoutfitname), Value(3000)});
						}
						else
						{
							Call(Member(context.Global("notify"), "error"), Values{Value("TinkerScripts - Re-Name"), Value("Select something first!"), Value(3000)});
						}
					}
					Call(Member(context.Global("ImGui"), "SameLine"), Values{});
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Copy All")}))))
					{
						auto AllOutfitNames = MakeCell(MakeTable({}));
						{
							const auto start_51 = Number(Value(1));
							const auto end_52 = Number(Value(20));
							const auto step_53 = Number(Value(1));
							for (double index_54 = start_51; step_53 > 0 ? index_54 <= end_52 : index_54 >= end_52; index_54 += step_53)
							{
								auto i_55 = MakeCell(Value(index_54));
								auto outfitname_56 = MakeCell(Or(First(Invoke(First(Invoke(*OR_ptr, "add", Values{Member(*OR_offsets, *i_55)})), "get_string", Values{})), [&] {
									return Value("(empty)");
								}));
								Call(Member(context.Global("table"), "insert"), Values{*AllOutfitNames, Concat(*i_55, Concat(Value(". "), *outfitname_56))});
							}
						}
						Call(Member(context.Global("ImGui"), "SetClipboardText"), Append(Values{}, Call(Member(context.Global("table"), "concat"), Values{*AllOutfitNames, Value("\n")})));
						Call(Member(context.Global("notify"), "info"), Values{Value("Outfit Renamer"), Value("All outfit names copied to clipboard!"), Value(3000)});
					}
				}
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(3.8)});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "TreeNodeEx"), Values{Concat(Value("Text Formatting  ##"), *nomoretreeplease), Value(8219)}))))
				{
					Call(Member(context.Global("ImGui"), "BeginGroup"), Values{});
					{
						Iterator iterator_57(Append(Values{}, Call(context.Global("ipairs"), Values{*ReN_Style})));
						for (auto row_58 = iterator_57.Next(); !row_58.empty() && !row_58[0].IsNil(); row_58 = iterator_57.Next())
						{
							auto i_59 = MakeCell(At(row_58, 0));
							auto textformat = MakeCell(At(row_58, 1));
							if (Truthy(Equal(Modulo(*i_59, Value(2)), Value(0))))
							{
								Call(Member(context.Global("ImGui"), "SameLine"), Values{});
							}
							if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Member(*textformat, "tbLabel"), Value(100)}))))
							{
								if (Truthy(Or(NotEqual(context.Global("selected_prop"), Value{}), [&] {
									    return Greater(*OR_selected, Value(0));
								    })))
								{
									*New_Name = Concat(*New_Name, Member(*textformat, "tStyle"));
								}
								else
								{
									Call(Member(context.Global("notify"), "error"), Values{Value("TinkerScripts - Re-Name"), Value("Select something first!"), Value(3000)});
								}
							}
							if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
							{
								Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Member(*textformat, "tStyle")});
							}
						}
					}
					Call(Member(context.Global("ImGui"), "EndGroup"), Values{});
					Call(Member(context.Global("ImGui"), "SameLine"), Values{});
					Call(Member(context.Global("ImGui"), "BeginGroup"), Values{});
					Call(Member(context.Global("ImGui"), "InputTextMultiline"), Values{Value("##Colors & Symbols"), Value("\nUse HTML/Hex if symbols don't work.\n\nR* Logo:    \t      &#8721;   |   &#x2211;\nR* Verified:\t     &#166;     |   &#xa6;\nR* Created: \t   &#8249;   |   &#x2039;\nLock Icon:           &#937;     |   &#x3a9;\nBlank Icon:         &#8250;   |   &#x203a;"), Value(280), Value(142), Member(context.Global("ImGuiInputTextFlags"), "ReadOnly")});
					Call(Member(context.Global("ImGui"), "EndGroup"), Values{});
					Call(Member(context.Global("ImGui"), "Spacing"), Values{});
					Call(Member(context.Global("ImGui"), "BeginGroup"), Values{});
					Call(Member(context.Global("ImGui"), "Text"), Values{Value("Colors")});
					{
						Iterator iterator_60(Append(Values{}, Call(context.Global("ipairs"), Values{*ReN_Color})));
						for (auto row_61 = iterator_60.Next(); !row_61.empty() && !row_61[0].IsNil(); row_61 = iterator_60.Next())
						{
							auto i_62 = MakeCell(At(row_61, 0));
							auto textcolor = MakeCell(At(row_61, 1));
							if (Truthy(NotEqual(Modulo(Subtract(*i_62, Value(1)), Value(4)), Value(0))))
							{
								Call(Member(context.Global("ImGui"), "SameLine"), Values{});
							}
							if (Truthy(First(Call(Member(context.Global("ImGui"), "ColorButton"), Values{Member(*textcolor, "bName"), Member(*textcolor, "bCol"), Value(64)}))))
							{
								if (Truthy(Or(NotEqual(context.Global("selected_prop"), Value{}), [&] {
									    return Greater(*OR_selected, Value(0));
								    })))
								{
									*New_Name = Concat(*New_Name, Member(*textcolor, "tColf"));
								}
								else
								{
									Call(Member(context.Global("notify"), "error"), Values{Value("TinkerScripts - Re-Name"), Value("Select something first!"), Value(3000)});
								}
							}
							if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
							{
								Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Concat(Member(*textcolor, "bTip"), Concat(Value(": "), Member(*textcolor, "tColf")))});
							}
						}
					}
					Call(Member(context.Global("ImGui"), "EndGroup"), Values{});
					Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				}
				Call(Member(context.Global("ImGui"), "EndTabItem"), Values{});
			}
			if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginTabItem"), Values{Value("Misc")}))))
			{
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("DuffelBagSave")})), "draw", Values{});
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("noclip_anim")})), "draw", Values{});
				Call(Member(context.Global("ImGui"), "SameLine"), Values{});
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("noclip_onscreen")})), "draw", Values{});
				if (Truthy(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("noclip_onscreen")})), "get_value", Values{}))))
				{
					Call(Member(context.Global("ImGui"), "SameLine"), Values{});
					*NoClipWindowTop = First(Call(Member(context.Global("ImGui"), "Checkbox"), Values{Value("Top"), *NoClipWindowTop}));
				}
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("rapidoxidation")})), "draw", Values{});
				if (Truthy(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("rapidoxidation")})), "get_value", Values{}))))
				{
					Call(Member(context.Global("ImGui"), "SameLine"), Values{});
					Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("phoenixnoclip")})), "draw", Values{});
				}
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("tinyplayer")})), "draw", Values{});
				if (Truthy(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("tinyplayer")})), "get_value", Values{}))))
				{
					Call(Member(context.Global("ImGui"), "SameLine"), Values{});
					Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("tinyp_ptfxtoggle")})), "draw", Values{});
					if (Truthy(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("tinyp_ptfxtoggle")})), "get_value", Values{}))))
					{
						Call(Member(context.Global("ImGui"), "SameLine"), Values{});
						Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("tinyp_ptfxlist")})), "draw", Values{});
					}
				}
				context.Global("PTFX") = First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("tinyp_ptfxlist")})), "get_value", Values{}));
				Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(200)});
				Call(Member(context.Global("ImGui"), "PushStyleColor"), Values{Member(context.Global("ImGuiCol"), "Button"), Value(0), Value(0), Value(0), Value(0)});
				Call(Member(context.Global("ImGui"), "PushStyleColor"), Values{Member(context.Global("ImGuiCol"), "ButtonHovered"), Value(0), Value(0.5), Value(1), Value(0.05)});
				Call(Member(context.Global("ImGui"), "PushStyleColor"), Values{Member(context.Global("ImGuiCol"), "ButtonActive"), Value(0), Value(0), Value(0), Value(0)});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value(" ##ModestMenuWelcomeMsg")}))))
				{
					Call(Member(context.Global("script"), "run_in_callback"), Values{MakeFunction([=, &context](const Values& args) -> Values {
						{
							const auto start_63 = Number(Value(1));
							const auto end_64 = Number(Multiply(Value(60), Value(9)));
							const auto step_65 = Number(Value(1));
							for (double index_66 = start_63; step_65 > 0 ? index_66 <= end_64 : index_66 >= end_64; index_66 += step_65)
							{
								auto i_67 = MakeCell(Value(index_66));
								Call(Member(context.Global("GRAPHICS"), "DRAW_RECT"), Values{*xPos, *yPos, *BoxWidth, *BoxHeight, Value(0), Value(0), Value(0), Value(200), Value(1)});
								Call(Member(context.Global("GRAPHICS"), "DRAW_SPRITE"), Values{Value("shared"), Value("info_icon_32"), Value(0.763), Value(0.09), Value(0.02), Value(0.036), Value(0.0), Value(255), Value(255), Value(255), Value(255), Value(0), Value(0)});
								Call(*ModestMenu, Values{*MM_msg, Value(0.756), Value(0.048)});
								Call(Member(context.Global("script"), "yield"), Values{Value(0)});
							}
						}
						return {};
					})});
				}
				if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
				{
					Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Triggers Nostalgia")});
				}
				Call(Member(context.Global("ImGui"), "PopStyleColor"), Values{});
				Call(Member(context.Global("ImGui"), "PopStyleColor"), Values{});
				Call(Member(context.Global("ImGui"), "PopStyleColor"), Values{});
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("citylights_blackout")})), "draw", Values{});
				if (Truthy(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("citylights_blackout")})), "get_value", Values{}))))
				{
					Call(Member(context.Global("ImGui"), "SameLine"), Values{});
					Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("vehicles_blackout")})), "draw", Values{});
				}
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("hiddenlocs")})), "draw", Values{});
				if (Truthy(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("hiddenlocs")})), "get_value", Values{}))))
				{
					Call(context.Global("CurPlayerCoords"), Values{});
					Call(Member(context.Global("ImGui"), "SetNextItemWidth"), Values{Value(240)});
					Call(Member(context.Global("ImGui"), "InputText"), Values{Value("Current Coords"), First(Call(Member(context.Global("string"), "format"), Values{Value("x: %.2f,   y: %.2f,   z: %.2f"), Member(context.Global("PlaCoords"), "x"), Member(context.Global("PlaCoords"), "y"), Member(context.Global("PlaCoords"), "z")})), Member(context.Global("ImGuiInputTextFlags"), "ReadOnly")});
					Call(Member(context.Global("ImGui"), "SameLine"), Values{});
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Copy To Clipboard")}))))
					{
						Call(Member(context.Global("ImGui"), "SetClipboardText"), Append(Values{}, Call(Member(context.Global("string"), "format"), Values{Value("%.2f, %.2f, %.2f"), Member(context.Global("PlaCoords"), "x"), Member(context.Global("PlaCoords"), "y"), Member(context.Global("PlaCoords"), "z")})));
						Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Hidden Locations"), Value("Current Coordinates copied to clipboard!"), Value(3000)});
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginListBox"), Values{Concat(Value("##hiddenlocsbox"), *nomoretreeplease), Value(354), Value(232)}))))
					{
						{
							Iterator iterator_68(Append(Values{}, Call(context.Global("ipairs"), Values{*LocationCoords})));
							for (auto row_69 = iterator_68.Next(); !row_69.empty() && !row_69[0].IsNil(); row_69 = iterator_68.Next())
							{
								auto __70 = MakeCell(At(row_69, 0));
								auto LocServ = MakeCell(At(row_69, 1));
								if (Truthy(First(Call(Member(context.Global("ImGui"), "Selectable"), Values{Member(*LocServ, "LocName")}))))
								{
									Call(Member(context.Global("PED"), "SET_PED_COORDS_KEEP_VEHICLE"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Member(Member(*LocServ, "LocCoords"), Value(1)), Member(Member(*LocServ, "LocCoords"), Value(2)), Member(Member(*LocServ, "LocCoords"), Value(3))});
								}
								if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
								{
									Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Member(*LocServ, "LocDesc")});
								}
							}
						}
						Call(Member(context.Global("ImGui"), "EndListBox"), Values{});
					}
				}
				Call(Member(context.Global("ImGui"), "EndTabItem"), Values{});
			}
			Call(Member(context.Global("ImGui"), "EndTabBar"), Values{});
			return {};
		})});
		auto HGgoodtypeT = MakeCell(MakeTable({{Value(1), MakeTable({{Value(1), Value(0)}, {Value(2), Value("Animal Materials")}})}, {Value(2), MakeTable({{Value(1), Value(1)}, {Value(2), Value("Art & Antiques")}})}, {Value(3), MakeTable({{Value(1), Value(2)}, {Value(2), Value("Chemicals")}})}, {Value(4), MakeTable({{Value(1), Value(3)}, {Value(2), Value("Counterfeit Goods")}})}, {Value(5), MakeTable({{Value(1), Value(4)}, {Value(2), Value("Jewelry & Gemstones")}})}, {Value(6), MakeTable({{Value(1), Value(5)}, {Value(2), Value("Medical Supplies")}})}, {Value(7), MakeTable({{Value(1), Value(6)}, {Value(2), Value("Narcotics ")}})}, {Value(8), MakeTable({{Value(1), Value(7)}, {Value(2), Value("Tobacco & Alcohol")}})}}));
		auto WHgoodtypeT = MakeCell(MakeTable({{Value(1), MakeTable({{Value(1), Value(0)}, {Value(2), Value("Medical Supplies")}})}, {Value(2), MakeTable({{Value(1), Value(1)}, {Value(2), Value("Tobacco & Alcohol")}})}, {Value(3), MakeTable({{Value(1), Value(2)}, {Value(2), Value("Art & Antiques")}})}, {Value(4), MakeTable({{Value(1), Value(3)}, {Value(2), Value("Electronic Goods")}})}, {Value(5), MakeTable({{Value(1), Value(4)}, {Value(2), Value("Weapons & Ammo")}})}, {Value(6), MakeTable({{Value(1), Value(5)}, {Value(2), Value("Narcotics")}})}, {Value(7), MakeTable({{Value(1), Value(6)}, {Value(2), Value("Gemstones")}})}, {Value(8), MakeTable({{Value(1), Value(7)}, {Value(2), Value("Animal Materials")}})}, {Value(9), MakeTable({{Value(1), Value(8)}, {Value(2), Value("Counterfeit Goods")}})}, {Value(10), MakeTable({{Value(1), Value(9)}, {Value(2), Value("Jewelry")}})}, {Value(11), MakeTable({{Value(1), Value(10)}, {Value(2), Value("Bullion")}})}}));
		auto PropGlob = MakeCell(MakeTable({{"whprop", Add(Add(Value(1845347), Value(260)), Value(128))}, {"hangarprop", Add(Add(Value(1845347), Value(260)), Value(304))}, {"bbizprop", Add(Add(Value(1845347), Value(260)), Value(205))}, {"ncprop", Add(Add(Value(1845347), Value(260)), Value(364))}, {"syprop", Add(Add(Value(1845347), Value(260)), Value(504))}, {"mcbar", Add(Add(Value(1845347), Value(260)), Value(394))}, {"ArraySize", Value(884)}, {"Safes", MakeTable({{Value(1), MakeTable({{"Property", Value("Nightclub")}, {"Owned", Value("MPX_NIGHTCLUB_OWNED")}, {"SafeVal", Value("MPX_CLUB_SAFE_CASH_VALUE")}, {"Earnings", Value(2708943)}})}, {Value(2), MakeTable({{"Property", Value("Arcade")}, {"Owned", Value("MPX_ARCADE_OWNED")}, {"SafeVal", Value("MPX_ARCADE_SAFE_CASH_VALUE")}, {"Earnings", Value(2708952)}})}, {Value(3), MakeTable({{"Property", Value("Agency")}, {"Owned", Value("MPX_FIXER_HQ_OWNED")}, {"SafeVal", Value("MPX_FIXER_SAFE_CASH_VALUE")}, {"Earnings", Value(2708961)}})}, {Value(4), MakeTable({{"Property", Value("Salvage Yard")}, {"Owned", Value("MPX_SALVAGE_YARD_OWNED")}, {"SafeVal", Value("MPX_SALVAGE_SAFE_CASH_VALUE")}, {"Earnings", Value(2708970)}})}, {Value(5), MakeTable({{"Property", Value("Bail Office")}, {"Owned", Value("MPX_BAIL_OFFICE_OWNED")}, {"SafeVal", Value("MPX_BAIL_SAFE_CASH_VALUE")}, {"Earnings", Value(2708979)}})}, {Value(6), MakeTable({{"Property", Value("Garment Factory")}, {"Owned", Value("MPX_HACKER_DEN_OWNED")}, {"SafeVal", Value("MPX_HDEN24_SAFE_CASH_VALUE")}, {"Earnings", Value(2708994)}})}, {Value(7), MakeTable({{"Property", Value("Car Wash")}, {"Owned", Value("MPX_SB_CAR_WASH_OWNED")}, {"SafeVal", Value("MPX_CWASH_SAFE_CASH_VALUE")}, {"Earnings", Value(2709001)}})}})}}));
		auto BizRe_RestockSU = MakeCell(MakeTable({{"methmc", MakeTable({{"manuprod", MakeTable({{Value(1), Value(1370024930)}, {Value(2), Value(1944848251)}, {Value(3), Value(1577999189)}, {Value(4), Value(1678460062)}})}, {"manucost", MakeTable({{Value(1), Negate(Value(730135062))}, {Value(2), Negate(Value(660914094))}})}})}, {"weedmc", MakeTable({{"manuprod", MakeTable({{Value(1), Negate(Value(635596193))}, {Value(2), Negate(Value(1694873660))}, {Value(3), Value(1575359233)}, {Value(4), Value(102029883)}})}, {"manucost", MakeTable({{Value(1), Negate(Value(373027461))}, {Value(2), Value(1195564032)}})}})}, {"crackmc", MakeTable({{"manuprod", MakeTable({{Value(1), Value(702413484)}, {Value(2), Value(2070857577)}, {Value(3), Negate(Value(1539796661))}, {Value(4), Value(396217128)}})}, {"manucost", MakeTable({{Value(1), Negate(Value(161187879))}, {Value(2), Value(1500658261)}})}})}, {"cashmc", MakeTable({{"manuprod", MakeTable({{Value(1), Value(1310272402)}, {Value(2), Value(1690071006)}, {Value(3), Negate(Value(1454958662))}, {Value(4), Negate(Value(1913260493))}})}, {"manucost", MakeTable({{Value(1), Value(631857857)}, {Value(2), Negate(Value(891680742))}})}})}, {"fakeidmc", MakeTable({{"manuprod", MakeTable({{Value(1), Negate(Value(959721585))}, {Value(2), Value(1672482518)}, {Value(3), Negate(Value(518264160))}, {Value(4), Value(489023341)}})}, {"manucost", MakeTable({{Value(1), Negate(Value(1839004359))}, {Value(2), Negate(Value(192060672))}})}})}, {"bunkermc", MakeTable({{"manuprod", MakeTable({{Value(1), Value(215868155)}, {Value(2), Value(631477612)}, {Value(3), Value(818645907)}})}, {"manucost", MakeTable({{Value(1), Negate(Value(1652502760))}, {Value(2), Value(1647327744)}})}})}, {"acidmc", MakeTable({{"manuprod", MakeTable({{Value(1), Negate(Value(672998848))}, {Value(2), Value(494316332)}, {Value(3), Negate(Value(40235252))}})}, {"manucost", MakeTable({{Value(1), Negate(Value(1506354854))}, {Value(2), Negate(Value(993236072))}})}})}, {"nightclub", MakeTable({{"manuprod", MakeTable({{Value(1), Negate(Value(147565853))}, {Value(2), Negate(Value(1390027611))}, {Value(3), Negate(Value(1292210552))}, {Value(4), Value(1007184806)}, {Value(5), Value(18969287)}, {Value(6), Negate(Value(863328938))}, {Value(7), Value(1607981264)}})}})}}));
		// OpenMCT preserves the feature callback and its captured state.
		auto OpenMCT = MakeCell();
		*OpenMCT = MakeFunction([=, &context](const Values& args) -> Values {
			if (Truthy(NotEqual(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Value(1951071)})), "get_int", Values{})), Value(0))))
			{
				Invoke(First(Call(context.Global("ScriptGlobal"), Values{Value(1951071)})), "set_int", Values{Value(0)});
			}
			Call(Member(context.Global("scripts"), "start_new_script"), Values{Value("apparcadebusinesshub"), Value(1424)});
			Call(Member(context.Global("script"), "run_in_callback"), Values{MakeFunction([=, &context](const Values& args) -> Values {
				while (Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("apparcadebusinesshub")}))))
				{
					if (Truthy(Equal(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Value(1971195)})), "get_int", Values{})), Negate(Value(1)))))
					{
						Invoke(First(Call(context.Global("ScriptGlobal"), Values{Value(1971195)})), "set_int", Values{Value(0)});
					}
					Call(Member(context.Global("script"), "yield"), Values{Value(0)});
				}
				return {};
			})});
			return {};
		});
		auto BizRe_running = MakeCell(Value(false));
		auto BizRe_HGonly = MakeCell(Value(false));
		auto BizRe_WHonly = MakeCell(Value(false));
		auto BizRe_cancel = MakeCell(Value(true));
		// BizRe_lotoggle preserves the feature callback and its captured state.
		auto BizRe_lotoggle = MakeCell();
		*BizRe_lotoggle = MakeFunction([=, &context](const Values& args) -> Values {
			if (Truthy(*BizRe_cancel))
			{
				Call(Member(context.Global("notify"), "warn"), Values{Value("TinkerScripts - Biz-Teroids"), Value("Logic Canceled."), Value(3000)});
				*BizRe_running = Value(false);
				Call(Member(context.Global("STATS"), "STAT_SAVE"), Values{Value(0), Value(0), Value(3), Value(0)});
			}
			return Values{*BizRe_cancel};
			return {};
		});
		auto MCbizlocs = MakeCell(([&] {
			auto table_71 = MakeTable();
			Member(table_71, Value(1)) = Value("Paleto Bay");
			Member(table_71, Value(6)) = Value("El Burro Heights");
			Member(table_71, Value(11)) = Value("Gran Senora Desert");
			Member(table_71, Value(16)) = Value("Terminal");
			Member(table_71, Value(2)) = Value("Mount Chiliad");
			Member(table_71, Value(7)) = Value("Downtown Vinewood");
			Member(table_71, Value(12)) = Value("San Chianski Mountain Range");
			Member(table_71, Value(17)) = Value("Elysian Island");
			Member(table_71, Value(3)) = Value("Paleto Bay");
			Member(table_71, Value(8)) = Value("Morningwood");
			Member(table_71, Value(13)) = Value("Alamo Sea");
			Member(table_71, Value(18)) = Value("Elysian Island");
			Member(table_71, Value(4)) = Value("Paleto Bay");
			Member(table_71, Value(9)) = Value("Vespucci Canals");
			Member(table_71, Value(14)) = Value("Gran Senora Desert");
			Member(table_71, Value(19)) = Value("Cypress Flats");
			Member(table_71, Value(5)) = Value("Paleto Bay");
			Member(table_71, Value(10)) = Value("Textile City");
			Member(table_71, Value(15)) = Value("Grapeseed");
			Member(table_71, Value(20)) = Value("Elysian Island");
			Member(table_71, Value(21)) = Value("Grand Senora Oilfields");
			Member(table_71, Value(22)) = Value("Grand Senora Desert");
			Member(table_71, Value(23)) = Value("Route 68");
			Member(table_71, Value(24)) = Value("Farmhouse");
			Member(table_71, Value(25)) = Value("Smoke Tree Road");
			Member(table_71, Value(26)) = Value("Thomson Scrapyard");
			Member(table_71, Value(27)) = Value("Grapeseed");
			Member(table_71, Value(28)) = Value("Paleto Forest");
			Member(table_71, Value(29)) = Value("Raton Canyon");
			Member(table_71, Value(30)) = Value("Lago Zancudo");
			Member(table_71, Value(31)) = Value("Chumash");
			return table_71;
		}()));
		auto MCbiz = MakeCell(MakeTable({{Value(1), MakeTable({{"MCBname", Value("Methamphetamine Lab")}, {"ID", MakeTable({{Value(1), Value(1)}, {Value(2), Value(6)}, {Value(3), Value(11)}, {Value(4), Value(16)}})}, {"Blip", Member(Member(Member(context.Global("Blips"), "Property"), "PRODUCTION_METH"), "ID")}})}, {Value(2), MakeTable({{"MCBname", Value("Weed Farm")}, {"ID", MakeTable({{Value(1), Value(2)}, {Value(2), Value(7)}, {Value(3), Value(12)}, {Value(4), Value(17)}})}, {"Blip", Member(Member(Member(context.Global("Blips"), "Property"), "PRODUCTION_WEED"), "ID")}})}, {Value(3), MakeTable({{"MCBname", Value("Cocaine Lockup")}, {"ID", MakeTable({{Value(1), Value(3)}, {Value(2), Value(8)}, {Value(3), Value(13)}, {Value(4), Value(18)}})}, {"Blip", Member(Member(Member(context.Global("Blips"), "Property"), "PRODUCTION_CRACK"), "ID")}})}, {Value(4), MakeTable({{"MCBname", Value("Counterfeit Cash Factory")}, {"ID", MakeTable({{Value(1), Value(4)}, {Value(2), Value(9)}, {Value(3), Value(14)}, {Value(4), Value(19)}})}, {"Blip", Member(Member(Member(context.Global("Blips"), "Property"), "PRODUCTION_MONEY"), "ID")}})}, {Value(5), MakeTable({{"MCBname", Value("Document Forgery Office")}, {"ID", MakeTable({{Value(1), Value(5)}, {Value(2), Value(10)}, {Value(3), Value(15)}, {Value(4), Value(20)}})}, {"Blip", Member(Member(Member(context.Global("Blips"), "Property"), "PRODUCTION_FAKE_ID"), "ID")}})}, {Value(6), MakeTable({{"MCBname", Value("Bunker")}, {"ID", MakeTable({{Value(1), Value(21)}, {Value(2), Value(22)}, {Value(3), Value(23)}, {Value(4), Value(24)}, {Value(5), Value(25)}, {Value(6), Value(26)}, {Value(7), Value(27)}, {Value(8), Value(28)}, {Value(9), Value(29)}, {Value(10), Value(30)}, {Value(11), Value(31)}})}, {"Blip", Member(Member(Member(context.Global("Blips"), "Property"), "PROPERTY_BUNKER"), "ID")}})}}));
		// GetBusinessSlot preserves the feature callback and its captured state.
		auto GetBusinessSlot = MakeCell();
		*GetBusinessSlot = MakeFunction([=, &context](const Values& args) -> Values {
			auto businessName = MakeCell(At(args, 0));
			auto Blog = MakeCell(At(args, 1));
			{
				Iterator iterator_72(Append(Values{}, Call(context.Global("ipairs"), Values{*MCbiz})));
				for (auto row_73 = iterator_72.Next(); !row_73.empty() && !row_73[0].IsNil(); row_73 = iterator_72.Next())
				{
					auto __74 = MakeCell(At(row_73, 0));
					auto business = MakeCell(At(row_73, 1));
					if (Truthy(Equal(Member(*business, "MCBname"), *businessName)))
					{
						{
							const auto start_75 = Number(Value(0));
							const auto end_76 = Number(Value(5));
							const auto step_77 = Number(Value(1));
							for (double index_78 = start_75; step_77 > 0 ? index_78 <= end_76 : index_78 >= end_76; index_78 += step_77)
							{
								auto i_79 = MakeCell(Value(index_78));
								auto slot = MakeCell(First(Call(Member(context.Global("stats"), "get_int"), Values{Concat(Value("MPX_FACTORYSLOT"), *i_79)})));
								if (Truthy(Greater(*slot, Value(0))))
								{
									{
										Iterator iterator_80(Append(Values{}, Call(context.Global("ipairs"), Values{Member(*business, "ID")})));
										for (auto row_81 = iterator_80.Next(); !row_81.empty() && !row_81[0].IsNil(); row_81 = iterator_80.Next())
										{
											auto __82 = MakeCell(At(row_81, 0));
											auto id = MakeCell(At(row_81, 1));
											if (Truthy(Equal(*slot, *id)))
											{
												if (Truthy(*Blog))
												{
													auto mcbizloc = MakeCell(Member(*MCbizlocs, *slot));
													Call(Member(context.Global("log"), "info"), Append(Values{}, Call(Member(context.Global("string"), "format"), Values{Value("\r\033[1;36mBusiness:\033[1;92m %-25s \033[m| \033[1;36mLocation:\033[m %-27s | \033[1;36mSlot:\033[m %d | \033[1;36mID:\033[m %s            \r"), *businessName, *mcbizloc, *i_79, *slot})));
												}
												return Values{Value(true), *i_79};
											}
										}
									}
								}
							}
						}
					}
				}
			}
			return {};
		});
		// BizRe_Hangar preserves the feature callback and its captured state.
		auto BizRe_Hangar = MakeCell();
		*BizRe_Hangar = MakeFunction([=, &context](const Values& args) -> Values {
			if (Truthy(GreaterEqual(First(Invoke(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Member(*PropGlob, "hangarprop")})), "at", Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_ID"), Values{})), Member(*PropGlob, "ArraySize")})), "get_int", Values{})), Value(1))))
			{
				if (Truthy(Value(!Truthy(First(Call(Member(context.Global("stats"), "get_packed_bool"), Values{Value(15966)}))))))
				{
					Call(Member(context.Global("stats"), "set_packed_bool"), Values{Value(15966), Value(true)});
				}
				if (Truthy(LessEqual(First(Invoke(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Member(*PropGlob, "hangarprop"), Value(3))})), "at", Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_ID"), Values{})), Member(*PropGlob, "ArraySize")})), "get_int", Values{})), Value(49))))
				{
					if (Truthy(context.Global("HGWHmaxgoods")))
					{
						if (Truthy(context.Global("HGsetgood")))
						{
							Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Value(1882787), Value(8))})), "set_int", Values{context.Global("HGgoodtype")});
						}
						Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Value(1882787), Value(7))})), "set_int", Values{Value(50)});
						Call(Member(context.Global("stats"), "set_packed_bool"), Values{Value(36828), Value(true)});
						Call(Member(context.Global("STATS"), "STAT_SAVE"), Values{Value(0), Value(0), Value(3), Value(0)});
						Call(Member(context.Global("notify"), "success"), Values{Value("Success!"), Value("Hangar goods replenished!"), Value(3000)});
					}
					else
					{
						Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Biz-Teroids"), Value("Instant Max Restock disabled. Hangar will be replenished with mixed goods."), Value(3000)});
						{
							const auto start_83 = Number(Value(0));
							const auto end_84 = Number(Value(49));
							const auto step_85 = Number(Value(1));
							for (double index_86 = start_83; step_85 > 0 ? index_86 <= end_84 : index_86 >= end_84; index_86 += step_85)
							{
								auto HGl = MakeCell(Value(index_86));
								if (Truthy(And(*BizRe_running, [&] {
									    return First(Call(*BizRe_lotoggle, Values{}));
								    })))
								{
									return Values{};
								}
								Call(Member(context.Global("stats"), "set_packed_bool"), Values{Value(36828), Value(true)});
								Call(Member(context.Global("script"), "yield"), Values{Value(1500)});
							}
						}
					}
				}
				else
				{
					Call(Member(context.Global("notify"), "warn"), Values{Value("Oops!"), Value("Hangar is at max capacity."), Value(3000)});
				}
				if (Truthy(*BizRe_HGonly))
				{
					*BizRe_cancel = Value(true);
					*BizRe_HGonly = Value(false);
				}
			}
			else
			{
				if (Truthy(*BizRe_HGonly))
				{
					*BizRe_cancel = Value(true);
					*BizRe_HGonly = Value(false);
				}
				Call(Member(context.Global("notify"), "error"), Values{Value("TinkerScripts - Biz-Teroids"), Value("You don't own a Hangar"), Value(3000)});
			}
			return {};
		});
		// BizRe_Warehouse preserves the feature callback and its captured state.
		auto BizRe_Warehouse = MakeCell();
		*BizRe_Warehouse = MakeFunction([=, &context](const Values& args) -> Values {
			if (Truthy(GreaterEqual(First(Invoke(First(Invoke(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Member(*PropGlob, "whprop")})), "at", Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_ID"), Values{})), Member(*PropGlob, "ArraySize")})), "at", Values{Value(0), Value(3)})), "get_int", Values{})), Value(1))))
			{
				if (Truthy(context.Global("HGWHmaxgoods")))
				{
					{
						Iterator iterator_87(Append(Values{}, Call(context.Global("ipairs"), Values{MakeTable({{Value(1), Value(32359)}, {Value(2), Value(32360)}, {Value(3), Value(32361)}, {Value(4), Value(32362)}, {Value(5), Value(32363)}})})));
						for (auto row_88 = iterator_87.Next(); !row_88.empty() && !row_88[0].IsNil(); row_88 = iterator_87.Next())
						{
							auto WHp = MakeCell(At(row_88, 0));
							auto c = MakeCell(At(row_88, 1));
							if (Truthy(And(*BizRe_running, [&] {
								    return First(Call(*BizRe_lotoggle, Values{}));
							    })))
							{
								return Values{};
							}
							if (Truthy(GreaterEqual(First(Invoke(First(Invoke(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Member(*PropGlob, "whprop")})), "at", Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_ID"), Values{})), Member(*PropGlob, "ArraySize")})), "at", Values{Subtract(*WHp, Value(1)), Value(3)})), "get_int", Values{})), Value(1))))
							{
								if (Truthy(And(*BizRe_running, [&] {
									    return First(Call(*BizRe_lotoggle, Values{}));
								    })))
								{
									return Values{};
								}
								if (Truthy(LessEqual(First(Invoke(First(Invoke(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Member(*PropGlob, "whprop"), Value(1))})), "at", Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_ID"), Values{})), Member(*PropGlob, "ArraySize")})), "at", Values{Subtract(*WHp, Value(1)), Value(3)})), "get_int", Values{})), Value(110))))
								{
									{
										const auto start_89 = Number(Value(1));
										const auto end_90 = Number(Value(2));
										const auto step_91 = Number(Value(1));
										for (double index_92 = start_89; step_91 > 0 ? index_92 <= end_90 : index_92 >= end_90; index_92 += step_91)
										{
											auto i_93 = MakeCell(Value(index_92));
											if (Truthy(And(*BizRe_running, [&] {
												    return First(Call(*BizRe_lotoggle, Values{}));
											    })))
											{
												return Values{};
											}
											if (Truthy(context.Global("WHsetgood")))
											{
												Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Value(1882762), Value(16))})), "set_int", Values{context.Global("WHgoodtype")});
											}
											Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Value(1882762), Value(13))})), "set_int", Values{Value(111)});
											Call(Member(context.Global("stats"), "set_packed_bool"), Values{*c, Value(true)});
											Call(Member(context.Global("script"), "yield"), Values{Value(2000)});
										}
									}
									Call(Member(context.Global("STATS"), "STAT_SAVE"), Values{Value(0), Value(0), Value(3), Value(0)});
									Call(Member(context.Global("notify"), "success"), Append(Values{Value("Success!")}, Call(Member(context.Global("string"), "format"), Values{Value("Warehouse(%d) goods replenished!"), *WHp})));
								}
								else
								{
									Call(Member(context.Global("notify"), "warn"), Append(Values{Value("Oops!")}, Call(Member(context.Global("string"), "format"), Values{Value("Warehouse(%d) is at max capacity."), *WHp})));
								}
							}
						}
					}
					if (Truthy(*BizRe_WHonly))
					{
						*BizRe_cancel = Value(true);
						*BizRe_WHonly = Value(false);
					}
				}
				else
				{
					Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Biz-Teroids"), Value("Instant Max Restock disabled. Warehouse will be replenished with mixed goods."), Value(3000)});
					Call(Member(context.Global("script"), "yield"), Values{Value(3000)});
					{
						const auto start_94 = Number(Value(1));
						const auto end_95 = Number(Value(60));
						const auto step_96 = Number(Value(1));
						for (double index_97 = start_94; step_96 > 0 ? index_97 <= end_95 : index_97 >= end_95; index_97 += step_96)
						{
							auto WHl = MakeCell(Value(index_97));
							if (Truthy(And(*BizRe_running, [&] {
								    return First(Call(*BizRe_lotoggle, Values{}));
							    })))
							{
								return Values{};
							}
							{
								Iterator iterator_98(Append(Values{}, Call(context.Global("ipairs"), Values{MakeTable({{Value(1), Value(32359)}, {Value(2), Value(32360)}, {Value(3), Value(32361)}, {Value(4), Value(32362)}, {Value(5), Value(32363)}})})));
								for (auto row_99 = iterator_98.Next(); !row_99.empty() && !row_99[0].IsNil(); row_99 = iterator_98.Next())
								{
									auto WHp_100 = MakeCell(At(row_99, 0));
									auto c_101 = MakeCell(At(row_99, 1));
									Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Value(1882762), Value(13))})), "set_int", Values{Value(3)});
									Call(Member(context.Global("stats"), "set_packed_bool"), Values{*c_101, Value(true)});
									Call(Member(context.Global("script"), "yield"), Values{Value(600)});
								}
							}
						}
					}
					if (Truthy(*BizRe_WHonly))
					{
						*BizRe_cancel = Value(true);
						*BizRe_WHonly = Value(false);
					}
				}
			}
			else
			{
				if (Truthy(*BizRe_WHonly))
				{
					*BizRe_cancel = Value(true);
					*BizRe_WHonly = Value(false);
				}
				Call(Member(context.Global("notify"), "error"), Values{Value("TinkerScripts - Biz-Teroids"), Value("You don't own a Warehouse"), Value(3000)});
			}
			return {};
		});
		// BizRe_MCresup preserves the feature callback and its captured state.
		auto BizRe_MCresup = MakeCell();
		*BizRe_MCresup = MakeFunction([=, &context](const Values& args) -> Values {
			Call(Member(context.Global("tunables"), "set_int"), Values{Value(1712674055), Value(1)});
			{
				const auto start_102 = Number(Value(0));
				const auto end_103 = Number(Value(7));
				const auto step_104 = Number(Value(1));
				for (double index_105 = start_102; step_104 > 0 ? index_105 <= end_103 : index_105 >= end_103; index_105 += step_104)
				{
					auto b = MakeCell(Value(index_105));
					if (Truthy(GreaterEqual(First(Invoke(First(Invoke(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Member(*PropGlob, "bbizprop")})), "at", Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_ID"), Values{})), Member(*PropGlob, "ArraySize")})), "at", Values{*b, Value(13)})), "get_int", Values{})), Value(1))))
					{
						{
							const auto start_106 = Number(Value(1));
							const auto end_107 = Number(Value(7));
							const auto step_108 = Number(Value(1));
							for (double index_109 = start_106; step_108 > 0 ? index_109 <= end_107 : index_109 >= end_107; index_109 += step_108)
							{
								auto bsup = MakeCell(Value(index_109));
								Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Value(1673820), *bsup)})), "set_int", Values{Value(1)});
							}
						}
					}
				}
			}
			Call(Member(context.Global("notify"), "success"), Values{Value("Success!"), Value("All businesses supplies have been replenished!"), Value(3000)});
			return {};
		});
		// BizRe_MCrestock preserves the feature callback and its captured state.
		auto BizRe_MCrestock = MakeCell();
		*BizRe_MCrestock = MakeFunction([=, &context](const Values& args) -> Values {
			Call(Member(context.Global("tunables"), "set_int"), Values{Value(728170457), Value(1)});
			Call(Member(context.Global("tunables"), "set_int"), Values{Negate(Value(2094564985)), Value(1)});
			auto values_110 = Append(Values{}, Call(*GetBusinessSlot, Values{Value("Methamphetamine Lab"), Value(true)}));
			auto methmc = MakeCell(At(values_110, 0));
			auto slot_111 = MakeCell(At(values_110, 1));
			if (Truthy(*methmc))
			{
				if (Truthy(LessEqual(First(Invoke(First(Invoke(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Member(*PropGlob, "bbizprop"), Value(1))})), "at", Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_ID"), Values{})), Member(*PropGlob, "ArraySize")})), "at", Values{*slot_111, Value(13)})), "get_int", Values{})), Value(19))))
				{
					{
						Iterator iterator_112(Append(Values{}, Call(context.Global("ipairs"), Values{Member(Member(*BizRe_RestockSU, "methmc"), "manuprod")})));
						for (auto row_113 = iterator_112.Next(); !row_113.empty() && !row_113[0].IsNil(); row_113 = iterator_112.Next())
						{
							auto __114 = MakeCell(At(row_113, 0));
							auto manuprod = MakeCell(At(row_113, 1));
							Call(Member(context.Global("tunables"), "set_int"), Values{*manuprod, Value(1)});
						}
					}
					{
						Iterator iterator_115(Append(Values{}, Call(context.Global("ipairs"), Values{Member(Member(*BizRe_RestockSU, "methmc"), "manucost")})));
						for (auto row_116 = iterator_115.Next(); !row_116.empty() && !row_116[0].IsNil(); row_116 = iterator_115.Next())
						{
							auto __117 = MakeCell(At(row_116, 0));
							auto manucost = MakeCell(At(row_116, 1));
							Call(Member(context.Global("tunables"), "set_int"), Values{*manucost, Value(1)});
						}
					}
					Call(Member(context.Global("script"), "yield"), Values{Value(2000)});
					Call(Member(context.Global("notify"), "success"), Values{Value("Success!"), Value("Meth Business restock ready!"), Value(3000)});
				}
				else
				{
					Call(Member(context.Global("notify"), "warn"), Values{Value("Oops!"), Value("Meth Business stock is at max capacity!"), Value(3000)});
				}
			}
			else
			{
				Call(Member(context.Global("notify"), "error"), Values{Value("TinkerScripts - Biz-Teroids"), Value("You don't own a Methamphetamine Lab."), Value(3000)});
			}
			if (Truthy(And(*BizRe_running, [&] {
				    return First(Call(*BizRe_lotoggle, Values{}));
			    })))
			{
				return Values{};
			}
			auto values_118 = Append(Values{}, Call(*GetBusinessSlot, Values{Value("Weed Farm"), Value(true)}));
			auto weedmc = MakeCell(At(values_118, 0));
			auto slot_119 = MakeCell(At(values_118, 1));
			if (Truthy(*weedmc))
			{
				if (Truthy(LessEqual(First(Invoke(First(Invoke(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Member(*PropGlob, "bbizprop"), Value(1))})), "at", Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_ID"), Values{})), Member(*PropGlob, "ArraySize")})), "at", Values{*slot_119, Value(13)})), "get_int", Values{})), Value(79))))
				{
					{
						Iterator iterator_120(Append(Values{}, Call(context.Global("ipairs"), Values{Member(Member(*BizRe_RestockSU, "weedmc"), "manuprod")})));
						for (auto row_121 = iterator_120.Next(); !row_121.empty() && !row_121[0].IsNil(); row_121 = iterator_120.Next())
						{
							auto __122 = MakeCell(At(row_121, 0));
							auto manuprod_123 = MakeCell(At(row_121, 1));
							Call(Member(context.Global("tunables"), "set_int"), Values{*manuprod_123, Value(1)});
						}
					}
					{
						Iterator iterator_124(Append(Values{}, Call(context.Global("ipairs"), Values{Member(Member(*BizRe_RestockSU, "weedmc"), "manucost")})));
						for (auto row_125 = iterator_124.Next(); !row_125.empty() && !row_125[0].IsNil(); row_125 = iterator_124.Next())
						{
							auto __126 = MakeCell(At(row_125, 0));
							auto manucost_127 = MakeCell(At(row_125, 1));
							Call(Member(context.Global("tunables"), "set_int"), Values{*manucost_127, Value(1)});
						}
					}
					Call(Member(context.Global("script"), "yield"), Values{Value(2000)});
					Call(Member(context.Global("notify"), "success"), Values{Value("Success!"), Value("Weed Business restock ready!"), Value(3000)});
				}
				else
				{
					Call(Member(context.Global("notify"), "warn"), Values{Value("Oops!"), Value("Weed Business stock is at max capacity!"), Value(3000)});
				}
			}
			else
			{
				Call(Member(context.Global("notify"), "error"), Values{Value("TinkerScripts - Biz-Teroids"), Value("You don't own a Weed Farm."), Value(3000)});
			}
			if (Truthy(And(*BizRe_running, [&] {
				    return First(Call(*BizRe_lotoggle, Values{}));
			    })))
			{
				return Values{};
			}
			auto values_128 = Append(Values{}, Call(*GetBusinessSlot, Values{Value("Cocaine Lockup"), Value(true)}));
			auto crackmc = MakeCell(At(values_128, 0));
			auto slot_129 = MakeCell(At(values_128, 1));
			if (Truthy(*crackmc))
			{
				if (Truthy(LessEqual(First(Invoke(First(Invoke(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Member(*PropGlob, "bbizprop"), Value(1))})), "at", Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_ID"), Values{})), Member(*PropGlob, "ArraySize")})), "at", Values{*slot_129, Value(13)})), "get_int", Values{})), Value(9))))
				{
					{
						Iterator iterator_130(Append(Values{}, Call(context.Global("ipairs"), Values{Member(Member(*BizRe_RestockSU, "crackmc"), "manuprod")})));
						for (auto row_131 = iterator_130.Next(); !row_131.empty() && !row_131[0].IsNil(); row_131 = iterator_130.Next())
						{
							auto __132 = MakeCell(At(row_131, 0));
							auto manuprod_133 = MakeCell(At(row_131, 1));
							Call(Member(context.Global("tunables"), "set_int"), Values{*manuprod_133, Value(1)});
						}
					}
					{
						Iterator iterator_134(Append(Values{}, Call(context.Global("ipairs"), Values{Member(Member(*BizRe_RestockSU, "crackmc"), "manucost")})));
						for (auto row_135 = iterator_134.Next(); !row_135.empty() && !row_135[0].IsNil(); row_135 = iterator_134.Next())
						{
							auto __136 = MakeCell(At(row_135, 0));
							auto manucost_137 = MakeCell(At(row_135, 1));
							Call(Member(context.Global("tunables"), "set_int"), Values{*manucost_137, Value(1)});
						}
					}
					Call(Member(context.Global("script"), "yield"), Values{Value(2000)});
					Call(Member(context.Global("notify"), "success"), Values{Value("Success!"), Value("Coke Business restock ready!"), Value(3000)});
				}
				else
				{
					Call(Member(context.Global("notify"), "warn"), Values{Value("Oops!"), Value("Coke Business stock is at max capacity!"), Value(3000)});
				}
			}
			else
			{
				Call(Member(context.Global("notify"), "error"), Values{Value("TinkerScripts - Biz-Teroids"), Value("You don't own a Cocaine Lockup."), Value(3000)});
			}
			if (Truthy(And(*BizRe_running, [&] {
				    return First(Call(*BizRe_lotoggle, Values{}));
			    })))
			{
				return Values{};
			}
			auto values_138 = Append(Values{}, Call(*GetBusinessSlot, Values{Value("Counterfeit Cash Factory"), Value(true)}));
			auto cashmc = MakeCell(At(values_138, 0));
			auto slot_139 = MakeCell(At(values_138, 1));
			if (Truthy(*cashmc))
			{
				if (Truthy(LessEqual(First(Invoke(First(Invoke(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Member(*PropGlob, "bbizprop"), Value(1))})), "at", Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_ID"), Values{})), Member(*PropGlob, "ArraySize")})), "at", Values{*slot_139, Value(13)})), "get_int", Values{})), Value(39))))
				{
					{
						Iterator iterator_140(Append(Values{}, Call(context.Global("ipairs"), Values{Member(Member(*BizRe_RestockSU, "cashmc"), "manuprod")})));
						for (auto row_141 = iterator_140.Next(); !row_141.empty() && !row_141[0].IsNil(); row_141 = iterator_140.Next())
						{
							auto __142 = MakeCell(At(row_141, 0));
							auto manuprod_143 = MakeCell(At(row_141, 1));
							Call(Member(context.Global("tunables"), "set_int"), Values{*manuprod_143, Value(1)});
						}
					}
					{
						Iterator iterator_144(Append(Values{}, Call(context.Global("ipairs"), Values{Member(Member(*BizRe_RestockSU, "cashmc"), "manucost")})));
						for (auto row_145 = iterator_144.Next(); !row_145.empty() && !row_145[0].IsNil(); row_145 = iterator_144.Next())
						{
							auto __146 = MakeCell(At(row_145, 0));
							auto manucost_147 = MakeCell(At(row_145, 1));
							Call(Member(context.Global("tunables"), "set_int"), Values{*manucost_147, Value(1)});
						}
					}
					Call(Member(context.Global("script"), "yield"), Values{Value(2000)});
					Call(Member(context.Global("notify"), "success"), Values{Value("Success!"), Value("Cash Business restock ready!"), Value(3000)});
				}
				else
				{
					Call(Member(context.Global("notify"), "warn"), Values{Value("Oops!"), Value("Cash Business stock is at max capacity!"), Value(3000)});
				}
			}
			else
			{
				Call(Member(context.Global("notify"), "error"), Values{Value("TinkerScripts - Biz-Teroids"), Value("You don't own a Counterfeit Cash Factory."), Value(3000)});
			}
			if (Truthy(And(*BizRe_running, [&] {
				    return First(Call(*BizRe_lotoggle, Values{}));
			    })))
			{
				return Values{};
			}
			auto values_148 = Append(Values{}, Call(*GetBusinessSlot, Values{Value("Document Forgery Office"), Value(true)}));
			auto fakeidmc = MakeCell(At(values_148, 0));
			auto slot_149 = MakeCell(At(values_148, 1));
			if (Truthy(*fakeidmc))
			{
				if (Truthy(LessEqual(First(Invoke(First(Invoke(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Member(*PropGlob, "bbizprop"), Value(1))})), "at", Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_ID"), Values{})), Member(*PropGlob, "ArraySize")})), "at", Values{*slot_149, Value(13)})), "get_int", Values{})), Value(59))))
				{
					{
						Iterator iterator_150(Append(Values{}, Call(context.Global("ipairs"), Values{Member(Member(*BizRe_RestockSU, "fakeidmc"), "manuprod")})));
						for (auto row_151 = iterator_150.Next(); !row_151.empty() && !row_151[0].IsNil(); row_151 = iterator_150.Next())
						{
							auto __152 = MakeCell(At(row_151, 0));
							auto manuprod_153 = MakeCell(At(row_151, 1));
							Call(Member(context.Global("tunables"), "set_int"), Values{*manuprod_153, Value(1)});
						}
					}
					{
						Iterator iterator_154(Append(Values{}, Call(context.Global("ipairs"), Values{Member(Member(*BizRe_RestockSU, "fakeidmc"), "manucost")})));
						for (auto row_155 = iterator_154.Next(); !row_155.empty() && !row_155[0].IsNil(); row_155 = iterator_154.Next())
						{
							auto __156 = MakeCell(At(row_155, 0));
							auto manucost_157 = MakeCell(At(row_155, 1));
							Call(Member(context.Global("tunables"), "set_int"), Values{*manucost_157, Value(1)});
						}
					}
					Call(Member(context.Global("script"), "yield"), Values{Value(2000)});
					Call(Member(context.Global("notify"), "success"), Values{Value("Success!"), Value("Documents Business restock ready!"), Value(3000)});
				}
				else
				{
					Call(Member(context.Global("notify"), "warn"), Values{Value("Oops!"), Value("Documents Business stock is at max capacity!"), Value(3000)});
				}
			}
			else
			{
				Call(Member(context.Global("notify"), "error"), Values{Value("TinkerScripts - Biz-Teroids"), Value("You don't own a Document Forgery Office."), Value(3000)});
			}
			if (Truthy(And(*BizRe_running, [&] {
				    return First(Call(*BizRe_lotoggle, Values{}));
			    })))
			{
				return Values{};
			}
			auto values_158 = Append(Values{}, Call(*GetBusinessSlot, Values{Value("Bunker"), Value(true)}));
			auto bunkermc = MakeCell(At(values_158, 0));
			auto slot_159 = MakeCell(At(values_158, 1));
			if (Truthy(*bunkermc))
			{
				if (Truthy(LessEqual(First(Invoke(First(Invoke(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Member(*PropGlob, "bbizprop"), Value(1))})), "at", Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_ID"), Values{})), Member(*PropGlob, "ArraySize")})), "at", Values{*slot_159, Value(13)})), "get_int", Values{})), Value(99))))
				{
					{
						Iterator iterator_160(Append(Values{}, Call(context.Global("ipairs"), Values{Member(Member(*BizRe_RestockSU, "bunkermc"), "manuprod")})));
						for (auto row_161 = iterator_160.Next(); !row_161.empty() && !row_161[0].IsNil(); row_161 = iterator_160.Next())
						{
							auto __162 = MakeCell(At(row_161, 0));
							auto manuprod_163 = MakeCell(At(row_161, 1));
							Call(Member(context.Global("tunables"), "set_int"), Values{*manuprod_163, Value(1)});
						}
					}
					{
						Iterator iterator_164(Append(Values{}, Call(context.Global("ipairs"), Values{Member(Member(*BizRe_RestockSU, "bunkermc"), "manucost")})));
						for (auto row_165 = iterator_164.Next(); !row_165.empty() && !row_165[0].IsNil(); row_165 = iterator_164.Next())
						{
							auto __166 = MakeCell(At(row_165, 0));
							auto manucost_167 = MakeCell(At(row_165, 1));
							Call(Member(context.Global("tunables"), "set_int"), Values{*manucost_167, Value(1)});
						}
					}
					Call(Member(context.Global("script"), "yield"), Values{Value(2000)});
					Call(Member(context.Global("notify"), "success"), Values{Value("Success!"), Value("Bunker Business restock ready!"), Value(3000)});
				}
			}
			else
			{
				Call(Member(context.Global("notify"), "error"), Values{Value("TinkerScripts - Biz-Teroids"), Value("You don't own a Bunker."), Value(3000)});
			}
			if (Truthy(And(*BizRe_running, [&] {
				    return First(Call(*BizRe_lotoggle, Values{}));
			    })))
			{
				return Values{};
			}
			if (Truthy(GreaterEqual(First(Invoke(First(Invoke(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Member(*PropGlob, "bbizprop")})), "at", Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_ID"), Values{})), Member(*PropGlob, "ArraySize")})), "at", Values{Value(6), Value(13)})), "get_int", Values{})), Value(1))))
			{
				if (Truthy(Value(!Truthy(First(Call(Member(context.Global("stats"), "get_packed_bool"), Values{Value(36689)}))))))
				{
					Call(Member(context.Global("stats"), "set_packed_bool"), Values{Value(36689), Value(true)});
				}
				if (Truthy(LessEqual(First(Invoke(First(Invoke(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Member(*PropGlob, "bbizprop"), Value(1))})), "at", Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_ID"), Values{})), Member(*PropGlob, "ArraySize")})), "at", Values{Value(6), Value(13)})), "get_int", Values{})), Value(159))))
				{
					{
						Iterator iterator_168(Append(Values{}, Call(context.Global("ipairs"), Values{Member(Member(*BizRe_RestockSU, "acidmc"), "manuprod")})));
						for (auto row_169 = iterator_168.Next(); !row_169.empty() && !row_169[0].IsNil(); row_169 = iterator_168.Next())
						{
							auto __170 = MakeCell(At(row_169, 0));
							auto manuprod_171 = MakeCell(At(row_169, 1));
							Call(Member(context.Global("tunables"), "set_int"), Values{*manuprod_171, Value(1)});
						}
					}
					{
						Iterator iterator_172(Append(Values{}, Call(context.Global("ipairs"), Values{Member(Member(*BizRe_RestockSU, "acidmc"), "manucost")})));
						for (auto row_173 = iterator_172.Next(); !row_173.empty() && !row_173[0].IsNil(); row_173 = iterator_172.Next())
						{
							auto __174 = MakeCell(At(row_173, 0));
							auto manucost_175 = MakeCell(At(row_173, 1));
							Call(Member(context.Global("tunables"), "set_int"), Values{*manucost_175, Value(1)});
						}
					}
					Call(Member(context.Global("script"), "yield"), Values{Value(2000)});
					Call(Member(context.Global("notify"), "success"), Values{Value("Success!"), Value("Acid Lab Business restock ready!"), Value(3000)});
				}
			}
			else
			{
				Call(Member(context.Global("notify"), "error"), Values{Value("TinkerScripts - Biz-Teroids"), Value("You don't own an Acid Lab."), Value(3000)});
			}
			Call(Member(context.Global("script"), "yield"), Values{Value(3000)});
			{
				const auto start_176 = Number(Value(0));
				const auto end_177 = Number(Value(6));
				const auto step_178 = Number(Value(1));
				for (double index_179 = start_176; step_178 > 0 ? index_179 <= end_177 : index_179 >= end_177; index_179 += step_178)
				{
					auto i_180 = MakeCell(Value(index_179));
					if (Truthy(GreaterEqual(First(Invoke(First(Invoke(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Member(*PropGlob, "bbizprop")})), "at", Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_ID"), Values{})), Member(*PropGlob, "ArraySize")})), "at", Values{*i_180, Value(13)})), "get_int", Values{})), Value(1))))
					{
						Invoke(First(Invoke(First(Invoke(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Member(*PropGlob, "bbizprop")})), "at", Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_ID"), Values{})), Member(*PropGlob, "ArraySize")})), "at", Values{*i_180, Value(13)})), "at", Values{Value(9)})), "set_int", Values{Value(1)});
					}
				}
			}
			Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Biz-Teroids"), Value("Restocking MC Businesses...\nCheck the progress with the MCT."), Value(3000)});
			return {};
		});
		// BizRe_NCrestock preserves the feature callback and its captured state.
		auto BizRe_NCrestock = MakeCell();
		*BizRe_NCrestock = MakeFunction([=, &context](const Values& args) -> Values {
			if (Truthy(GreaterEqual(First(Invoke(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Member(*PropGlob, "ncprop")})), "at", Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_ID"), Values{})), Member(*PropGlob, "ArraySize")})), "get_int", Values{})), Value(1))))
			{
				if (Truthy(LessEqual(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Member(*PropGlob, "ncprop"), Value(4))})), "get_float", Values{})), Value(99.0))))
				{
					Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_CLUB_POPULARITY"), Value(1000)});
					Call(Member(context.Global("notify"), "success"), Values{Value("Success!"), Value("Nightclub Popularity has been maxed out!"), Value(3000)});
				}
				{
					Iterator iterator_181(Append(Values{}, Call(context.Global("ipairs"), Values{Member(Member(*BizRe_RestockSU, "nightclub"), "manuprod")})));
					for (auto row_182 = iterator_181.Next(); !row_182.empty() && !row_182[0].IsNil(); row_182 = iterator_181.Next())
					{
						auto __183 = MakeCell(At(row_182, 0));
						auto manuprod_184 = MakeCell(At(row_182, 1));
						Call(Member(context.Global("tunables"), "set_int"), Values{*manuprod_184, Value(100)});
					}
				}
				Call(Member(context.Global("notify"), "success"), Values{Value("Success!"), Value("Nightclub restock ready!."), Value(3000)});
				{
					const auto start_185 = Number(Value(0));
					const auto end_186 = Number(Value(6));
					const auto step_187 = Number(Value(1));
					for (double index_188 = start_185; step_187 > 0 ? index_188 <= end_186 : index_188 >= end_186; index_188 += step_187)
					{
						auto i_189 = MakeCell(Value(index_188));
						Invoke(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Value(2709197)})), "at", Values{*i_189, Value(1)})), "set_int", Values{Value(1)});
					}
				}
			}
			else
			{
				Call(Member(context.Global("notify"), "error"), Values{Value("TinkerScripts - Biz-Teroids"), Value("You don't own a Nightclub."), Value(3000)});
			}
			return {};
		});
		// BizRe_SalvPop preserves the feature callback and its captured state.
		auto BizRe_SalvPop = MakeCell();
		*BizRe_SalvPop = MakeFunction([=, &context](const Values& args) -> Values {
			if (Truthy(GreaterEqual(First(Invoke(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Member(*PropGlob, "syprop")})), "at", Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_ID"), Values{})), Member(*PropGlob, "ArraySize")})), "get_int", Values{})), Value(1))))
			{
				Call(Member(context.Global("stats"), "set_packed_int"), Values{Value(51051), Value(100)});
				Call(Member(context.Global("notify"), "success"), Values{Value("Success!"), Value("Salvage Yard Popularity has been maxed out!"), Value(3000)});
			}
			else
			{
				Call(Member(context.Global("notify"), "error"), Values{Value("TinkerScripts - Biz-Teroids"), Value("You don't own a Salvage Yard."), Value(3000)});
			}
			return {};
		});
		// BizRe_MFheat preserves the feature callback and its captured state.
		auto BizRe_MFheat = MakeCell();
		*BizRe_MFheat = MakeFunction([=, &context](const Values& args) -> Values {
			if (Truthy(Equal(First(Call(Member(context.Global("stats"), "get_int"), Values{Value("MPX_SB_CAR_WASH_OWNED")})), Value(1))))
			{
				{
					const auto start_190 = Number(Value(24924));
					const auto end_191 = Number(Value(24926));
					const auto step_192 = Number(Value(1));
					for (double index_193 = start_190; step_192 > 0 ? index_193 <= end_191 : index_193 >= end_191; index_193 += step_192)
					{
						auto tycoonh = MakeCell(Value(index_193));
						Call(Member(context.Global("stats"), "set_packed_int"), Values{*tycoonh, Value(0)});
					}
				}
				Call(Member(context.Global("notify"), "success"), Values{Value("Success!"), Value("Money Fronts Businesses Heat Removed!"), Value(3000)});
				Call(Member(context.Global("script"), "yield"), Values{Value(2000)});
			}
			else
			{
				Call(Member(context.Global("notify"), "error"), Values{Value("TinkerScripts - Biz-Teroids"), Value("You don't own Hands on Car Wash."), Value(3000)});
			}
			return {};
		});
		auto BizRe_count = MakeCell(Value(true));
		// BusinessOnSteroids preserves the feature callback and its captured state.
		auto BusinessOnSteroids = MakeCell();
		*BusinessOnSteroids = MakeFunction([=, &context](const Values& args) -> Values {
			// Countdown preserves the feature callback and its captured state.
			auto Countdown = MakeCell();
			*Countdown = MakeFunction([=, &context](const Values& args) -> Values {
				auto cd_cls = MakeCell(Value("\r                                                                                \r"));
				{
					const auto start_194 = Number(Value(10));
					const auto end_195 = Number(Value(1));
					const auto step_196 = Number(Negate(Value(1)));
					for (double index_197 = start_194; step_196 > 0 ? index_197 <= end_195 : index_197 >= end_195; index_197 += step_196)
					{
						auto i_198 = MakeCell(Value(index_197));
						if (Truthy(First(Call(*BizRe_lotoggle, Values{}))))
						{
							return Values{};
						}
						Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Biz-Teroids by ImagineNothing"), Concat(Value("Main logic will start in "), Concat(*i_198, Value("."))), Value(999)});
						Call(Member(context.Global("log"), "info"), Values{Concat(*cd_cls, Concat(Value("Main logic will start in: "), Concat(*i_198, Value("  \r\033[1A"))))});
						Call(Member(context.Global("script"), "yield"), Values{Value(999)});
					}
				}
				Call(Member(context.Global("log"), "info"), Values{Concat(*cd_cls, Value("\033[1AStarting...                           \r"))});
				Call(Member(context.Global("log"), "info"), Values{Concat(*cd_cls, Value("\033[1A\033[42;30m Start \033[m                           \r\n"))});
				return {};
			});
			Call(Member(context.Global("log"), "warn"), Values{Concat(Value("\r                                                                                 \r"), Concat(Value("\n           __     This script will Restock/Refill:\n  \\ ______/ V`-,  "), Concat(Value("\033[38;5;124mHangar and Warehouses\033[m\n"), Concat(Value("\n   }        /~~  "), Concat(Value("\033[32mMC Businesses\033[m\n"), Concat(Value("\n  /_)^ --,r'    "), Concat(Value("\033[95mNightclub\033[m and \033[95mPopularity Bar\033[m"), Concat(Value("\n\n |b      |b   "), Concat(Value("\033[38;5;27mSalvage Yard Reputation\033[m"), Concat(Value("\n "), Concat(Value("Remove heat levels: \033[38;5;208mMoney Fronts Businesses\033[m"), Concat(Value("\n "), Value("\n\033[2;37;3mYOU MUST RESTART YOUR MC BUSINESSES AND RE-ASSIGN YOUR NIGHTCLUB TECHS AFTER RUNNING THE SCRIPT IF *RESTOCK* IS ENABLED.\033[m\n\n")))))))))))))});
			if (Truthy(*BizRe_count))
			{
				Call(*Countdown, Values{});
			}
			if (Truthy(First(Call(*BizRe_lotoggle, Values{}))))
			{
				return Values{};
			}
			if (Truthy(context.Global("HGWHrestock")))
			{
				Call(*BizRe_Hangar, Values{});
				if (Truthy(First(Call(*BizRe_lotoggle, Values{}))))
				{
					return Values{};
				}
				Call(*BizRe_Warehouse, Values{});
			}
			else
			{
				Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Biz-Teroids"), Value("Hangar & Warehouse restock disabled. Skipping..."), Value(3000)});
			}
			Call(Member(context.Global("script"), "yield"), Values{Value(3000)});
			if (Truthy(First(Call(*BizRe_lotoggle, Values{}))))
			{
				return Values{};
			}
			if (Truthy(context.Global("MCresup")))
			{
				Call(*BizRe_MCresup, Values{});
			}
			else
			{
				Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Biz-Teroids"), Value("MC Businesses Resupply disabled. Skipping..."), Value(3000)});
			}
			Call(Member(context.Global("script"), "yield"), Values{Value(3000)});
			if (Truthy(First(Call(*BizRe_lotoggle, Values{}))))
			{
				return Values{};
			}
			if (Truthy(context.Global("MCrestock")))
			{
				Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Biz-Teroids"), Value("MC Businesses Restock enabled."), Value(3000)});
				Call(*BizRe_MCrestock, Values{});
			}
			else
			{
				Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Biz-Teroids"), Value("MC Businesses Restock disabled. Skipping..."), Value(3000)});
			}
			Call(Member(context.Global("script"), "yield"), Values{Value(3000)});
			if (Truthy(First(Call(*BizRe_lotoggle, Values{}))))
			{
				return Values{};
			}
			if (Truthy(context.Global("NCgoods")))
			{
				Call(*BizRe_NCrestock, Values{});
			}
			else
			{
				Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Biz-Teroids"), Value("Nigtclub Restock disabled. Skipping..."), Value(3000)});
			}
			Call(Member(context.Global("script"), "yield"), Values{Value(3000)});
			Call(*BizRe_SalvPop, Values{});
			if (Truthy(First(Call(*BizRe_lotoggle, Values{}))))
			{
				return Values{};
			}
			Call(*BizRe_MFheat, Values{});
			Call(Member(context.Global("STATS"), "STAT_SAVE"), Values{Value(0), Value(0), Value(3), Value(0)});
			*BizRe_cancel = Value(true);
			*BizRe_running = Value(false);
			Call(Member(context.Global("log"), "info"), Values{Value("\r                                                                                \r\n\r\033[1B\033[42;30m End \033[m\n")});
			Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Biz-Teroids"), Value("Finished!"), Value(3000)});
			return {};
		});
		Call(Member(context.Global("commandmgr"), "add_bool_command"), Values{Value("HGWH_restock"), Value("Hangar/Warehouse Restock"), Value("Should the script restock your Hangar and Warehouses?"), Value(true), Value{}});
		Call(Member(context.Global("commandmgr"), "add_bool_command"), Values{Value("HGWH_maxgoods"), Value("Instant Max Restock"), Value("Should the script instantly max out your Hangar and Warehouses stock?\nIf disabled, restock will be a bit slower, but you'll receive mixed goods."), Value(true), Value{}});
		Call(Member(context.Global("commandmgr"), "add_bool_command"), Values{Value("HG_setgood"), Value("Hangar Set Good"), Value("Goods type for Hangar will be random if this is disabled."), Value(false), Value{}});
		Call(Member(context.Global("commandmgr"), "add_bool_command"), Values{Value("WH_setgood"), Value("Warehouse Set Good"), Value("Goods type for Warehouses will be random if this is disabled."), Value(false), Value{}});
		Call(Member(context.Global("commandmgr"), "add_bool_command"), Values{Value("MC_resup"), Value("MC Businesses Resupply"), Value("Should the script resupply all your MC Businesses?"), Value(true), Value{}});
		Call(Member(context.Global("commandmgr"), "add_bool_command"), Values{Value("MC_restock"), Value("MC Businesses Restock"), Value("Should the script restock your MC Businesses?"), Value(true), Value{}});
		Call(Member(context.Global("commandmgr"), "add_bool_command"), Values{Value("NC_goods"), Value("Nightclub Restock"), Value("Should the script restock your Nightclub?"), Value(true), Value{}});
		Call(Member(context.Global("commandmgr"), "add_list_command"), Values{Value("HG_goodtype"), Value("Hangar Good type"), Value(""), *HGgoodtypeT, Value(0), Value{}});
		Call(Member(context.Global("commandmgr"), "add_list_command"), Values{Value("WH_goodtype"), Value("Warehouse Good type"), Value(""), *WHgoodtypeT, Value(0), Value{}});
		auto allwh = MakeCell(Value(false));
		auto WH_specialitem = MakeCell(MakeTable({{Value(1), MakeTable({{Value(1), Value(0)}, {Value(2), Value("Ornamental Egg")}, {"Type", Value(2)}})}, {Value(2), MakeTable({{Value(1), Value(1)}, {Value(2), Value("Golden Minigun")}, {"Type", Value(4)}})}, {Value(3), MakeTable({{Value(1), Value(2)}, {Value(2), Value("Extra Large Diamond")}, {"Type", Value(6)}})}, {Value(4), MakeTable({{Value(1), Value(3)}, {Value(2), Value("Sasquatch Hide")}, {"Type", Value(7)}})}, {Value(5), MakeTable({{Value(1), Value(4)}, {Value(2), Value("Film Reel")}, {"Type", Value(8)}})}, {Value(6), MakeTable({{Value(1), Value(5)}, {Value(2), Value("Rare Pocket Watch")}, {"Type", Value(9)}})}}));
		Call(Member(context.Global("commandmgr"), "add_list_command"), Values{Value("specialitem"), Value(""), Value(""), *WH_specialitem, Value(0), Value{}});
		auto CEO_instantbuy_amount = MakeCell(Value(1));
		auto ncgotgood = MakeCell(Value(false));
		auto ncstealamount = MakeCell(Value(1));
		auto NC_goodstype = MakeCell(MakeTable({{Value(1), MakeTable({{Value(1), Value(1)}, {Value(2), Value("Cargo and Shipments")}, {"goodTunable", Value(910739098)}, {"goodDefaultAmount", Value(1)}})}, {Value(2), MakeTable({{Value(1), Value(2)}, {Value(2), Value("Sporting Goods")}, {"goodTunable", Negate(Value(188055253))}, {"goodDefaultAmount", Value(2)}})}, {Value(3), MakeTable({{Value(1), Value(3)}, {Value(2), Value("South American Imports")}, {"goodTunable", Negate(Value(1924406700))}, {"goodDefaultAmount", Value(1)}})}, {Value(4), MakeTable({{Value(1), Value(4)}, {Value(2), Value("Pharmaceutical Research")}, {"goodTunable", Value(1386540504)}, {"goodDefaultAmount", Value(1)}})}, {Value(5), MakeTable({{Value(1), Value(5)}, {Value(2), Value("Organic Produce")}, {"goodTunable", Negate(Value(1407470929))}, {"goodDefaultAmount", Value(7)}})}, {Value(6), MakeTable({{Value(1), Value(6)}, {Value(2), Value("Printing & Copying")}, {"goodTunable", Negate(Value(681418413))}, {"goodDefaultAmount", Value(10)}})}, {Value(7), MakeTable({{Value(1), Value(7)}, {Value(2), Value("Cash Creation")}, {"goodTunable", Negate(Value(655270102))}, {"goodDefaultAmount", Value(3)}})}}));
		Call(Member(context.Global("commandmgr"), "add_list_command"), Values{Value("NC_goodtype"), Value("Goods##ncgoodtype"), Value(""), *NC_goodstype, Value(0), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                      auto NC_goodst = MakeCell(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("NC_goodtype")})), "get_value", Values{})));
			                                                                      if (Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("fm_content_club_source")}))))
			                                                                      {
				                                                                      Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_content_club_source"), Add(Add(Value(3726), Value(728)), Value(3))})), "set_int", Values{Subtract(*NC_goodst, Value(1))});
				                                                                      *ncstealamount = Member(Member(*NC_goodstype, *NC_goodst), "goodDefaultAmount");
			                                                                      }
			                                                                      return {};
		                                                                      })});
		auto resizepanel = MakeCell(Value(314));
		Invoke(*TS_Bizteroids, "imgui", Values{MakeFunction([=, &context](const Values& args) -> Values {
			if (Truthy(Value(!Truthy(First(Call(*IsOnline, Values{}))))))
			{
				Call(Member(context.Global("ImGui"), "TextDisabled"), Values{Value("Please join a freemode session.")});
				return Values{};
			}
			context.Global("HGWHrestock") = First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("HGWH_restock")})), "get_value", Values{}));
			context.Global("HGWHmaxgoods") = First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("HGWH_maxgoods")})), "get_value", Values{}));
			context.Global("HGsetgood") = First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("HG_setgood")})), "get_value", Values{}));
			context.Global("WHsetgood") = First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("WH_setgood")})), "get_value", Values{}));
			context.Global("MCresup") = First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("MC_resup")})), "get_value", Values{}));
			context.Global("MCrestock") = First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("MC_restock")})), "get_value", Values{}));
			context.Global("NCgoods") = First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("NC_goods")})), "get_value", Values{}));
			context.Global("HGgoodtype") = First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("HG_goodtype")})), "get_value", Values{}));
			context.Global("WHgoodtype") = First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("WH_goodtype")})), "get_value", Values{}));
			Call(Member(context.Global("ImGui"), "Text"), Values{Value("Control Panel")});
			Call(Member(context.Global("ImGui"), "Separator"), Values{});
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			Call(Member(context.Global("ImGui"), "BeginGroup"), Values{});
			if (Truthy(Or(context.Global("HGsetgood"), [&] {
				    return context.Global("WHsetgood");
			    })))
			{
				*resizepanel = Value(346);
			}
			else
			{
				*resizepanel = Value(314);
			}
			if (Truthy(And(context.Global("HGsetgood"), [&] {
				    return context.Global("WHsetgood");
			    })))
			{
				*resizepanel = Value(379);
			}
			if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginChild"), Values{Value("##bizteroidspanelchild"), Value(360), *resizepanel, Value(1)}))))
			{
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("HGWH_restock")})), "draw", Values{});
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("HGWH_maxgoods")})), "draw", Values{});
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("HG_setgood")})), "draw", Values{});
				if (Truthy(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("HG_setgood")})), "get_value", Values{}))))
				{
					Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("HG_goodtype")})), "draw", Values{});
					Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				}
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("WH_setgood")})), "draw", Values{});
				if (Truthy(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("WH_setgood")})), "get_value", Values{}))))
				{
					Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("WH_goodtype")})), "draw", Values{});
					Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				}
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("MC_resup")})), "draw", Values{});
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("MC_restock")})), "draw", Values{});
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("NC_goods")})), "draw", Values{});
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				if (Truthy(And(*BizRe_cancel, [&] {
					    return First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Run!"), Value(80), Value(40)}));
				    })))
				{
					*BizRe_running = Value(true);
					*BizRe_cancel = Value(false);
					Call(Member(context.Global("script"), "run_in_callback"), Values{*BusinessOnSteroids});
				}
				if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
				{
					Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Bless the Businesses.")});
				}
				if (Truthy(*BizRe_cancel))
				{
					Call(Member(context.Global("ImGui"), "SameLine"), Values{});
					Call(Member(context.Global("ImGui"), "BeginGroup"), Values{});
					Call(Member(context.Global("ImGui"), "Spacing"), Values{});
					Call(Member(context.Global("ImGui"), "Spacing"), Values{});
					*BizRe_count = First(Call(Member(context.Global("ImGui"), "Checkbox"), Values{Value("Countdown"), *BizRe_count}));
					Call(Member(context.Global("ImGui"), "EndGroup"), Values{});
				}
				if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
				{
					Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Allows you to disable the 10s countdown when running Biz-Teroids")});
				}
				if (Truthy(And(Value(!Truthy(*BizRe_cancel)), [&] {
					    return *BizRe_running;
				    })))
				{
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Cancel"), Value(80), Value(40)}))))
					{
						*BizRe_cancel = Value(true);
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
					{
						Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Cancels Biz-Teroids logic.")});
					}
				}
			}
			Call(Member(context.Global("ImGui"), "EndChild"), Values{});
			Call(Member(context.Global("ImGui"), "EndGroup"), Values{});
			Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(20)});
			Call(Member(context.Global("ImGui"), "BeginGroup"), Values{});
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			if (Truthy(First(Call(Member(context.Global("ImGui"), "TreeNodeEx"), Values{Concat(Value("Teleport to Business  ##"), *nomoretreeplease), Value(8219)}))))
			{
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "SameLine"), Values{Negate(Value(10.8)), Value(0)});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginChild"), Values{Value("##bizteroidstptoprop"), Value(220), Value(270), Value(1)}))))
				{
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Hangar"), Value(193)}))))
					{
						if (Truthy(GreaterEqual(First(Call(Member(context.Global("stats"), "get_int"), Values{Value("MPX_HANGAR_OWNED")})), Value(1))))
						{
							Call(*TpToBlip, Values{Value(false), Member(Member(Member(context.Global("Blips"), "Property"), "SM_HANGAR"), "ID"), Value(false)});
						}
						else
						{
							Call(Member(context.Global("notify"), "error"), Values{Value("TinkerScripts - Biz-Teroids"), Value("You don't own a Hangar.")});
						}
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Warehouse"), Value(193)}))))
					{
						if (Truthy(GreaterEqual(First(Call(Member(context.Global("stats"), "get_int"), Values{Value("MPX_WARHOUSESLOT0")})), Value(1))))
						{
							Call(*TpToBlip, Values{Value(false), Member(Member(Member(context.Global("Blips"), "Property"), "WAREHOUSE"), "ID"), Value(false)});
						}
						else
						{
							Call(Member(context.Global("notify"), "error"), Values{Value("TinkerScripts - Biz-Teroids"), Value("You don't own a Warehouse.")});
						}
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
					{
						Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Too lazy to add other warehouses, sry.")});
					}
					{
						Iterator iterator_199(Append(Values{}, Call(context.Global("ipairs"), Values{*MCbiz})));
						for (auto row_200 = iterator_199.Next(); !row_200.empty() && !row_200[0].IsNil(); row_200 = iterator_199.Next())
						{
							auto __201 = MakeCell(At(row_200, 0));
							auto Business = MakeCell(At(row_200, 1));
							if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Member(*Business, "MCBname"), Value(193)}))))
							{
								auto values_202 = Append(Values{}, Call(*GetBusinessSlot, Values{Member(*Business, "MCBname"), Value(false)}));
								auto owned = MakeCell(At(values_202, 0));
								auto __203 = MakeCell(At(values_202, 1));
								if (Truthy(*owned))
								{
									Call(*TpToBlip, Values{Value(false), Member(*Business, "Blip"), Value(false)});
								}
								else
								{
									Call(Member(context.Global("notify"), "error"), Append(Values{Value("TinkerScripts - Biz-Teroids")}, Call(Member(context.Global("string"), "format"), Values{Value("You don't own a %s."), Member(*Business, "MCBname")})));
								}
							}
						}
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Nightclub"), Value(193)}))))
					{
						if (Truthy(GreaterEqual(First(Call(Member(context.Global("stats"), "get_int"), Values{Value("MPX_NIGHTCLUB_OWNED")})), Value(1))))
						{
							Call(*TpToBlip, Values{Value(false), Member(Member(Member(context.Global("Blips"), "Property"), "BAT_CLUB_PROPERTY"), "ID"), Value(false)});
						}
						else
						{
							Call(Member(context.Global("notify"), "error"), Values{Value("TinkerScripts - Biz-Teroids"), Value("Nightclub.")});
						}
					}
					{
						Iterator iterator_204(Append(Values{}, Call(context.Global("ipairs"), Values{MakeTable({{Value(1), Value("SMOKE_SHOP")}, {Value(2), Value("HELITOURS")}, {Value(3), Value("CAR_WASH_BUSINESS")}})})));
						for (auto row_205 = iterator_204.Next(); !row_205.empty() && !row_205[0].IsNil(); row_205 = iterator_204.Next())
						{
							auto __206 = MakeCell(At(row_205, 0));
							auto Mfronts = MakeCell(At(row_205, 1));
							auto Business_207 = MakeCell(Member(Member(context.Global("Blips"), "Property"), *Mfronts));
							if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Member(*Business_207, "Name"), Value(193)}))))
							{
								if (Truthy(Equal(First(Call(Member(context.Global("stats"), "get_int"), Values{Member(*Business_207, "Stat")})), Value(1))))
								{
									Call(*TpToBlip, Values{Value(false), Member(*Business_207, "ID"), Value(false)});
								}
								else
								{
									Call(Member(context.Global("notify"), "error"), Append(Values{Value("TinkerScripts - Biz-Teroids")}, Call(Member(context.Global("string"), "format"), Values{Value("You don't own a %s."), Member(*Business_207, "Name")})));
								}
							}
						}
					}
				}
				Call(Member(context.Global("ImGui"), "EndChild"), Values{});
			}
			Call(Member(context.Global("ImGui"), "EndGroup"), Values{});
			Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(16)});
			Call(Member(context.Global("ImGui"), "BeginGroup"), Values{});
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			if (Truthy(First(Call(Member(context.Global("ImGui"), "TreeNodeEx"), Values{Concat(Value("   Miscellaneous       ##"), *nomoretreeplease), Value(8219)}))))
			{
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "SameLine"), Values{Negate(Value(5.2)), Value(0)});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginChild"), Values{Value("##bizmiscchild"), Value(258), Value(270), Value(1)}))))
				{
					Call(Member(context.Global("ImGui"), "Spacing"), Values{});
					Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(3.8)});
					if (Truthy(First(Call(Member(context.Global("ImGui"), "TreeNodeEx"), Values{Concat(Value("Safe Earnings  ##"), *nomoretreeplease), Value(8219)}))))
					{
						Call(Member(context.Global("ImGui"), "BeginChild"), Values{Value("##safechild"), Value(220), Value(130), Value(1)});
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Claim All Safe Earnings")}))))
						{
							Call(Member(context.Global("script"), "run_in_callback"), Values{MakeFunction([=, &context](const Values& args) -> Values {
								{
									Iterator iterator_208(Append(Values{}, Call(context.Global("ipairs"), Values{Member(*PropGlob, "Safes")})));
									for (auto row_209 = iterator_208.Next(); !row_209.empty() && !row_209[0].IsNil(); row_209 = iterator_208.Next())
									{
										auto __210 = MakeCell(At(row_209, 0));
										auto claimsafe = MakeCell(At(row_209, 1));
										if (Truthy(And(NotEqual(First(Call(Member(context.Global("stats"), "get_int"), Values{Member(*claimsafe, "Owned")})), Value(0)), [&] {
											    return NotEqual(First(Call(Member(context.Global("stats"), "get_int"), Values{Member(*claimsafe, "SafeVal")})), Value(0));
										    })))
										{
											Invoke(First(Call(context.Global("ScriptGlobal"), Values{Member(*claimsafe, "Earnings")})), "set_int", Values{Value(1)});
										}
										Call(Member(context.Global("script"), "yield"), Values{Value(2000)});
									}
								}
								return {};
							})});
						}
						{
							Iterator iterator_211(Append(Values{}, Call(context.Global("ipairs"), Values{Member(*PropGlob, "Safes")})));
							for (auto row_212 = iterator_211.Next(); !row_212.empty() && !row_212[0].IsNil(); row_212 = iterator_211.Next())
							{
								auto __213 = MakeCell(At(row_212, 0));
								auto safecash = MakeCell(At(row_212, 1));
								if (Truthy(NotEqual(First(Call(Member(context.Global("stats"), "get_int"), Values{Member(*safecash, "Owned")})), Value(0))))
								{
									Call(Member(context.Global("ImGui"), "TextDisabled"), Append(Values{}, Call(Member(context.Global("string"), "format"), Append(Values{Value("%s: $%d"), Member(*safecash, "Property")}, Call(Member(context.Global("stats"), "get_int"), Values{Member(*safecash, "SafeVal")})))));
								}
							}
						}
						Call(Member(context.Global("ImGui"), "EndChild"), Values{});
					}
					Call(Member(context.Global("ImGui"), "Spacing"), Values{});
					Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(3.8)});
					if (Truthy(First(Call(Member(context.Global("ImGui"), "TreeNodeEx"), Values{Concat(Value("Source Special Item  ##"), *nomoretreeplease), Value(8219)}))))
					{
						Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("specialitem")})), "draw", Values{});
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Source")}))))
						{
							auto values_214 = Values{};
							auto specialitem_whpb = MakeCell(At(values_214, 0));
							if (Truthy(*allwh))
							{
								*specialitem_whpb = MakeTable({{Value(1), Value(32359)}, {Value(2), Value(32360)}, {Value(3), Value(32361)}, {Value(4), Value(32362)}, {Value(5), Value(32363)}});
							}
							else
							{
								*specialitem_whpb = MakeTable({{Value(1), Value(32359)}});
							}
							Call(Member(context.Global("script"), "run_in_callback"), Values{MakeFunction([=, &context](const Values& args) -> Values {
								{
									Iterator iterator_215(Append(Values{}, Call(context.Global("ipairs"), Values{*specialitem_whpb})));
									for (auto row_216 = iterator_215.Next(); !row_216.empty() && !row_216[0].IsNil(); row_216 = iterator_215.Next())
									{
										auto __217 = MakeCell(At(row_216, 0));
										auto item_218 = MakeCell(At(row_216, 1));
										Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Value(1882762), Value(15))})), "set_int", Values{Value(1)});
										Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Value(1882762), Value(14))})), "set_int", Append(Values{}, Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("specialitem")})), "get_value", Values{})));
										Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Value(1882762), Value(13))})), "set_int", Values{Value(1)});
										Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Value(1882762), Value(16))})), "set_int", Values{Member(Member(*WH_specialitem, Add(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("specialitem")})), "get_value", Values{})), Value(1))), "Type")});
										Call(Member(context.Global("stats"), "set_packed_bool"), Values{*item_218, Value(true)});
										Call(Member(context.Global("script"), "yield"), Values{Value(4200)});
									}
								}
								return {};
							})});
						}
						Call(Member(context.Global("ImGui"), "SameLine"), Values{});
						*allwh = First(Call(Member(context.Global("ImGui"), "Checkbox"), Values{Value("All Warehouses"), *allwh}));
						if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
						{
							Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Sources the selected Special Item for all your warehouses.\nIf set to false, only your first warehouse will receive the Special Item.")});
						}
					}
					Call(Member(context.Global("ImGui"), "Spacing"), Values{});
					Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(3.8)});
					if (Truthy(First(Call(Member(context.Global("ImGui"), "TreeNodeEx"), Values{Concat(Value("Excess Weapon Parts  ##"), *nomoretreeplease), Value(8219)}))))
					{
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Trigger Excess Weapon Parts")}))))
						{
							Call(*GlobalSetBit, Values{First(Invoke(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Value(2658296)})), "at", Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_ID"), Values{})), Value(468)})), "at", Values{Value(465)})), Value(1), Value(1)});
						}
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Deliver")}))))
						{
							if (Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("fm_content_ammunation")}))))
							{
								Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_content_ammunation"), Add(Value(2217), Value(208))})), "set_int", Values{Value(3)});
								Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_content_ammunation"), Add(Value(2154), Value(1))})), "set_int", Values{Negate(Value(1071628608))});
							}
						}
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Trigger Alien Egg Mission")}))))
					{
						if (Truthy(Less(First(Call(Member(context.Global("stats"), "get_int"), Values{Value("MPX_LFETIME_BIKER_BUY_COMPLET5")})), Value(600))))
						{
							Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_LFETIME_BIKER_BUY_COMPLET5"), Value(600)});
							Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_LFETIME_BIKER_BUY_UNDERTA5"), Value(600)});
						}
						Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Add(Value(2733326), Value(3989)), Value(348))})), "set_int", Values{Value(20)});
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
					{
						Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Sets your next Bunker Resupply Mission.")});
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Trigger Bar Earnings")}))))
					{
						Call(Member(context.Global("script"), "run_in_callback"), Values{MakeFunction([=, &context](const Values& args) -> Values {
							{
								const auto start_219 = Number(Value(1));
								const auto end_220 = Number(Value(5));
								const auto step_221 = Number(Value(1));
								for (double index_222 = start_219; step_221 > 0 ? index_222 <= end_220 : index_222 >= end_220; index_222 += step_221)
								{
									auto i_223 = MakeCell(Value(index_222));
									Call(Member(context.Global("stats"), "set_packed_int"), Values{Value(36620), Value(1)});
									Call(Member(context.Global("script"), "yield"), Values{Value(5000)});
									Invoke(First(Call(context.Global("ScriptGlobal"), Values{Value(1882760)})), "set_int", Values{Value(1)});
								}
							}
							return {};
						})});
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
					{
						Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Concat(Value("Fills your MC Clubhouse Bag\nCurrent Bar Earnings: $"), First(Call(Member(context.Global("stats"), "get_int"), Values{Value("MPX_BIKER_BAR_RESUPPLY_CASH")})))});
					}
				}
				Call(Member(context.Global("ImGui"), "EndChild"), Values{});
			}
			Call(Member(context.Global("ImGui"), "EndGroup"), Values{});
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			Call(Member(context.Global("ImGui"), "Text"), Values{Value("Individual Features")});
			Call(Member(context.Global("ImGui"), "Separator"), Values{});
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			Call(Member(context.Global("ImGui"), "BeginGroup"), Values{});
			if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Master Control Terminal"), Value(185)}))))
			{
				Call(Member(context.Global("script"), "run_in_callback"), Values{*OpenMCT});
			}
			if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
			{
				Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Manage your businesses.")});
			}
			Call(Member(context.Global("ImGui"), "SameLine"), Values{});
			if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Owned Businesses"), Value(185)}))))
			{
				{
					Iterator iterator_224(Append(Values{}, Call(context.Global("ipairs"), Values{*MCbiz})));
					for (auto row_225 = iterator_224.Next(); !row_225.empty() && !row_225[0].IsNil(); row_225 = iterator_224.Next())
					{
						auto __226 = MakeCell(At(row_225, 0));
						auto Business_227 = MakeCell(At(row_225, 1));
						Call(*GetBusinessSlot, Values{Member(*Business_227, "MCBname"), Value(true)});
					}
				}
			}
			if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
			{
				Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Prints your owned businesses to console.")});
			}
			Call(Member(context.Global("ImGui"), "NewLine"), Values{});
			if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Restock Hangar"), Value(185)}))))
			{
				*BizRe_HGonly = Value(true);
				*BizRe_running = Value(true);
				*BizRe_cancel = Value(false);
				Call(Member(context.Global("script"), "run_in_callback"), Values{*BizRe_Hangar});
			}
			if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
			{
				Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Restocks your Hangar.")});
			}
			Call(Member(context.Global("ImGui"), "SameLine"), Values{});
			if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Restock Warehouses"), Value(185)}))))
			{
				*BizRe_WHonly = Value(true);
				*BizRe_running = Value(true);
				*BizRe_cancel = Value(false);
				Call(Member(context.Global("script"), "run_in_callback"), Values{*BizRe_Warehouse});
			}
			if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
			{
				Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Restocks your Warehouse(s).")});
			}
			if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Restock MC Businesses"), Value(185)}))))
			{
				Call(Member(context.Global("script"), "run_in_callback"), Values{*BizRe_MCrestock});
			}
			if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
			{
				Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Restocks all your Motorcycle Club Businesses")});
			}
			Call(Member(context.Global("ImGui"), "SameLine"), Values{});
			if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Resupply MC Businesses"), Value(185)}))))
			{
				Call(*BizRe_MCresup, Values{});
			}
			if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
			{
				Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Resupplies all your Motorcycle Club Businesses.")});
			}
			if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Restock Nightclub"), Value(185)}))))
			{
				Call(Member(context.Global("script"), "run_in_callback"), Values{*BizRe_NCrestock});
			}
			if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
			{
				Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Restocks your Nightclub.")});
			}
			Call(Member(context.Global("ImGui"), "SameLine"), Values{});
			if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Businesses Heat"), Value(185)}))))
			{
				Call(Member(context.Global("script"), "run_in_callback"), Values{*BizRe_MFheat});
			}
			if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
			{
				Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Removes Money Fronts Heat.")});
			}
			if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Nightclub Popularity"), Value(185)}))))
			{
				if (Truthy(GreaterEqual(First(Invoke(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Member(*PropGlob, "ncprop")})), "at", Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_ID"), Values{})), Member(*PropGlob, "ArraySize")})), "get_int", Values{})), Value(1))))
				{
					if (Truthy(LessEqual(First(Call(Member(context.Global("stats"), "get_int"), Values{Value("MPX_CLUB_POPULARITY")})), Value(999))))
					{
						Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_CLUB_POPULARITY"), Value(1000)});
						Call(Member(context.Global("notify"), "success"), Values{Value("Success!"), Value("Nightclub Popularity has been maxed out!"), Value(3000)});
					}
				}
				else
				{
					Call(Member(context.Global("notify"), "error"), Values{Value("TinkerScripts - Biz-Teroids"), Value("You don't own a Nightclub."), Value(3000)});
				}
			}
			if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
			{
				Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Sets your Nightclub popularity to max.")});
			}
			Call(Member(context.Global("ImGui"), "SameLine"), Values{});
			if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Salvage Yard Popularity"), Value(185)}))))
			{
				Call(*BizRe_SalvPop, Values{});
			}
			if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
			{
				Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Sets your Salvage Yard popularity to max.")});
			}
			Call(Member(context.Global("ImGui"), "EndGroup"), Values{});
			Call(Member(context.Global("ImGui"), "SetWindowFontScale"), Values{Value(0.6)});
			Call(Member(context.Global("ImGui"), "TextDisabled"), Values{Value("\nYOU MUST RESTART YOUR MC BUSINESSES AND RE-ASSIGN YOUR NIGHTCLUB TECHS IF *RESTOCK* IS ENABLED")});
			Call(Member(context.Global("ImGui"), "SetWindowFontScale"), Values{Value(1.0)});
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			Call(Member(context.Global("ImGui"), "Text"), Values{Value("Instant-Sell")});
			Call(Member(context.Global("ImGui"), "Separator"), Values{});
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Special Cargo##sell"), Value(185)}))))
			{
				if (Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("gb_contraband_sell")}))))
				{
					Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("gb_contraband_sell"), Add(Value(576), Value(1))})), "set_int", Values{Value(67230)});
				}
				else
				{
					Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Biz-Teroids"), Value("You must start a Special Cargo Sell Mission first!"), Value(3000)});
				}
			}
			Call(Member(context.Global("ImGui"), "SameLine"), Values{});
			if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Air-Freight Cargo (Air)"), Value(185)}))))
			{
				if (Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("gb_smuggler")}))))
				{
					Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("gb_smuggler"), Add(Value(1998), Value(1035))})), "set_int", Values{Value(0)});
					Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("gb_smuggler"), Add(Value(1998), Value(1078))})), "set_int", Values{Value(1)});
				}
				else
				{
					Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Biz-Teroids"), Value("You must start a Air-Freight Sell Mission first!"), Value(3000)});
				}
			}
			if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Weapons"), Value(185)}))))
			{
				if (Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("gb_gunrunning")}))))
				{
					Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("gb_gunrunning"), Add(Value(1275), Value(774))})), "set_int", Values{Value(0)});
				}
				else
				{
					Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Biz-Teroids"), Value("You must start a Weapons Sell Mission first!"), Value(3000)});
				}
			}
			Call(Member(context.Global("ImGui"), "SameLine"), Values{});
			if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("MC Business Product"), Value(185)}))))
			{
				if (Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("gb_biker_contraband_sell")}))))
				{
					Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("gb_biker_contraband_sell"), Add(Value(738), Value(122))})), "set_int", Append(Values{}, Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("gb_biker_contraband_sell"), Add(Value(738), Value(174))})), "get_int", Values{})));
				}
				else
				{
					Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Biz-Teroids"), Value("You must start a Product Sell Mission first!"), Value(3000)});
				}
			}
			if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Nightclub Goods"), Value(185)}))))
			{
				if (Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("business_battles_sell")}))))
				{
					Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("business_battles_sell"), Add(Value(2395), Value(205))})), "set_int", Values{Value(0)});
					Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("business_battles_sell"), Add(Value(2395), Value(204))})), "set_int", Values{Value(1)});
					Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("business_battles_sell"), Add(Value(2395), Value(33))})), "set_int", Values{Value(0)});
				}
				else
				{
					Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Biz-Teroids"), Value("You must start a Goods Sell Mission first!"), Value(3000)});
				}
			}
			if (Truthy(And(First(Call(Member(context.Global("CAMERA"), "IS_SCREEN_FADED_OUT"), Values{})), [&] {
				    return Or(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("gb_biker_contraband_sell")})), [&] {
					    return First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("business_battles_sell")}));
				    });
			    })))
			{
				Call(Member(context.Global("script"), "run_in_callback"), Values{MakeFunction([=, &context](const Values& args) -> Values {
					Call(Member(context.Global("CAMERA"), "DO_SCREEN_FADE_IN"), Values{Value(500)});
					Call(Member(context.Global("script"), "yield"), Values{Value(2000)});
					Call(Member(context.Global("CAMERA"), "DO_SCREEN_FADE_IN"), Values{Value(500)});
					return {};
				})});
			}
			Call(Member(context.Global("ImGui"), "SameLine"), Values{});
			if (Truthy(And(First(Call(Member(context.Global("CAMERA"), "IS_SCREEN_FADED_OUT"), Values{})), [&] {
				    return Or(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("gb_biker_contraband_sell")})), [&] {
					    return First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("business_battles_sell")}));
				    });
			    })))
			{
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Fade In")}))))
				{
					Call(Member(context.Global("CAMERA"), "DO_SCREEN_FADE_IN"), Values{Value(500)});
				}
			}
			if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
			{
				Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Use this if you're stuck on a black screen.")});
			}
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			Call(Member(context.Global("ImGui"), "Text"), Values{Value("Instant-Buy")});
			Call(Member(context.Global("ImGui"), "Separator"), Values{});
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Special Cargo##buy"), Value(185)}))))
			{
				if (Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("gb_contraband_buy")}))))
				{
					Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("gb_contraband_buy"), Add(Value(634), Value(5))})), "set_int", Values{Value(1)});
					Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("gb_contraband_buy"), Add(Value(634), Value(1))})), "set_int", Values{*CEO_instantbuy_amount});
					Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("gb_contraband_buy"), Add(Value(634), Value(191))})), "set_int", Values{Value(6)});
					Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("gb_contraband_buy"), Add(Value(634), Value(192))})), "set_int", Values{Value(4)});
				}
				else
				{
					Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Biz-Teroids"), Value("You must start a Special Cargo Buy Mission first!"), Value(3000)});
				}
			}
			Call(Member(context.Global("ImGui"), "PushItemWidth"), Values{Value(100)});
			Call(Member(context.Global("ImGui"), "SameLine"), Values{});
			*CEO_instantbuy_amount = First(Call(Member(context.Global("ImGui"), "InputInt"), Values{Value("Crates Amount"), *CEO_instantbuy_amount}));
			Call(Member(context.Global("ImGui"), "PopItemWidth"), Values{});
			if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Nightclub Goods##steal"), Value(185)}))))
			{
				if (Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("fm_content_club_source")}))))
				{
					Call(Member(context.Global("script"), "run_in_callback"), Values{MakeFunction([=, &context](const Values& args) -> Values {
						Call(*TpToBlip, Values{Value(true), Member(context.Global("Blips"), "LEVEL"), Value(false)});
						Call(Member(context.Global("script"), "yield"), Values{Value(500)});
						Call(*TpToBlip, Values{Value(true), Member(context.Global("Blips"), "CONTRABAND"), Value(false)});
						Call(context.Global("CurPlayerCoords"), Values{});
						auto veh = MakeCell(First(Call(Member(context.Global("VEHICLE"), "GET_RANDOM_VEHICLE_IN_SPHERE"), Values{Member(context.Global("PlaCoords"), "x"), Member(context.Global("PlaCoords"), "y"), Member(context.Global("PlaCoords"), "z"), Value(10), Value(2053223216), Value(101247)})));
						if (Truthy(Equal(First(Invoke(First(Call(context.Global("Entity"), Values{*veh})), "get_model", Values{})), Value(2053223216))))
						{
							auto drivernomore_228 = MakeCell(First(Call(Member(context.Global("VEHICLE"), "GET_PED_IN_VEHICLE_SEAT"), Values{*veh, Negate(Value(1)), Value(0)})));
							if (Truthy(Value(!Truthy(First(Invoke(First(Call(context.Global("Vehicle"), Values{*veh})), "is_seat_free", Values{Negate(Value(1))}))))))
							{
								Invoke(First(Call(context.Global("Entity"), Values{*drivernomore_228})), "delete", Values{});
							}
							Call(Member(context.Global("script"), "yield"), Values{Value(500)});
							Call(Member(context.Global("PED"), "SET_PED_INTO_VEHICLE"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), *veh, Negate(Value(1))});
							Call(*TpToBlip, Values{Value(true), Member(context.Global("Blips"), "LEVEL"), Value(false)});
						}
						Call(Member(context.Global("script"), "yield"), Values{Value(500)});
						Call(*TpToBlip, Values{Value(true), Member(context.Global("Blips"), "LEVEL"), Value(false)});
						return {};
					})});
				}
				else
				{
					Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Biz-Teroids"), Value("You must start a Goods Steal Mission first!"), Value(3000)});
				}
			}
			Call(Member(context.Global("ImGui"), "SameLine"), Values{});
			Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("NC_goodtype")})), "draw", Values{});
			Call(Member(context.Global("ImGui"), "SetNextItemWidth"), Values{Value(100)});
			Call(Member(context.Global("ImGui"), "SameLine"), Values{});
			Call(Member(context.Global("ImGui"), "SameLine"), Values{});
			*ncstealamount = First(Call(Member(context.Global("ImGui"), "InputInt"), Values{Value("Amount##ncgoodtype"), *ncstealamount}));
			if (Truthy(Equal(*ncstealamount, Value(0))))
			{
				*ncstealamount = Value(1);
			}
			if (Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("fm_content_club_source")}))))
			{
				if (Truthy(Value(!Truthy(*ncgotgood))))
				{
					Call(Member(context.Global("script"), "run_in_callback"), Values{MakeFunction([=, &context](const Values& args) -> Values {
						Call(Member(context.Global("script"), "yield"), Values{Value(300)});
						auto ncgoodtype = MakeCell(Add(First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_content_club_source"), Add(Add(Value(3726), Value(728)), Value(3))})), "get_int", Values{})), Value(1)));
						*ncstealamount = First(Call(Member(context.Global("tunables"), "get_int"), Values{Member(Member(*NC_goodstype, *ncgoodtype), "goodTunable")}));
						Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("NC_goodtype")})), "set_value", Values{*ncgoodtype});
						*ncgotgood = Value(true);
						return {};
					})});
				}
				else
				{
					Call(Member(context.Global("tunables"), "set_int"), Values{Member(Member(*NC_goodstype, First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("NC_goodtype")})), "get_value", Values{}))), "goodTunable"), *ncstealamount});
				}
			}
			else
			{
				*ncgotgood = Value(false);
			}
			if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Vehicle Cargo##steal"), Value(185)}))))
			{
				if (Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("gb_vehicle_export")}))))
				{
					Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("gb_vehicle_export"), Add(Value(889), Value(459))})), "set_int", Values{Value(12)});
					Call(Member(context.Global("script"), "run_in_callback"), Values{MakeFunction([=, &context](const Values& args) -> Values {
						Call(*TpToBlip, Values{Value(true), Member(context.Global("Blips"), "LEVEL"), Value(false)});
						Call(Member(context.Global("script"), "yield"), Values{Value(500)});
						Call(*TpToBlip, Values{Value(true), Member(context.Global("Blips"), "SPORTS_CAR"), Value(false)});
						Call(*Joyrider, Values{Value(10)});
						Call(Member(context.Global("script"), "yield"), Values{Value(500)});
						Call(*TpToBlip, Values{Value(true), Member(context.Global("Blips"), "LEVEL"), Value(false)});
						Invoke(First(Call(Member(context.Global("players"), "get_local"), Values{})), "set_wanted_level", Values{Value(0)});
						return {};
					})});
				}
				else
				{
					Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Biz-Teroids"), Value("You must start a Vehicle Cargo Steal Mission first!"), Value(3000)});
				}
			}
			return {};
		})});
		// PhoneMaster preserves the feature callback and its captured state.
		auto PhoneMaster = MakeCell();
		*PhoneMaster = MakeFunction([=, &context](const Values& args) -> Values {
			auto deftheme = MakeCell(At(args, 0));
			auto theme = MakeCell(At(args, 1));
			auto oldmodel = MakeCell(At(args, 2));
			Invoke(First(Call(context.Global("ScriptGlobal"), Values{Value(80362)})), "set_int", Values{*deftheme});
			Invoke(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Value(114990), Value(14060))})), "at", Values{First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Value(21666)})), "get_int", Values{})), Value(20)})), "set_string", Values{*theme});
			Invoke(First(Call(context.Global("ScriptGlobal"), Values{Value(21609)})), "set_int", Values{*oldmodel});
			return {};
		});
		// LetMeTakeASelfie preserves the feature callback and its captured state.
		auto LetMeTakeASelfie = MakeCell();
		*LetMeTakeASelfie = MakeFunction([=, &context](const Values& args) -> Values {
			Call(Member(context.Global("script"), "run_in_callback"), Values{MakeFunction([=, &context](const Values& args) -> Values {
				if (Truthy(First(Call(Member(context.Global("PAD"), "IS_CONTROL_JUST_PRESSED"), Values{Value(0), Value(73)}))))
				{
					Invoke(First(Call(context.Global("ScriptGlobal"), Values{Value(24076)})), "set_int", Values{Value(1)});
					Call(Member(context.Global("script"), "yield"), Values{Value(500)});
					Invoke(First(Call(context.Global("ScriptGlobal"), Values{Value(24075)})), "set_int", Values{Value(8)});
					Call(Member(context.Global("script"), "yield"), Values{Value(2000)});
					Invoke(First(Call(context.Global("ScriptGlobal"), Values{Value(24075)})), "set_int", Values{Value(0)});
				}
				return {};
			})});
			return {};
		});
		auto pm_ContactOverrideList = MakeCell(MakeTable({{Value(1), MakeTable({{Value(1), Value(104)}, {Value(2), Value("Juliet")}})}, {Value(2), MakeTable({{Value(1), Value(105)}, {Value(2), Value("Nikki")}})}, {Value(3), MakeTable({{Value(1), Value(106)}, {Value(2), Value("Chastity")}})}, {Value(4), MakeTable({{Value(1), Value(107)}, {Value(2), Value("Cheetah")}})}, {Value(5), MakeTable({{Value(1), Value(108)}, {Value(2), Value("Sapphire")}})}, {Value(6), MakeTable({{Value(1), Value(109)}, {Value(2), Value("Infernus")}})}, {Value(7), MakeTable({{Value(1), Value(110)}, {Value(2), Value("Fufu")}})}, {Value(8), MakeTable({{Value(1), Value(111)}, {Value(2), Value("Peach")}})}}));
		auto pm_Themes = MakeCell(MakeTable({{Value(1), MakeTable({{Value(1), Value(1)}, {Value(2), Value("Default Theme")}, {Value(3), Value("CELLPHONE_IFRUIT")}})}, {Value(2), MakeTable({{Value(1), Value(2)}, {Value(2), Value("Franklin")}, {Value(3), Value("CELLPHONE_BADGER")}})}, {Value(3), MakeTable({{Value(1), Value(3)}, {Value(2), Value("Michael")}, {Value(3), Value("CELLPHONE_IFRUIT")}})}, {Value(4), MakeTable({{Value(1), Value(4)}, {Value(2), Value("Trevor")}, {Value(3), Value("CELLPHONE_FACADE")}})}, {Value(5), MakeTable({{Value(1), Value(5)}, {Value(2), Value("Celltowa")}, {Value(3), Value("CELLPHONE_PROLOGUE")}})}}));
		auto pm_SoundSets = MakeCell(MakeTable({{Value(1), MakeTable({{Value(1), Value(1)}, {Value(2), Value("Franklin")}, {Value(3), Value("Phone_SoundSet_Franklin")}})}, {Value(2), MakeTable({{Value(1), Value(2)}, {Value(2), Value("Michael (Default)")}, {Value(3), Value("Phone_SoundSet_Michael")}})}, {Value(3), MakeTable({{Value(1), Value(3)}, {Value(2), Value("Trevor")}, {Value(3), Value("Phone_SoundSet_Trevor")}})}, {Value(4), MakeTable({{Value(1), Value(4)}, {Value(2), Value("Prologue")}, {Value(3), Value("Phone_SoundSet_Prologue")}})}, {Value(5), MakeTable({{Value(1), Value(5)}, {Value(2), Value("Default (?)")}, {Value(3), Value("Phone_SoundSet_Default")}})}}));
		auto pm_Background = MakeCell(MakeTable({{Value(1), MakeTable({{Value(1), Value(0)}, {Value(2), Value("Crew Emblem")}})}, {Value(2), MakeTable({{Value(1), Value(10)}, {Value(2), Value("Blue Angles")}})}, {Value(3), MakeTable({{Value(1), Value(11)}, {Value(2), Value("Blue Shards")}})}, {Value(4), MakeTable({{Value(1), Value(12)}, {Value(2), Value("Blue Circles")}})}, {Value(5), MakeTable({{Value(1), Value(13)}, {Value(2), Value("Diamonds")}})}, {Value(6), MakeTable({{Value(1), Value(14)}, {Value(2), Value("Green Glow")}})}, {Value(7), MakeTable({{Value(1), Value(9)}, {Value(2), Value("Green Shards")}})}, {Value(8), MakeTable({{Value(1), Value(5)}, {Value(2), Value("Green Squares")}})}, {Value(9), MakeTable({{Value(1), Value(8)}, {Value(2), Value("Green Triangles")}})}, {Value(10), MakeTable({{Value(1), Value(15)}, {Value(2), Value("Orange 8-Bit")}})}, {Value(11), MakeTable({{Value(1), Value(7)}, {Value(2), Value("Orange Halftone")}})}, {Value(12), MakeTable({{Value(1), Value(6)}, {Value(2), Value("Orange Herringbone")}})}, {Value(13), MakeTable({{Value(1), Value(16)}, {Value(2), Value("Orange Triangles")}})}, {Value(14), MakeTable({{Value(1), Value(4)}, {Value(2), Value("Purple Glow")}})}, {Value(15), MakeTable({{Value(1), Value(17)}, {Value(2), Value("Purple Tartan")}})}}));
		auto pm_ColorScheme = MakeCell(MakeTable({{Value(1), MakeTable({{Value(1), Value(1)}, {Value(2), Value("Blue")}})}, {Value(2), MakeTable({{Value(1), Value(2)}, {Value(2), Value("Green")}})}, {Value(3), MakeTable({{Value(1), Value(3)}, {Value(2), Value("Red")}})}, {Value(4), MakeTable({{Value(1), Value(4)}, {Value(2), Value("Orange")}})}, {Value(5), MakeTable({{Value(1), Value(5)}, {Value(2), Value("Gray")}})}, {Value(6), MakeTable({{Value(1), Value(6)}, {Value(2), Value("Purple")}})}, {Value(7), MakeTable({{Value(1), Value(7)}, {Value(2), Value("Pink")}})}}));
		auto pm_celltowaon = MakeCell(Value(false));
		auto selected_theme = MakeCell(Member(*pm_Themes, Value(1)));
		// pm_AnimToggle preserves the feature callback and its captured state.
		auto pm_AnimToggle = MakeCell();
		*pm_AnimToggle = MakeFunction([=, &context](const Values& args) -> Values {
			auto anim = MakeCell(At(args, 0));
			Call(Member(context.Global("PED"), "SET_PED_CONFIG_FLAG"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value(242), Value(!Truthy(*anim))});
			Call(Member(context.Global("PED"), "SET_PED_CONFIG_FLAG"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value(243), Value(!Truthy(*anim))});
			Call(Member(context.Global("PED"), "SET_PED_CONFIG_FLAG"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value(244), Value(!Truthy(*anim))});
			if (Truthy(Or(First(Call(Member(context.Global("PAD"), "IS_CONTROL_JUST_PRESSED"), Values{Value(0), Value(172)})), [&] {
				    return First(Call(Member(context.Global("PAD"), "IS_CONTROL_JUST_PRESSED"), Values{Value(0), Value(181)}));
			    })))
			{
				Call(Member(context.Global("GTA"), "CELL_SET_INPUT"), Values{Value(1)});
			}
			else if (Truthy(Or(First(Call(Member(context.Global("PAD"), "IS_CONTROL_JUST_PRESSED"), Values{Value(0), Value(173)})), [&] {
				         return First(Call(Member(context.Global("PAD"), "IS_CONTROL_JUST_PRESSED"), Values{Value(0), Value(180)}));
			         })))
			{
				Call(Member(context.Global("GTA"), "CELL_SET_INPUT"), Values{Value(2)});
			}
			else if (Truthy(First(Call(Member(context.Global("PAD"), "IS_CONTROL_JUST_PRESSED"), Values{Value(0), Value(174)}))))
			{
				Call(Member(context.Global("GTA"), "CELL_SET_INPUT"), Values{Value(3)});
			}
			else if (Truthy(First(Call(Member(context.Global("PAD"), "IS_CONTROL_JUST_PRESSED"), Values{Value(0), Value(175)}))))
			{
				Call(Member(context.Global("GTA"), "CELL_SET_INPUT"), Values{Value(4)});
			}
			else if (Truthy(Or(First(Call(Member(context.Global("PAD"), "IS_CONTROL_JUST_PRESSED"), Values{Value(0), Value(176)})), [&] {
				         return First(Call(Member(context.Global("PAD"), "IS_CONTROL_JUST_PRESSED"), Values{Value(0), Value(177)}));
			         })))
			{
				Call(Member(context.Global("GTA"), "CELL_SET_INPUT"), Values{Value(5)});
			}
			if (Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("appmpemail")}))))
			{
				Call(Member(context.Global("GTA"), "CELL_HORIZONTAL_MODE_TOGGLE"), Values{Value(true)});
			}
			else if (Truthy(Value(!Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("appmpemail")}))))))
			{
				if (Truthy(Value(!Truthy(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("pm_LeanKey")})), "get_value", Values{}))))))
				{
					Call(Member(context.Global("GTA"), "CELL_HORIZONTAL_MODE_TOGGLE"), Values{Value(false)});
				}
			}
			Call(Member(context.Global("script"), "run_in_callback"), Values{MakeFunction([=, &context](const Values& args) -> Values {
				if (Truthy(And(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("appemail")})), [&] {
					    return Value(!Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("appmpemail")}))));
				    })))
				{
					if (Truthy(NotEqual(First(Call(Member(context.Global("CAMERA"), "GET_FOLLOW_PED_CAM_VIEW_MODE"), Values{})), Value(4))))
					{
						Call(*PhoneMaster, Values{Value(1), Member(*selected_theme, Value(3)), Value(0)});
					}
					Call(Member(context.Global("SCRIPT"), "REQUEST_SCRIPT"), Values{Value("appmpemail")});
					Call(Member(context.Global("script"), "yield"), Values{Value(50)});
					if (Truthy(First(Call(Member(context.Global("SCRIPT"), "HAS_SCRIPT_LOADED"), Values{Value("appmpemail")}))))
					{
						Call(Member(context.Global("scripts"), "run_as_script"), Values{Value("appemail"), MakeFunction([=, &context](const Values& args) -> Values {
							                                                                Call(Member(context.Global("SCRIPT"), "TERMINATE_THIS_THREAD"), Values{});
							                                                                return {};
						                                                                })});
						Call(Member(context.Global("ScriptNatives"), "START_NEW_SCRIPT"), Values{Value("appmpemail"), Value(2600)});
						Call(Member(context.Global("SCRIPT"), "SET_SCRIPT_AS_NO_LONGER_NEEDED"), Values{Value("appmpemail")});
					}
				}
				return {};
			})});
			if (Truthy(Equal(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("pm_ThemeList")})), "get_value", Values{})), Value(1))))
			{
				if (Truthy(Equal(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Value(21666), Value(1))})), "get_int", Values{})), Value(3))))
				{
					Call(*PhoneMaster, Values{Value(1), Value("CELLPHONE_IFRUIT"), Value(0)});
				}
				else if (Truthy(And(Equal(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Value(21666), Value(1))})), "get_int", Values{})), Value(6)), [&] {
					         return NotEqual(First(Call(Member(context.Global("CAMERA"), "GET_FOLLOW_PED_CAM_VIEW_MODE"), Values{})), Value(4));
				         })))
				{
					Call(*PhoneMaster, Values{Value(1), Value("CELLPHONE_IFRUIT"), Value(0)});
				}
				else if (Truthy(And(GreaterEqual(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Value(21666), Value(1))})), "get_int", Values{})), Value(6)), [&] {
					         return Equal(First(Call(Member(context.Global("CAMERA"), "GET_FOLLOW_PED_CAM_VIEW_MODE"), Values{})), Value(4));
				         })))
				{
					Call(*PhoneMaster, Values{Value(0), Value("CELLPHONE_IFRUIT"), Value(0)});
				}
			}
			else if (Truthy(GreaterEqual(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("pm_ThemeList")})), "get_value", Values{})), Value(1))))
			{
				if (Truthy(Equal(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Value(21666), Value(1))})), "get_int", Values{})), Value(6))))
				{
					Call(*PhoneMaster, Values{Value(0), Member(*selected_theme, Value(3)), Value(0)});
				}
			}
			return {};
		});
		auto pm_pitch = MakeCell(Negate(Value(90.0)));
		auto pm_yaw = MakeCell(Value(0));
		auto pm_roll = MakeCell(Value(0));
		auto pm_gotphonepos = MakeCell(Value(false));
		auto pm_isphonelean = MakeCell(Value(false));
		// pm_Lean preserves the feature callback and its captured state.
		auto pm_Lean = MakeCell();
		*pm_Lean = MakeFunction([=, &context](const Values& args) -> Values {
			if (Truthy(First(Call(Member(context.Global("PAD"), "IS_CONTROL_JUST_PRESSED"), Values{Value(0), Value(183)}))))
			{
				Call(Member(context.Global("GTA"), "CELL_HORIZONTAL_MODE_TOGGLE"), Values{Value(!Truthy(*pm_isphonelean))});
				*pm_isphonelean = Value(!Truthy(*pm_isphonelean));
			}
			return {};
		});
		Call(Member(context.Global("commandmgr"), "add_looped_command"), Values{Value("pm_Anims"), Value("Phone Animations"), Value("Enables Phone Animations"), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                        Call(*pm_AnimToggle, Values{Value(true)});
			                                                                        return {};
		                                                                        }),
		                                                                     MakeFunction([=, &context](const Values& args) -> Values {
			                                                                     if (Truthy(Greater(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("pm_ThemeList")})), "get_value", Values{})), Value(1))))
			                                                                     {
				                                                                     Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("pm_Anims")})), "set_value", Values{Value(false)});
				                                                                     Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Phone Master"), Value("Enable \"Phone Animations\" before selecting a Theme."), Value(3000)});
			                                                                     }
			                                                                     if (Truthy(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("pm_ModelCelltowa")})), "get_value", Values{}))))
			                                                                     {
				                                                                     Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Phone Master"), Value("Enable \"Phone Animations\" before enabling \"Celltowa Phone Model\"."), Value(3000)});
			                                                                     }
			                                                                     return {};
		                                                                     }),
		                                                                     MakeFunction([=, &context](const Values& args) -> Values {
			                                                                     Call(*pm_AnimToggle, Values{Value(false)});
			                                                                     if (Truthy(Equal(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("pm_ThemeList")})), "get_value", Values{})), Value(1))))
			                                                                     {
				                                                                     Call(*PhoneMaster, Values{Value(1), Value("CELLPHONE_IFRUIT"), Value(0)});
			                                                                     }
			                                                                     return {};
		                                                                     })});
		Call(Member(context.Global("commandmgr"), "add_looped_command"), Values{Value("pm_LeanKey"), Value("Manual Horizontal Mode"), Value("Put your phone in horizontal mode by pressing [G]."), *pm_Lean, Value{}, MakeFunction([=, &context](const Values& args) -> Values {
			                                                                        Call(*pm_AnimToggle, Values{Value(false)});
			                                                                        return {};
		                                                                        })});
		Call(Member(context.Global("commandmgr"), "add_looped_command"), Values{Value("pm_ModelCelltowa"), Value("Celltowa Phone Model"), Value("eww... iFruit"), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                        Call(*PhoneMaster, Values{Value(0), Value("CELLPHONE_IFRUIT"), Value(1)});
			                                                                        if (Truthy(Equal(First(Call(Member(context.Global("CAMERA"), "GET_FOLLOW_PED_CAM_VIEW_MODE"), Values{})), Value(4))))
			                                                                        {
				                                                                        Call(*pm_AnimToggle, Values{Value(true)});
			                                                                        }
			                                                                        if (Truthy(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("pm_Anims")})), "get_value", Values{}))))
			                                                                        {
				                                                                        Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("pm_Anims")})), "set_value", Values{Value(false)});
			                                                                        }
			                                                                        if (Truthy(Greater(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("pm_ThemeList")})), "get_value", Values{})), Value(1))))
			                                                                        {
				                                                                        Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("pm_ThemeList")})), "set_value", Values{Value(1)});
				                                                                        Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Phone Master"), Value("Disable \"Celltowa Phone Model\" before selecting a Phone Theme."), Value(3000)});
			                                                                        }
			                                                                        return {};
		                                                                        }),
		                                                                     MakeFunction([=, &context](const Values& args) -> Values {
			                                                                     *pm_celltowaon = Value(true);
			                                                                     return {};
		                                                                     }),
		                                                                     MakeFunction([=, &context](const Values& args) -> Values {
			                                                                     Call(*pm_AnimToggle, Values{Value(false)});
			                                                                     Call(*PhoneMaster, Values{Value(1), Value("CELLPHONE_IFRUIT"), Value(0)});
			                                                                     *pm_celltowaon = Value(false);
			                                                                     return {};
		                                                                     })});
		Call(Member(context.Global("commandmgr"), "add_looped_command"), Values{Value("pm_ScreenShot"), Value("Remote Snapmatic"), Value("Allows you to take a screenshot with Snapmatic at any moment by pressing [X]."), *LetMeTakeASelfie});
		Call(Member(context.Global("commandmgr"), "add_list_command"), Values{Value("pm_contactslist"), Value("Strippers"), Value(""), *pm_ContactOverrideList, Value(104)});
		Call(Member(context.Global("commandmgr"), "add_looped_command"), Values{Value("pm_ContactOverride"), Value("Contact Override"), Value("Redirects the call to a custom selected contact.\nOverrides Lester.\nOnly Strippers available."), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                        if (Truthy(Equal(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Value(8817)})), "get_int", Values{})), Value(12))))
			                                                                        {
				                                                                        Invoke(First(Call(context.Global("ScriptGlobal"), Values{Value(8817)})), "set_int", Append(Values{}, Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("pm_contactslist")})), "get_value", Values{})));
			                                                                        }
			                                                                        return {};
		                                                                        })});
		Call(Member(context.Global("commandmgr"), "add_list_command"), Values{Value("pm_ThemeList"), Value("Theme"), Value("Allows you to use phone themes from Story Mode"), *pm_Themes, Value(1), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                      *selected_theme = Member(*pm_Themes, First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("pm_ThemeList")})), "get_value", Values{})));
			                                                                      Call(*PhoneMaster, Values{Value(0), Member(*selected_theme, Value(3)), Value(0)});
			                                                                      if (Truthy(Equal(Member(*selected_theme, Value(1)), Value(1))))
			                                                                      {
				                                                                      Call(*PhoneMaster, Values{Value(1), Value("CELLPHONE_IFRUIT"), Value(0)});
			                                                                      }
			                                                                      return {};
		                                                                      })});
		Call(Member(context.Global("commandmgr"), "add_list_command"), Values{Value("pm_SoundSetList"), Value("Sound Set"), Value("Allows you to use phone Sound Sets from Story Mode"), *pm_SoundSets, Value(2), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                      context.Global("selected_soundset") = Member(*pm_SoundSets, First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("pm_SoundSetList")})), "get_value", Values{})));
			                                                                      if (Truthy(Equal(Member(context.Global("selected_soundset"), Value(1)), Value(1))))
			                                                                      {
				                                                                      Invoke(First(Call(context.Global("ScriptGlobal"), Values{Value(21655)})), "set_string", Values{Value("Phone_SoundSet_Michael")});
			                                                                      }
			                                                                      return {};
		                                                                      })});
		Call(Member(context.Global("commandmgr"), "add_list_command"), Values{Value("pm_BackgroundList"), Value("Background"), Value("Allows you to use phone Sound Sets from Story Mode"), *pm_Background, First(Call(Member(context.Global("stats"), "get_int"), Values{Value("MPX_FM_CELLPHONE_BACKGROUND")})), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                      auto selected_background = MakeCell(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("pm_BackgroundList")})), "get_value", Values{})));
			                                                                      Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_FM_CELLPHONE_BACKGROUND"), *selected_background});
			                                                                      return {};
		                                                                      })});
		Call(Member(context.Global("commandmgr"), "add_list_command"), Values{Value("pm_ColorSchemeList"), Value("Color Scheme"), Value("Allows you to use phone Sound Sets from Story Mode"), *pm_ColorScheme, First(Call(Member(context.Global("stats"), "get_int"), Values{Value("MPX_FM_CELLPHONE_THEME")})), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                      auto selected_colscheme = MakeCell(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("pm_ColorSchemeList")})), "get_value", Values{})));
			                                                                      Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_FM_CELLPHONE_THEME"), *selected_colscheme});
			                                                                      return {};
		                                                                      })});
		auto pm_resetpos = MakeCell(Value(false));
		// ResetonSP preserves the feature callback and its captured state.
		auto ResetonSP = MakeCell();
		*ResetonSP = MakeFunction([=, &context](const Values& args) -> Values {
			if (Truthy(And(Value(!Truthy(First(Call(Member(context.Global("network"), "is_session_started"), Values{})))), [&] {
				    return First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("completionpercentage_controller")}));
			    })))
			{
				Invoke(First(Call(context.Global("ScriptGlobal"), Values{Value(80362)})), "set_int", Values{Value(0)});
				while (Truthy(Value(true)))
				{
					auto spchar = MakeCell(First(Call(Member(context.Global("ENTITY"), "GET_ENTITY_MODEL"), Append(Values{}, Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})))));
					if (Truthy(Equal(*spchar, Value(225514697))))
					{
						Call(*PhoneMaster, Values{Value(0), Value("CELLPHONE_IFRUIT"), Value(0)});
						return Values{};
					}
					else if (Truthy(Equal(*spchar, Negate(Value(1692214353)))))
					{
						Call(*PhoneMaster, Values{Value(0), Value("CELLPHONE_BADGER"), Value(0)});
						return Values{};
					}
					else if (Truthy(Equal(*spchar, Negate(Value(1686040670)))))
					{
						Call(*PhoneMaster, Values{Value(0), Value("CELLPHONE_FACADE"), Value(0)});
						return Values{};
					}
					Call(Member(context.Global("script"), "yield"), Values{Value(10000)});
				}
			}
			return {};
		});
		Invoke(*TS_PhoneMaster, "imgui", Values{MakeFunction([=, &context](const Values& args) -> Values {
			if (Truthy(Value(!Truthy(First(Call(*IsOnline, Values{}))))))
			{
				Call(Member(context.Global("ImGui"), "TextDisabled"), Values{Value("Please join a freemode session.")});
				Call(Member(context.Global("script"), "run_in_callback"), Values{*ResetonSP});
				return Values{};
			}
			Call(Member(context.Global("ImGui"), "Text"), Values{Value("Main")});
			Call(Member(context.Global("ImGui"), "Separator"), Values{});
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			if (Truthy(And(And(Value(!Truthy(*pm_gotphonepos)), [&] {
				    return Equal(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Value(44938)})), "get_int", Values{})), Value(15));
			    }),
			        [&] {
				        return Equal(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Value(21666), Value(1))})), "get_int", Values{})), Value(6));
			        })))
			{
				context.Global("pm_gotStartpos") = First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Value(21612), Value(1))})), "get_vector3", Values{}));
				context.Global("pm_gotEndpos") = First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Value(21619), Value(1))})), "get_vector3", Values{}));
				context.Global("pm_StartXPos") = Member(context.Global("pm_gotStartpos"), "x");
				context.Global("pm_StartYPos") = Member(context.Global("pm_gotStartpos"), "y");
				context.Global("pm_StartZPos") = Member(context.Global("pm_gotStartpos"), "z");
				context.Global("pm_EndXPos") = Member(context.Global("pm_gotEndpos"), "x");
				context.Global("pm_EndYPos") = Member(context.Global("pm_gotEndpos"), "y");
				context.Global("pm_EndZPos") = Member(context.Global("pm_gotEndpos"), "z");
				*pm_gotphonepos = Value(true);
			}
			else if (Truthy(And(And(*pm_gotphonepos, [&] {
				         return Equal(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Value(44938)})), "get_int", Values{})), Value(15));
			         }),
			             [&] {
				             return GreaterEqual(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Value(21666)})), "get_int", Values{})), Value(2));
			             })))
			{
				Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Value(21612), Value(1))})), "set_vector3", Append(Values{}, Call(context.Global("Vector3"), Values{context.Global("pm_StartXPos"), context.Global("pm_StartYPos"), context.Global("pm_StartZPos")})));
				Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Value(21619), Value(1))})), "set_vector3", Append(Values{}, Call(context.Global("Vector3"), Values{context.Global("pm_EndXPos"), context.Global("pm_EndYPos"), context.Global("pm_EndZPos")})));
				Invoke(First(Call(context.Global("ScriptGlobal"), Values{Value(21626)})), "set_float", Values{*pm_pitch});
				Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Value(21626), Value(1))})), "set_float", Values{*pm_yaw});
				Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Value(21626), Value(2))})), "set_float", Values{*pm_roll});
			}
			if (Truthy(context.Global("selected_soundset")))
			{
				Invoke(First(Call(context.Global("ScriptGlobal"), Values{Value(21655)})), "set_string", Values{Member(context.Global("selected_soundset"), Value(3))});
			}
			Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("pm_Anims")})), "draw", Values{});
			if (Truthy(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("pm_Anims")})), "get_value", Values{}))))
			{
				Call(Member(context.Global("ImGui"), "SameLine"), Values{});
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("pm_LeanKey")})), "draw", Values{});
			}
			Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("pm_ScreenShot")})), "draw", Values{});
			Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("pm_ContactOverride")})), "draw", Values{});
			if (Truthy(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("pm_ContactOverride")})), "get_value", Values{}))))
			{
				Call(Member(context.Global("ImGui"), "SameLine"), Values{});
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("pm_contactslist")})), "draw", Values{});
			}
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			Call(Member(context.Global("ImGui"), "Text"), Values{Value("Customize")});
			Call(Member(context.Global("ImGui"), "Separator"), Values{});
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("pm_ThemeList")})), "draw", Values{});
			Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("pm_SoundSetList")})), "draw", Values{});
			if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
			{
				Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Last one is not the \"Default\" in freemode.")});
			}
			if (Truthy(Equal(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("pm_BackgroundList")})), "get_value", Values{})), Value{})))
			{
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("pm_BackgroundList")})), "set_value", Append(Values{}, Call(Member(context.Global("stats"), "get_int"), Values{Value("MPX_FM_CELLPHONE_BACKGROUND")})));
			}
			if (Truthy(Equal(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("pm_ColorSchemeList")})), "get_value", Values{})), Value{})))
			{
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("pm_ColorSchemeList")})), "set_value", Append(Values{}, Call(Member(context.Global("stats"), "get_int"), Values{Value("MPX_FM_CELLPHONE_THEME")})));
			}
			Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("pm_BackgroundList")})), "draw", Values{});
			Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("pm_ColorSchemeList")})), "draw", Values{});
			if (Truthy(And(Greater(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("pm_ThemeList")})), "get_value", Values{})), Value(1)), [&] {
				    return NotEqual(First(Call(Member(context.Global("CAMERA"), "GET_FOLLOW_PED_CAM_VIEW_MODE"), Values{})), Value(4));
			    })))
			{
				Call(*pm_AnimToggle, Values{Value(false)});
			}
			if (Truthy(And(Greater(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("pm_ThemeList")})), "get_value", Values{})), Value(1)), [&] {
				    return Equal(First(Call(Member(context.Global("CAMERA"), "GET_FOLLOW_PED_CAM_VIEW_MODE"), Values{})), Value(4));
			    })))
			{
				Call(*pm_AnimToggle, Values{Value(true)});
			}
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("pm_ModelCelltowa")})), "draw", Values{});
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			Call(Member(context.Global("ImGui"), "Text"), Values{Value("Phone Orientation")});
			Call(Member(context.Global("ImGui"), "Separator"), Values{});
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			Call(Member(context.Global("ImGui"), "PushItemWidth"), Values{Value(350)});
			*pm_pitch = First(Call(Member(context.Global("ImGui"), "SliderFloat"), Values{Value("Pitch"), *pm_pitch, Negate(Value(360.0)), Value(360.0)}));
			if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
			{
				Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("+ = Forward\n- = Backward")});
			}
			Call(Member(context.Global("ImGui"), "SameLine"), Values{});
			if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Reset##pitch")}))))
			{
				*pm_pitch = Negate(Value(90.0));
			}
			*pm_yaw = First(Call(Member(context.Global("ImGui"), "SliderFloat"), Values{Value("Yaw"), *pm_yaw, Negate(Value(360.0)), Value(360.0)}));
			if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
			{
				Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("+ = Right\n- = Left")});
			}
			Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(13)});
			if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Reset##yaw")}))))
			{
				*pm_yaw = Value(0);
			}
			*pm_roll = First(Call(Member(context.Global("ImGui"), "SliderFloat"), Values{Value("Roll"), *pm_roll, Value(360.0), Negate(Value(360.0))}));
			if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
			{
				Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("+ = Left\n- = Right")});
			}
			Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(16)});
			if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Reset##roll")}))))
			{
				*pm_roll = Value(0);
			}
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Reset Phone Orientation")}))))
			{
				*pm_pitch = Negate(Value(90.0));
				*pm_yaw = Value(0);
				*pm_roll = Value(0);
			}
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			Call(Member(context.Global("ImGui"), "PopItemWidth"), Values{});
			Call(Member(context.Global("ImGui"), "Text"), Values{Value("Phone Position")});
			if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
			{
				Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Think of this as a keyframe animation")});
			}
			Call(Member(context.Global("ImGui"), "Separator"), Values{});
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			if (Truthy(*pm_gotphonepos))
			{
				Call(Member(context.Global("ImGui"), "PushItemWidth"), Values{Value(350)});
				context.Global("pm_StartXPos") = First(Call(Member(context.Global("ImGui"), "SliderFloat"), Values{Value("Start X Position"), context.Global("pm_StartXPos"), Negate(Value(360.0)), Value(360.0)}));
				Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(7)});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Reset##StartXPos")}))))
				{
					context.Global("pm_StartXPos") = Member(context.Global("pm_gotStartpos"), "x");
				}
				context.Global("pm_StartYPos") = First(Call(Member(context.Global("ImGui"), "SliderFloat"), Values{Value("Start Y Position"), context.Global("pm_StartYPos"), Negate(Value(360.0)), Value(360.0)}));
				Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(7)});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Reset##StartYPos")}))))
				{
					context.Global("pm_StartYPos") = Member(context.Global("pm_gotStartpos"), "y");
				}
				context.Global("pm_StartZPos") = First(Call(Member(context.Global("ImGui"), "SliderFloat"), Values{Value("Start Z Position"), context.Global("pm_StartZPos"), Negate(Value(360.0)), Value(360.0)}));
				Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(6)});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Reset##StartZPos")}))))
				{
					context.Global("pm_StartZPos") = Member(context.Global("pm_gotStartpos"), "z");
				}
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				context.Global("pm_EndXPos") = First(Call(Member(context.Global("ImGui"), "SliderFloat"), Values{Value("End X Position"), context.Global("pm_EndXPos"), Negate(Value(360.0)), Value(360.0)}));
				Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(7)});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Reset##EndXPos")}))))
				{
					context.Global("pm_EndXPos") = Member(context.Global("pm_gotEndpos"), "x");
				}
				context.Global("pm_EndYPos") = First(Call(Member(context.Global("ImGui"), "SliderFloat"), Values{Value("End Y Position"), context.Global("pm_EndYPos"), Negate(Value(360.0)), Value(360.0)}));
				Call(Member(context.Global("ImGui"), "SameLine"), Values{});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Reset##EndYPos")}))))
				{
					context.Global("pm_EndYPos") = Member(context.Global("pm_gotEndpos"), "y");
				}
				context.Global("pm_EndZPos") = First(Call(Member(context.Global("ImGui"), "SliderFloat"), Values{Value("End Z Position"), context.Global("pm_EndZPos"), Negate(Value(360.0)), Value(360.0)}));
				Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(6)});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Reset##EndZPos")}))))
				{
					context.Global("pm_EndZPos") = Member(context.Global("pm_gotEndpos"), "z");
				}
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				if (Truthy(Or(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Reset Phone Position")})), [&] {
					    return *pm_resetpos;
				    })))
				{
					context.Global("pm_StartXPos") = Member(context.Global("pm_gotStartpos"), "x");
					context.Global("pm_StartYPos") = Member(context.Global("pm_gotStartpos"), "y");
					context.Global("pm_StartZPos") = Member(context.Global("pm_gotStartpos"), "z");
					context.Global("pm_EndXPos") = Member(context.Global("pm_gotEndpos"), "x");
					context.Global("pm_EndYPos") = Member(context.Global("pm_gotEndpos"), "y");
					context.Global("pm_EndZPos") = Member(context.Global("pm_gotEndpos"), "z");
					*pm_resetpos = Value(false);
				}
				Call(Member(context.Global("ImGui"), "PopItemWidth"), Values{});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Reset All")}))))
				{
					*pm_pitch = Negate(Value(90.0));
					*pm_yaw = Value(0);
					*pm_roll = Value(0);
					*pm_resetpos = Value(true);
				}
			}
			else
			{
				Call(Member(context.Global("ImGui"), "TextDisabled"), Values{Value("Pull out your phone to be able to use this feature.")});
			}
			return {};
		})});
		auto GVweapons = MakeCell(([&] {
			auto table_229 = MakeTable();
			Member(table_229, Value(1)) = MakeTable({{Value(1), Value(0)}, {Value(2), Value("")}});
			Member(table_229, Value(2)) = MakeTable({{Value(1), Negate(Value(1357824103))}, {Value(2), Value("Advanced Rifle")}});
			Member(table_229, Value(3)) = MakeTable({{Value(1), Negate(Value(1834847097))}, {Value(2), Value("Antique Cavalry Dagger")}});
			Member(table_229, Value(4)) = MakeTable({{Value(1), Value(584646201)}, {Value(2), Value("AP Pistol")}});
			Member(table_229, Value(5)) = MakeTable({{Value(1), Value(961495388)}, {Value(2), Value("Assault Rifle Mk II")}});
			Member(table_229, Value(6)) = MakeTable({{Value(1), Negate(Value(1074790547))}, {Value(2), Value("Assault Rifle")}});
			Member(table_229, Value(7)) = MakeTable({{Value(1), Negate(Value(494615257))}, {Value(2), Value("Assault Shotgun")}});
			Member(table_229, Value(8)) = MakeTable({{Value(1), Negate(Value(270015777))}, {Value(2), Value("Assault SMG")}});
			Member(table_229, Value(9)) = MakeTable({{Value(1), Negate(Value(1786099057))}, {Value(2), Value("Baseball Bat")}});
			Member(table_229, Value(10)) = MakeTable({{Value(1), Negate(Value(853065399))}, {Value(2), Value("Battle Axe")}});
			Member(table_229, Value(11)) = MakeTable({{Value(1), Value(1924557585)}, {Value(2), Value("Battle Rifle")}});
			Member(table_229, Value(12)) = MakeTable({{Value(1), Negate(Value(102323637))}, {Value(2), Value("Bottle")}});
			Member(table_229, Value(13)) = MakeTable({{Value(1), Negate(Value(2066285827))}, {Value(2), Value("Bullpup Rifle Mk II")}});
			Member(table_229, Value(14)) = MakeTable({{Value(1), Value(2132975508)}, {Value(2), Value("Bullpup Rifle")}});
			Member(table_229, Value(15)) = MakeTable({{Value(1), Negate(Value(1654528753))}, {Value(2), Value("Bullpup Shotgun")}});
			Member(table_229, Value(16)) = MakeTable({{Value(1), Value(1703483498)}, {Value(2), Value("Candy Cane")}});
			Member(table_229, Value(17)) = MakeTable({{Value(1), Negate(Value(86904375))}, {Value(2), Value("Carbine Rifle Mk II")}});
			Member(table_229, Value(18)) = MakeTable({{Value(1), Negate(Value(2084633992))}, {Value(2), Value("Carbine Rifle")}});
			Member(table_229, Value(19)) = MakeTable({{Value(1), Value(727643628)}, {Value(2), Value("Ceramic Pistol")}});
			Member(table_229, Value(20)) = MakeTable({{Value(1), Negate(Value(608341376))}, {Value(2), Value("Combat MG Mk II")}});
			Member(table_229, Value(21)) = MakeTable({{Value(1), Value(2144741730)}, {Value(2), Value("Combat MG")}});
			Member(table_229, Value(22)) = MakeTable({{Value(1), Value(171789620)}, {Value(2), Value("Combat PDW")}});
			Member(table_229, Value(23)) = MakeTable({{Value(1), Value(1593441988)}, {Value(2), Value("Combat Pistol")}});
			Member(table_229, Value(24)) = MakeTable({{Value(1), Value(94989220)}, {Value(2), Value("Combat Shotgun")}});
			Member(table_229, Value(25)) = MakeTable({{Value(1), Negate(Value(618237638))}, {Value(2), Value("Compact EMP Launcher")}});
			Member(table_229, Value(26)) = MakeTable({{Value(1), Value(125959754)}, {Value(2), Value("Compact Grenade Launcher")}});
			Member(table_229, Value(27)) = MakeTable({{Value(1), Value(1649403952)}, {Value(2), Value("Compact Rifle")}});
			Member(table_229, Value(28)) = MakeTable({{Value(1), Negate(Value(2067956739))}, {Value(2), Value("Crowbar")}});
			Member(table_229, Value(29)) = MakeTable({{Value(1), Negate(Value(275439685))}, {Value(2), Value("Double Barrel Shotgun")}});
			Member(table_229, Value(30)) = MakeTable({{Value(1), Negate(Value(1746263880))}, {Value(2), Value("Double-Action Revolver")}});
			Member(table_229, Value(31)) = MakeTable({{Value(1), Negate(Value(1916886713))}, {Value(2), Value("El Strickler")}});
			Member(table_229, Value(32)) = MakeTable({{Value(1), Value(2138347493)}, {Value(2), Value("Firework Launcher")}});
			Member(table_229, Value(33)) = MakeTable({{Value(1), Value(1198879012)}, {Value(2), Value("Flare Gun")}});
			Member(table_229, Value(34)) = MakeTable({{Value(1), Negate(Value(1951375401))}, {Value(2), Value("Flashlight")}});
			Member(table_229, Value(35)) = MakeTable({{Value(1), Negate(Value(1568386805))}, {Value(2), Value("Grenade Launcher")}});
			Member(table_229, Value(36)) = MakeTable({{Value(1), Value(1627465347)}, {Value(2), Value("Gusenberg Sweeper")}});
			Member(table_229, Value(37)) = MakeTable({{Value(1), Value(1317494643)}, {Value(2), Value("Hammer")}});
			Member(table_229, Value(38)) = MakeTable({{Value(1), Negate(Value(102973651))}, {Value(2), Value("Hatchet")}});
			Member(table_229, Value(39)) = MakeTable({{Value(1), Negate(Value(771403250))}, {Value(2), Value("Heavy Pistol")}});
			Member(table_229, Value(40)) = MakeTable({{Value(1), Negate(Value(879347409))}, {Value(2), Value("Heavy Revolver Mk II")}});
			Member(table_229, Value(41)) = MakeTable({{Value(1), Negate(Value(1045183535))}, {Value(2), Value("Heavy Revolver")}});
			Member(table_229, Value(42)) = MakeTable({{Value(1), Negate(Value(947031628))}, {Value(2), Value("Heavy Rifle")}});
			Member(table_229, Value(43)) = MakeTable({{Value(1), Value(984333226)}, {Value(2), Value("Heavy Shotgun")}});
			Member(table_229, Value(44)) = MakeTable({{Value(1), Value(177293209)}, {Value(2), Value("Heavy Sniper Mk II")}});
			Member(table_229, Value(45)) = MakeTable({{Value(1), Value(205991906)}, {Value(2), Value("Heavy Sniper")}});
			Member(table_229, Value(46)) = MakeTable({{Value(1), Value(1672152130)}, {Value(2), Value("Homing Launcher")}});
			Member(table_229, Value(47)) = MakeTable({{Value(1), Negate(Value(1716189206))}, {Value(2), Value("Knife")}});
			Member(table_229, Value(48)) = MakeTable({{Value(1), Negate(Value(656458692))}, {Value(2), Value("Knuckle Duster")}});
			Member(table_229, Value(49)) = MakeTable({{Value(1), Negate(Value(581044007))}, {Value(2), Value("Machete")}});
			Member(table_229, Value(50)) = MakeTable({{Value(1), Negate(Value(619010992))}, {Value(2), Value("Machine Pistol")}});
			Member(table_229, Value(51)) = MakeTable({{Value(1), Negate(Value(598887786))}, {Value(2), Value("Marksman Pistol")}});
			Member(table_229, Value(52)) = MakeTable({{Value(1), Value(1785463520)}, {Value(2), Value("Marksman Rifle Mk II")}});
			Member(table_229, Value(53)) = MakeTable({{Value(1), Negate(Value(952879014))}, {Value(2), Value("Marksman Rifle")}});
			Member(table_229, Value(54)) = MakeTable({{Value(1), Negate(Value(1660422300))}, {Value(2), Value("MG")}});
			Member(table_229, Value(55)) = MakeTable({{Value(1), Value(324215364)}, {Value(2), Value("Micro SMG")}});
			Member(table_229, Value(56)) = MakeTable({{Value(1), Negate(Value(1658906650))}, {Value(2), Value("Military Rifle")}});
			Member(table_229, Value(57)) = MakeTable({{Value(1), Negate(Value(1121678507))}, {Value(2), Value("Mini SMG")}});
			Member(table_229, Value(58)) = MakeTable({{Value(1), Value(1119849093)}, {Value(2), Value("Minigun")}});
			Member(table_229, Value(59)) = MakeTable({{Value(1), Negate(Value(1466123874))}, {Value(2), Value("Musket")}});
			Member(table_229, Value(60)) = MakeTable({{Value(1), Negate(Value(1853920116))}, {Value(2), Value("Navy Revolver")}});
			Member(table_229, Value(61)) = MakeTable({{Value(1), Value(1737195953)}, {Value(2), Value("Nightstick")}});
			Member(table_229, Value(62)) = MakeTable({{Value(1), Value(1470379660)}, {Value(2), Value("Perico Pistol")}});
			Member(table_229, Value(63)) = MakeTable({{Value(1), Value(419712736)}, {Value(2), Value("Pipe Wrench")}});
			Member(table_229, Value(64)) = MakeTable({{Value(1), Negate(Value(1716589765))}, {Value(2), Value("Pistol .50")}});
			Member(table_229, Value(65)) = MakeTable({{Value(1), Negate(Value(1075685676))}, {Value(2), Value("Pistol Mk II")}});
			Member(table_229, Value(66)) = MakeTable({{Value(1), Value(453432689)}, {Value(2), Value("Pistol")}});
			Member(table_229, Value(67)) = MakeTable({{Value(1), Negate(Value(1810795771))}, {Value(2), Value("Pool Cue")}});
			Member(table_229, Value(68)) = MakeTable({{Value(1), Value(1853742572)}, {Value(2), Value("Precision Rifle")}});
			Member(table_229, Value(69)) = MakeTable({{Value(1), Value(1432025498)}, {Value(2), Value("Pump Shotgun Mk II")}});
			Member(table_229, Value(70)) = MakeTable({{Value(1), Value(487013001)}, {Value(2), Value("Pump Shotgun")}});
			Member(table_229, Value(71)) = MakeTable({{Value(1), Negate(Value(22923932))}, {Value(2), Value("Railgun")}});
			Member(table_229, Value(72)) = MakeTable({{Value(1), Negate(Value(1312131151))}, {Value(2), Value("RPG")}});
			Member(table_229, Value(73)) = MakeTable({{Value(1), Value(2017895192)}, {Value(2), Value("Sawed-Off Shotgun")}});
			Member(table_229, Value(74)) = MakeTable({{Value(1), Negate(Value(774507221))}, {Value(2), Value("Service Carbine")}});
			Member(table_229, Value(75)) = MakeTable({{Value(1), Value(2024373456)}, {Value(2), Value("SMG Mk II")}});
			Member(table_229, Value(76)) = MakeTable({{Value(1), Value(736523883)}, {Value(2), Value("SMG")}});
			Member(table_229, Value(77)) = MakeTable({{Value(1), Value(100416529)}, {Value(2), Value("Sniper Rifle")}});
			Member(table_229, Value(78)) = MakeTable({{Value(1), Value(62870901)}, {Value(2), Value("Snowball Launcher")}});
			Member(table_229, Value(79)) = MakeTable({{Value(1), Negate(Value(2009644972))}, {Value(2), Value("SNS Pistol Mk II")}});
			Member(table_229, Value(80)) = MakeTable({{Value(1), Negate(Value(1076751822))}, {Value(2), Value("SNS Pistol")}});
			Member(table_229, Value(81)) = MakeTable({{Value(1), Negate(Value(1768145561))}, {Value(2), Value("Special Carbine Mk II")}});
			Member(table_229, Value(82)) = MakeTable({{Value(1), Negate(Value(1063057011))}, {Value(2), Value("Special Carbine")}});
			Member(table_229, Value(83)) = MakeTable({{Value(1), Value(1171102963)}, {Value(2), Value("Stun Gun")}});
			Member(table_229, Value(84)) = MakeTable({{Value(1), Value(317205821)}, {Value(2), Value("Sweeper Shotgun")}});
			Member(table_229, Value(85)) = MakeTable({{Value(1), Negate(Value(538741184))}, {Value(2), Value("Switchblade")}});
			Member(table_229, Value(86)) = MakeTable({{Value(1), Value(350597077)}, {Value(2), Value("Tactical SMG")}});
			Member(table_229, Value(87)) = MakeTable({{Value(1), Negate(Value(624951259))}, {Value(2), Value("The Shocker")}});
			Member(table_229, Value(88)) = MakeTable({{Value(1), Value(1198256469)}, {Value(2), Value("Unholy Hellbringer")}});
			Member(table_229, Value(89)) = MakeTable({{Value(1), Negate(Value(1355376991))}, {Value(2), Value("Up-n-Atomizer")}});
			Member(table_229, Value(90)) = MakeTable({{Value(1), Value(137902532)}, {Value(2), Value("Vintage Pistol")}});
			Member(table_229, Value(91)) = MakeTable({{Value(1), Negate(Value(1238556825))}, {Value(2), Value("Widowmaker")}});
			return table_229;
		}()));
		auto GVthrowweapons = MakeCell(MakeTable({{Value(1), MakeTable({{Value(1), Value(0)}, {Value(2), Value("")}})}, {Value(2), MakeTable({{Value(1), Negate(Value(1813897027))}, {Value(2), Value("Grenade")}})}, {Value(3), MakeTable({{Value(1), Negate(Value(37975472))}, {Value(2), Value("Tear Gas")}})}, {Value(4), MakeTable({{Value(1), Value(741814745)}, {Value(2), Value("Sticky Bomb")}})}, {Value(5), MakeTable({{Value(1), Negate(Value(1420407917))}, {Value(2), Value("Proximity Mine")}})}, {Value(6), MakeTable({{Value(1), Negate(Value(1169823560))}, {Value(2), Value("Pipe Bomb")}})}, {Value(7), MakeTable({{Value(1), Value(883325847)}, {Value(2), Value("Jerry Can")}})}, {Value(8), MakeTable({{Value(1), Value(615608432)}, {Value(2), Value("Molotov")}})}}));
		{
			const auto start_230 = Number(Value(1));
			const auto end_231 = Number(Value(10));
			const auto step_232 = Number(Value(1));
			for (double index_233 = start_230; step_232 > 0 ? index_233 <= end_231 : index_233 >= end_231; index_233 += step_232)
			{
				auto i_234 = MakeCell(Value(index_233));
				Call(Member(context.Global("commandmgr"), "add_list_command"), Values{Concat(Value("gv_weaponlist"), *i_234), Concat(Value("Slot "), Concat(*i_234, Concat(Value("##weapl"), *i_234))), Value(""), *GVweapons, First(Call(Member(context.Global("tunables"), "get_int"), Values{Concat(Value("XM22_GUN_VAN_SLOT_WEAPON_TYPE_"), Subtract(*i_234, Value(1)))})), MakeFunction([=, &context](const Values& args) -> Values {
					                                                                      auto GV_selectedweap = MakeCell(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Concat(Value("gv_weaponlist"), *i_234)})), "get_value", Values{})));
					                                                                      Call(Member(context.Global("tunables"), "set_int"), Values{Concat(Value("XM22_GUN_VAN_SLOT_WEAPON_TYPE_"), Subtract(*i_234, Value(1))), *GV_selectedweap});
					                                                                      return {};
				                                                                      })});
			}
		}
		{
			const auto start_235 = Number(Value(1));
			const auto end_236 = Number(Value(3));
			const auto step_237 = Number(Value(1));
			for (double index_238 = start_235; step_237 > 0 ? index_238 <= end_236 : index_238 >= end_236; index_238 += step_237)
			{
				auto i_239 = MakeCell(Value(index_238));
				Call(Member(context.Global("commandmgr"), "add_list_command"), Values{Concat(Value("gv_throwlist"), *i_239), Concat(Value("Slot "), Concat(*i_239, Concat(Value("##throwl"), *i_239))), Value(""), *GVthrowweapons, First(Call(Member(context.Global("tunables"), "get_int"), Values{Concat(Value("XM22_GUN_VAN_SLOT_THROWABLE_TYPE_"), Subtract(*i_239, Value(1)))})), MakeFunction([=, &context](const Values& args) -> Values {
					                                                                      auto GV_selectedthrow = MakeCell(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Concat(Value("gv_throwlist"), *i_239)})), "get_value", Values{})));
					                                                                      Call(Member(context.Global("tunables"), "set_int"), Values{Concat(Value("XM22_GUN_VAN_SLOT_THROWABLE_TYPE_"), Subtract(*i_239, Value(1))), *GV_selectedthrow});
					                                                                      return {};
				                                                                      })});
			}
		}
		Call(Member(context.Global("commandmgr"), "add_bool_command"), Values{Value("gv_getcurweap"), Value("##getcurweap"), Value(""), Value(false), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                      {
				                                                                      const auto start_240 = Number(Value(1));
				                                                                      const auto end_241 = Number(Value(10));
				                                                                      const auto step_242 = Number(Value(1));
				                                                                      for (double index_243 = start_240; step_242 > 0 ? index_243 <= end_241 : index_243 >= end_241; index_243 += step_242)
				                                                                      {
					                                                                      auto i_244 = MakeCell(Value(index_243));
					                                                                      Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Concat(Value("gv_weaponlist"), *i_244)})), "set_value", Append(Values{}, Call(Member(context.Global("tunables"), "get_int"), Values{Concat(Value("XM22_GUN_VAN_SLOT_WEAPON_TYPE_"), Subtract(*i_244, Value(1)))})));
				                                                                      }
			                                                                      }
			                                                                      {
				                                                                      const auto start_245 = Number(Value(1));
				                                                                      const auto end_246 = Number(Value(3));
				                                                                      const auto step_247 = Number(Value(1));
				                                                                      for (double index_248 = start_245; step_247 > 0 ? index_248 <= end_246 : index_248 >= end_246; index_248 += step_247)
				                                                                      {
					                                                                      auto i_249 = MakeCell(Value(index_248));
					                                                                      Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Concat(Value("gv_throwlist"), *i_249)})), "set_value", Append(Values{}, Call(Member(context.Global("tunables"), "get_int"), Values{Concat(Value("XM22_GUN_VAN_SLOT_THROWABLE_TYPE_"), Subtract(*i_249, Value(1)))})));
				                                                                      }
			                                                                      }
			                                                                      return {};
		                                                                      })});
		auto gunvanlocs = MakeCell(([&] {
			auto table_250 = MakeTable();
			Member(table_250, Value(1)) = MakeTable({{"locname", Value("Paleto Bay")}, {"GVcoords", MakeTable({{Value(1), Negate(Value(29.532))}, {Value(2), Value(6435.136)}, {Value(3), Value(31.162)}})}});
			Member(table_250, Value(2)) = MakeTable({{"locname", Value("Grapeseed")}, {"GVcoords", MakeTable({{Value(1), Value(1705.214)}, {Value(2), Value(4819.167)}, {Value(3), Value(41.75)}})}});
			Member(table_250, Value(3)) = MakeTable({{"locname", Value("Sandy Shores")}, {"GVcoords", MakeTable({{Value(1), Value(1795.522)}, {Value(2), Value(3899.753)}, {Value(3), Value(33.869)}})}});
			Member(table_250, Value(4)) = MakeTable({{"locname", Value("Grand Senora Desert")}, {"GVcoords", MakeTable({{Value(1), Value(1335.536)}, {Value(2), Value(2758.746)}, {Value(3), Value(51.099)}})}});
			Member(table_250, Value(5)) = MakeTable({{"locname", Value("Vinewood Sign")}, {"GVcoords", MakeTable({{Value(1), Value(795.583)}, {Value(2), Value(1210.78)}, {Value(3), Value(338.962)}})}});
			Member(table_250, Value(6)) = MakeTable({{"locname", Value("Chumash")}, {"GVcoords", MakeTable({{Value(1), Negate(Value(3192.67))}, {Value(2), Value(1077.205)}, {Value(3), Value(20.594)}})}});
			Member(table_250, Value(7)) = MakeTable({{"locname", Value("Paleto Forest")}, {"GVcoords", MakeTable({{Value(1), Negate(Value(789.719))}, {Value(2), Value(5400.921)}, {Value(3), Value(33.915)}})}});
			Member(table_250, Value(8)) = MakeTable({{"locname", Value("Zancudo River")}, {"GVcoords", MakeTable({{Value(1), Negate(Value(24.384))}, {Value(2), Value(3048.167)}, {Value(3), Value(40.703)}})}});
			Member(table_250, Value(9)) = MakeTable({{"locname", Value("Power Station")}, {"GVcoords", MakeTable({{Value(1), Value(2666.786)}, {Value(2), Value(1469.324)}, {Value(3), Value(24.237)}})}});
			Member(table_250, Value(10)) = MakeTable({{"locname", Value("Lago Zancudo")}, {"GVcoords", MakeTable({{Value(1), Negate(Value(1454.966))}, {Value(2), Value(2667.503)}, {Value(3), Value(3.2)}})}});
			Member(table_250, Value(11)) = MakeTable({{"locname", Value("Thomson Scrapyard")}, {"GVcoords", MakeTable({{Value(1), Value(2340.418)}, {Value(2), Value(3054.188)}, {Value(3), Value(47.888)}})}});
			Member(table_250, Value(12)) = MakeTable({{"locname", Value("El Burro Heights")}, {"GVcoords", MakeTable({{Value(1), Value(1509.183)}, {Value(2), Negate(Value(2146.795))}, {Value(3), Value(76.853)}})}});
			Member(table_250, Value(13)) = MakeTable({{"locname", Value("Murrieta Heights")}, {"GVcoords", MakeTable({{Value(1), Value(1137.404)}, {Value(2), Negate(Value(1358.654))}, {Value(3), Value(34.322)}})}});
			Member(table_250, Value(14)) = MakeTable({{"locname", Value("Elysian Island")}, {"GVcoords", MakeTable({{Value(1), Negate(Value(57.208))}, {Value(2), Negate(Value(2658.793))}, {Value(3), Value(5.737)}})}});
			Member(table_250, Value(15)) = MakeTable({{"locname", Value("Tataviam Mountains")}, {"GVcoords", MakeTable({{Value(1), Value(1905.017)}, {Value(2), Value(565.222)}, {Value(3), Value(175.558)}})}});
			Member(table_250, Value(16)) = MakeTable({{"locname", Value("La Mesa")}, {"GVcoords", MakeTable({{Value(1), Value(974.484)}, {Value(2), Negate(Value(1718.798))}, {Value(3), Value(30.296)}})}});
			Member(table_250, Value(17)) = MakeTable({{"locname", Value("Terminal")}, {"GVcoords", MakeTable({{Value(1), Value(779.077)}, {Value(2), Negate(Value(3266.297))}, {Value(3), Value(5.719)}})}});
			Member(table_250, Value(18)) = MakeTable({{"locname", Value("La Puerta")}, {"GVcoords", MakeTable({{Value(1), Negate(Value(587.728))}, {Value(2), Negate(Value(1637.208))}, {Value(3), Value(19.611)}})}});
			Member(table_250, Value(19)) = MakeTable({{"locname", Value("La Mesa")}, {"GVcoords", MakeTable({{Value(1), Value(733.99)}, {Value(2), Negate(Value(736.803))}, {Value(3), Value(26.165)}})}});
			Member(table_250, Value(20)) = MakeTable({{"locname", Value("Del Perro")}, {"GVcoords", MakeTable({{Value(1), Negate(Value(1694.632))}, {Value(2), Negate(Value(454.082))}, {Value(3), Value(40.712)}})}});
			Member(table_250, Value(21)) = MakeTable({{"locname", Value("Magellan Ave")}, {"GVcoords", MakeTable({{Value(1), Negate(Value(1330.726))}, {Value(2), Negate(Value(1163.948))}, {Value(3), Value(4.313)}})}});
			Member(table_250, Value(22)) = MakeTable({{"locname", Value("West Vinewood")}, {"GVcoords", MakeTable({{Value(1), Negate(Value(496.618))}, {Value(2), Value(40.231)}, {Value(3), Value(52.316)}})}});
			Member(table_250, Value(23)) = MakeTable({{"locname", Value("Downtown Vinewood")}, {"GVcoords", MakeTable({{Value(1), Value(275.527)}, {Value(2), Value(66.509)}, {Value(3), Value(94.108)}})}});
			Member(table_250, Value(24)) = MakeTable({{"locname", Value("Pillbox Hill")}, {"GVcoords", MakeTable({{Value(1), Value(260.928)}, {Value(2), Negate(Value(763.35))}, {Value(3), Value(30.559)}})}});
			Member(table_250, Value(25)) = MakeTable({{"locname", Value("Little Seoul")}, {"GVcoords", MakeTable({{Value(1), Negate(Value(478.025))}, {Value(2), Negate(Value(741.45))}, {Value(3), Value(30.299)}})}});
			Member(table_250, Value(26)) = MakeTable({{"locname", Value("Alamo Sea")}, {"GVcoords", MakeTable({{Value(1), Value(894.94)}, {Value(2), Value(3603.911)}, {Value(3), Value(32.56)}})}});
			Member(table_250, Value(27)) = MakeTable({{"locname", Value("North Chumash")}, {"GVcoords", MakeTable({{Value(1), Negate(Value(2166.511))}, {Value(2), Value(4289.503)}, {Value(3), Value(48.733)}})}});
			Member(table_250, Value(28)) = MakeTable({{"locname", Value("Mount Chiliad")}, {"GVcoords", MakeTable({{Value(1), Value(1465.633)}, {Value(2), Value(6553.67)}, {Value(3), Value(13.771)}})}});
			Member(table_250, Value(29)) = MakeTable({{"locname", Value("Mirror Park")}, {"GVcoords", MakeTable({{Value(1), Value(1101.032)}, {Value(2), Negate(Value(335.172))}, {Value(3), Value(66.944)}})}});
			Member(table_250, Value(30)) = MakeTable({{"locname", Value("Davis")}, {"GVcoords", MakeTable({{Value(1), Value(149.683)}, {Value(2), Negate(Value(1655.674))}, {Value(3), Value(29.028)}})}});
			return table_250;
		}()));
		Invoke(*TS_GoonVan, "imgui", Values{MakeFunction([=, &context](const Values& args) -> Values {
			if (Truthy(Value(!Truthy(First(Call(*IsOnline, Values{}))))))
			{
				Call(Member(context.Global("ImGui"), "TextDisabled"), Values{Value("Please join a freemode session.")});
				return Values{};
			}
			Call(Member(context.Global("ImGui"), "Text"), Values{Value("Current Gun Van Location")});
			Call(Member(context.Global("ImGui"), "Separator"), Values{});
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			auto GVteleport = MakeCell(Member(*gunvanlocs, Add(First(Call(Member(context.Global("stats"), "get_packed_int"), Values{Value(41239)})), Value(1))));
			if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Concat(Value("Teleport to "), Member(*GVteleport, "locname"))}))))
			{
				auto GVloc = MakeCell(Member(*GVteleport, "GVcoords"));
				Call(Member(context.Global("PED"), "SET_PED_COORDS_KEEP_VEHICLE"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Member(*GVloc, Value(1)), Member(*GVloc, Value(2)), Add(Member(*GVloc, Value(3)), Value(1.0))});
			}
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			Call(Member(context.Global("ImGui"), "Text"), Values{Value("Slots Editor")});
			Call(Member(context.Global("ImGui"), "Separator"), Values{});
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			Call(Member(context.Global("ImGui"), "BeginGroup"), Values{});
			Call(Member(context.Global("ImGui"), "Text"), Values{Value("Weapon Slots")});
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			{
				const auto start_251 = Number(Value(1));
				const auto end_252 = Number(Value(10));
				const auto step_253 = Number(Value(1));
				for (double index_254 = start_251; step_253 > 0 ? index_254 <= end_252 : index_254 >= end_252; index_254 += step_253)
				{
					auto i_255 = MakeCell(Value(index_254));
					Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Concat(Value("gv_weaponlist"), *i_255)})), "draw", Values{});
				}
			}
			Call(Member(context.Global("ImGui"), "EndGroup"), Values{});
			Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(20)});
			Call(Member(context.Global("ImGui"), "BeginGroup"), Values{});
			Call(Member(context.Global("ImGui"), "Text"), Values{Value("Throwable Slots")});
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			{
				const auto start_256 = Number(Value(1));
				const auto end_257 = Number(Value(3));
				const auto step_258 = Number(Value(1));
				for (double index_259 = start_256; step_258 > 0 ? index_259 <= end_257 : index_259 >= end_257; index_259 += step_258)
				{
					auto i_260 = MakeCell(Value(index_259));
					Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Concat(Value("gv_throwlist"), *i_260)})), "draw", Values{});
				}
			}
			Call(Member(context.Global("ImGui"), "EndGroup"), Values{});
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			if (Truthy(Value(!Truthy(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("gv_getcurweap")})), "get_value", Values{}))))))
			{
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("gv_getcurweap")})), "set_value", Values{Value(true)});
			}
			if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Reload Slots")}))))
			{
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("gv_getcurweap")})), "set_value", Values{Value(false)});
			}
			if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
			{
				Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Reloads Gun Van slots.")});
			}
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			Call(Member(context.Global("ImGui"), "Text"), Values{Value("Others")});
			Call(Member(context.Global("ImGui"), "Separator"), Values{});
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(3.8)});
			Call(Member(context.Global("ImGui"), "BeginGroup"), Values{});
			if (Truthy(First(Call(Member(context.Global("ImGui"), "TreeNodeEx"), Values{Concat(Value("Unlock Weapons  ##"), *nomoretreeplease), Value(8219)}))))
			{
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "SameLine"), Values{Negate(Value(3.8)), Value(0)});
				Call(Member(context.Global("ImGui"), "BeginGroup"), Values{});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("WM29 Pistol")}))))
				{
					if (Truthy(First(Call(Member(context.Global("transactions"), "can_use_transactions"), Values{}))))
					{
						Call(Member(context.Global("transactions"), "run_service"), Append(Values{Value("CATEGORY_SERVICE"), Value("NET_SHOP_ACTION_SPEND"), Value("WP_WT_PISTOLXM3_t0_v0")}, Call(Member(context.Global("NETSHOPPING"), "NET_GAMESERVER_GET_PRICE"), Values{Value("WP_WT_PISTOLXM3_t0_v0"), Value("CATEGORY_WEAPON"), Value(1)})));
					}
					else
					{
						Call(Member(context.Global("stats"), "set_masked_int"), Values{Value("MPX_CHAR_WEAP_FM_PURCHASE3"), Value(31), Value(0), Value(1)});
						Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Gun Van Halen"), Value("WM29 Pistol unlocked!\nPlease change sesions to see the changes"), Value(3000)});
					}
				}
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Golf Club")}))))
				{
					Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_GCLUB_FM_AMMO_BOUGHT"), Value(1)});
					Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Gun Van Halen"), Value("Golf Club unlocked!"), Value(3000)});
				}
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Stone Hatchet")}))))
				{
					Call(Member(context.Global("stats"), "set_packed_int"), Values{Value(7315), Value(6)});
					Call(Member(context.Global("stats"), "set_packed_bool"), Values{Value(22073), Value(true)});
					Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Gun Van Halen"), Value("Stone Hatchet unlocked!"), Value(3000)});
				}
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Up-n-Atomizer")}))))
				{
					Call(Member(context.Global("stats"), "set_packed_bool"), Values{Value(25002), Value(true)});
					Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Gun Van Halen"), Value("Up-n-Atomizer unlocked!"), Value(3000)});
				}
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Service Carbine")}))))
				{
					Call(Member(context.Global("stats"), "set_packed_bool"), Values{Value(32318), Value(true)});
					Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Gun Van Halen"), Value("Service Carbine unlocked!"), Value(3000)});
				}
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Snowball Launcher")}))))
				{
					Call(Member(context.Global("stats"), "set_packed_bool"), Values{Value(42148), Value(true)});
					Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Gun Van Halen"), Value("Snowball Launcher unlocked!"), Value(3000)});
				}
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Candy Cane")}))))
				{
					Call(Member(context.Global("stats"), "set_packed_bool"), Values{Value(42249), Value(true)});
					Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Gun Van Halen"), Value("Candy Cane unlocked!"), Value(3000)});
				}
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("The Shocker")}))))
				{
					Call(Member(context.Global("stats"), "set_packed_bool"), Values{Value(51196), Value(true)});
					Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Gun Van Halen"), Value("The Shocker unlocked!"), Value(3000)});
				}
				Call(Member(context.Global("ImGui"), "EndGroup"), Values{});
				Call(Member(context.Global("ImGui"), "SameLine"), Values{});
				Call(Member(context.Global("ImGui"), "Dummy"), Values{Value(15.7), Value(0)});
			}
			else
			{
				if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
				{
					Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Unlock login bonus weapons and others that can't be purchased from the Gun Van.\nChange sessions if the unlocked weapon doesn't appear on the Weapon Wheel.")});
				}
			}
			Call(Member(context.Global("ImGui"), "EndGroup"), Values{});
			Call(Member(context.Global("ImGui"), "SameLine"), Values{});
			Call(Member(context.Global("ImGui"), "BeginGroup"), Values{});
			if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Enable Bat & Knife Liveries")}))))
			{
				Call(Member(context.Global("stats"), "set_packed_bool_range"), Values{Value(36789), Value(36808), Value(true)});
			}
			if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
			{
				Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Enables Bat & Knife Liveries permanently")});
			}
			Call(Member(context.Global("ImGui"), "EndGroup"), Values{});
			return {};
		})});
		auto hue_speed = MakeCell(Value(0.0574));
		auto flicker_intensity = MakeCell(Value(15));
		auto rainbow_speed_boost = MakeCell(Value(false));
		auto rainbow_boost_amount = MakeCell(Value(0.1));
		auto speed_level_max = MakeCell(Value(10));
		auto xenon = MakeCell(Value(true));
		auto xenon_lspeed = MakeCell(Value(2));
		auto set_plate = MakeCell(Value(true));
		auto plate_speed = MakeCell(Value(0.2));
		auto set_plate_styles = MakeCell(Value(true));
		auto set_plate_animT = MakeCell(MakeTable({{Value(1), MakeTable({{Value(1), Value(1)}, {Value(2), Value("Scroll")}})}, {Value(2), MakeTable({{Value(1), Value(2)}, {Value(2), Value("Flicker")}})}}));
		auto plate_text_scroll = MakeCell(Value("Unknown Cheats FTW    "));
		auto plate_scroll_speed = MakeCell(Value(200));
		auto plate_text_flicker = MakeCell(MakeTable({{Value(1), Value("Unknown")}, {Value(2), Value("Cheats")}, {Value(3), Value("FTW")}}));
		auto set_wheels = MakeCell(Value(true));
		auto hue = MakeCell(Value(0.0));
		auto color_modeT = MakeCell(MakeTable({{Value(1), MakeTable({{Value(1), Value(1)}, {Value(2), Value("Rainbow")}})}, {Value(2), MakeTable({{Value(1), Value(2)}, {Value(2), Value("Pastel")}})}, {Value(3), MakeTable({{Value(1), Value(3)}, {Value(2), Value("Flicker")}})}, {Value(4), MakeTable({{Value(1), Value(4)}, {Value(2), Value("Black/White Pulse")}})}, {Value(5), MakeTable({{Value(1), Value(5)}, {Value(2), Value("Overheat")}})}}));
		auto plate_char = MakeCell(Value(0));
		auto plate_text_current = MakeCell(Value(1));
		auto wheel = MakeCell(Value(0));
		auto speed_level = MakeCell(Value(0));
		// RainbowDash preserves the feature callback and its captured state.
		auto RainbowDash = MakeCell();
		*RainbowDash = MakeFunction([=, &context](const Values& args) -> Values {
			auto PlaVeh_261 = MakeCell(First(Call(Member(context.Global("PED"), "GET_VEHICLE_PED_IS_IN"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value(0)})));
			if (Truthy(First(Call(Member(context.Global("PED"), "IS_PED_SITTING_IN_ANY_VEHICLE"), Append(Values{}, Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{}))))))
			{
				auto dynamic_speed = MakeCell(Add(*hue_speed, Multiply(*speed_level, *rainbow_boost_amount)));
				*hue = Add(*hue, *dynamic_speed);
				if (Truthy(Equal(context.Global("color_mode"), Value(1))))
				{
					context.Global("r") = First(Call(Member(context.Global("math"), "floor"), Values{Add(Multiply(First(Call(Member(context.Global("math"), "sin"), Values{*hue})), Value(127)), Value(128))}));
					context.Global("g") = First(Call(Member(context.Global("math"), "floor"), Values{Add(Multiply(First(Call(Member(context.Global("math"), "sin"), Values{Add(*hue, Value(2))})), Value(127)), Value(128))}));
					context.Global("b") = First(Call(Member(context.Global("math"), "floor"), Values{Add(Multiply(First(Call(Member(context.Global("math"), "sin"), Values{Add(*hue, Value(4))})), Value(127)), Value(128))}));
				}
				else if (Truthy(Equal(context.Global("color_mode"), Value(2))))
				{
					context.Global("r") = First(Call(Member(context.Global("math"), "floor"), Values{Add(Multiply(First(Call(Member(context.Global("math"), "sin"), Values{*hue})), Value(50)), Value(205))}));
					context.Global("g") = First(Call(Member(context.Global("math"), "floor"), Values{Add(Multiply(First(Call(Member(context.Global("math"), "sin"), Values{Add(*hue, Value(2))})), Value(50)), Value(205))}));
					context.Global("b") = First(Call(Member(context.Global("math"), "floor"), Values{Add(Multiply(First(Call(Member(context.Global("math"), "sin"), Values{Add(*hue, Value(4))})), Value(50)), Value(205))}));
				}
				else if (Truthy(Equal(context.Global("color_mode"), Value(3))))
				{
					if (Truthy(LessEqual(First(Call(Member(context.Global("math"), "random"), Values{Value(1), Value(200)})), *flicker_intensity)))
					{
						auto values_262 = Append(Values{First(Call(Member(context.Global("math"), "random"), Values{Value(0), Value(255)})), First(Call(Member(context.Global("math"), "random"), Values{Value(0), Value(255)}))}, Call(Member(context.Global("math"), "random"), Values{Value(0), Value(255)}));
						context.Global("r") = At(values_262, 0);
						context.Global("g") = At(values_262, 1);
						context.Global("b") = At(values_262, 2);
					}
				}
				else if (Truthy(Equal(context.Global("color_mode"), Value(4))))
				{
					context.Global("r") = First(Call(Member(context.Global("math"), "floor"), Values{Add(Multiply(First(Call(Member(context.Global("math"), "sin"), Values{*hue})), Value(127)), Value(128))}));
					context.Global("g") = First(Call(Member(context.Global("math"), "floor"), Values{Add(Multiply(First(Call(Member(context.Global("math"), "sin"), Values{*hue})), Value(127)), Value(128))}));
					context.Global("b") = First(Call(Member(context.Global("math"), "floor"), Values{Add(Multiply(First(Call(Member(context.Global("math"), "sin"), Values{*hue})), Value(127)), Value(128))}));
				}
				else if (Truthy(Equal(context.Global("color_mode"), Value(5))))
				{
					auto speed = MakeCell(Multiply(First(Invoke(First(Call(context.Global("Vehicle"), Values{*PlaVeh_261})), "get_speed", Values{})), Value(3.6)));
					if (Truthy(Less(*speed, Value(2))))
					{
						*speed_level = Value(0);
					}
					else if (Truthy(Less(*speed, Value(5))))
					{
						auto values_263 = Values{Value(0), Value(0), Value(0)};
						context.Global("r") = At(values_263, 0);
						context.Global("g") = At(values_263, 1);
						context.Global("b") = At(values_263, 2);
					}
					else if (Truthy(Less(*speed, Value(40))))
					{
						*speed_level = Value(4);
						auto moo = MakeCell(Divide(Subtract(*speed, Value(5)), Value(35)));
						context.Global("r") = Value(0);
						context.Global("g") = First(Call(Member(context.Global("math"), "floor"), Values{Multiply(*moo, Value(255))}));
						context.Global("b") = First(Call(Member(context.Global("math"), "floor"), Values{Multiply(*moo, Value(255))}));
					}
					else if (Truthy(Less(*speed, Value(80))))
					{
						*speed_level = Value(6);
						auto moo_264 = MakeCell(Divide(Subtract(*speed, Value(40)), Value(40)));
						context.Global("r") = First(Call(Member(context.Global("math"), "floor"), Values{Multiply(*moo_264, Value(255))}));
						context.Global("g") = Value(255);
						context.Global("b") = First(Call(Member(context.Global("math"), "floor"), Values{Subtract(Value(255), Multiply(*moo_264, Value(255)))}));
					}
					else if (Truthy(Less(*speed, Value(120))))
					{
						*speed_level = Value(8);
						auto moo_265 = MakeCell(Divide(Subtract(*speed, Value(80)), Value(40)));
						context.Global("r") = Value(255);
						context.Global("g") = First(Call(Member(context.Global("math"), "floor"), Values{Subtract(Value(255), Multiply(*moo_265, Value(255)))}));
						context.Global("b") = Value(0);
					}
					else
					{
						*speed_level = Value(10);
						context.Global("r") = Value(255);
						context.Global("g") = First(Call(Member(context.Global("math"), "min"), Append(Values{Value(255)}, Call(Member(context.Global("math"), "floor"), Values{Multiply(Divide(Subtract(*speed, Value(120)), Value(60)), Value(255))}))));
						context.Global("b") = First(Call(Member(context.Global("math"), "min"), Append(Values{Value(255)}, Call(Member(context.Global("math"), "floor"), Values{Multiply(Divide(Subtract(*speed, Value(120)), Value(60)), Value(255))}))));
					}
					if (Truthy(Equal(*speed, Value(0))))
					{
						*plate_scroll_speed = Value(100000);
						*plate_speed = Value(0);
					}
					else
					{
						*plate_scroll_speed = Subtract(Value(200), Multiply(*speed_level, Value(18)));
						*plate_speed = Add(Value(0.2), Divide(*speed_level, Value(160)));
					}
				}
				Call(Member(context.Global("VEHICLE"), "SET_VEHICLE_CUSTOM_PRIMARY_COLOUR"), Values{*PlaVeh_261, context.Global("r"), context.Global("g"), context.Global("b")});
				Call(Member(context.Global("VEHICLE"), "SET_VEHICLE_CUSTOM_SECONDARY_COLOUR"), Values{*PlaVeh_261, context.Global("r"), context.Global("g"), context.Global("b")});
				Call(Member(context.Global("VEHICLE"), "TOGGLE_VEHICLE_MOD"), Values{*PlaVeh_261, Value(20), Value(1)});
				Call(Member(context.Global("VEHICLE"), "TOGGLE_VEHICLE_MOD"), Values{*PlaVeh_261, Value(22), Value(1)});
				Call(Member(context.Global("VEHICLE"), "TOGGLE_VEHICLE_MOD"), Values{*PlaVeh_261, Value(23), Value(1)});
				{
					const auto start_266 = Number(Value(0));
					const auto end_267 = Number(Value(3));
					const auto step_268 = Number(Value(1));
					for (double index_269 = start_266; step_268 > 0 ? index_269 <= end_267 : index_269 >= end_267; index_269 += step_268)
					{
						auto i_270 = MakeCell(Value(index_269));
						Call(Member(context.Global("VEHICLE"), "SET_VEHICLE_NEON_ENABLED"), Values{*PlaVeh_261, *i_270, Value(1)});
					}
				}
				Call(Member(context.Global("VEHICLE"), "SET_VEHICLE_TYRE_SMOKE_COLOR"), Values{*PlaVeh_261, context.Global("r"), context.Global("g"), context.Global("b")});
				Call(Member(context.Global("VEHICLE"), "SET_VEHICLE_NEON_COLOUR"), Values{*PlaVeh_261, context.Global("r"), context.Global("g"), context.Global("b")});
				Call(Member(context.Global("VEHICLE"), "SET_VEHICLE_WHEEL_TYPE"), Values{*PlaVeh_261, Value(8)});
				if (Truthy(*xenon))
				{
					auto xenon_i = MakeCell(MakeTable({{Value(1), Value(5)}, {Value(2), Value(6)}, {Value(3), Value(7)}, {Value(4), Value(8)}, {Value(5), Value(9)}, {Value(6), Value(10)}, {Value(7), Value(11)}, {Value(8), Value(12)}, {Value(9), Value(1)}, {Value(10), Value(2)}, {Value(11), Value(3)}, {Value(12), Value(4)}}));
					auto xenon_color = MakeCell(Add(First(Call(Member(context.Global("math"), "floor"), Values{Modulo(Multiply(*hue, *xenon_lspeed), Length(*xenon_i))})), Value(1)));
					Call(Member(context.Global("VEHICLE"), "SET_VEHICLE_XENON_LIGHT_COLOR_INDEX"), Values{*PlaVeh_261, Member(*xenon_i, *xenon_color)});
				}
				if (Truthy(*set_plate))
				{
					if (Truthy(Equal(context.Global("set_plate_anim"), Value(1))))
					{
						if (Truthy(Greater(Subtract(First(Call(Member(context.Global("MISC"), "GET_GAME_TIMER"), Values{})), *plate_char), *plate_scroll_speed)))
						{
							auto plate_segment = MakeCell(First(Call(Member(context.Global("string"), "sub"), Values{*plate_text_scroll, *plate_text_current, Add(*plate_text_current, Value(7))})));
							if (Truthy(Less(Length(*plate_segment), Value(8))))
							{
								*plate_segment = Concat(*plate_segment, First(Call(Member(context.Global("string"), "sub"), Values{*plate_text_scroll, Value(1), Subtract(Value(8), Length(*plate_segment))})));
							}
							Invoke(First(Call(context.Global("Vehicle"), Values{*PlaVeh_261})), "set_plate_text", Values{*plate_segment});
							*plate_char = First(Call(Member(context.Global("MISC"), "GET_GAME_TIMER"), Values{}));
							*plate_text_current = Add(*plate_text_current, Value(1));
							if (Truthy(Greater(*plate_text_current, Length(*plate_text_scroll))))
							{
								*plate_text_current = Value(1);
							}
						}
					}
					else if (Truthy(Equal(context.Global("set_plate_anim"), Value(2))))
					{
						auto plate_iterf = MakeCell(Add(First(Call(Member(context.Global("math"), "floor"), Values{Modulo(Multiply(*hue, *plate_speed), Length(*plate_text_flicker))})), Value(1)));
						Invoke(First(Call(context.Global("Vehicle"), Values{*PlaVeh_261})), "set_plate_text", Values{Member(*plate_text_flicker, *plate_iterf)});
					}
					if (Truthy(*set_plate_styles))
					{
						auto plate_style_id = MakeCell(MakeTable({{Value(1), Value(0)}, {Value(2), Value(1)}, {Value(3), Value(2)}, {Value(4), Value(3)}, {Value(5), Value(4)}, {Value(6), Value(5)}, {Value(7), Value(6)}, {Value(8), Value(7)}, {Value(9), Value(8)}, {Value(10), Value(9)}, {Value(11), Value(10)}, {Value(12), Value(11)}, {Value(13), Value(12)}}));
						auto plate_style = MakeCell(Add(First(Call(Member(context.Global("math"), "floor"), Values{Modulo(Multiply(*hue, *plate_speed), Length(*plate_style_id))})), Value(1)));
						Call(Member(context.Global("VEHICLE"), "SET_VEHICLE_NUMBER_PLATE_TEXT_INDEX"), Values{*PlaVeh_261, Member(*plate_style_id, *plate_style)});
					}
				}
				if (Truthy(*set_wheels))
				{
					Call(Member(context.Global("VEHICLE"), "SET_VEHICLE_MOD"), Values{*PlaVeh_261, Value(23), Value(115), Value(1)});
					*wheel = Add(*wheel, Value(1));
					if (Truthy(Greater(*wheel, Value(160))))
					{
						*wheel = Value(0);
					}
					Call(Member(context.Global("VEHICLE"), "SET_VEHICLE_EXTRA_COLOURS"), Values{*PlaVeh_261, Value(157), *wheel});
				}
				Call(Member(context.Global("VEHICLE"), "SET_VEHICLE_MOD_KIT"), Values{*PlaVeh_261, Value(0)});
			}
			return {};
		});
		// RainbowVehReset preserves the feature callback and its captured state.
		auto RainbowVehReset = MakeCell();
		*RainbowVehReset = MakeFunction([=, &context](const Values& args) -> Values {
			*speed_level = Value(0);
			*plate_scroll_speed = Value(200);
			*plate_speed = Value(0.2);
			*flicker_intensity = Value(15);
			return {};
		});
		auto rainbowveh_up = MakeCell(Value(true));
		// RainbowSpeed preserves the feature callback and its captured state.
		auto RainbowSpeed = MakeCell();
		*RainbowSpeed = MakeFunction([=, &context](const Values& args) -> Values {
			if (Truthy(And(NotEqual(context.Global("color_mode"), Value(5)), [&] {
				    return *rainbowveh_up;
			    })))
			{
				*speed_level = Add(*speed_level, Value(1));
				*plate_scroll_speed = Subtract(*plate_scroll_speed, Value(20));
				*plate_speed = Add(*plate_speed, Value(0.05));
				*flicker_intensity = Add(*flicker_intensity, Value(10));
			}
			else if (Truthy(And(NotEqual(context.Global("color_mode"), Value(5)), [&] {
				         return Value(!Truthy(*rainbowveh_up));
			         })))
			{
				if (Truthy(Less(*speed_level, Value(1))))
				{
					*speed_level = Value(1);
					*plate_scroll_speed = Value(200);
					*flicker_intensity = Value(15);
				}
				*speed_level = Subtract(*speed_level, Value(1));
				*plate_scroll_speed = Add(*plate_scroll_speed, Value(20));
				*plate_speed = Subtract(*plate_speed, Value(0.05));
				*flicker_intensity = Subtract(*flicker_intensity, Value(10));
			}
			if (Truthy(Equal(*speed_level, *speed_level_max)))
			{
				Call(Member(context.Global("HUD"), "BEGIN_TEXT_COMMAND_THEFEED_POST"), Values{Value("STRING")});
				Call(Member(context.Global("HUD"), "THEFEED_SET_BACKGROUND_COLOR_FOR_NEXT_POST"), Values{Value(6)});
				Call(Member(context.Global("HUD"), "ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME"), Values{Value("Rainbow Speed: ~y~~h~Ω MAX Ω")});
				Call(Member(context.Global("HUD"), "END_TEXT_COMMAND_THEFEED_POST_TICKER"), Values{Value(1), Value(0)});
			}
			else if (Truthy(Greater(*speed_level, *speed_level_max)))
			{
				Call(*RainbowVehReset, Values{});
				Call(Member(context.Global("HUD"), "BEGIN_TEXT_COMMAND_THEFEED_POST"), Values{Value("STRING")});
				Call(Member(context.Global("HUD"), "ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME"), Values{Value("Rainbow Speed: ~b~Default")});
				Call(Member(context.Global("HUD"), "END_TEXT_COMMAND_THEFEED_POST_TICKER"), Values{Value(1), Value(0)});
			}
			else
			{
				Call(Member(context.Global("HUD"), "BEGIN_TEXT_COMMAND_THEFEED_POST"), Values{Value("STRING")});
				Call(Member(context.Global("HUD"), "THEFEED_SET_BACKGROUND_COLOR_FOR_NEXT_POST"), Values{Value(105)});
				Call(Member(context.Global("HUD"), "ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME"), Values{Concat(Value("Rainbow Speed: ~y~"), *speed_level)});
				Call(Member(context.Global("HUD"), "END_TEXT_COMMAND_THEFEED_POST_TICKER"), Values{Value(1), Value(0)});
			}
			return {};
		});
		Call(Member(context.Global("commandmgr"), "add_looped_command"), Values{Value("rainbowveh_toggle"), Value("Rainbow Vehicles"), Value("Enables Rainbow Vehicles"), *RainbowDash});
		Call(Member(context.Global("commandmgr"), "add_bool_command"), Values{Value("rainbowveh_rainbowsb"), Value("Rainbow Speed Boost"), Value("Enables Rainbow Speed Boost"), Value(true), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                      if (Truthy(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("rainbowveh_toggle")})), "get_value", Values{}))))
			                                                                      {
				                                                                      Call(Member(context.Global("notify"), "warn"), Values{Value("WARNING: RAINBOW SPEED BOOST ENABLED!"), Value("This may potentially trigger seizures for people with photosensitive epilepsy."), Value(3000)});
			                                                                      }
			                                                                      return {};
		                                                                      }),
		                                                                   *RainbowVehReset});
		Call(Member(context.Global("commandmgr"), "add_list_command"), Values{Value("rainbowveh_colormode"), Value("Rainbow/Color mode"), Value(""), *color_modeT, Value(1), *RainbowVehReset});
		Call(Member(context.Global("commandmgr"), "add_bool_command"), Values{Value("rainbowveh_setplates"), Value("Custom Plates"), Value("Enables Custom Plates"), Value(true), Value{}});
		Call(Member(context.Global("commandmgr"), "add_bool_command"), Values{Value("rainbowveh_setplatestyle"), Value("Plate Styles Cycle"), Value("Enables Custom Plate Styles Cycle"), Value(true), Value{}});
		Call(Member(context.Global("commandmgr"), "add_list_command"), Values{Value("rainbowveh_plateanimtype"), Value("Plate Text Animation Type"), Value(""), *set_plate_animT, Value(2), Value{}});
		Call(Member(context.Global("commandmgr"), "add_bool_command"), Values{Value("rainbowveh_xenonl"), Value("Xenon lights"), Value("Enables Xenon lights"), Value(true), Value{}});
		Call(Member(context.Global("commandmgr"), "add_bool_command"), Values{Value("rainbowveh_setwheels"), Value("Custom Wheels"), Value("Enables Custom Wheels"), Value(true), Value{}});
		auto newtext = MakeCell(Value(3));
		auto values_271 = Values{Value(0), Value(0), Value(0)};
		auto NewFlicker1 = MakeCell(At(values_271, 0));
		auto NewFlicker2 = MakeCell(At(values_271, 1));
		auto NewFlicker3 = MakeCell(At(values_271, 2));
		auto testcheckbox = MakeCell(Value(false));
		Invoke(*TS_RainbowVeh, "imgui", Values{MakeFunction([=, &context](const Values& args) -> Values {
			context.Global("color_mode") = First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("rainbowveh_colormode")})), "get_value", Values{}));
			*rainbow_speed_boost = First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("rainbowveh_rainbowsb")})), "get_value", Values{}));
			*set_plate = First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("rainbowveh_setplates")})), "get_value", Values{}));
			context.Global("set_plate_anim") = First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("rainbowveh_plateanimtype")})), "get_value", Values{}));
			*set_plate_styles = First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("rainbowveh_setplatestyle")})), "get_value", Values{}));
			*xenon = First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("rainbowveh_xenonl")})), "get_value", Values{}));
			*set_wheels = First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("rainbowveh_setwheels")})), "get_value", Values{}));
			Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("rainbowveh_toggle")})), "draw", Values{});
			if (Truthy(Value(!Truthy(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("rainbowveh_toggle")})), "get_value", Values{}))))))
			{
				Call(Member(context.Global("ImGui"), "TextDisabled"), Values{Value("Somewhere over the rainbow\nOppressors fly.\nOppressors fly over the rainbow\nWhy, then, oh why don't I have money for GTA VI and a PS5?")});
				return Values{};
			}
			Call(Member(context.Global("ImGui"), "SameLine"), Values{});
			Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("rainbowveh_colormode")})), "draw", Values{});
			Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("rainbowveh_rainbowsb")})), "draw", Values{});
			if (Truthy(*rainbow_speed_boost))
			{
				Call(Member(context.Global("ImGui"), "SetNextItemWidth"), Values{Value(40)});
				Call(Member(context.Global("ImGui"), "SameLine"), Values{});
				Call(Member(context.Global("ImGui"), "InputText"), Values{Value("##rainbowspeedorcycleidk"), Concat(Value("  "), *speed_level), Member(context.Global("ImGuiInputTextFlags"), "ReadOnly")});
				Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(4)});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("-"), Value(25)}))))
				{
					*rainbowveh_up = Value(false);
					Call(*RainbowSpeed, Values{});
				}
				Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(4)});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("+"), Value(25)}))))
				{
					*rainbowveh_up = Value(true);
					Call(*RainbowSpeed, Values{});
				}
				Call(Member(context.Global("ImGui"), "SameLine"), Values{});
				Call(Member(context.Global("ImGui"), "Text"), Values{Value("Rainbow Speed")});
			}
			Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("rainbowveh_setplates")})), "draw", Values{});
			if (Truthy(*set_plate))
			{
				Call(Member(context.Global("ImGui"), "SameLine"), Values{});
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("rainbowveh_setplatestyle")})), "draw", Values{});
				Call(Member(context.Global("ImGui"), "Indent"), Values{});
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("rainbowveh_plateanimtype")})), "draw", Values{});
				Call(Member(context.Global("ImGui"), "SameLine"), Values{});
				if (Truthy(And(Equal(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("rainbowveh_plateanimtype")})), "get_value", Values{})), Value(2)), [&] {
					    return First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Add Text")}));
				    })))
				{
					*newtext = Add(*newtext, Value(1));
					if (Truthy(Equal(*newtext, Value(4))))
					{
						*NewFlicker1 = Value(4);
						Call(Member(context.Global("table"), "insert"), Values{*plate_text_flicker, Value("")});
					}
					else if (Truthy(Equal(*newtext, Value(5))))
					{
						*NewFlicker2 = Value(5);
						Call(Member(context.Global("table"), "insert"), Values{*plate_text_flicker, Value("")});
					}
					else if (Truthy(Equal(*newtext, Value(6))))
					{
						*NewFlicker3 = Value(6);
						Call(Member(context.Global("table"), "insert"), Values{*plate_text_flicker, Value("")});
					}
					else if (Truthy(Greater(*newtext, Value(6))))
					{
						*newtext = Value(6);
						Call(Member(context.Global("notify"), "warn"), Values{Value("TinkerScripts - Rainbow Vehicles"), Value("You can't add more words!"), Value(3000)});
						return Values{};
					}
				}
				if (Truthy(Greater(*newtext, Value(3))))
				{
					Call(Member(context.Global("ImGui"), "SameLine"), Values{});
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Remove Text")}))))
					{
						if (Truthy(Equal(*newtext, Value(6))))
						{
							*NewFlicker3 = Value(0);
							*newtext = Subtract(*newtext, Value(1));
							Call(Member(context.Global("table"), "remove"), Values{*plate_text_flicker, Value(6)});
						}
						else if (Truthy(Equal(*newtext, Value(5))))
						{
							*NewFlicker2 = Value(0);
							*newtext = Subtract(*newtext, Value(1));
							Call(Member(context.Global("table"), "remove"), Values{*plate_text_flicker, Value(5)});
						}
						else if (Truthy(Equal(*newtext, Value(4))))
						{
							*NewFlicker1 = Value(0);
							*newtext = Subtract(*newtext, Value(1));
							Call(Member(context.Global("table"), "remove"), Values{*plate_text_flicker, Value(4)});
						}
						else if (Truthy(Greater(*newtext, Value(3))))
						{
							*newtext = Value(3);
							return Values{};
						}
					}
				}
				if (Truthy(Equal(context.Global("set_plate_anim"), Value(2))))
				{
					if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
					{
						Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("8 characters limit per word")});
					}
					Call(Member(context.Global("ImGui"), "PushItemWidth"), Values{Value(100)});
					Member(*plate_text_flicker, Value(1)) = First(Call(Member(context.Global("ImGui"), "InputText"), Values{Value("Plate Text 1"), Member(*plate_text_flicker, Value(1))}));
					Member(*plate_text_flicker, Value(2)) = First(Call(Member(context.Global("ImGui"), "InputText"), Values{Value("Plate Text 2"), Member(*plate_text_flicker, Value(2))}));
					Member(*plate_text_flicker, Value(3)) = First(Call(Member(context.Global("ImGui"), "InputText"), Values{Value("Plate Text 3"), Member(*plate_text_flicker, Value(3))}));
					if (Truthy(Equal(*NewFlicker1, Value(4))))
					{
						Member(*plate_text_flicker, *NewFlicker1) = First(Call(Member(context.Global("ImGui"), "InputText"), Values{Concat(Value("Plate Text "), Concat(*NewFlicker1, Value(""))), Member(*plate_text_flicker, *NewFlicker1)}));
					}
					if (Truthy(Equal(*NewFlicker2, Value(5))))
					{
						Member(*plate_text_flicker, *NewFlicker2) = First(Call(Member(context.Global("ImGui"), "InputText"), Values{Concat(Value("Plate Text "), Concat(*NewFlicker2, Value(""))), Member(*plate_text_flicker, *NewFlicker2)}));
					}
					if (Truthy(Equal(*NewFlicker3, Value(6))))
					{
						Member(*plate_text_flicker, *NewFlicker3) = First(Call(Member(context.Global("ImGui"), "InputText"), Values{Concat(Value("Plate Text "), Concat(*NewFlicker3, Value(""))), Member(*plate_text_flicker, *NewFlicker3)}));
					}
					Call(Member(context.Global("ImGui"), "PopItemWidth"), Values{});
				}
				else
				{
					Call(Member(context.Global("ImGui"), "NewLine"), Values{});
					Call(Member(context.Global("ImGui"), "PushItemWidth"), Values{Value(250)});
					*plate_text_scroll = First(Call(Member(context.Global("ImGui"), "InputText"), Values{Value("Plate Text"), *plate_text_scroll}));
					Call(Member(context.Global("ImGui"), "PopItemWidth"), Values{});
				}
				Call(Member(context.Global("ImGui"), "Unindent"), Values{});
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			}
			Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("rainbowveh_xenonl")})), "draw", Values{});
			Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("rainbowveh_setwheels")})), "draw", Values{});
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			Call(Member(context.Global("ImGui"), "Spacing"), Values{});
			*testcheckbox = First(Call(Member(context.Global("ImGui"), "Checkbox"), Values{Value("Base Settings"), *testcheckbox}));
			if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
			{
				Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Allows you to edit the base variables")});
			}
			if (Truthy(*testcheckbox))
			{
				Call(Member(context.Global("ImGui"), "SameLine"), Values{});
				Call(Member(context.Global("ImGui"), "TextDisabled"), Values{Value("Please be careful with these.")});
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "PushItemWidth"), Values{Value(180)});
				*hue_speed = First(Call(Member(context.Global("ImGui"), "SliderFloat"), Values{Value("Hue Speed"), *hue_speed, Value(0.001), Value(2.0)}));
				*rainbow_boost_amount = First(Call(Member(context.Global("ImGui"), "SliderFloat"), Values{Value("Rainbow Speed Multiplier"), *rainbow_boost_amount, Value(0.001), Value(2.0)}));
				*speed_level_max = First(Call(Member(context.Global("ImGui"), "SliderInt"), Values{Value("Max Rainbow Speed"), *speed_level_max, Value(5), Value(50)}));
				Call(Member(context.Global("ImGui"), "PopItemWidth"), Values{});
			}
			else
			{
				*hue_speed = Value(0.0574);
				*rainbow_boost_amount = Value(0.1);
				*speed_level_max = Value(10);
			}
			return {};
		})});
		// CrackCB preserves the feature callback and its captured state.
		auto CrackCB = MakeCell();
		*CrackCB = MakeFunction([=, &context](const Values& args) -> Values {
			if (Truthy(Equal(First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_2020"), Add(Value(29373), Value(12))})), "get_int", Values{})), Negate(Value(1)))))
			{
				Call(Member(context.Global("notify"), "error"), Values{Value("TinkerScripts - Cluckin' Raider"), Value("You must be at the Safe first!"), Value(3000)});
				return Values{};
			}
			{
				const auto start_272 = Number(Value(0));
				const auto end_273 = Number(Value(2));
				const auto step_274 = Number(Value(1));
				for (double index_275 = start_272; step_274 > 0 ? index_275 <= end_273 : index_275 >= end_273; index_275 += step_274)
				{
					auto i_276 = MakeCell(Value(index_275));
					Invoke(First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_2020"), Add(Add(Value(32726), Value(1)), Value(1))})), "at", Values{*i_276, Value(2)})), "set_int", Values{Value(0)});
					Invoke(First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_2020"), Add(Value(32726), Value(1))})), "at", Values{*i_276, Value(2)})), "set_float", Values{Value(0)});
					Call(Member(context.Global("script"), "yield"), Values{Value(100)});
					Call(Member(context.Global("PAD"), "SET_CONTROL_VALUE_NEXT_FRAME"), Values{Value(0), Value(237), Value(1.0)});
				}
			}
			return {};
		});
		// CBTpToPC preserves the feature callback and its captured state.
		auto CBTpToPC = MakeCell();
		*CBTpToPC = MakeFunction([=, &context](const Values& args) -> Values {
			auto CB_PCcoords = MakeCell(First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_2020"), Add(Value(34538), Value(5))})), "get_vector3", Values{})));
			Call(Member(context.Global("PED"), "SET_PED_COORDS_KEEP_VEHICLE"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Member(*CB_PCcoords, "x"), Member(*CB_PCcoords, "y"), Add(Member(*CB_PCcoords, "z"), Value(1.0))});
			return {};
		});
		// CBTpToCrate preserves the feature callback and its captured state.
		auto CBTpToCrate = MakeCell();
		*CBTpToCrate = MakeFunction([=, &context](const Values& args) -> Values {
			if (Truthy(NotEqual(First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_2020"), Value(21088)})), "get_int", Values{})), Value(1149534208))))
			{
				Call(Member(context.Global("notify"), "error"), Values{Value("TinkerScripts - Cluckin' Raider"), Value("You must be at the Storage Facility first!"), Value(3000)});
				return Values{};
			}
			auto CB_Cratecoords = MakeCell(First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_2020"), Add(Value(29373), Value(1192))})), "get_vector3", Values{})));
			Call(Member(context.Global("PED"), "SET_PED_COORDS_KEEP_VEHICLE"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Member(*CB_Cratecoords, "x"), Member(*CB_Cratecoords, "y"), Member(*CB_Cratecoords, "z")});
			return {};
		});
		Call(Member(context.Global("commandmgr"), "add_looped_command"), Values{Value("cbr_opendoors"), Value("Open All Doors##cbr"), Value("Opens All Doors"), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                        {
				                                                                        Iterator iterator_277(Append(Values{}, Call(context.Global("ipairs"), Append(Values{}, Call(Member(context.Global("entities"), "get_all_objects_as_handles"), Values{})))));
				                                                                        for (auto row_278 = iterator_277.Next(); !row_278.empty() && !row_278[0].IsNil(); row_278 = iterator_277.Next())
				                                                                        {
					                                                                        auto __279 = MakeCell(At(row_278, 0));
					                                                                        auto h_280 = MakeCell(At(row_278, 1));
					                                                                        if (Truthy(Equal(First(Invoke(First(Call(context.Global("Entity"), Values{*h_280})), "get_model", Values{})), Value(258537691))))
					                                                                        {
						                                                                        if (Truthy(First(Call(Member(context.Global("ENTITY"), "IS_ENTITY_A_MISSION_ENTITY"), Values{*h_280}))))
						                                                                        {
							                                                                        Call(Member(context.Global("ENTITY"), "SET_ENTITY_AS_MISSION_ENTITY"), Values{*h_280, Value(false), Value(true)});
						                                                                        }
						                                                                        Invoke(First(Call(context.Global("Entity"), Values{*h_280})), "delete", Values{});
					                                                                        }
				                                                                        }
			                                                                        }
			                                                                        if (Truthy(Value(!Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("fm_mission_controller_2020")}))))))
			                                                                        {
				                                                                        Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("cbr_opendoors")})), "set_value", Values{Value(false)});
			                                                                        }
			                                                                        return {};
		                                                                        }),
		                                                                     MakeFunction([=, &context](const Values& args) -> Values {
			                                                                     {
				                                                                     Iterator iterator_281(Append(Values{}, Call(context.Global("ipairs"), Append(Values{}, Call(Member(context.Global("entities"), "get_all_objects_as_handles"), Values{})))));
				                                                                     for (auto row_282 = iterator_281.Next(); !row_282.empty() && !row_282[0].IsNil(); row_282 = iterator_281.Next())
				                                                                     {
					                                                                     auto __283 = MakeCell(At(row_282, 0));
					                                                                     auto h_284 = MakeCell(At(row_282, 1));
					                                                                     if (Truthy(Equal(First(Invoke(First(Call(context.Global("Entity"), Values{*h_284})), "get_model", Values{})), Negate(Value(952356348)))))
					                                                                     {
						                                                                     if (Truthy(First(Call(Member(context.Global("ENTITY"), "IS_ENTITY_A_MISSION_ENTITY"), Values{*h_284}))))
						                                                                     {
							                                                                     Call(Member(context.Global("ENTITY"), "SET_ENTITY_AS_MISSION_ENTITY"), Values{*h_284, Value(false), Value(true)});
						                                                                     }
						                                                                     Invoke(First(Call(context.Global("Entity"), Values{*h_284})), "delete", Values{});
						                                                                     break;
					                                                                     }
				                                                                     }
			                                                                     }
			                                                                     return {};
		                                                                     })});
		// CrackSH preserves the feature callback and its captured state.
		auto CrackSH = MakeCell();
		*CrackSH = MakeFunction([=, &context](const Values& args) -> Values {
			if (Truthy(Equal(First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_content_stash_house"), Add(Value(153), Value(16))})), "get_int", Values{})), Negate(Value(1)))))
			{
				Call(Member(context.Global("notify"), "error"), Values{Value("TinkerScripts - Stash House"), Value("You must be at the Safe first!"), Value(3000)});
				return Values{};
			}
			{
				const auto start_285 = Number(Value(0));
				const auto end_286 = Number(Value(2));
				const auto step_287 = Number(Value(1));
				for (double index_288 = start_285; step_287 > 0 ? index_288 <= end_286 : index_288 >= end_286; index_288 += step_287)
				{
					auto i_289 = MakeCell(Value(index_288));
					Invoke(First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_content_stash_house"), Add(Add(Value(153), Value(22)), Value(1))})), "at", Values{*i_289, Value(2)})), "set_int", Values{Value(0)});
					Invoke(First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_content_stash_house"), Add(Value(153), Value(22))})), "at", Values{*i_289, Value(2)})), "set_float", Values{Value(0)});
					Call(Member(context.Global("script"), "yield"), Values{Value(100)});
					Call(Member(context.Global("PAD"), "SET_CONTROL_VALUE_NEXT_FRAME"), Values{Value(0), Value(237), Value(1.0)});
				}
			}
			return {};
		});
		// TS_KortzStart preserves the feature callback and its captured state.
		auto TS_KortzStart = MakeCell();
		*TS_KortzStart = MakeFunction([=, &context](const Values& args) -> Values {
			if (Truthy(Equal(First(Invoke(First(Invoke(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Value(1893070)})), "at", Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_ID"), Values{})), Value(625)})), "at", Values{Value(10)})), "get_int", Values{})), Negate(Value(1)))))
			{
				Call(Member(context.Global("notify"), "error"), Values{Value("TinkerScripts - Kortz Center Cracker"), Value("You must register as a Boss first!"), Value(3000)});
				return Values{};
			}
			if (Truthy(Value(!Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("fm_maintain_transition_players")}))))))
			{
				Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Add(Value(1980570), Value(732)), Value(5))})), "set_int", Values{Value(1)});
				{
					const auto start_290 = Number(Value(1));
					const auto end_291 = Number(Value(5));
					const auto step_292 = Number(Value(1));
					for (double index_293 = start_290; step_292 > 0 ? index_293 <= end_291 : index_293 >= end_291; index_293 += step_292)
					{
						auto i_294 = MakeCell(Value(index_293));
						Call(Member(context.Global("script"), "yield"), Values{Value(100)});
						Call(Member(context.Global("PAD"), "SET_CONTROL_VALUE_NEXT_FRAME"), Values{Value(0), Value(187), Value(1.0)});
						Call(Member(context.Global("script"), "yield"), Values{Value(100)});
						Call(Member(context.Global("PAD"), "SET_CONTROL_VALUE_NEXT_FRAME"), Values{Value(0), Value(190), Value(1.0)});
					}
				}
				Call(Member(context.Global("script"), "yield"), Values{Value(100)});
				Call(Member(context.Global("PAD"), "SET_CONTROL_VALUE_NEXT_FRAME"), Values{Value(0), Value(201), Value(1.0)});
			}
			else
			{
				Call(Member(context.Global("script"), "yield"), Values{Value(100)});
				Call(Member(context.Global("PAD"), "SET_CONTROL_VALUE_NEXT_FRAME"), Values{Value(0), Value(188), Value(1.0)});
				Call(Member(context.Global("script"), "yield"), Values{Value(100)});
				Call(Member(context.Global("PAD"), "SET_CONTROL_VALUE_NEXT_FRAME"), Values{Value(0), Value(201), Value(1.0)});
			}
			return {};
		});
		// TS_KortzFP preserves the feature callback and its captured state.
		auto TS_KortzFP = MakeCell();
		*TS_KortzFP = MakeFunction([=, &context](const Values& args) -> Values {
			Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_v3"), Value(26866)})), "set_int", Values{Value(5)});
			return {};
		});
		// TS_KortzPCAC preserves the feature callback and its captured state.
		auto TS_KortzPCAC = MakeCell();
		*TS_KortzPCAC = MakeFunction([=, &context](const Values& args) -> Values {
			{
				const auto start_295 = Number(Value(0));
				const auto end_296 = Number(Value(2));
				const auto step_297 = Number(Value(1));
				for (double index_298 = start_295; step_297 > 0 ? index_298 <= end_296 : index_298 >= end_296; index_298 += step_297)
				{
					auto i_299 = MakeCell(Value(index_298));
					Invoke(First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_v3"), Add(Add(Value(32818), Value(1)), Value(1))})), "at", Values{*i_299, Value(2)})), "set_int", Values{Value(0)});
					Call(Member(context.Global("script"), "yield"), Values{Value(100)});
					Call(Member(context.Global("PAD"), "SET_CONTROL_VALUE_NEXT_FRAME"), Values{Value(0), Value(237), Value(1.0)});
				}
			}
			return {};
		});
		auto K26_Leasy = MakeCell(Value(false));
		auto K26_Lnodamage = MakeCell(Value(false));
		// TS_KortzPwrL preserves the feature callback and its captured state.
		auto TS_KortzPwrL = MakeCell();
		*TS_KortzPwrL = MakeFunction([=, &context](const Values& args) -> Values {
			Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_v3"), Value(70416)})), "set_int", Values{Value(4294784)});
			Invoke(First(Call(context.Global("ScriptGlobal"), Values{Value(1935711)})), "set_int", Values{Value(1)});
			return {};
		});
		// TS_KortzSN preserves the feature callback and its captured state.
		auto TS_KortzSN = MakeCell();
		*TS_KortzSN = MakeFunction([=, &context](const Values& args) -> Values {
			Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_v3"), Value(27914)})), "set_int", Values{Value(5)});
			return {};
		});
		// TS_KortzPaint preserves the feature callback and its captured state.
		auto TS_KortzPaint = MakeCell();
		*TS_KortzPaint = MakeFunction([=, &context](const Values& args) -> Values {
			Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_v3"), Add(Value(29355), Value(11))})), "set_int", Values{Value(15)});
			Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_v3"), Add(Value(29355), Value(11))})), "set_int", Values{Value(17)});
			Call(Member(context.Global("script"), "yield"), Values{Value(1000)});
			Call(Member(context.Global("PAD"), "SET_CONTROL_VALUE_NEXT_FRAME"), Values{Value(0), Value(237), Value(1.0)});
			return {};
		});
		// TS_KortzPaint2 preserves the feature callback and its captured state.
		auto TS_KortzPaint2 = MakeCell();
		*TS_KortzPaint2 = MakeFunction([=, &context](const Values& args) -> Values {
			Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_v3"), Add(Value(29355), Value(11))})), "set_int", Values{Value(3)});
			Call(Member(context.Global("PAD"), "SET_CONTROL_VALUE_NEXT_FRAME"), Values{Value(0), Value(219), Value(1.0)});
			return {};
		});
		// TS_KortzDC preserves the feature callback and its captured state.
		auto TS_KortzDC = MakeCell();
		*TS_KortzDC = MakeFunction([=, &context](const Values& args) -> Values {
			{
				const auto start_300 = Number(Value(0));
				const auto end_301 = Number(Value(7));
				const auto step_302 = Number(Value(1));
				for (double index_303 = start_300; step_302 > 0 ? index_303 <= end_301 : index_303 >= end_301; index_303 += step_302)
				{
					auto b_304 = MakeCell(Value(index_303));
					Invoke(First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_v3"), Value(1388)})), "at", Values{*b_304, Value(4)})), "set_int", Values{Value(1)});
				}
			}
			return {};
		});
		// TS_KortzGlass preserves the feature callback and its captured state.
		auto TS_KortzGlass = MakeCell();
		*TS_KortzGlass = MakeFunction([=, &context](const Values& args) -> Values {
			{
				const auto start_305 = Number(Value(0));
				const auto end_306 = Number(Value(4));
				const auto step_307 = Number(Value(1));
				for (double index_308 = start_305; step_307 > 0 ? index_308 <= end_306 : index_308 >= end_306; index_308 += step_307)
				{
					auto g = MakeCell(Value(index_308));
					Invoke(First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_v3"), Add(Value(32855), Value(3))})), "at", Values{*g, Value(13)})), "set_float", Values{Value(100)});
				}
			}
			return {};
		});
		// TS_KortzFinish preserves the feature callback and its captured state.
		auto TS_KortzFinish = MakeCell();
		*TS_KortzFinish = MakeFunction([=, &context](const Values& args) -> Values {
			Call(Member(context.Global("script"), "run_in_callback"), Values{MakeFunction([=, &context](const Values& args) -> Values {
				while (Truthy(Value(true)))
				{
					if (Truthy(And(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("fm_mission_controller_v3")})), [&] {
						    return Equal(First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_v3"), Add(Value(57250), Value(1))})), "get_int", Values{})), Value(17718273));
					    })))
					{
						Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_v3"), Value(57250)})), "set_int", Values{Value(9)});
						Invoke(First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_v3"), Add(Value(57250), Value(1776))})), "at", Values{Value(0), Value(1)})), "set_int", Values{Value(1250)});
					}
					else if (Truthy(And(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("fm_mission_controller_v3")})), [&] {
						         return Equal(First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_v3"), Add(Value(57250), Value(1))})), "get_int", Values{})), Value(17456129));
					         })))
					{
						Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_v3"), Value(57250)})), "set_int", Values{Value(9)});
						Invoke(First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_v3"), Add(Value(57250), Value(1776))})), "at", Values{Value(0), Value(1)})), "set_int", Values{Value(1250)});
					}
					else if (Truthy(And(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("fm_mission_controller_v3")})), [&] {
						         return Equal(First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_v3"), Add(Value(57250), Value(1))})), "get_int", Values{})), Value(17456128));
					         })))
					{
						if (Truthy(First(Call(Member(context.Global("HUD"), "IS_HELP_MESSAGE_BEING_DISPLAYED"), Values{}))))
						{
							Call(context.Global("FindNewSession"), Values{});
							return Values{};
						}
					}
					Call(Member(context.Global("script"), "yield"), Values{Value(0)});
				}
				return {};
			})});
			return {};
		});
		auto kortz_doors = MakeCell(([&] {
			auto table_309 = MakeTable();
			Member(table_309, Value(1)) = Negate(Value(1989843262));
			Member(table_309, Value(2)) = Negate(Value(636649650));
			Member(table_309, Value(3)) = Negate(Value(763344841));
			Member(table_309, Value(4)) = Value(1671614326);
			Member(table_309, Value(5)) = Value(275743652);
			Member(table_309, Value(6)) = Value(507130236);
			Member(table_309, Value(7)) = Negate(Value(1286517865));
			Member(table_309, Value(8)) = Negate(Value(421484378));
			Member(table_309, Value(9)) = Negate(Value(1864833911));
			Member(table_309, Value(10)) = Negate(Value(1925236194));
			Member(table_309, Value(11)) = Value(2049614127);
			Member(table_309, Value(12)) = Value(454689787);
			Member(table_309, Value(13)) = Negate(Value(79540289));
			Member(table_309, Value(14)) = Value(390646248);
			Member(table_309, Value(15)) = Value(1895050696);
			Member(table_309, Value(16)) = Value(361395225);
			Member(table_309, Value(17)) = Value(1430444526);
			Member(table_309, Value(18)) = Negate(Value(861563755));
			Member(table_309, Value(19)) = Value(956014487);
			Member(table_309, Value(20)) = Value(897048274);
			Member(table_309, Value(21)) = Negate(Value(1120166274));
			Member(table_309, Value(22)) = Negate(Value(1717616443));
			Member(table_309, Value(23)) = Negate(Value(1854411159));
			Member(table_309, Value(24)) = Negate(Value(1158448783));
			Member(table_309, Value(25)) = Negate(Value(646395663));
			Member(table_309, Value(26)) = Negate(Value(780184685));
			Member(table_309, Value(27)) = Value(511378549);
			Member(table_309, Value(28)) = Negate(Value(159586315));
			Member(table_309, Value(29)) = Negate(Value(434711109));
			Member(table_309, Value(30)) = Negate(Value(1993257312));
			Member(table_309, Value(31)) = Value(186527079);
			return table_309;
		}()));
		Call(Member(context.Global("commandmgr"), "add_looped_command"), Values{Value("kortz_opendoors"), Value("Open All Doors##kortz"), Value("Opens All Doors"), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                        {
				                                                                        Iterator iterator_310(Append(Values{}, Call(context.Global("ipairs"), Values{*kortz_doors})));
				                                                                        for (auto row_311 = iterator_310.Next(); !row_311.empty() && !row_311[0].IsNil(); row_311 = iterator_310.Next())
				                                                                        {
					                                                                        auto __312 = MakeCell(At(row_311, 0));
					                                                                        auto d = MakeCell(At(row_311, 1));
					                                                                        Call(Member(context.Global("OBJECT"), "DOOR_SYSTEM_SET_DOOR_STATE"), Values{*d, Value(0), Value(false), Value(true)});
				                                                                        }
			                                                                        }
			                                                                        if (Truthy(Value(!Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("fm_mission_controller_v3")}))))))
			                                                                        {
				                                                                        Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("kortz_opendoors")})), "set_value", Values{Value(false)});
			                                                                        }
			                                                                        return {};
		                                                                        }),
		                                                                     MakeFunction([=, &context](const Values& args) -> Values {
			                                                                     {
				                                                                     Iterator iterator_313(Append(Values{}, Call(context.Global("ipairs"), Values{MakeTable({{Value(1), Value(0)}, {Value(2), Value(1)}, {Value(3), Value(5)}, {Value(4), Value(6)}, {Value(5), Value(7)}, {Value(6), Value(20)}, {Value(7), Value(21)}})})));
				                                                                     for (auto row_314 = iterator_313.Next(); !row_314.empty() && !row_314[0].IsNil(); row_314 = iterator_313.Next())
				                                                                     {
					                                                                     auto __315 = MakeCell(At(row_314, 0));
					                                                                     auto t = MakeCell(At(row_314, 1));
					                                                                     Invoke(First(Invoke(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Value(4980736), Value(29174))})), "at", Values{*t, Value(333)})), "at", Values{Value(68)})), "set_int", Values{Value(0)});
					                                                                     Invoke(First(Invoke(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Value(4980736), Value(29174))})), "at", Values{*t, Value(333)})), "at", Values{Value(143)})), "set_int", Values{Value(0)});
				                                                                     }
			                                                                     }
			                                                                     {
				                                                                     Iterator iterator_316(Append(Values{}, Call(context.Global("ipairs"), Append(Values{}, Call(Member(context.Global("entities"), "get_all_objects_as_handles"), Values{})))));
				                                                                     for (auto row_317 = iterator_316.Next(); !row_317.empty() && !row_317[0].IsNil(); row_317 = iterator_316.Next())
				                                                                     {
					                                                                     auto __318 = MakeCell(At(row_317, 0));
					                                                                     auto h_319 = MakeCell(At(row_317, 1));
					                                                                     if (Truthy(Equal(First(Invoke(First(Call(context.Global("Entity"), Values{*h_319})), "get_model", Values{})), Negate(Value(1974437642)))))
					                                                                     {
						                                                                     if (Truthy(First(Call(Member(context.Global("ENTITY"), "IS_ENTITY_A_MISSION_ENTITY"), Values{*h_319}))))
						                                                                     {
							                                                                     Call(Member(context.Global("ENTITY"), "SET_ENTITY_AS_MISSION_ENTITY"), Values{*h_319, Value(false), Value(true)});
						                                                                     }
						                                                                     Invoke(First(Call(context.Global("Entity"), Values{*h_319})), "delete", Values{});
						                                                                     break;
					                                                                     }
				                                                                     }
			                                                                     }
			                                                                     return {};
		                                                                     })});
		auto kortz_cams = MakeCell(MakeTable({{Negate(Value(630156392)), Value(true)}, {Value(1301284833), Value(true)}, {Value(19326568), Value(true)}, {Value(302330246), Value(true)}, {Value(506184131), Value(true)}, {Value(825790471), Value(true)}, {Value(365062191), Value(true)}, {Negate(Value(301330169)), Value(true)}, {Value(1417894488), Value(true)}, {Negate(Value(247409812)), Value(true)}, {Negate(Value(1233322078)), Value(true)}}));
		auto kortzcamtimer = MakeCell(Value(0));
		// KortzDelCams preserves the feature callback and its captured state.
		auto KortzDelCams = MakeCell();
		*KortzDelCams = MakeFunction([=, &context](const Values& args) -> Values {
			{
				Iterator iterator_320(Append(Values{}, Call(context.Global("ipairs"), Append(Values{}, Call(Member(context.Global("entities"), "get_all_objects_as_handles"), Values{})))));
				for (auto row_321 = iterator_320.Next(); !row_321.empty() && !row_321[0].IsNil(); row_321 = iterator_320.Next())
				{
					auto __322 = MakeCell(At(row_321, 0));
					auto h_323 = MakeCell(At(row_321, 1));
					if (Truthy(Member(*kortz_cams, First(Invoke(First(Call(context.Global("Entity"), Values{*h_323})), "get_model", Values{})))))
					{
						if (Truthy(First(Call(Member(context.Global("ENTITY"), "IS_ENTITY_A_MISSION_ENTITY"), Values{*h_323}))))
						{
							Call(Member(context.Global("ENTITY"), "SET_ENTITY_AS_MISSION_ENTITY"), Values{*h_323, Value(false), Value(true)});
						}
						Invoke(First(Call(context.Global("Entity"), Values{*h_323})), "delete", Values{});
					}
				}
			}
			return {};
		});
		Call(Member(context.Global("commandmgr"), "add_looped_command"), Values{Value("kortz_delcams"), Value("Disable All Cameras"), Value("Disables All Cameras"), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                        if (Truthy(Less(First(Call(Member(context.Global("util"), "time"), Values{})), Add(*kortzcamtimer, Value(12000)))))
			                                                                        {
				                                                                        Call(*KortzDelCams, Values{});
			                                                                        }
			                                                                        else
			                                                                        {
				                                                                        Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("kortz_delcams")})), "set_value", Values{Value(false)});
			                                                                        }
			                                                                        if (Truthy(Value(!Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("fm_mission_controller_v3")}))))))
			                                                                        {
				                                                                        Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("kortz_delcams")})), "set_value", Values{Value(false)});
			                                                                        }
			                                                                        return {};
		                                                                        }),
		                                                                     MakeFunction([=, &context](const Values& args) -> Values {
			                                                                     *kortzcamtimer = First(Call(Member(context.Global("util"), "time"), Values{}));
			                                                                     Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Kortz Center Cracker"), Value("Cameras Disabled!\nCameras rebooting in 12 seconds.")});
			                                                                     return {};
		                                                                     }),
		                                                                     MakeFunction([=, &context](const Values& args) -> Values {
			                                                                     *kortzcamtimer = Value(0);
			                                                                     return {};
		                                                                     })});
		auto ch_doors = MakeCell(([&] {
			auto table_324 = MakeTable();
			Member(table_324, Value(1)) = Negate(Value(104143029));
			Member(table_324, Value(2)) = Value(1633148016);
			Member(table_324, Value(3)) = Negate(Value(1730152782));
			Member(table_324, Value(4)) = Negate(Value(1450870697));
			Member(table_324, Value(5)) = Negate(Value(1870500531));
			Member(table_324, Value(6)) = Value(686757512);
			Member(table_324, Value(7)) = Value(1512975268);
			Member(table_324, Value(8)) = Negate(Value(1260608781));
			Member(table_324, Value(9)) = Negate(Value(503379382));
			Member(table_324, Value(10)) = Value(1649238266);
			Member(table_324, Value(11)) = Negate(Value(230581957));
			Member(table_324, Value(12)) = Value(277015006);
			Member(table_324, Value(13)) = Negate(Value(791522594));
			Member(table_324, Value(14)) = Negate(Value(1854580890));
			Member(table_324, Value(15)) = Value(714193794);
			Member(table_324, Value(16)) = Value(1037783830);
			Member(table_324, Value(17)) = Negate(Value(1798315374));
			Member(table_324, Value(18)) = Negate(Value(219406021));
			Member(table_324, Value(19)) = Negate(Value(7449383));
			Member(table_324, Value(20)) = Value(1467373945);
			Member(table_324, Value(21)) = Value(280344137);
			Member(table_324, Value(22)) = Value(1391515407);
			Member(table_324, Value(23)) = Value(320263035);
			Member(table_324, Value(24)) = Negate(Value(545816134));
			Member(table_324, Value(25)) = Negate(Value(1735742237));
			Member(table_324, Value(26)) = Value(327933309);
			Member(table_324, Value(27)) = Negate(Value(1164440132));
			Member(table_324, Value(28)) = Negate(Value(1865839062));
			Member(table_324, Value(29)) = Negate(Value(1608865147));
			Member(table_324, Value(30)) = Negate(Value(1783267585));
			Member(table_324, Value(31)) = Negate(Value(104143029));
			return table_324;
		}()));
		auto ch_lstacks = MakeCell(MakeTable({{Negate(Value(2143906120)), Value(true)}, {Value(1228147776), Value(true)}, {Negate(Value(2143192170)), Value(true)}, {Negate(Value(180074230)), Value(true)}, {Value(822904642), Value(true)}}));
		auto ch_pnts = MakeCell(MakeTable({{Negate(Value(327407186)), Value(true)}, {Negate(Value(590378411)), Value(true)}, {Negate(Value(1035053785)), Value(true)}, {Negate(Value(775883720)), Value(true)}, {Negate(Value(1073786699)), Value(true)}, {Negate(Value(1227899306)), Value(true)}, {Negate(Value(1701280280)), Value(true)}, {Negate(Value(1968970241)), Value(true)}}));
		// ch_delcam preserves the feature callback and its captured state.
		auto ch_delcam = MakeCell();
		*ch_delcam = MakeFunction([=, &context](const Values& args) -> Values {
			{
				Iterator iterator_325(Append(Values{}, Call(context.Global("ipairs"), Append(Values{}, Call(Member(context.Global("entities"), "get_all_objects_as_handles"), Values{})))));
				for (auto row_326 = iterator_325.Next(); !row_326.empty() && !row_326[0].IsNil(); row_326 = iterator_325.Next())
				{
					auto __327 = MakeCell(At(row_326, 0));
					auto h_328 = MakeCell(At(row_326, 1));
					if (Truthy(Equal(First(Invoke(First(Call(context.Global("Entity"), Values{*h_328})), "get_model", Values{})), Negate(Value(1233322078)))))
					{
						if (Truthy(First(Call(Member(context.Global("ENTITY"), "IS_ENTITY_A_MISSION_ENTITY"), Values{*h_328}))))
						{
							Call(Member(context.Global("ENTITY"), "SET_ENTITY_AS_MISSION_ENTITY"), Values{*h_328, Value(false), Value(true)});
						}
						Invoke(First(Call(context.Global("Entity"), Values{*h_328})), "delete", Values{});
					}
				}
			}
			return {};
		});
		Call(Member(context.Global("commandmgr"), "add_looped_command"), Values{Value("ch_opendoors"), Value("Open All Doors##Cayo"), Value("Opens All Doors"), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                        {
				                                                                        Iterator iterator_329(Append(Values{}, Call(context.Global("ipairs"), Values{*ch_doors})));
				                                                                        for (auto row_330 = iterator_329.Next(); !row_330.empty() && !row_330[0].IsNil(); row_330 = iterator_329.Next())
				                                                                        {
					                                                                        auto __331 = MakeCell(At(row_330, 0));
					                                                                        auto d_332 = MakeCell(At(row_330, 1));
					                                                                        Call(Member(context.Global("OBJECT"), "DOOR_SYSTEM_SET_DOOR_STATE"), Values{*d_332, Value(0), Value(false), Value(true)});
				                                                                        }
			                                                                        }
			                                                                        if (Truthy(Value(!Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("fm_mission_controller_2020")}))))))
			                                                                        {
				                                                                        Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("ch_opendoors")})), "set_value", Values{Value(false)});
			                                                                        }
			                                                                        return {};
		                                                                        }),
		                                                                     MakeFunction([=, &context](const Values& args) -> Values {
			                                                                     {
				                                                                     Iterator iterator_333(Append(Values{}, Call(context.Global("ipairs"), Append(Values{}, Call(Member(context.Global("entities"), "get_all_objects_as_handles"), Values{})))));
				                                                                     for (auto row_334 = iterator_333.Next(); !row_334.empty() && !row_334[0].IsNil(); row_334 = iterator_333.Next())
				                                                                     {
					                                                                     auto __335 = MakeCell(At(row_334, 0));
					                                                                     auto h_336 = MakeCell(At(row_334, 1));
					                                                                     if (Truthy(Member(*ch_lstacks, First(Invoke(First(Call(context.Global("Entity"), Values{*h_336})), "get_model", Values{})))))
					                                                                     {
						                                                                     if (Truthy(First(Call(Member(context.Global("ENTITY"), "IS_ENTITY_A_MISSION_ENTITY"), Values{*h_336}))))
						                                                                     {
							                                                                     Call(Member(context.Global("ENTITY"), "SET_ENTITY_AS_MISSION_ENTITY"), Values{*h_336, Value(false), Value(true)});
						                                                                     }
						                                                                     auto blip = MakeCell(First(Call(Member(context.Global("HUD"), "ADD_BLIP_FOR_ENTITY"), Values{*h_336})));
						                                                                     Call(Member(context.Global("HUD"), "SET_BLIP_SPRITE"), Values{*blip, Value(618)});
						                                                                     Call(Member(context.Global("HUD"), "SET_BLIP_COLOUR"), Values{*blip, Value(4)});
						                                                                     Call(Member(context.Global("HUD"), "SET_BLIP_SCALE"), Values{*blip, Value(0.8)});
						                                                                     Call(Member(context.Global("HUD"), "SET_BLIP_PRIORITY"), Values{*blip, Value(7)});
					                                                                     }
					                                                                     if (Truthy(Member(*ch_pnts, First(Invoke(First(Call(context.Global("Entity"), Values{*h_336})), "get_model", Values{})))))
					                                                                     {
						                                                                     if (Truthy(First(Call(Member(context.Global("ENTITY"), "IS_ENTITY_A_MISSION_ENTITY"), Values{*h_336}))))
						                                                                     {
							                                                                     Call(Member(context.Global("ENTITY"), "SET_ENTITY_AS_MISSION_ENTITY"), Values{*h_336, Value(false), Value(true)});
						                                                                     }
						                                                                     auto blip_337 = MakeCell(First(Call(Member(context.Global("HUD"), "ADD_BLIP_FOR_ENTITY"), Values{*h_336})));
						                                                                     Call(Member(context.Global("HUD"), "SET_BLIP_SPRITE"), Values{*blip_337, Value(776)});
						                                                                     Call(Member(context.Global("HUD"), "SET_BLIP_COLOUR"), Values{*blip_337, Value(4)});
						                                                                     Call(Member(context.Global("HUD"), "SET_BLIP_SCALE"), Values{*blip_337, Value(0.8)});
						                                                                     Call(Member(context.Global("HUD"), "SET_BLIP_PRIORITY"), Values{*blip_337, Value(7)});
					                                                                     }
				                                                                     }
			                                                                     }
			                                                                     return {};
		                                                                     })});
		Call(Member(context.Global("commandmgr"), "add_looped_command"), Values{Value("autoloot"), Value("Loot Auto-Grab"), Value("Auto-Grabs the loot in heists."), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                        if (Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("fm_mission_controller")}))))
			                                                                        {
				                                                                        Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller"), Value(10713)})), "set_int", Values{Value(4)});
				                                                                        Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller"), Add(Value(10713), Value(14))})), "set_float", Values{Value(2)});
				                                                                        if (Truthy(Equal(First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller"), Value(10713)})), "get_int", Values{})), Value(5))))
				                                                                        {
					                                                                        Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("autoloot")})), "set_value", Values{Value(false)});
				                                                                        }
			                                                                        }
			                                                                        if (Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("fm_mission_controller_2020")}))))
			                                                                        {
				                                                                        Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_2020"), Value(31275)})), "set_int", Values{Value(4)});
				                                                                        Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_2020"), Add(Value(31275), Value(14))})), "set_float", Values{Value(2)});
				                                                                        Call(Member(context.Global("script"), "yield"), Values{Value(100)});
				                                                                        if (Truthy(Equal(First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_2020"), Value(31275)})), "get_int", Values{})), Value(5))))
				                                                                        {
					                                                                        Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("autoloot")})), "set_value", Values{Value(false)});
				                                                                        }
			                                                                        }
			                                                                        return {};
		                                                                        })});
		// Fmmc2020Finish preserves the feature callback and its captured state.
		auto Fmmc2020Finish = MakeCell();
		*Fmmc2020Finish = MakeFunction([=, &context](const Values& args) -> Values {
			if (Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("fm_mission_controller_2020")}))))
			{
				Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_2020"), Value(56504)})), "set_int", Values{Value(9)});
				Invoke(First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_2020"), Add(Value(56504), Value(1776))})), "at", Values{Value(0), Value(1)})), "set_float", Values{Value(50)});
			}
			return {};
		});
		auto criminalmmbonus = MakeCell(Value(false));
		auto AutoShop_Contract = MakeCell(MakeTable({{"Contract", MakeTable({{Value(1), MakeTable({{Value(1), Value(0)}, {Value(2), Value("The Union Depository")}, {Value(3), Value("The Robbery")}})}, {Value(2), MakeTable({{Value(1), Value(1)}, {Value(2), Value("The Superdollar Deal")}, {Value(3), Value("The Robbery")}})}, {Value(3), MakeTable({{Value(1), Value(2)}, {Value(2), Value("The Bank Contract")}, {Value(3), Value("The Robberies")}})}, {Value(4), MakeTable({{Value(1), Value(3)}, {Value(2), Value("The E.C.U Job")}, {Value(3), Value("The Robbery")}})}, {Value(5), MakeTable({{Value(1), Value(4)}, {Value(2), Value("The Prison Contract")}, {Value(3), Value("The Hit")}})}, {Value(6), MakeTable({{Value(1), Value(5)}, {Value(2), Value("The Agency Deal")}, {Value(3), Value("The Raid")}})}, {Value(7), MakeTable({{Value(1), Value(6)}, {Value(2), Value("The LOST Contract")}, {Value(3), Value("The Job")}})}, {Value(8), MakeTable({{Value(1), Value(7)}, {Value(2), Value("The Data Contract")}, {Value(3), Value("The Robbery")}})}, {Value(9), MakeTable({{Value(1), Value(8)}, {Value(2), Value("None")}})}})}, {"AutoShop_Refresh", MakeFunction([=, &context](const Values& args) -> Values {
			                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    if (Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("tuner_planning")}))))
			                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    {
				                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("tuner_planning"), Value(418)})), "set_int", Values{Value(2)});
			                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    }
			                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    return {};
		                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    })}}));
		Call(Member(context.Global("commandmgr"), "add_list_command"), Values{Value("autoshop_contracts"), Value("Contracts"), Value(""), Member(*AutoShop_Contract, "Contract"), First(Call(Member(context.Global("stats"), "get_int"), Values{Value("MPX_TUNER_CURRENT")})), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                      Call(Member(context.Global("stats"), "set_int"), Append(Values{Value("MPX_TUNER_CURRENT")}, Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("autoshop_contracts")})), "get_value", Values{})));
			                                                                      auto selected_autoshopcontract = MakeCell(Member(Member(Member(*AutoShop_Contract, "Contract"), Add(First(Call(Member(context.Global("stats"), "get_int"), Values{Value("MPX_TUNER_CURRENT")})), Value(1))), Value(2)));
			                                                                      if (Truthy(NotEqual(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("autoshop_contracts")})), "get_value", Values{})), Value(8))))
			                                                                      {
				                                                                      Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Auto Shop Contract"), Concat(Value("Contract Selected: "), *selected_autoshopcontract), Value(3000)});
			                                                                      }
			                                                                      else
			                                                                      {
				                                                                      Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_TUNER_CURRENT"), Negate(Value(1))});
			                                                                      }
			                                                                      Call(Member(context.Global("STATS"), "STAT_SAVE"), Values{Value(0), Value(0), Value(3), Value(0)});
			                                                                      return {};
		                                                                      })});
		auto CluckinBellRaid = MakeCell(MakeTable({{"GetSetupStats", MakeTable({{"ApproachPB", Or(And(First(Call(Member(context.Global("stats"), "get_packed_bool"), Values{Value(42108)})), [&] {
			                                                                         return Value(1);
		                                                                         }),
		                                                                                           [&] {
			                                                                                           return Value(0);
		                                                                                           })},
		                                                                 {"WeaponsPI", First(Call(Member(context.Global("stats"), "get_packed_int"), Values{Value(51019)}))},
		                                                                 {"GearPI", First(Call(Member(context.Global("stats"), "get_packed_int"), Values{Value(51021)}))},
		                                                                 {"GetawayVehiclePI", First(Call(Member(context.Global("stats"), "get_packed_int"), Values{Value(51023)}))}})},
		    {"Approach", MakeTable({{Value(1), MakeTable({{Value(1), Value(0)}, {Value(2), Value("Sneaky Approach")}})}, {Value(2), MakeTable({{Value(1), Value(1)}, {Value(2), Value("Aggressive Approach")}})}})},
		    {"Weapons", MakeTable({{Value(1), MakeTable({{Value(1), Value(0)}, {Value(2), Value("Marabunta Grande")}})}, {Value(2), MakeTable({{Value(1), Value(1)}, {Value(2), Value("The Professionals")}})}, {Value(3), MakeTable({{Value(1), Value(2)}, {Value(2), Value("Unnamed Militia")}})}})},
		    {"Gear", MakeTable({{Value(1), MakeTable({{Value(1), Value(0)}, {Value(2), Value("Marabunta Grande")}})}, {Value(2), MakeTable({{Value(1), Value(1)}, {Value(2), Value("The Professionals")}})}, {Value(3), MakeTable({{Value(1), Value(2)}, {Value(2), Value("Unnamed Militia")}})}})},
		    {"GetawayVehicle", MakeTable({{Value(1), MakeTable({{Value(1), Value(0)}, {Value(2), Value("Declasse Tulip")}})}, {Value(2), MakeTable({{Value(1), Value(1)}, {Value(2), Value("Declasse Moonbeam Custom")}})}, {Value(3), MakeTable({{Value(1), Value(2)}, {Value(2), Value("Declasse Impaler LX")}})}, {Value(4), MakeTable({{Value(1), Value(3)}, {Value(2), Value("Ocelot Jugular")}})}, {Value(5), MakeTable({{Value(1), Value(4)}, {Value(2), Value("Dinka Sugoi")}})}, {Value(6), MakeTable({{Value(1), Value(5)}, {Value(2), Value("Coil Raiden")}})}, {Value(7), MakeTable({{Value(1), Value(6)}, {Value(2), Value("Mammoth Patriot Mil-Spec")}})}, {Value(8), MakeTable({{Value(1), Value(7)}, {Value(2), Value("Canis Terminus")}})}, {Value(9), MakeTable({{Value(1), Value(8)}, {Value(2), Value("Mammoth Squaddie")}})}})}}));
		auto OzcarGuzmanFliesAgain = MakeCell(MakeTable({{"Difficulty", MakeTable({{"HardMode", Value(false)}, {"DifficultyPB", Value(51272)}})}, {"S_Mogul", MakeTable({{"Setup", Value(false)}, {"SetupPB", MakeTable({{Value(1), Value(51265)}, {Value(2), Value(51266)}})}})}, {"S_IronMule", MakeTable({{"Setup", Value(false)}, {"SetupPB", MakeTable({{Value(1), Value(51264)}, {Value(2), Value(51268)}, {Value(3), Value(51269)}, {Value(4), Value(51270)}, {Value(5), Value(51271)}})}})}, {"S_Ammunition", MakeTable({{"Setup", Value(false)}, {"SetupPB", MakeTable({{Value(1), Value(51260)}, {Value(2), Value(51261)}, {Value(3), Value(51262)}, {Value(4), Value(51263)}})}})}}));
		auto TheKortzCenterHeist = MakeCell(MakeTable({{"Stat", MakeTable({{"Robbery", Value("MPX_K26_ROBBERY_PROG")}, {"General", Value("MPX_K26_GENERAL_BS")}, {"Scoping", Value("MPX_K26_SCOPING_BS")}, {"POI", Value("MPX_K26_POI_BS")}, {"Stolen", Value("MPX_K26_STOLENLAST_BS")}})}, {"Difficulty", MakeTable({{"HardMode", Value(false)}, {"DifficultyBIT", Value(28)}})}, {"S_WeakGuards", MakeTable({{"Setup", Value(false)}, {"SetupBIT", Value(31)}})}, {"Setups", MakeTable({{Value(1), MakeTable({{"Setup", Value(false)}, {"Label", Value("Scope Out Kortz Center")}, {"SetupBIT", Value(0)}})}, {Value(2), MakeTable({{"Setup", Value(false)}, {"Label", Value("Alpha Mail Disguise")}, {"SetupBIT", Value(1)}})}, {Value(3), MakeTable({{"Setup", Value(false)}, {"Label", Value("Hazmat Suit")}, {"SetupBIT", Value(2)}})}, {Value(4), MakeTable({{"Setup", Value(false)}, {"Label", Value("Staff Key Card")}, {"SetupBIT", Value(3)}})}, {Value(5), MakeTable({{"Setup", Value(false)}, {"Label", Value("Tactical Equipment")}, {"SetupBIT", Value(4)}})}, {Value(6), MakeTable({{"Setup", Value(false)}, {"Label", Value("Hacking Device")}, {"SetupBIT", Value(5)}})}, {Value(7), MakeTable({{"Setup", Value(false)}, {"Label", Value("Access Code")}, {"SetupBIT", Value(6)}})}, {Value(8), MakeTable({{"Setup", Value(false)}, {"Label", Value("Unmarked Weapons")}, {"SetupBIT", Value(7)}, {"Setup2BIT", MakeTable({{Value(1), Value(9)}, {Value(2), Value(10)}, {Value(3), Value(11)}})}})}, {Value(9), MakeTable({{"Setup", Value(false)}, {"Label", Value("Armored Caracara")}, {"SetupBIT", Value(8)}})}, {Value(10), MakeTable({{"Setup", Value(false)}, {"Label", Value("Annihilator Stealth")}, {"SetupBIT", Value(9)}})}, {Value(11), MakeTable({{"Setup", Value(false)}, {"Label", Value("Manchez")}, {"SetupBIT", Value(10)}, {"Setup2BIT", MakeTable({{Value(1), Value(17)}, {Value(2), Value(18)}, {Value(3), Value(19)}, {Value(4), Value(20)}})}})}, {Value(12), MakeTable({{"Setup", Value(false)}, {"Label", Value("EMP Charges")}, {"SetupBIT", Value(11)}, {"Setup2BIT", MakeTable({{Value(1), Value(8)}})}})}, {Value(13), MakeTable({{"Setup", Value(false)}, {"Label", Value("Guard Shipments")}, {"SetupBIT", Value(12)}})}, {Value(14), MakeTable({{"Setup", Value(false)}, {"Label", Value("Guard Routes")}, {"SetupBIT", Value(13)}, {"Setup2BIT", MakeTable({{Value(1), Value(5)}})}})}, {Value(15), MakeTable({{"Setup", Value(false)}, {"Label", Value("Glass Cutter")}, {"SetupBIT", Value(14)}, {"Setup2BIT", MakeTable({{Value(1), Value(6)}})}})}, {Value(16), MakeTable({{"Setup", Value(false)}, {"Label", Value("Power Drills")}, {"SetupBIT", Value(15)}, {"Setup2BIT", MakeTable({{Value(1), Value(7)}})}})}})}, {"Others", MakeTable({{Value(1), MakeTable({{"Label", Value("Vault Keypad")}, {"SetupBIT", Value(4)}})}, {Value(2), MakeTable({{"Label", Value("Staff Entrance")}, {"SetupBIT", Value(12)}})}, {Value(3), MakeTable({{"Label", Value("Skylight")}, {"SetupBIT", Value(13)}})}, {Value(4), MakeTable({{"Label", Value("Loading Bay")}, {"SetupBIT", Value(14)}})}, {Value(5), MakeTable({{"Label", Value("Sewer")}, {"SetupBIT", Value(15)}})}, {Value(6), MakeTable({{"Label", Value("Manhole Key Location")}, {"SetupBIT", Value(16)}})}, {Value(7), MakeTable({{"Label", Value("Advanced Avionics")}, {"SetupBIT", Value(23)}})}, {Value(8), MakeTable({{"Label", Value("Manhole Key")}, {"SetupBIT", Value(27)}})}})}, {"K26_Refresh", MakeFunction([=, &context](const Values& args) -> Values {
			                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  if (Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("kortz_planning")}))))
			                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  {
				                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("kortz_planning"), Value(599)})), "set_int", Values{Value(2)});
			                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  }
			                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  return {};
		                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  })},
		    {"ChildRes", Value(162)}}));
		auto BountyTargets = MakeCell(MakeTable({{"Target", Value(0)}, {"TargetStats", MakeTable({{"MostWantedTargetPI", First(Call(Member(context.Global("stats"), "get_packed_int"), Values{Value(19014)}))}})}, {"TargetMission", MakeTable({{Value(1), MakeTable({{Value(1), Value(0)}, {Value(2), Value("Arcade")}})}, {Value(2), MakeTable({{Value(1), Value(1)}, {Value(2), Value("CCTV")}})}, {Value(3), MakeTable({{Value(1), Value(2)}, {Value(2), Value("Clubhouse")}})}, {Value(4), MakeTable({{Value(1), Value(3)}, {Value(2), Value("Mansion Intimidation")}})}, {Value(5), MakeTable({{Value(1), Value(4)}, {Value(2), Value("Prison Bus")}})}, {Value(6), MakeTable({{Value(1), Value(5)}, {Value(2), Value("Bank Robbery")}})}})}, {"TargetCategory", MakeTable({{Value(0), MakeTable({{Value(1), Value("Beau Duggan")}, {Value(2), Value("Bill Duggan")}, {Value(3), Value("Hunter Duggan")}})}, {Value(1), MakeTable({{Value(1), Value("Lil Prince")}, {Value(2), Value("Xavier Fremond")}, {Value(3), Value("Jalen Kennedy")}})}, {Value(2), MakeTable({{Value(1), Value("Rylee Rose")}, {Value(2), Value("Serenity Pierce")}, {Value(3), Value("Angel Kenney")}})}, {Value(3), MakeTable({{Value(1), Value("Sabrina Gray")}, {Value(2), Value("India Wood")}, {Value(3), Value("Brigitte Foster")}})}, {Value(4), MakeTable({{Value(1), Value("Cook Kenzie")}, {Value(2), Value("Marquel Green")}, {Value(3), Value("Colby Wright")}})}, {Value(5), MakeTable({{Value(1), Value("Ricky Ji")}, {Value(2), Value("Tommy Lim")}, {Value(3), Value("LJ Ha")}})}})}, {"MostWantedTarget", MakeTable({{Value(1), MakeTable({{Value(1), Value(0)}, {Value(2), Value("Grace Whitney")}})}, {Value(2), MakeTable({{Value(1), Value(1)}, {Value(2), Value("Chaz Lieberman")}})}, {Value(3), MakeTable({{Value(1), Value(2)}, {Value(2), Value("Leroy O'Neil")}})}, {Value(4), MakeTable({{Value(1), Value(3)}, {Value(2), Value("Brock Thompson")}})}, {Value(5), MakeTable({{Value(1), Value(4)}, {Value(2), Value("Cleo Song")}})}, {Value(6), MakeTable({{Value(1), Value(5)}, {Value(2), Value("Omar Garcia")}})}})}, {"BailOffice_Refresh", MakeFunction([=, &context](const Values& args) -> Values {
			                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              if (Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("appbailoffice")}))))
			                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              {
				                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              Call(Member(context.Global("scripts"), "start_new_script"), Values{Value("appbailoffice"), Value(4592)});
			                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              }
			                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              return {};
		                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              })},
		    {"ChildRes", Value(250)}}));
		auto FIB_File = MakeCell(MakeTable({{"Files", MakeTable({{Value(1), MakeTable({{Value(1), Value(0)}, {Value(2), Value("The Black Box File")}})}, {Value(2), MakeTable({{Value(1), Value(1)}, {Value(2), Value("The Brute Force File")}})}, {Value(3), MakeTable({{Value(1), Value(2)}, {Value(2), Value("The Fine Art File")}})}, {Value(4), MakeTable({{Value(1), Value(3)}, {Value(2), Value("The Project Breakaway File")}})}, {Value(5), MakeTable({{Value(1), Value(4)}, {Value(2), Value("None")}})}})}, {"Finish", MakeTable({{Value(1), MakeTable({{"ScriptName", Value("fm_content_hacker_cargo_finale")}, {"Local1", Add(Value(7823), Value(1319))}, {"Local2", Add(Value(7657), Value(1))}})}, {Value(2), MakeTable({{"ScriptName", Value("fm_content_hacker_zancudo_fin")}, {"Local1", Add(Value(8657), Value(1182))}, {"Local2", Add(Value(8537), Value(1))}})}, {Value(3), MakeTable({{"ScriptName", Value("fm_content_hacker_house_finale")}, {"Local1", Add(Value(8267), Value(1192))}, {"Local2", Add(Value(8123), Value(1))}})}, {Value(4), MakeTable({{"ScriptName", Value("fm_content_hacker_whistle_fin")}, {"Local1", Add(Value(6782), Value(1176))}, {"Local2", Add(Value(6616), Value(1))}})}})}, {"HackerDen_Refresh", MakeFunction([=, &context](const Values& args) -> Values {
			                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           Call(Member(context.Global("scripts"), "start_new_script"), Values{Value("apphackerden"), Value(4592)});
			                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           return {};
		                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           })}}));
		auto SALV23_Robbery = MakeCell(MakeTable({{"Name", MakeTable({{Value(1), MakeTable({{Value(1), Value(0)}, {Value(2), Value("The Cargo Ship Robbery")}})}, {Value(2), MakeTable({{Value(1), Value(1)}, {Value(2), Value("The Gangbanger Robbery")}})}, {Value(3), MakeTable({{Value(1), Value(2)}, {Value(2), Value("The Duggan Robbery")}})}, {Value(4), MakeTable({{Value(1), Value(3)}, {Value(2), Value("The Podium Robbery")}})}, {Value(5), MakeTable({{Value(1), Value(4)}, {Value(2), Value("The McTony Robbery")}})}})}, {"Slot", MakeTable({{Value(0), Value(1152433341)}, {Value(1), Value(852564222)}, {Value(2), Value(552662330)}})}, {"ClaimableVeh", MakeTable({{Value(1), Negate(Value(1700733442))}, {Value(2), Negate(Value(1547046832))}, {Value(3), Value(1830093543)}})}, {"Finish", MakeTable({{Value(1), MakeTable({{"ScriptName", Value("fm_content_vehrob_cargo_ship")}, {"Local1", Add(Value(7372), Value(1250))}, {"Local2", Add(Value(7227), Value(1))}})}, {Value(2), MakeTable({{"ScriptName", Value("fm_content_vehrob_police")}, {"Local1", Add(Value(9183), Value(1306))}, {"Local2", Add(Value(9050), Value(1))}})}, {Value(3), MakeTable({{"ScriptName", Value("fm_content_vehrob_arena")}, {"Local1", Add(Value(8073), Value(1315))}, {"Local2", Add(Value(7953), Value(1))}})}, {Value(4), MakeTable({{"ScriptName", Value("fm_content_vehrob_casino_prize")}, {"Local1", Add(Value(9377), Value(1259))}, {"Local2", Add(Value(9235), Value(1))}})}, {Value(5), MakeTable({{"ScriptName", Value("fm_content_vehrob_submarine")}, {"Local1", Add(Value(6420), Value(1160))}, {"Local2", Add(Value(6259), Value(1))}})}, {Value(6), MakeTable({{"ScriptName", Value("fm_content_tow_truck_work")}, {"Local1", Add(Value(1885), Value(93))}, {"Local2", Add(Value(1828), Value(1))}})}})}, {"SALV23_Refresh", MakeFunction([=, &context](const Values& args) -> Values {
			                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              if (Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("vehrob_planning")}))))
			                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              {
				                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("vehrob_planning"), Value(547)})), "set_int", Values{Value(2)});
			                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              }
			                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              return {};
		                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              })},
		    {"Lift", Add(Value(1882779), Value(4))},
		    {"TruckCD", Value(1521767918)},
		    {"ChildRes", Value(250)}}));
		auto MinPayoutContactMiss = MakeCell(MakeTable({{"MMpayout", Value(0)}, {"Tunables", ([&] {
			                                                                         auto table_338 = MakeTable();
			                                                                         Member(table_338, Value(1)) = MakeTable({{"hash", Negate(Value(578005226))}, {"val", Value(1)}});
			                                                                         Member(table_338, Value(2)) = MakeTable({{"hash", Value(1957048982)}, {"val", Value(1)}});
			                                                                         Member(table_338, Value(3)) = MakeTable({{"hash", Value(1424087600)}, {"val", Value(1)}});
			                                                                         Member(table_338, Value(4)) = MakeTable({{"hash", Value(595756597)}, {"val", Value(1)}});
			                                                                         Member(table_338, Value(5)) = MakeTable({{"hash", Negate(Value(150996699))}, {"val", Value(1)}});
			                                                                         Member(table_338, Value(6)) = MakeTable({{"hash", Negate(Value(1592895685))}, {"val", Value(1)}});
			                                                                         Member(table_338, Value(7)) = MakeTable({{"hash", Negate(Value(892967027))}, {"val", Value(1)}});
			                                                                         Member(table_338, Value(8)) = MakeTable({{"hash", Negate(Value(181908601))}, {"val", Value(1)}});
			                                                                         Member(table_338, Value(9)) = MakeTable({{"hash", Negate(Value(1317337001))}, {"val", Value(1)}});
			                                                                         Member(table_338, Value(10)) = MakeTable({{"hash", Value(1225778387)}, {"val", Value(1)}});
			                                                                         Member(table_338, Value(11)) = MakeTable({{"hash", Negate(Value(658150922))}, {"val", Value(1)}});
			                                                                         Member(table_338, Value(12)) = MakeTable({{"hash", Negate(Value(1817092145))}, {"val", Value(1)}});
			                                                                         Member(table_338, Value(13)) = MakeTable({{"hash", Value(2099720891)}, {"val", Value(1)}});
			                                                                         Member(table_338, Value(14)) = MakeTable({{"hash", Value(876224738)}, {"val", Value(1)}});
			                                                                         Member(table_338, Value(15)) = MakeTable({{"hash", Value(50165909)}, {"val", Value(1)}});
			                                                                         Member(table_338, Value(16)) = MakeTable({{"hash", Negate(Value(933403272))}, {"val", Value(0)}});
			                                                                         Member(table_338, Value(17)) = MakeTable({{"hash", Negate(Value(934443667))}, {"val", Value(0)}});
			                                                                         Member(table_338, Value(18)) = MakeTable({{"hash", Negate(Value(560105293))}, {"val", Value(0)}});
			                                                                         Member(table_338, Value(19)) = MakeTable({{"hash", Value(1196094314)}, {"val", Value(0)}});
			                                                                         Member(table_338, Value(20)) = MakeTable({{"hash", Value(1239120893)}, {"val", Value(0)}});
			                                                                         Member(table_338, Value(21)) = MakeTable({{"hash", Negate(Value(397217433))}, {"val", Value(0)}});
			                                                                         Member(table_338, Value(22)) = MakeTable({{"hash", Value(103951584)}, {"val", Value(0)}});
			                                                                         Member(table_338, Value(23)) = MakeTable({{"hash", Negate(Value(953814516))}, {"val", Value(0)}});
			                                                                         Member(table_338, Value(24)) = MakeTable({{"hash", Negate(Value(1917315321))}, {"val", Value(0)}});
			                                                                         Member(table_338, Value(25)) = MakeTable({{"hash", Value(240235414)}, {"val", Value(0)}});
			                                                                         Member(table_338, Value(26)) = MakeTable({{"hash", Value(878296414)}, {"val", Value(0)}});
			                                                                         Member(table_338, Value(27)) = MakeTable({{"hash", Value(1047150785)}, {"val", Value(0)}});
			                                                                         Member(table_338, Value(28)) = MakeTable({{"hash", Value(1116015813)}, {"val", Value(1)}});
			                                                                         return table_338;
		                                                                         }())}}));
		auto UnnaturalSelection = MakeCell(MakeTable({{"UGC", MakeTable({{"Global", Add(Value(1845347), Value(151))}, {"Cayo", Value("4bpPLwLW7UKal3bHdKhRHw")}})}, {"Finish", MakeTable({{"Classic", MakeTable({{"ScriptName", Value("fm_horde_controler")}, {"Base", Value(11333)}})}, {"New", MakeTable({{"ScriptName", Value("fm_survival_controller")}, {"Base", Value(10769)}, {"Endless", Value(560)}})}, {"WaveOnly", Value(false)}, {"ToWave", Value(1)}})}, {"Idle", Value(2699645)}, {"ChildRes", Value(130)}}));
		Call(Member(context.Global("commandmgr"), "add_list_command"), Values{Value("salv23currobbery"), Value(""), Value(""), Member(*SALV23_Robbery, "Name"), First(Call(Member(context.Global("stats"), "get_int"), Values{Value("MPX_SALV23_VEH_ROB")})), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                      Call(Member(context.Global("stats"), "set_int"), Append(Values{Value("MPX_SALV23_VEH_ROB")}, Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("salv23currobbery")})), "get_value", Values{})));
			                                                                      return {};
		                                                                      })});
		{
			const auto start_339 = Number(Value(0));
			const auto end_340 = Number(Value(2));
			const auto step_341 = Number(Value(1));
			for (double index_342 = start_339; step_341 > 0 ? index_342 <= end_340 : index_342 >= end_340; index_342 += step_341)
			{
				auto robberyslot = MakeCell(Value(index_342));
				Call(Member(context.Global("commandmgr"), "add_list_command"), Values{Concat(Value("salv23robbery"), *robberyslot), Concat(Value("Slot "), Add(*robberyslot, Value(1))), Value(""), Member(*SALV23_Robbery, "Name"), First(Call(Member(context.Global("tunables"), "get_int"), Values{Member(Member(*SALV23_Robbery, "Slot"), *robberyslot)})), MakeFunction([=, &context](const Values& args) -> Values {
					                                                                      Call(Member(context.Global("tunables"), "set_int"), Append(Values{Member(Member(*SALV23_Robbery, "Slot"), *robberyslot)}, Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Concat(Value("salv23robbery"), *robberyslot)})), "get_value", Values{})));
					                                                                      return {};
				                                                                      })});
			}
		}
		{
			const auto start_343 = Number(Value(0));
			const auto end_344 = Number(Value(2));
			const auto step_345 = Number(Value(1));
			for (double index_346 = start_343; step_345 > 0 ? index_346 <= end_344 : index_346 >= end_344; index_346 += step_345)
			{
				auto bountymission = MakeCell(Value(index_346));
				Call(Member(context.Global("commandmgr"), "add_list_command"), Values{Concat(Value("bountytargetmission"), *bountymission), Concat(Value("Mission##"), *bountymission), Value(""), Member(*BountyTargets, "TargetMission"), Negate(Value(1)), MakeFunction([=, &context](const Values& args) -> Values {
					                                                                      Call(Member(context.Global("stats"), "set_int"), Append(Values{Concat(Value("MPX_BOUNTY24_STD_TARG_MIS_"), *bountymission)}, Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Concat(Value("bountytargetmission"), *bountymission)})), "get_value", Values{})));
					                                                                      return {};
				                                                                      })});
			}
		}
		Call(Member(context.Global("commandmgr"), "add_list_command"), Values{Value("mwtarget"), Value("Target"), Value(""), Member(*BountyTargets, "MostWantedTarget"), Member(Member(*BountyTargets, "TargetStats"), "MostWantedTargetPI"), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                      Call(Member(context.Global("stats"), "set_packed_int"), Append(Values{Value(19014)}, Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("mwtarget")})), "get_value", Values{})));
			                                                                      return {};
		                                                                      })});
		Call(Member(context.Global("commandmgr"), "add_list_command"), Values{Value("cbr_approach"), Value("Approach"), Value(""), Member(*CluckinBellRaid, "Approach"), Member(Member(*CluckinBellRaid, "GetSetupStats"), "ApproachPB"), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                      if (Truthy(Equal(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("cbr_approach")})), "get_value", Values{})), Value(1))))
			                                                                      {
				                                                                      Call(Member(context.Global("stats"), "set_packed_bool"), Values{Value(42108), Value(true)});
			                                                                      }
			                                                                      else
			                                                                      {
				                                                                      Call(Member(context.Global("stats"), "set_packed_bool"), Values{Value(42108), Value(false)});
			                                                                      }
			                                                                      return {};
		                                                                      })});
		Call(Member(context.Global("commandmgr"), "add_list_command"), Values{Value("cbr_weapons"), Value("Weapons"), Value(""), Member(*CluckinBellRaid, "Weapons"), Member(Member(*CluckinBellRaid, "GetSetupStats"), "WeaponsPI"), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                      Call(Member(context.Global("stats"), "set_packed_int"), Append(Values{Value(51019)}, Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("cbr_weapons")})), "get_value", Values{})));
			                                                                      return {};
		                                                                      })});
		Call(Member(context.Global("commandmgr"), "add_list_command"), Values{Value("cbr_gear"), Value("Gear"), Value(""), Member(*CluckinBellRaid, "Gear"), Member(Member(*CluckinBellRaid, "GetSetupStats"), "GearPI"), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                      Call(Member(context.Global("stats"), "set_packed_int"), Append(Values{Value(51021)}, Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("cbr_gear")})), "get_value", Values{})));
			                                                                      return {};
		                                                                      })});
		Call(Member(context.Global("commandmgr"), "add_list_command"), Values{Value("cbr_getawayvehicle"), Value("Getaway Vehicle"), Value(""), Member(*CluckinBellRaid, "GetawayVehicle"), Member(Member(*CluckinBellRaid, "GetSetupStats"), "GetawayVehiclePI"), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                      Call(Member(context.Global("stats"), "set_packed_int"), Append(Values{Value(51023)}, Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("cbr_getawayvehicle")})), "get_value", Values{})));
			                                                                      return {};
		                                                                      })});
		Call(Member(context.Global("commandmgr"), "add_list_command"), Values{Value("fib_files"), Value("Files"), Value(""), Member(*FIB_File, "Files"), First(Call(Member(context.Global("stats"), "get_int"), Values{Value("MPX_HACKER24_ACTIVE_ROB")})), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                      Call(Member(context.Global("stats"), "set_int"), Append(Values{Value("MPX_HACKER24_ACTIVE_ROB")}, Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("fib_files")})), "get_value", Values{})));
			                                                                      auto selected_fibfile = MakeCell(Member(Member(Member(*FIB_File, "Files"), Add(First(Call(Member(context.Global("stats"), "get_int"), Values{Value("MPX_HACKER24_ACTIVE_ROB")})), Value(1))), Value(2)));
			                                                                      if (Truthy(NotEqual(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("fib_files")})), "get_value", Values{})), Value(4))))
			                                                                      {
				                                                                      Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - FIB File"), Concat(Value("File Selected: "), *selected_fibfile), Value(3000)});
			                                                                      }
			                                                                      else
			                                                                      {
				                                                                      Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_HACKER24_ACTIVE_ROB"), Negate(Value(1))});
			                                                                      }
			                                                                      Call(Member(context.Global("STATS"), "STAT_SAVE"), Values{Value(0), Value(0), Value(3), Value(0)});
			                                                                      return {};
		                                                                      })});
		Call(Member(context.Global("commandmgr"), "add_looped_command"), Values{Value("minmissionpayout"), Value("Min. Mission Payout"), Value("Applies only to Contact Missions and Classic Survivals."), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                        Call(Member(context.Global("tunables"), "set_int"), Values{Negate(Value(268882405)), Member(*MinPayoutContactMiss, "MMpayout")});
			                                                                        Call(Member(context.Global("tunables"), "set_int"), Values{Value(1312791102), Member(*MinPayoutContactMiss, "MMpayout")});
			                                                                        Call(Member(context.Global("tunables"), "set_int"), Values{Negate(Value(1244930138)), Divide(Member(*MinPayoutContactMiss, "MMpayout"), Value(100))});
			                                                                        {
				                                                                        Iterator iterator_347(Append(Values{}, Call(context.Global("ipairs"), Values{Member(*MinPayoutContactMiss, "Tunables")})));
				                                                                        for (auto row_348 = iterator_347.Next(); !row_348.empty() && !row_348[0].IsNil(); row_348 = iterator_347.Next())
				                                                                        {
					                                                                        auto __349 = MakeCell(At(row_348, 0));
					                                                                        auto min = MakeCell(At(row_348, 1));
					                                                                        Call(Member(context.Global("tunables"), "set_float"), Values{Member(*min, "hash"), Member(*min, "val")});
				                                                                        }
			                                                                        }
			                                                                        return {};
		                                                                        })});
		Call(Member(context.Global("commandmgr"), "add_looped_command"), Values{Value("survivals_noidlekick"), Value("No Idle Kick"), Value("Prevents you from being kicked while idling in Survivals."), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                        Invoke(First(Call(context.Global("ScriptGlobal"), Values{Member(*UnnaturalSelection, "Idle")})), "set_int", Values{*value_INT_MAX});
			                                                                        return {};
		                                                                        }),
		                                                                     Value{},
		                                                                     MakeFunction([=, &context](const Values& args) -> Values {
			                                                                     Invoke(First(Call(context.Global("ScriptGlobal"), Values{Member(*UnnaturalSelection, "Idle")})), "set_int", Values{Negate(Value(1))});
			                                                                     return {};
		                                                                     })});
		Call(Member(context.Global("commandmgr"), "add_list_command"), Values{Value("achievementsonly"), Value("Achievements"), Value(""), context.Global("Rplus_Achievements"), Negate(Value(1)), Value{}});
		Call(Member(context.Global("commandmgr"), "add_list_command"), Values{Value("rprank"), Value("Rank"), Value(""), Member(context.Global("Rplus_Ranks"), "ToMax"), Value(1), Value{}});
		{
			const auto start_350 = Number(Value(0));
			const auto end_351 = Number(Value(4));
			const auto step_352 = Number(Value(1));
			for (double index_353 = start_350; step_352 > 0 ? index_353 <= end_351 : index_353 >= end_351; index_353 += step_352)
			{
				auto i_354 = MakeCell(Value(index_353));
				Call(Member(context.Global("commandmgr"), "add_list_command"), Values{Concat(Value("crewrank"), *i_354), Concat(Value("Slot "), Add(*i_354, Value(1))), Value(""), Member(context.Global("Rplus_Ranks"), "ToMax"), Value(1), Value{}});
			}
		}
		Call(Member(context.Global("commandmgr"), "add_bool_command"), Values{Value("Tunaggle_Independence"), Value("Independence Day"), Value("Enables Independence Day Content"), Value(false), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                      {
				                                                                      Iterator iterator_355(Append(Values{}, Call(context.Global("ipairs"), Values{context.Global("RplusTuna_IndependenceDay")})));
				                                                                      for (auto row_356 = iterator_355.Next(); !row_356.empty() && !row_356[0].IsNil(); row_356 = iterator_355.Next())
				                                                                      {
					                                                                      auto __357 = MakeCell(At(row_356, 0));
					                                                                      auto independenceday = MakeCell(At(row_356, 1));
					                                                                      Call(Member(context.Global("tunables"), "set_bool"), Values{Member(*independenceday, "Tunable"), Member(*independenceday, "tunaVal")});
				                                                                      }
			                                                                      }
			                                                                      Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Independence Day"), Value("Independence Day Content enabled!"), Value(3000)});
			                                                                      return {};
		                                                                      }),
		                                                                   MakeFunction([=, &context](const Values& args) -> Values {
			                                                                   {
				                                                                   Iterator iterator_358(Append(Values{}, Call(context.Global("ipairs"), Values{context.Global("RplusTuna_IndependenceDay")})));
				                                                                   for (auto row_359 = iterator_358.Next(); !row_359.empty() && !row_359[0].IsNil(); row_359 = iterator_358.Next())
				                                                                   {
					                                                                   auto __360 = MakeCell(At(row_359, 0));
					                                                                   auto independenceday_361 = MakeCell(At(row_359, 1));
					                                                                   Call(Member(context.Global("tunables"), "set_bool"), Values{Member(*independenceday_361, "Tunable"), Value(!Truthy(Member(*independenceday_361, "tunaVal")))});
				                                                                   }
			                                                                   }
			                                                                   Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Independence Day"), Value("Independence Day Content disabled."), Value(3000)});
			                                                                   return {};
		                                                                   })});
		Call(Member(context.Global("commandmgr"), "add_bool_command"), Values{Value("Tunaggle_Halloween"), Value("Halloween"), Value("Enables Halloween Content"), Value(false), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                      {
				                                                                      Iterator iterator_362(Append(Values{}, Call(context.Global("ipairs"), Values{context.Global("RplusTuna_Halloween")})));
				                                                                      for (auto row_363 = iterator_362.Next(); !row_363.empty() && !row_363[0].IsNil(); row_363 = iterator_362.Next())
				                                                                      {
					                                                                      auto __364 = MakeCell(At(row_363, 0));
					                                                                      auto halloween = MakeCell(At(row_363, 1));
					                                                                      Call(Member(context.Global("tunables"), "set_bool"), Values{Member(*halloween, "Tunable"), Member(*halloween, "tunaVal")});
				                                                                      }
			                                                                      }
			                                                                      Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Halloween"), Value("Halloween Content enabled!"), Value(3000)});
			                                                                      return {};
		                                                                      }),
		                                                                   MakeFunction([=, &context](const Values& args) -> Values {
			                                                                   {
				                                                                   Iterator iterator_365(Append(Values{}, Call(context.Global("ipairs"), Values{context.Global("RplusTuna_Halloween")})));
				                                                                   for (auto row_366 = iterator_365.Next(); !row_366.empty() && !row_366[0].IsNil(); row_366 = iterator_365.Next())
				                                                                   {
					                                                                   auto __367 = MakeCell(At(row_366, 0));
					                                                                   auto halloween_368 = MakeCell(At(row_366, 1));
					                                                                   Call(Member(context.Global("tunables"), "set_bool"), Values{Member(*halloween_368, "Tunable"), Value(!Truthy(Member(*halloween_368, "tunaVal")))});
				                                                                   }
			                                                                   }
			                                                                   Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Halloween"), Value("Halloween Content disabled."), Value(3000)});
			                                                                   return {};
		                                                                   })});
		auto xmas_snow = MakeCell(Value(false));
		Call(Member(context.Global("commandmgr"), "add_bool_command"), Values{Value("Tunaggle_Xmas"), Value("Christmas"), Value("Enables Christmas Content"), Value(false), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                      {
				                                                                      Iterator iterator_369(Append(Values{}, Call(context.Global("ipairs"), Values{context.Global("RplusTuna_Christmas")})));
				                                                                      for (auto row_370 = iterator_369.Next(); !row_370.empty() && !row_370[0].IsNil(); row_370 = iterator_369.Next())
				                                                                      {
					                                                                      auto __371 = MakeCell(At(row_370, 0));
					                                                                      auto xmas = MakeCell(At(row_370, 1));
					                                                                      Call(Member(context.Global("tunables"), "set_bool"), Values{Member(*xmas, "Tunable"), Member(*xmas, "tunaVal")});
				                                                                      }
			                                                                      }
			                                                                      Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Christmas"), Value("Christmas Content enabled!"), Value(3000)});
			                                                                      return {};
		                                                                      }),
		                                                                   MakeFunction([=, &context](const Values& args) -> Values {
			                                                                   {
				                                                                   Iterator iterator_372(Append(Values{}, Call(context.Global("ipairs"), Values{context.Global("RplusTuna_Christmas")})));
				                                                                   for (auto row_373 = iterator_372.Next(); !row_373.empty() && !row_373[0].IsNil(); row_373 = iterator_372.Next())
				                                                                   {
					                                                                   auto __374 = MakeCell(At(row_373, 0));
					                                                                   auto xmas_375 = MakeCell(At(row_373, 1));
					                                                                   Call(Member(context.Global("tunables"), "set_bool"), Values{Member(*xmas_375, "Tunable"), Value(!Truthy(Member(*xmas_375, "tunaVal")))});
				                                                                   }
			                                                                   }
			                                                                   Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Christmas"), Value("Christmas Content disabled."), Value(3000)});
			                                                                   return {};
		                                                                   })});
		auto SindyBar = MakeCell(MakeTable({{"Loop", Value(false)}, {"Money", Value(100000)}, {"Delay", Value(3000)}, {"ManualBarMoney", Value(false)}}));
		Call(Member(context.Global("commandmgr"), "add_list_command"), Values{Value("humcanv_fp1list"), Value("##fp1list"), Value(""), context.Global("HumanC_FP"), First(Call(Member(context.Global("stats"), "get_packed_int"), Values{Value(451)})), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                      Call(Member(context.Global("stats"), "set_packed_int"), Append(Values{Value(451)}, Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("humcanv_fp1list")})), "get_value", Values{})));
			                                                                      return {};
		                                                                      })});
		Call(Member(context.Global("commandmgr"), "add_list_command"), Values{Value("humcanv_fp2list"), Value("##fp2list"), Value(""), context.Global("HumanC_FP2"), First(Call(Member(context.Global("stats"), "get_packed_int"), Values{Value(3942)})), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                      Call(Member(context.Global("stats"), "set_packed_int"), Append(Values{Value(3942)}, Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("humcanv_fp2list")})), "get_value", Values{})));
			                                                                      return {};
		                                                                      })});
		Invoke(*TS_RecoveryPlus, "imgui", Values{MakeFunction([=, &context](const Values& args) -> Values {
			if (Truthy(Value(!Truthy(First(Call(Member(context.Global("network"), "is_session_started"), Values{}))))))
			{
				Call(Member(context.Global("ImGui"), "TextDisabled"), Values{Value("Please join a freemode session.")});
				return Values{};
			}
			Call(Member(context.Global("ImGui"), "BeginTabBar"), Values{Value("recoplus")});
			if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginTabItem"), Values{Value("Mission Master")}))))
			{
				Call(Member(context.Global("ImGui"), "BeginGroup"), Values{});
				Call(Member(context.Global("ImGui"), "BeginChild"), Values{Value("##ineedanameforthis"), Value(424), Value(584), Value(1)});
				Call(*CenteredText, Values{Value("Heists"), Value(false), Value(true)});
				Call(Member(context.Global("ImGui"), "BeginTabBar"), Values{Value("Heists"), Add(Member(context.Global("ImGuiTabBarFlags"), "NoTabListScrollingButtons"), Member(context.Global("ImGuiTabBarFlags"), "FittingPolicyScroll"))});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginTabItem"), Values{Value("Cluckin' Bell Farm Raid")}))))
				{
					Call(Member(context.Global("ImGui"), "Text"), Values{Value("Cluckin' Raider")});
					Call(Member(context.Global("ImGui"), "Separator"), Values{});
					Call(Member(context.Global("ImGui"), "Spacing"), Values{});
					Call(Member(context.Global("ImGui"), "BeginDisabled"), Values{Value(!Truthy(First(Call(Member(context.Global("STREAMING"), "IS_IPL_ACTIVE"), Values{Value("cs1_02_cf_onmission4")}))))});
					Call(Member(context.Global("ImGui"), "BeginGroup"), Values{});
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Teleport to Signal"), Value(200)}))))
					{
						Call(*CBTpToPC, Values{});
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
					{
						Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Teleports you to the PC with the strongest signal")});
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Teleport to Crate"), Value(200)}))))
					{
						Call(*CBTpToCrate, Values{});
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
					{
						Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Teleports you to the stashed drugs.\nTeleports you to the Office PC if there are no crates left to search.")});
					}
					if (Truthy(And(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("fm_mission_controller_2020")})), [&] {
						    return Equal(First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_2020"), Value(21088)})), "get_int", Values{})), Value(1149534208));
					    })))
					{
						if (Truthy(Value(!Truthy(First(Call(Member(context.Global("WEAPON"), "HAS_PED_GOT_WEAPON"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value("WEAPON_CROWBAR"), Value(false)}))))))
						{
							if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Get Crowbar"), Value(200)}))))
							{
								Call(Member(context.Global("WEAPON"), "GIVE_WEAPON_TO_PED"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value("WEAPON_CROWBAR"), Value(1), Value(false), Value(true)});
							}
						}
					}
					Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("cbr_opendoors")})), "draw", Values{});
					Call(Member(context.Global("ImGui"), "EndGroup"), Values{});
					Call(Member(context.Global("ImGui"), "SameLine"), Values{});
					Call(Member(context.Global("ImGui"), "BeginGroup"), Values{});
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Crack Cluckin' Bell Safe"), Value(200)}))))
					{
						Call(Member(context.Global("script"), "run_in_callback"), Values{*CrackCB});
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
					{
						Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Breaks into the safe.")});
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Teleport to Office Keys##cbr"), Value(200)}))))
					{
						Call(*TpToEntity, Values{Negate(Value(1423372530))});
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Instant Finish##cbr"), Value(200)}))))
					{
						Call(*Fmmc2020Finish, Values{});
					}
					Call(Member(context.Global("ImGui"), "EndGroup"), Values{});
					Call(Member(context.Global("ImGui"), "EndDisabled"), Values{});
					Call(Member(context.Global("ImGui"), "EndTabItem"), Values{});
				}
				if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginTabItem"), Values{Value("Kortz Center Heist")}))))
				{
					Call(Member(context.Global("ImGui"), "Text"), Values{Value("Kortz Center Cracker")});
					Call(Member(context.Global("ImGui"), "Separator"), Values{});
					Call(Member(context.Global("ImGui"), "Spacing"), Values{});
					if (Truthy(Equal(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Value(1980570), Value(1090))})), "get_int", Values{})), Value(3))))
					{
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Quick Start Heist"), Value(200)}))))
						{
							Call(Member(context.Global("script"), "run_in_callback"), Values{*TS_KortzStart});
						}
					}
					Call(Member(context.Global("ImGui"), "BeginDisabled"), Values{Value(!Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("fm_mission_controller_v3")}))))});
					Call(Member(context.Global("ImGui"), "BeginGroup"), Values{});
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Skip Data Crack"), Value(200)}))))
					{
						Call(*TS_KortzDC, Values{});
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Skip Fingerprint Hacking"), Value(200)}))))
					{
						Call(*TS_KortzFP, Values{});
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Auto-Enter PC Access Code"), Value(200)}))))
					{
						Call(Member(context.Global("script"), "run_in_callback"), Values{*TS_KortzPCAC});
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
					{
						Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Enters the Access Code automatically")});
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Disable Lasers"), Value(200)}))))
					{
						Call(*TS_KortzPwrL, Values{});
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
					{
						Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Disables Laser Grid")});
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Skip Vault Door Hacking"), Value(200)}))))
					{
						Call(*TS_KortzSN, Values{});
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
					{
						Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Skips Signal Nodes Hacking")});
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Teleport to Exit Keys"), Value(200)}))))
					{
						Call(*TpToBlip, Values{Value(true), Member(context.Global("Blips"), "CRIM_CUFF_KEYS"), Value(false)});
					}
					Call(Member(context.Global("ImGui"), "EndGroup"), Values{});
					Call(Member(context.Global("ImGui"), "SameLine"), Values{});
					Call(Member(context.Global("ImGui"), "BeginGroup"), Values{});
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Take Primary Target"), Value(200)}))))
					{
						Call(Member(context.Global("script"), "run_in_callback"), Values{*TS_KortzPaint});
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
					{
						Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Takes Primary Target instantly")});
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Take Secondary Target"), Value(200)}))))
					{
						Call(*TS_KortzPaint2, Values{});
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
					{
						Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Takes Secondary Target instantly")});
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Cut Glass"), Value(200)}))))
					{
						Call(*TS_KortzGlass, Values{});
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
					{
						Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Cuts Display Cases Glass instantly")});
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Disable All Cameras"), Value(200)}))))
					{
						if (Truthy(And(Equal(First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_v3"), Add(Value(57250), Value(1))})), "get_int", Values{})), Value(17456129)), [&] {
							    return Value(!Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("fmmc_lasers")}))));
						    })))
						{
							if (Truthy(Value(!Truthy(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("kortz_delcams")})), "get_value", Values{}))))))
							{
								Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("kortz_delcams")})), "set_value", Values{Value(true)});
							}
							else
							{
								Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("kortz_delcams")})), "set_value", Values{Value(false)});
							}
						}
						else
						{
							Invoke(First(Invoke(First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_v3"), Value(70176)})), "at", Values{Value(0), Value(295)})), "at", Values{Add(Value(237), Value(1))})), "set_int", Values{Negate(Value(2145222410))});
						}
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
					{
						if (Truthy(And(Equal(First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_v3"), Add(Value(57250), Value(1))})), "get_int", Values{})), Value(17456129)), [&] {
							    return Value(!Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("fmmc_lasers")}))));
						    })))
						{
							Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Exterior: Disables All Cameras for 12 seconds\nMISSION WILL BREAK IF YOU ABUSE THIS OPTION!")});
						}
						else
						{
							Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Interior: Disables CCTV system.")});
						}
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Teleport to Key Card##k26_card"), Value(200)}))))
					{
						Call(*TpToBlip, Values{Value(true), Member(context.Global("Blips"), "KEYCARD"), Value(false)});
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Instant Finish##k26"), Value(200)}))))
					{
						if (Truthy(Equal(First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_v3"), Add(Value(57250), Value(1))})), "get_int", Values{})), Value(17718273))))
						{
							Call(*TS_KortzFinish, Values{});
						}
						else
						{
							Call(Member(context.Global("notify"), "error"), Values{Value("TinkerScripts - Kortz Center Cracker"), Value("You must be inside the building to use Instant-Finish!"), Value(3000)});
						}
					}
					Call(Member(context.Global("ImGui"), "EndGroup"), Values{});
					if (Truthy(Equal(First(Call(Member(context.Global("stats"), "get_masked_int"), Values{Value("MPX_K26_GENERAL_BS"), Value(27), Value(1)})), Value(0))))
					{
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Teleport to Manhole Key"), Value(200)}))))
						{
							Call(*TpToBlip, Values{Value(false), Member(context.Global("Blips"), "MANHOLE_KEY"), Value(true)});
						}
						Call(Member(context.Global("ImGui"), "SameLine"), Values{});
					}
					Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("kortz_opendoors")})), "draw", Values{});
					if (Truthy(Less(First(Call(Member(context.Global("util"), "time"), Values{})), Add(*kortzcamtimer, Value(12000)))))
					{
						Call(Member(context.Global("ImGui"), "SameLine"), Values{});
						Call(Member(context.Global("ImGui"), "TextDisabled"), Append(Values{}, Call(Member(context.Global("string"), "format"), Values{Value("Disabled Cameras: %.1f"), Divide(Subtract(Add(*kortzcamtimer, Value(12001)), First(Call(Member(context.Global("util"), "time"), Values{}))), Value(1000))})));
					}
					if (Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("fmmc_lasers")}))))
					{
						*K26_Leasy = First(Call(Member(context.Global("ImGui"), "Checkbox"), Values{Value("Easy Lasers"), *K26_Leasy}));
						Call(Member(context.Global("ImGui"), "SameLine"), Values{});
						*K26_Lnodamage = First(Call(Member(context.Global("ImGui"), "Checkbox"), Values{Value("No Damage Lasers"), *K26_Lnodamage}));
						if (Truthy(*K26_Leasy))
						{
							Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fmmc_lasers"), Value(1058)})), "set_int", Values{Value(1)});
							Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fmmc_lasers"), Value(1059)})), "set_int", Values{Value(0)});
						}
						else
						{
							Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fmmc_lasers"), Value(1058)})), "set_int", Values{Value(0)});
							Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fmmc_lasers"), Value(1059)})), "set_int", Values{Value(1)});
						}
						if (Truthy(*K26_Lnodamage))
						{
							Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fmmc_lasers"), Value(1060)})), "set_int", Values{Value(1)});
						}
						else
						{
							Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fmmc_lasers"), Value(1060)})), "set_int", Values{Value(0)});
						}
						if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
						{
							Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Walk through the lasers without receiving damage.")});
						}
					}
					if (Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("fm_mission_controller_v3")}))))
					{
						Call(Member(context.Global("ImGui"), "Text"), Values{Value("Loot")});
						Call(Member(context.Global("ImGui"), "Separator"), Values{});
						Call(Member(context.Global("ImGui"), "Spacing"), Values{});
						auto values_376 = Values{};
						auto k26_take = MakeCell(At(values_376, 0));
						Call(Member(context.Global("ImGui"), "SetNextItemWidth"), Values{Value(140)});
						if (Truthy(NotEqual(First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_v3"), Add(Add(Value(60737), Value(1376)), Value(60))})), "get_int", Values{})), Value{})))
						{
							*k26_take = First(Call(Member(context.Global("ImGui"), "InputInt"), Append(Values{Value("Take")}, Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_v3"), Add(Add(Value(60737), Value(1376)), Value(60))})), "get_int", Values{}))));
						}
						Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_v3"), Add(Add(Value(60737), Value(1376)), Value(60))})), "set_int", Values{*k26_take});
						Call(Member(context.Global("ImGui"), "SetNextItemWidth"), Values{Value(120)});
						auto k26_bagcap = MakeCell(First(Call(Member(context.Global("ImGui"), "InputInt"), Append(Values{Value("Bag Capacity")}, Call(Member(context.Global("tunables"), "get_int"), Values{Value(6971439)})))));
						Call(Member(context.Global("tunables"), "set_int"), Values{Value(6971439), *k26_bagcap});
					}
					Call(Member(context.Global("ImGui"), "EndDisabled"), Values{});
					Call(Member(context.Global("ImGui"), "EndTabItem"), Values{});
				}
				if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginTabItem"), Values{Value("Cayo Perico Heist")}))))
				{
					Call(Member(context.Global("ImGui"), "Text"), Values{Value("Cayo Perico")});
					Call(Member(context.Global("ImGui"), "Separator"), Values{});
					Call(Member(context.Global("ImGui"), "Spacing"), Values{});
					Call(Member(context.Global("ImGui"), "BeginDisabled"), Values{And(Value(!Truthy(First(Call(Member(context.Global("STREAMING"), "IS_IPL_ACTIVE"), Values{Value("h4_islandx_sea_mines")})))), [&] {
						return Value(!Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("fm_mission_controller_2020")}))));
					})});
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Solo Mantrap"), Value(200)}))))
					{
						Call(Member(context.Global("script"), "run_in_callback"), Values{MakeFunction([=, &context](const Values& args) -> Values {
							Invoke(First(Invoke(First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_2020"), Value(69374)})), "at", Values{Value(0), Value(293)})), "at", Values{Value(236)})), "set_int", Values{Value(7)});
							Call(Member(context.Global("script"), "yield"), Values{Value(200)});
							Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_2020"), Value(21215)})), "set_int", Values{Value(1)});
							return {};
						})});
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
					{
						Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Bypass Dual Keycard systems")});
					}
					Call(Member(context.Global("ImGui"), "SameLine"), Values{});
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Take Secondary Target##Cayo"), Value(200)}))))
					{
						Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_2020"), Add(Value(29373), Value(11))})), "set_int", Values{Value(3)});
						Call(Member(context.Global("PAD"), "SET_CONTROL_VALUE_NEXT_FRAME"), Values{Value(0), Value(219), Value(1.0)});
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
					{
						Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Takes Secondary Target instantly")});
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Teleport to Key Card"), Value(200)}))))
					{
						Call(*TpToBlip, Values{Value(true), Member(context.Global("Blips"), "KEYCARD"), Value(false)});
						Call(Member(context.Global("script"), "run_in_callback"), Values{MakeFunction([=, &context](const Values& args) -> Values {
							Call(Member(context.Global("script"), "yield"), Values{Value(500)});
							Call(Member(context.Global("PED"), "SET_PED_COORDS_KEEP_VEHICLE"), Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_PED_ID"), Values{})), Value(5012.4), Negate(Value(5755.6)), Value(28.9)});
							return {};
						})});
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
					{
						Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Kills all enemies, and teleports you to available Key Card")});
					}
					Call(Member(context.Global("ImGui"), "SameLine"), Values{});
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Teleport to Gate Keys"), Value(200)}))))
					{
						Call(*TpToBlip, Values{Value(true), Member(context.Global("Blips"), "CRIM_CUFF_KEYS"), Value(false)});
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Delete Cameras"), Value(200)}))))
					{
						Call(Member(context.Global("script"), "run_in_callback"), Values{*ch_delcam});
					}
					Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("ch_opendoors")})), "draw", Values{});
					if (Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("fm_mission_controller_2020")}))))
					{
						Call(Member(context.Global("ImGui"), "Text"), Values{Value("Loot")});
						Call(Member(context.Global("ImGui"), "Separator"), Values{});
						Call(Member(context.Global("ImGui"), "Spacing"), Values{});
						auto values_377 = Values{};
						auto ch_take = MakeCell(At(values_377, 0));
						Call(Member(context.Global("ImGui"), "SetNextItemWidth"), Values{Value(140)});
						if (Truthy(NotEqual(First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_2020"), Add(Add(Value(59986), Value(1376)), Value(60))})), "get_int", Values{})), Value{})))
						{
							*ch_take = First(Call(Member(context.Global("ImGui"), "InputInt"), Append(Values{Value("Take")}, Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_2020"), Add(Add(Value(59986), Value(1376)), Value(60))})), "get_int", Values{}))));
						}
						Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_mission_controller_2020"), Add(Add(Value(59986), Value(1376)), Value(60))})), "set_int", Values{*ch_take});
						Call(Member(context.Global("ImGui"), "SetNextItemWidth"), Values{Value(120)});
						auto ch_bagcap = MakeCell(First(Call(Member(context.Global("ImGui"), "InputInt"), Append(Values{Value("Bag Capacity")}, Call(Member(context.Global("tunables"), "get_int"), Values{Value(1859395035)})))));
						Call(Member(context.Global("tunables"), "set_int"), Values{Value(1859395035), *ch_bagcap});
					}
					Call(Member(context.Global("ImGui"), "EndDisabled"), Values{});
					Call(Member(context.Global("ImGui"), "EndTabItem"), Values{});
				}
				Call(Member(context.Global("ImGui"), "EndTabBar"), Values{});
				Call(Member(context.Global("ImGui"), "EndChild"), Values{});
				Call(Member(context.Global("ImGui"), "EndGroup"), Values{});
				Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(6.8)});
				Call(Member(context.Global("ImGui"), "BeginGroup"), Values{});
				Call(Member(context.Global("ImGui"), "BeginChild"), Values{Value("##missionskips"), Value(406), Value(584), Value(1)});
				Call(*CenteredText, Values{Value("Mission Setups & Skips"), Value(false), Value(true)});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
				{
					Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Only those that can't be replayed through the Pause Menu")});
				}
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(3.8)});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "TreeNodeEx"), Values{Concat(Value("Auto Shop Contract  ##"), *nomoretreeplease), Value(8219)}))))
				{
					if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginChild"), Values{Value("##autoshopchild"), Value(285), Value(162), Value(1)}))))
					{
						auto values_378 = Values{};
						auto autoshop_contractfinale = MakeCell(At(values_378, 0));
						if (Truthy(Greater(First(Call(Member(context.Global("stats"), "get_int"), Values{Value("MPX_TUNER_CURRENT")})), Negate(Value(1)))))
						{
							*autoshop_contractfinale = Member(Member(Member(*AutoShop_Contract, "Contract"), Add(First(Call(Member(context.Global("stats"), "get_int"), Values{Value("MPX_TUNER_CURRENT")})), Value(1))), Value(3));
							if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Concat(Value("Skip: "), *autoshop_contractfinale), Negate(Value(1))}))))
							{
								Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_TUNER_GEN_BS"), Negate(Value(1))});
								Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Auto Shop Skip"), Concat(Value("Success! All preps skipped.\nNext mission: "), *autoshop_contractfinale), Value(3000)});
							}
						}
						else
						{
							Call(*CenteredText, Values{Value("No Active Contract"), Value(false), Value(false)});
						}
						Call(Member(context.Global("ImGui"), "Separator"), Values{});
						Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("autoshop_contracts")})), "draw", Values{});
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Refresh Board")}))))
						{
							Call(Member(*AutoShop_Contract, "AutoShop_Refresh"), Values{});
						}
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Teleport to Auto Shop")}))))
						{
							Call(*TpToBlip, Values{Value(false), Member(Member(Member(context.Global("Blips"), "Property"), "AUTO_SHOP_PROPERTY"), "ID"), Value(false)});
						}
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Instant Finish")}))))
						{
							Call(*Fmmc2020Finish, Values{});
						}
					}
					Call(Member(context.Global("ImGui"), "EndChild"), Values{});
				}
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(3.8)});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "TreeNodeEx"), Values{Concat(Value("Dr. Dre Contract  ##"), *nomoretreeplease), Value(8219)}))))
				{
					if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginChild"), Values{Value("##drecontract"), Value(210), Value(104), Value(1)}))))
					{
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Skip: Don't F* With Dre"), Negate(Value(1))}))))
						{
							Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_FIXER_STORY_BS"), Value(4095)});
							Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Dr. Dre Contract Skip"), Value("Success! All preps skipped.\nNext mission: Don't F* With Dre"), Value(3000)});
						}
						Call(Member(context.Global("ImGui"), "Separator"), Values{});
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Teleport to Agency")}))))
						{
							Call(*TpToBlip, Values{Value(false), Member(Member(Member(context.Global("Blips"), "Property"), "AGENCY"), "ID"), Value(false)});
						}
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Instant Finish")}))))
						{
							Call(*Fmmc2020Finish, Values{});
						}
					}
					Call(Member(context.Global("ImGui"), "EndChild"), Values{});
				}
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(3.8)});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "TreeNodeEx"), Values{Concat(Value("Cluckin' Bell Farm Raid  ##"), *nomoretreeplease), Value(8219)}))))
				{
					if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginChild"), Values{Value("##cbrchild"), Value(390), Value(192), Value(1)}))))
					{
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Skip: Scene of the Crime"), Negate(Value(1))}))))
						{
							Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_SALV23_INST_PROG"), Value(31)});
							Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Cluckin' Bell Farm Raid Skip"), Value("Success! All preps skipped.\nNext mission: Scene of the Crime"), Value(3000)});
						}
						Call(Member(context.Global("ImGui"), "Separator"), Values{});
						Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("cbr_approach")})), "draw", Values{});
						Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("cbr_weapons")})), "draw", Values{});
						Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("cbr_gear")})), "draw", Values{});
						Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("cbr_getawayvehicle")})), "draw", Values{});
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Teleport to L.S.P.D.")}))))
						{
							Call(*TpToBlip, Values{Value(false), Member(context.Global("Blips"), "VINCENT"), Value(false)});
						}
					}
					Call(Member(context.Global("ImGui"), "EndChild"), Values{});
				}
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(3.8)});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "TreeNodeEx"), Values{Concat(Value("LSA Operations ##"), *nomoretreeplease), Value(8219)}))))
				{
					if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginChild"), Values{Value("##avopchild"), Value(260), Value(42), Value(1)}))))
					{
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Instant Finish"), Negate(Value(1))}))))
						{
							if (Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("fm_content_smuggler_ops")}))))
							{
								Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_content_smuggler_ops"), Add(Value(7881), Value(1306))})), "set_int", Values{Value(1)});
								Call(*LocalSetBit, Values{First(Call(context.Global("ScriptLocal"), Values{Value("fm_content_smuggler_ops"), Add(Value(7743), Value(1))})), Value(1), Value(11)});
							}
						}
					}
					Call(Member(context.Global("ImGui"), "EndChild"), Values{});
				}
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(3.8)});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "TreeNodeEx"), Values{Concat(Value("Salvage Yard ##"), *nomoretreeplease), Value(8219)}))))
				{
					if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginChild"), Values{Value("##salv23child"), Value(288), Member(*SALV23_Robbery, "ChildRes"), Value(1)}))))
					{
						Member(*SALV23_Robbery, "ChildRes") = Value(250);
						if (Truthy(NotEqual(First(Call(Member(context.Global("stats"), "get_int"), Values{Value("MPX_SALV23_VEH_ROB")})), Negate(Value(1)))))
						{
							if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Concat(Value("Skip: "), Member(Member(Member(*SALV23_Robbery, "Name"), Add(First(Call(Member(context.Global("stats"), "get_int"), Values{Value("MPX_SALV23_VEH_ROB")})), Value(1))), Value(2))), Negate(Value(1))}))))
							{
								Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_SALV23_GEN_BS"), Negate(Value(1))});
								Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_SALV23_SCOPE_BS"), Negate(Value(1))});
								Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_SALV23_FM_PROG"), Negate(Value(1))});
							}
						}
						else if (Truthy(And(Equal(First(Call(Member(context.Global("stats"), "get_int"), Values{Value("MPX_SALV23_VEH_ROB")})), Negate(Value(1))), [&] {
							         return Equal(First(Call(Member(context.Global("stats"), "get_masked_bool"), Values{Value("MPX_SALV23_GEN_BS"), Value(0)})), Value(false));
						         })))
						{
							Call(*CenteredText, Values{Value("No Active Robbery"), Value(false), Value(false)});
						}
						else if (Truthy(And(Equal(First(Call(Member(context.Global("stats"), "get_int"), Values{Value("MPX_SALV23_VEH_ROB")})), Negate(Value(1))), [&] {
							         return Equal(First(Call(Member(context.Global("stats"), "get_masked_bool"), Values{Value("MPX_SALV23_GEN_BS"), Value(0)})), Value(true));
						         })))
						{
							if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Fix Salvage Yard PC"), Negate(Value(1))}))))
							{
								Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_SALV23_GEN_BS"), Negate(Value(2123778))});
								Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Salvage Yard"), Value("Salvage Yard PC has been fixed! Please leave and enter The Salvage Yard again to see the changes.")});
							}
						}
						Call(Member(context.Global("ImGui"), "Separator"), Values{});
						if (Truthy(NotEqual(First(Call(Member(context.Global("stats"), "get_int"), Values{Value("MPX_SALV23_VEH_ROB")})), Negate(Value(1)))))
						{
							Call(Member(context.Global("ImGui"), "Spacing"), Values{});
							Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(3.8)});
							if (Truthy(First(Call(Member(context.Global("ImGui"), "TreeNodeEx"), Values{Concat(Value("Current Robbery ##"), *nomoretreeplease), Value(8219)}))))
							{
								Member(*SALV23_Robbery, "ChildRes") = Add(Member(*SALV23_Robbery, "ChildRes"), Value(62));
								Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("salv23currobbery")})), "draw", Values{});
								if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Clear Current Robbery")}))))
								{
									{
										const auto start_379 = Number(Value(0));
										const auto end_380 = Number(Value(2));
										const auto step_381 = Number(Value(1));
										for (double index_382 = start_379; step_381 > 0 ? index_382 <= end_380 : index_382 >= end_380; index_382 += step_381)
										{
											auto i_383 = MakeCell(Value(index_382));
											if (Truthy(GreaterEqual(First(Call(Member(context.Global("stats"), "get_int"), Values{Concat(Value("MPX_SALV23_VEHROB_STATUS"), *i_383)})), Value(1))))
											{
												Call(Member(context.Global("stats"), "set_int"), Values{Concat(Value("MPX_SALV23_VEHROB_STATUS"), *i_383), Value(0)});
												Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_SALV23_VEH_ROB"), Negate(Value(1))});
												Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_SALV23_GEN_BS"), Negate(Value(2123778))});
											}
										}
									}
								}
								Call(Member(context.Global("ImGui"), "Separator"), Values{});
							}
						}
						Call(Member(context.Global("ImGui"), "Spacing"), Values{});
						Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(3.8)});
						if (Truthy(First(Call(Member(context.Global("ImGui"), "TreeNodeEx"), Values{Concat(Value("Robbery Slots ##"), *nomoretreeplease), Value(8219)}))))
						{
							Member(*SALV23_Robbery, "ChildRes") = Add(Member(*SALV23_Robbery, "ChildRes"), Value(148));
							{
								const auto start_384 = Number(Value(0));
								const auto end_385 = Number(Value(2));
								const auto step_386 = Number(Value(1));
								for (double index_387 = start_384; step_386 > 0 ? index_387 <= end_385 : index_387 >= end_385; index_387 += step_386)
								{
									auto robslots = MakeCell(Value(index_387));
									Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Concat(Value("salv23robbery"), *robslots)})), "draw", Values{});
								}
							}
							if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Reset Vehicles Status")}))))
							{
								Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_SALV23_VEHROB_STATUS0"), Value(0)});
								Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_SALV23_VEHROB_STATUS1"), Value(0)});
								Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_SALV23_VEHROB_STATUS2"), Value(0)});
							}
							if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Set All Vehicles as Claimable")}))))
							{
								{
									Iterator iterator_388(Append(Values{}, Call(context.Global("ipairs"), Values{Member(*SALV23_Robbery, "ClaimableVeh")})));
									for (auto row_389 = iterator_388.Next(); !row_389.empty() && !row_389[0].IsNil(); row_389 = iterator_388.Next())
									{
										auto __390 = MakeCell(At(row_389, 0));
										auto salv23slot = MakeCell(At(row_389, 1));
										Call(Member(context.Global("tunables"), "set_int"), Values{*salv23slot, Value(1)});
									}
								}
							}
							Call(Member(context.Global("ImGui"), "Separator"), Values{});
						}
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Refresh Planning PC")}))))
						{
							Call(Member(*SALV23_Robbery, "SALV23_Refresh"), Values{});
						}
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Teleport to Salvage Yard")}))))
						{
							Call(*TpToBlip, Values{Value(false), Member(Member(Member(context.Global("Blips"), "Property"), "SALVAGE_YARD"), "ID"), Value(false)});
						}
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Instant Salvage Parts")}))))
						{
							if (Truthy(NotEqual(First(Call(Member(context.Global("stats"), "get_int"), Values{Value("MPX_MPSV_MODEL_SALVAGE_LIFT1")})), Value(0))))
							{
								Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Member(*SALV23_Robbery, "Lift"), Value(1))})), "set_int", Values{Value(1)});
							}
							if (Truthy(NotEqual(First(Call(Member(context.Global("stats"), "get_int"), Values{Value("MPX_MPSV_MODEL_SALVAGE_LIFT2")})), Value(0))))
							{
								Invoke(First(Call(context.Global("ScriptGlobal"), Values{Add(Member(*SALV23_Robbery, "Lift"), Value(2))})), "set_int", Values{Value(1)});
							}
						}
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Remove Tow Truck Cooldown")}))))
						{
							Call(Member(context.Global("tunables"), "set_int"), Values{Member(*SALV23_Robbery, "TruckCD"), Value(1)});
						}
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Instant Finish")}))))
						{
							{
								Iterator iterator_391(Append(Values{}, Call(context.Global("ipairs"), Values{Member(*SALV23_Robbery, "Finish")})));
								for (auto row_392 = iterator_391.Next(); !row_392.empty() && !row_392[0].IsNil(); row_392 = iterator_391.Next())
								{
									auto __393 = MakeCell(At(row_392, 0));
									auto FinishFile = MakeCell(At(row_392, 1));
									if (Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Member(*FinishFile, "ScriptName")}))))
									{
										if (Truthy(NotEqual(Member(*FinishFile, "ScriptName"), Value("fm_content_tow_truck_work"))))
										{
											Invoke(First(Call(context.Global("ScriptLocal"), Values{Member(*FinishFile, "ScriptName"), Member(*FinishFile, "Local1")})), "set_int", Values{Value(1)});
										}
										else
										{
											Invoke(First(Call(context.Global("ScriptLocal"), Values{Member(*FinishFile, "ScriptName"), Member(*FinishFile, "Local1")})), "set_int", Values{Value(3)});
										}
										Call(*LocalSetBit, Values{First(Call(context.Global("ScriptLocal"), Values{Member(*FinishFile, "ScriptName"), Member(*FinishFile, "Local2")})), Value(1), Value(11)});
									}
								}
							}
						}
						if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
						{
							Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Instant Finish Robberies and Tow Truck Service Missions.")});
						}
					}
					Call(Member(context.Global("ImGui"), "EndChild"), Values{});
				}
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(3.8)});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "TreeNodeEx"), Values{Concat(Value("Bounty Targets ##"), *nomoretreeplease), Value(8219)}))))
				{
					if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginChild"), Values{Value("##bountytargetschild"), Value(300), Member(*BountyTargets, "ChildRes"), Value(1)}))))
					{
						Member(*BountyTargets, "ChildRes") = Value(250);
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Skip: Jenette Dialogues"), Negate(Value(1))}))))
						{
							Call(Member(context.Global("stats"), "set_packed_bool_range"), Values{Value(51192), Value(51195), Value(true)});
							Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Jenette The Mutette"), Value("Success! Jenette dialogues have been skipped!")});
						}
						if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
						{
							Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Skips Jenette dialogues for all current targets.")});
						}
						Call(Member(context.Global("ImGui"), "Separator"), Values{});
						{
							const auto start_394 = Number(Value(0));
							const auto end_395 = Number(Value(2));
							const auto step_396 = Number(Value(1));
							for (double index_397 = start_394; step_396 > 0 ? index_397 <= end_395 : index_397 >= end_395; index_397 += step_396)
							{
								auto bounty = MakeCell(Value(index_397));
								Call(Member(context.Global("ImGui"), "Spacing"), Values{});
								Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(3.8)});
								if (Truthy(First(Call(Member(context.Global("ImGui"), "TreeNodeEx"), Values{Concat(Value("Bounty Target "), Concat(Add(*bounty, Value(1)), Concat(Value(" ##"), *nomoretreeplease))), Value(8219)}))))
								{
									Member(*BountyTargets, "ChildRes") = Add(Member(*BountyTargets, "ChildRes"), Value(60));
									if (Truthy(Equal(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Concat(Value("bountytargetmission"), *bounty)})), "get_value", Values{})), Negate(Value(1)))))
									{
										Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Concat(Value("bountytargetmission"), *bounty)})), "set_value", Append(Values{}, Call(Member(context.Global("stats"), "get_int"), Values{Concat(Value("MPX_BOUNTY24_STD_TARG_MIS_"), *bounty)})));
									}
									Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Concat(Value("bountytargetmission"), *bounty)})), "draw", Values{});
									Call(Member(context.Global("ImGui"), "SetNextItemWidth"), Values{Value(189)});
									auto values_398 = Append(Values{}, Call(Member(context.Global("ImGui"), "Combo"), Values{Concat(Value("Target##"), *bounty), First(Call(Member(context.Global("stats"), "get_int"), Values{Concat(Value("MPX_BOUNTY24_STD_TARG_TAR_"), *bounty)})), Member(Member(*BountyTargets, "TargetCategory"), First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Concat(Value("bountytargetmission"), *bounty)})), "get_value", Values{}))), Value(3)}));
									Member(*BountyTargets, "Target") = At(values_398, 0);
									context.Global("targetchanged") = At(values_398, 1);
									if (Truthy(context.Global("targetchanged")))
									{
										Call(Member(context.Global("stats"), "set_int"), Values{Concat(Value("MPX_BOUNTY24_STD_TARG_TAR_"), *bounty), Member(*BountyTargets, "Target")});
									}
								}
							}
						}
						Call(Member(context.Global("ImGui"), "Spacing"), Values{});
						Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(3.8)});
						if (Truthy(First(Call(Member(context.Global("ImGui"), "TreeNodeEx"), Values{Concat(Value("Most Wanted Target ##"), *nomoretreeplease), Value(8219)}))))
						{
							Member(*BountyTargets, "ChildRes") = Add(Member(*BountyTargets, "ChildRes"), Value(30));
							Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("mwtarget")})), "draw", Values{});
						}
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Refresh PC")}))))
						{
							Call(Member(*BountyTargets, "BailOffice_Refresh"), Values{});
						}
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Teleport to Bail Office")}))))
						{
							Call(*TpToBlip, Values{Value(false), Member(Member(Member(context.Global("Blips"), "Property"), "BAIL_BONDS_OFFICE"), "ID"), Value(false)});
						}
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Instant Finish")}))))
						{
							if (Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("fm_content_bounty_targets")}))))
							{
								Invoke(First(Call(context.Global("ScriptLocal"), Values{Value("fm_content_bounty_targets"), Add(Value(7289), Value(1284))})), "set_int", Values{Value(1)});
								Call(*LocalSetBit, Values{First(Call(context.Global("ScriptLocal"), Values{Value("fm_content_bounty_targets"), Add(Value(7150), Value(1))})), Value(1), Value(11)});
							}
							Call(*Fmmc2020Finish, Values{});
						}
					}
					Call(Member(context.Global("ImGui"), "EndChild"), Values{});
				}
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(3.8)});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "TreeNodeEx"), Values{Concat(Value("FIB Files ##fibsetups"), *nomoretreeplease), Value(8219)}))))
				{
					if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginChild"), Values{Value("#fibchild#"), Value(288), Value(162), Value(1)}))))
					{
						auto values_399 = Values{};
						auto fib_filefinale = MakeCell(At(values_399, 0));
						if (Truthy(Greater(First(Call(Member(context.Global("stats"), "get_int"), Values{Value("MPX_HACKER24_ACTIVE_ROB")})), Negate(Value(1)))))
						{
							*fib_filefinale = Member(Member(Member(*FIB_File, "Files"), Add(First(Call(Member(context.Global("stats"), "get_int"), Values{Value("MPX_HACKER24_ACTIVE_ROB")})), Value(1))), Value(2));
							if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Concat(Value("Skip: "), *fib_filefinale), Negate(Value(1))}))))
							{
								Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_HACKER24_GEN_BS"), Negate(Value(1))});
								Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - FIB File Skip"), Concat(Value("Success! All preps skipped.\nNext mission: "), *fib_filefinale), Value(3000)});
							}
							Call(Member(context.Global("ImGui"), "Separator"), Values{});
						}
						Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("fib_files")})), "draw", Values{});
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Refresh PC")}))))
						{
							Call(Member(*FIB_File, "HackerDen_Refresh"), Values{});
						}
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Teleport to Garment Factory")}))))
						{
							Call(*TpToBlip, Values{Value(false), Member(Member(Member(context.Global("Blips"), "Property"), "GARMENT_FACTORY"), "ID"), Value(false)});
						}
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Instant Finish")}))))
						{
							{
								Iterator iterator_400(Append(Values{}, Call(context.Global("ipairs"), Values{Member(*FIB_File, "Finish")})));
								for (auto row_401 = iterator_400.Next(); !row_401.empty() && !row_401[0].IsNil(); row_401 = iterator_400.Next())
								{
									auto __402 = MakeCell(At(row_401, 0));
									auto FinishFile_403 = MakeCell(At(row_401, 1));
									if (Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Member(*FinishFile_403, "ScriptName")}))))
									{
										Invoke(First(Call(context.Global("ScriptLocal"), Values{Member(*FinishFile_403, "ScriptName"), Member(*FinishFile_403, "Local1")})), "set_int", Values{Value(1)});
										Call(*LocalSetBit, Values{First(Call(context.Global("ScriptLocal"), Values{Member(*FinishFile_403, "ScriptName"), Member(*FinishFile_403, "Local2")})), Value(1), Value(11)});
									}
								}
							}
						}
					}
					Call(Member(context.Global("ImGui"), "EndChild"), Values{});
				}
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(3.8)});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "TreeNodeEx"), Values{Concat(Value("Oscar Guzman Flies Again  ##"), *nomoretreeplease), Value(8219)}))))
				{
					if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginChild"), Values{Value("##ogfachild"), Value(320), Value(220), Value(1)}))))
					{
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Skip: The Titan Job"), Negate(Value(1))}))))
						{
							Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_HACKER24_INST_BS"), Negate(Value(1))});
							Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Oscar Guzman Skip"), Value("Success! All preps skipped.\nNext mission: The Titan Job"), Value(3000)});
						}
						Call(Member(context.Global("ImGui"), "Separator"), Values{});
						auto values_404 = Append(Values{}, Call(Member(context.Global("ImGui"), "Checkbox"), Append(Values{Value("Hard Mode")}, Call(Member(context.Global("stats"), "get_packed_bool"), Values{Member(Member(*OzcarGuzmanFliesAgain, "Difficulty"), "DifficultyPB")}))));
						Member(Member(*OzcarGuzmanFliesAgain, "Difficulty"), "HardMode") = At(values_404, 0);
						context.Global("ogfamode") = At(values_404, 1);
						Member(Member(*OzcarGuzmanFliesAgain, "S_Mogul"), "Setup") = First(Call(Member(context.Global("ImGui"), "Checkbox"), Values{Value("Setup: Mogul"), Member(Member(*OzcarGuzmanFliesAgain, "S_Mogul"), "Setup")}));
						Member(Member(*OzcarGuzmanFliesAgain, "S_IronMule"), "Setup") = First(Call(Member(context.Global("ImGui"), "Checkbox"), Values{Value("Setup: Iron Mule"), Member(Member(*OzcarGuzmanFliesAgain, "S_IronMule"), "Setup")}));
						Member(Member(*OzcarGuzmanFliesAgain, "S_Ammunition"), "Setup") = First(Call(Member(context.Global("ImGui"), "Checkbox"), Values{Value("Setup: Ammunition"), Member(Member(*OzcarGuzmanFliesAgain, "S_Ammunition"), "Setup")}));
						if (Truthy(context.Global("ogfamode")))
						{
							Call(Member(context.Global("stats"), "set_packed_bool"), Values{Member(Member(*OzcarGuzmanFliesAgain, "Difficulty"), "DifficultyPB"), Member(Member(*OzcarGuzmanFliesAgain, "Difficulty"), "HardMode")});
						}
						{
							Iterator iterator_405(Append(Values{}, Call(context.Global("ipairs"), Values{Member(Member(*OzcarGuzmanFliesAgain, "S_Mogul"), "SetupPB")})));
							for (auto row_406 = iterator_405.Next(); !row_406.empty() && !row_406[0].IsNil(); row_406 = iterator_405.Next())
							{
								auto __407 = MakeCell(At(row_406, 0));
								auto pbstat = MakeCell(At(row_406, 1));
								Call(Member(context.Global("stats"), "set_packed_bool"), Values{*pbstat, Member(Member(*OzcarGuzmanFliesAgain, "S_Mogul"), "Setup")});
							}
						}
						{
							Iterator iterator_408(Append(Values{}, Call(context.Global("ipairs"), Values{Member(Member(*OzcarGuzmanFliesAgain, "S_IronMule"), "SetupPB")})));
							for (auto row_409 = iterator_408.Next(); !row_409.empty() && !row_409[0].IsNil(); row_409 = iterator_408.Next())
							{
								auto __410 = MakeCell(At(row_409, 0));
								auto pbstat_411 = MakeCell(At(row_409, 1));
								Call(Member(context.Global("stats"), "set_packed_bool"), Values{*pbstat_411, Member(Member(*OzcarGuzmanFliesAgain, "S_IronMule"), "Setup")});
							}
						}
						{
							Iterator iterator_412(Append(Values{}, Call(context.Global("ipairs"), Values{Member(Member(*OzcarGuzmanFliesAgain, "S_Ammunition"), "SetupPB")})));
							for (auto row_413 = iterator_412.Next(); !row_413.empty() && !row_413[0].IsNil(); row_413 = iterator_412.Next())
							{
								auto __414 = MakeCell(At(row_413, 0));
								auto pbstat_415 = MakeCell(At(row_413, 1));
								Call(Member(context.Global("stats"), "set_packed_bool"), Values{*pbstat_415, Member(Member(*OzcarGuzmanFliesAgain, "S_Ammunition"), "Setup")});
							}
						}
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Teleport to McKenzie Field Hangar")}))))
						{
							Call(*TpToBlip, Values{Value(false), Member(Member(Member(context.Global("Blips"), "Property"), "FIELD_HANGAR"), "ID"), Value(false)});
						}
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Instant Finish")}))))
						{
							Call(*Fmmc2020Finish, Values{});
						}
					}
					Call(Member(context.Global("ImGui"), "EndChild"), Values{});
				}
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(3.8)});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "TreeNodeEx"), Values{Concat(Value("KnoWay Out ##"), *nomoretreeplease), Value(8219)}))))
				{
					if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginChild"), Values{Value("##knowayoutchild"), Value(260), Value(104), Value(1)}))))
					{
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Skip: A Clean Break"), Negate(Value(1))}))))
						{
							Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_M25_AVI_MISSION_CURRENT"), Value(4)});
							Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - KnoWay Out Skip"), Value("Success! All preps skipped.\nNext mission: A Clean Break"), Value(3000)});
						}
						Call(Member(context.Global("ImGui"), "Separator"), Values{});
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Teleport to Payphone")}))))
						{
							Call(*TpToBlip, Values{Value(false), Member(context.Global("Blips"), "ARMENIAN_FAMILY"), Value(false)});
						}
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Instant Finish")}))))
						{
							Call(*Fmmc2020Finish, Values{});
						}
					}
					Call(Member(context.Global("ImGui"), "EndChild"), Values{});
				}
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(3.8)});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "TreeNodeEx"), Values{Concat(Value("The Kortz Center Heist ##"), *nomoretreeplease), Value(8219)}))))
				{
					if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginChild"), Values{Value("##thek26child"), Value(320), Member(*TheKortzCenterHeist, "ChildRes"), Value(1)}))))
					{
						Member(*TheKortzCenterHeist, "ChildRes") = Value(162);
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Skip: All Preps"), Negate(Value(1))}))))
						{
							Call(Member(context.Global("stats"), "set_int"), Values{Member(Member(*TheKortzCenterHeist, "Stat"), "General"), Value(266338303)});
							Call(Member(context.Global("stats"), "set_int"), Values{Member(Member(*TheKortzCenterHeist, "Stat"), "Robbery"), Value(65535)});
							Call(Member(context.Global("stats"), "set_int"), Values{Member(Member(*TheKortzCenterHeist, "Stat"), "Scoping"), Negate(Value(1))});
							Call(Member(context.Global("stats"), "set_int"), Values{Member(Member(*TheKortzCenterHeist, "Stat"), "POI"), Negate(Value(1))});
							Call(Member(*TheKortzCenterHeist, "K26_Refresh"), Values{});
						}
						Call(Member(context.Global("ImGui"), "Separator"), Values{});
						Call(Member(context.Global("ImGui"), "Spacing"), Values{});
						Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(3.8)});
						if (Truthy(First(Call(Member(context.Global("ImGui"), "TreeNodeEx"), Values{Concat(Value("Setups ##"), *nomoretreeplease), Value(8219)}))))
						{
							Member(*TheKortzCenterHeist, "ChildRes") = Add(Member(*TheKortzCenterHeist, "ChildRes"), Value(120));
							auto values_416 = Append(Values{}, Call(Member(context.Global("ImGui"), "Checkbox"), Append(Values{Value("Hard Mode")}, Call(Member(context.Global("stats"), "get_masked_bool"), Values{Member(Member(*TheKortzCenterHeist, "Stat"), "General"), Value(28)}))));
							Member(Member(*TheKortzCenterHeist, "Difficulty"), "HardMode") = At(values_416, 0);
							context.Global("k26mode") = At(values_416, 1);
							auto values_417 = Append(Values{}, Call(Member(context.Global("ImGui"), "Checkbox"), Append(Values{Value("Weak Guards")}, Call(Member(context.Global("stats"), "get_masked_bool"), Values{Member(Member(*TheKortzCenterHeist, "Stat"), "General"), Value(31)}))));
							Member(Member(*TheKortzCenterHeist, "S_WeakGuards"), "Setup") = At(values_417, 0);
							context.Global("k26weakguards") = At(values_417, 1);
							if (Truthy(context.Global("k26mode")))
							{
								Call(Member(context.Global("stats"), "set_masked_bool"), Values{Member(Member(*TheKortzCenterHeist, "Stat"), "General"), Member(Member(*TheKortzCenterHeist, "Difficulty"), "DifficultyBIT"), Member(Member(*TheKortzCenterHeist, "Difficulty"), "HardMode")});
							}
							if (Truthy(context.Global("k26weakguards")))
							{
								Call(Member(context.Global("stats"), "set_masked_bool"), Values{Member(Member(*TheKortzCenterHeist, "Stat"), "General"), Member(Member(*TheKortzCenterHeist, "S_WeakGuards"), "SetupBIT"), Member(Member(*TheKortzCenterHeist, "S_WeakGuards"), "Setup")});
							}
							{
								Iterator iterator_418(Append(Values{}, Call(context.Global("ipairs"), Values{Member(*TheKortzCenterHeist, "Setups")})));
								for (auto row_419 = iterator_418.Next(); !row_419.empty() && !row_419[0].IsNil(); row_419 = iterator_418.Next())
								{
									auto __420 = MakeCell(At(row_419, 0));
									auto setupk26 = MakeCell(At(row_419, 1));
									auto values_421 = Append(Values{}, Call(Member(context.Global("ImGui"), "Checkbox"), Append(Values{Member(*setupk26, "Label")}, Call(Member(context.Global("stats"), "get_masked_bool"), Values{Member(Member(*TheKortzCenterHeist, "Stat"), "Robbery"), Member(*setupk26, "SetupBIT")}))));
									Member(*setupk26, "Setup") = At(values_421, 0);
									context.Global("setupk26changed") = At(values_421, 1);
									if (Truthy(context.Global("setupk26changed")))
									{
										Call(Member(context.Global("stats"), "set_masked_bool"), Values{Member(Member(*TheKortzCenterHeist, "Stat"), "Robbery"), Member(*setupk26, "SetupBIT"), Member(*setupk26, "Setup")});
										if (Truthy(NotEqual(Member(*setupk26, "Setup2BIT"), Value{})))
										{
											{
												Iterator iterator_422(Append(Values{}, Call(context.Global("ipairs"), Values{Member(*setupk26, "Setup2BIT")})));
												for (auto row_423 = iterator_422.Next(); !row_423.empty() && !row_423[0].IsNil(); row_423 = iterator_422.Next())
												{
													auto __424 = MakeCell(At(row_423, 0));
													auto bit = MakeCell(At(row_423, 1));
													Call(Member(context.Global("stats"), "set_masked_bool"), Values{Member(Member(*TheKortzCenterHeist, "Stat"), "General"), *bit, Member(*setupk26, "Setup")});
												}
											}
										}
									}
								}
							}
							if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Others")}))))
							{
								{
									Iterator iterator_425(Append(Values{}, Call(context.Global("ipairs"), Values{Member(*TheKortzCenterHeist, "Others")})));
									for (auto row_426 = iterator_425.Next(); !row_426.empty() && !row_426[0].IsNil(); row_426 = iterator_425.Next())
									{
										auto __427 = MakeCell(At(row_426, 0));
										auto setupothersk26 = MakeCell(At(row_426, 1));
										Call(Member(context.Global("stats"), "set_masked_bool"), Values{Member(Member(*TheKortzCenterHeist, "Stat"), "General"), Member(*setupothersk26, "SetupBIT"), Value(true)});
									}
								}
							}
							if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
							{
								Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Unlocks other setups:\nAll Entry Points,\nManhole Key,\nVault Keypad,\nAdvanced Avionics.")});
							}
						}
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Reset Stolen Targets")}))))
						{
							Call(Member(context.Global("stats"), "set_int"), Values{Member(Member(*TheKortzCenterHeist, "Stat"), "Stolen"), Value(0)});
						}
						if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
						{
							Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Removes \"On Loan\" signs.")});
						}
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Refresh Board")}))))
						{
							Call(Member(*TheKortzCenterHeist, "K26_Refresh"), Values{});
						}
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Teleport to Mansion")}))))
						{
							Call(*TpToBlip, Values{Value(false), Member(context.Global("Blips"), "PROPERTY_MANSION_ART_HEIST"), Value(false)});
						}
					}
					Call(Member(context.Global("ImGui"), "EndChild"), Values{});
				}
				Call(Member(context.Global("ImGui"), "EndChild"), Values{});
				Call(Member(context.Global("ImGui"), "EndGroup"), Values{});
				Call(Member(context.Global("ImGui"), "Text"), Values{Value("Misc")});
				Call(Member(context.Global("ImGui"), "Separator"), Values{});
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("autoloot")})), "draw", Values{});
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("minmissionpayout")})), "draw", Values{});
				if (Truthy(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("minmissionpayout")})), "get_value", Values{}))))
				{
					Call(Member(context.Global("ImGui"), "SameLine"), Values{});
					Call(Member(context.Global("ImGui"), "SetNextItemWidth"), Values{Value(140)});
					Member(*MinPayoutContactMiss, "MMpayout") = First(Call(Member(context.Global("ImGui"), "SliderInt"), Values{Value("Payout"), Member(*MinPayoutContactMiss, "MMpayout"), Value(0), Value(100000)}));
				}
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(3.8)});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "TreeNodeEx"), Values{Concat(Value("Survivals ##"), *nomoretreeplease), Value(8219)}))))
				{
					Call(*CustomChild, Values{Value{}, Value("##survivalschild"), Value(280), Member(*UnnaturalSelection, "ChildRes"), Value(1), MakeFunction([=, &context](const Values& args) -> Values {
						                          Member(*UnnaturalSelection, "ChildRes") = Value(130);
						                          if (Truthy(And(First(Call(Member(context.Global("scripts"), "is_active"), Values{Member(Member(Member(*UnnaturalSelection, "Finish"), "Classic"), "ScriptName")})), [&] {
							                              return NotEqual(First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Member(Member(Member(*UnnaturalSelection, "Finish"), "Classic"), "ScriptName"), Add(Member(Member(Member(*UnnaturalSelection, "Finish"), "Classic"), "Base"), Value(5))})), "get_int", Values{})), Value(0));
						                              })))
						                          {
							                          auto ClassicSurvival = MakeCell(Member(Member(*UnnaturalSelection, "Finish"), "Classic"));
							                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Start Wave")}))))
							                          {
								                          Invoke(First(Call(context.Global("ScriptLocal"), Values{Member(*ClassicSurvival, "ScriptName"), Add(Member(*ClassicSurvival, "Base"), Value(5))})), "set_int", Values{Subtract(First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Member(*ClassicSurvival, "ScriptName"), Add(Member(*ClassicSurvival, "Base"), Value(5))})), "get_int", Values{})), Value(19999))});
							                          }
							                          Call(Member(context.Global("ImGui"), "SameLine"), Values{});
							                          Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("survivals_noidlekick")})), "draw", Values{});
							                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Instant Finish")}))))
							                          {
								                          if (Truthy(Value(!Truthy(Member(Member(*UnnaturalSelection, "Finish"), "WaveOnly")))))
								                          {
									                          Invoke(First(Call(context.Global("ScriptLocal"), Values{Member(*ClassicSurvival, "ScriptName"), Add(Member(*ClassicSurvival, "Base"), Value(1295))})), "set_int", Values{Value(10)});
								                          }
								                          Invoke(First(Call(context.Global("ScriptLocal"), Values{Member(*ClassicSurvival, "ScriptName"), Add(Member(*ClassicSurvival, "Base"), Value(9))})), "set_int", Values{Value(4)});
							                          }
							                          Call(Member(context.Global("ImGui"), "SameLine"), Values{});
							                          Member(Member(*UnnaturalSelection, "Finish"), "WaveOnly") = First(Call(Member(context.Global("ImGui"), "Checkbox"), Values{Value("Wave Only"), Member(Member(*UnnaturalSelection, "Finish"), "WaveOnly")}));
							                          Member(Member(*UnnaturalSelection, "Finish"), "ToWave") = First(Call(Member(context.Global("ImGui"), "InputInt"), Values{Value("Skip to Wave"), Member(Member(*UnnaturalSelection, "Finish"), "ToWave")}));
							                          Call(Member(context.Global("ImGui"), "SameLine"), Values{});
							                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Skip")}))))
							                          {
								                          Invoke(First(Call(context.Global("ScriptLocal"), Values{Member(*ClassicSurvival, "ScriptName"), Add(Member(*ClassicSurvival, "Base"), Value(1295))})), "set_int", Values{Subtract(Member(Member(*UnnaturalSelection, "Finish"), "ToWave"), Value(1))});
								                          Invoke(First(Call(context.Global("ScriptLocal"), Values{Member(*ClassicSurvival, "ScriptName"), Add(Member(*ClassicSurvival, "Base"), Value(9))})), "set_int", Values{Value(4)});
							                          }
							                          if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
							                          {
								                          Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Skips to Selected Wave.")});
							                          }
						                          }
						                          else if (Truthy(And(First(Call(Member(context.Global("scripts"), "is_active"), Values{Member(Member(Member(*UnnaturalSelection, "Finish"), "New"), "ScriptName")})), [&] {
							                                   return NotEqual(First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Member(Member(Member(*UnnaturalSelection, "Finish"), "New"), "ScriptName"), Add(Member(Member(Member(*UnnaturalSelection, "Finish"), "New"), "Base"), Value(3))})), "get_int", Values{})), Value(0));
						                                   })))
						                          {
							                          auto NewSurvival = MakeCell(Member(Member(*UnnaturalSelection, "Finish"), "New"));
							                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Start Wave")}))))
							                          {
								                          Invoke(First(Call(context.Global("ScriptLocal"), Values{Member(*NewSurvival, "ScriptName"), Add(Member(*NewSurvival, "Base"), Value(3))})), "set_int", Values{Subtract(First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Member(*NewSurvival, "ScriptName"), Add(Member(*NewSurvival, "Base"), Value(3))})), "get_int", Values{})), Value(19999))});
							                          }
							                          Call(Member(context.Global("ImGui"), "SameLine"), Values{});
							                          Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("survivals_noidlekick")})), "draw", Values{});
							                          if (Truthy(And(Equal(First(Invoke(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Member(Member(*UnnaturalSelection, "UGC"), "Global")})), "at", Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_ID"), Values{})), Member(*PropGlob, "ArraySize")})), "get_string", Values{})), Member(Member(*UnnaturalSelection, "UGC"), "Cayo")), [&] {
								                              return Equal(First(Call(Member(context.Global("bit"), "band"), Append(Values{First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Member(Member(Member(*UnnaturalSelection, "Finish"), "New"), "ScriptName"), Add(Member(Member(Member(*UnnaturalSelection, "Finish"), "New"), "Base"), Member(Member(Member(*UnnaturalSelection, "Finish"), "New"), "Endless"))})), "get_int", Values{}))}, Call(Member(context.Global("bit"), "lshift"), Values{Value(1), Value(4)})))), Value(0));
							                              })))
							                          {
								                          Member(*UnnaturalSelection, "ChildRes") = Add(Member(*UnnaturalSelection, "ChildRes"), Value(30));
								                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Enable Endless Waves Menu")}))))
								                          {
									                          Invoke(First(Call(context.Global("ScriptLocal"), Values{Member(*NewSurvival, "ScriptName"), Add(Member(*NewSurvival, "Base"), Value(18))})), "set_int", Values{Value(7)});
								                          }
							                          }
							                          else if (Truthy(And(NotEqual(First(Invoke(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Member(Member(*UnnaturalSelection, "UGC"), "Global")})), "at", Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_ID"), Values{})), Member(*PropGlob, "ArraySize")})), "get_string", Values{})), Member(Member(*UnnaturalSelection, "UGC"), "Cayo")), [&] {
								                                   return Equal(First(Call(Member(context.Global("bit"), "band"), Append(Values{First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Member(Member(Member(*UnnaturalSelection, "Finish"), "New"), "ScriptName"), Add(Member(Member(Member(*UnnaturalSelection, "Finish"), "New"), "Base"), Member(Member(Member(*UnnaturalSelection, "Finish"), "New"), "Endless"))})), "get_int", Values{}))}, Call(Member(context.Global("bit"), "lshift"), Values{Value(1), Value(4)})))), Value(0));
							                                   })))
							                          {
								                          Member(*UnnaturalSelection, "ChildRes") = Add(Member(*UnnaturalSelection, "ChildRes"), Value(30));
								                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Enable Endless Waves")}))))
								                          {
									                          Call(*LocalSetBit, Values{First(Call(context.Global("ScriptLocal"), Values{Member(Member(Member(*UnnaturalSelection, "Finish"), "New"), "ScriptName"), Add(Member(Member(Member(*UnnaturalSelection, "Finish"), "New"), "Base"), Member(Member(Member(*UnnaturalSelection, "Finish"), "New"), "Endless"))})), Value(1), Value(4)});
								                          }
							                          }
							                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Instant Finish")}))))
							                          {
								                          if (Truthy(Value(!Truthy(Member(Member(*UnnaturalSelection, "Finish"), "WaveOnly")))))
								                          {
									                          Invoke(First(Call(context.Global("ScriptLocal"), Values{Member(*NewSurvival, "ScriptName"), Add(Member(*NewSurvival, "Base"), Value(247))})), "set_int", Values{Value(10)});
								                          }
								                          Invoke(First(Call(context.Global("ScriptLocal"), Values{Member(*NewSurvival, "ScriptName"), Add(Member(*NewSurvival, "Base"), Value(18))})), "set_int", Values{Value(4)});
							                          }
							                          Call(Member(context.Global("ImGui"), "SameLine"), Values{});
							                          Member(Member(*UnnaturalSelection, "Finish"), "WaveOnly") = First(Call(Member(context.Global("ImGui"), "Checkbox"), Values{Value("Wave Only"), Member(Member(*UnnaturalSelection, "Finish"), "WaveOnly")}));
							                          Call(Member(context.Global("ImGui"), "SetNextItemWidth"), Values{Value(120)});
							                          Member(Member(*UnnaturalSelection, "Finish"), "ToWave") = First(Call(Member(context.Global("ImGui"), "InputInt"), Values{Value("Skip to Wave"), Member(Member(*UnnaturalSelection, "Finish"), "ToWave")}));
							                          Call(Member(context.Global("ImGui"), "SameLine"), Values{});
							                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Skip")}))))
							                          {
								                          Invoke(First(Call(context.Global("ScriptLocal"), Values{Member(*NewSurvival, "ScriptName"), Add(Member(*NewSurvival, "Base"), Value(247))})), "set_int", Values{Subtract(Member(Member(*UnnaturalSelection, "Finish"), "ToWave"), Value(1))});
								                          Invoke(First(Call(context.Global("ScriptLocal"), Values{Member(*NewSurvival, "ScriptName"), Add(Member(*NewSurvival, "Base"), Value(18))})), "set_int", Values{Value(4)});
							                          }
							                          if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
							                          {
								                          Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Skips to selected wave.")});
							                          }
						                          }
						                          else
						                          {
							                          Call(*CenteredText, Values{Value("No Active Survival"), Value(false), Value(false)});
							                          if (Truthy(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("survivals_noidlekick")})), "get_value", Values{}))))
							                          {
								                          Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("survivals_noidlekick")})), "set_value", Values{Value(false)});
							                          }
						                          }
						                          if (Truthy(Greater(Member(Member(*UnnaturalSelection, "Finish"), "ToWave"), Value(10))))
						                          {
							                          if (Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Member(Member(Member(*UnnaturalSelection, "Finish"), "Classic"), "ScriptName")}))))
							                          {
								                          Member(Member(*UnnaturalSelection, "Finish"), "ToWave") = Value(10);
							                          }
							                          else if (Truthy(And(First(Call(Member(context.Global("scripts"), "is_active"), Values{Member(Member(Member(*UnnaturalSelection, "Finish"), "New"), "ScriptName")})), [&] {
								                                   return Equal(First(Call(Member(context.Global("bit"), "band"), Append(Values{First(Invoke(First(Call(context.Global("ScriptLocal"), Values{Member(Member(Member(*UnnaturalSelection, "Finish"), "New"), "ScriptName"), Add(Member(Member(Member(*UnnaturalSelection, "Finish"), "New"), "Base"), Member(Member(Member(*UnnaturalSelection, "Finish"), "New"), "Endless"))})), "get_int", Values{}))}, Call(Member(context.Global("bit"), "lshift"), Values{Value(1), Value(4)})))), Value(0));
							                                   })))
							                          {
								                          Member(Member(*UnnaturalSelection, "Finish"), "ToWave") = Value(10);
							                          }
						                          }
						                          else if (Truthy(Less(Member(Member(*UnnaturalSelection, "Finish"), "ToWave"), Value(1))))
						                          {
							                          Member(Member(*UnnaturalSelection, "Finish"), "ToWave") = Value(1);
						                          }
						                          return {};
					                          })});
				}
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Enable Weekly Boost")}))))
				{
					Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_WEEKLY_BOOST_BS"), Value(0)});
					Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Weekly Boost"), Value("Weekly Boost has been reset! You'll get the maximun payout in heist finales."), Value(3000)});
				}
				if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
				{
					Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Resetting the weekly boost will get you the max payout in heist finales.\nNo one has *reported getting banned for this (yet).")});
				}
				Call(Member(context.Global("ImGui"), "SameLine"), Values{});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Criminal Mastermind Bonus")}))))
				{
					if (Truthy(Value(!Truthy(*criminalmmbonus))))
					{
						Call(Member(context.Global("stats"), "set_int"), Values{Value("MPPLY_HEISTFLOWORDERPROGRESS"), Value(268435455)});
						Call(Member(context.Global("stats"), "set_int"), Values{Value("MPPLY_HEISTTEAMPROGRESSBITSET"), Value(268435455)});
						Call(Member(context.Global("stats"), "set_int"), Values{Value("MPPLY_HEISTNODEATHPROGREITSET"), Value(268435455)});
						Call(Member(context.Global("stats"), "set_bool"), Values{Value("MPPLY_AWD_HST_ORDER"), Value(false)});
						Call(Member(context.Global("stats"), "set_bool"), Values{Value("MPPLY_AWD_HST_SAME_TEAM"), Value(false)});
						Call(Member(context.Global("stats"), "set_bool"), Values{Value("MPPLY_AWD_HST_ULT_CHAL"), Value(false)});
						*criminalmmbonus = Value(true);
						Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Criminal Mastermind Challenge"), Value("$12M Bonus Enabled!"), Value(3000)});
					}
					else
					{
						Call(Member(context.Global("stats"), "set_int"), Values{Value("MPPLY_HEISTFLOWORDERPROGRESS"), Negate(Value(1))});
						Call(Member(context.Global("stats"), "set_int"), Values{Value("MPPLY_HEISTTEAMPROGRESSBITSET"), Negate(Value(1))});
						Call(Member(context.Global("stats"), "set_int"), Values{Value("MPPLY_HEISTNODEATHPROGREITSET"), Negate(Value(1))});
						Call(Member(context.Global("stats"), "set_bool"), Values{Value("MPPLY_AWD_HST_ORDER"), Value(true)});
						Call(Member(context.Global("stats"), "set_bool"), Values{Value("MPPLY_AWD_HST_SAME_TEAM"), Value(true)});
						Call(Member(context.Global("stats"), "set_bool"), Values{Value("MPPLY_AWD_HST_ULT_CHAL"), Value(true)});
						*criminalmmbonus = Value(false);
						Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Criminal Mastermind Challenge"), Value("$12M Bonus Disabled."), Value(3000)});
					}
				}
				if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
				{
					Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Enables the $12M payout bonus for completing the Criminal Mastermind Challenge.")});
				}
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Casino Heist P.O.I Reset")}))))
				{
					Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_H3OPT_POI"), Value(0)});
					Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_H3OPT_ACCESSPOINTS"), Value(0)});
					Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Casino Heist P.O.I Reset"), Value("Diamond Casino Heist P.O.I and Access Points have been reset."), Value(3000)});
				}
				Call(Member(context.Global("ImGui"), "SameLine"), Values{});
				if (Truthy(Or(NotEqual(First(Call(Member(context.Global("stats"), "get_int"), Values{Value("MPX_CH_ARC_CAB_DISPLAY_SLOT36")})), Value(18)), [&] {
					    return NotEqual(First(Call(Member(context.Global("stats"), "get_int"), Values{Value("MPX_CH_ARC_CAB_DISPLAY_SLOT37")})), Value(19));
				    })))
				{
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Casino Heist Extras Reset")}))))
					{
						Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_CAS_HEIST_FLOW"), Negate(Value(1610744001))});
						Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Casino Heist Extras Rese!"), Value("Secure Keypad & Vault Door replicas have been Fixed."), Value(3000)});
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
					{
						Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Resets Secure Keypad & Vault Door at the Arcade, making them available for purchase again.\nYou'll only need this if you can't use them. Once purchased, you'll never have this problem again.")});
					}
				}
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Crack Stash House Safe")}))))
				{
					if (Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("fm_content_stash_house")}))))
					{
						Call(Member(context.Global("script"), "run_in_callback"), Values{*CrackSH});
					}
				}
				if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
				{
					Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Breaks into the safe.")});
				}
				Call(Member(context.Global("ImGui"), "EndTabItem"), Values{});
			}
			if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginTabItem"), Values{Value("Unlocks")}))))
			{
				Call(Member(context.Global("ImGui"), "BeginGroup"), Values{});
				Call(*CustomChild, Values{Value("Content Unlocks"), Value("##contentunlockschild"), Value(360), Value(336), Value(0), MakeFunction([=, &context](const Values& args) -> Values {
					                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Bunker")}))))
					                          {
						                          Call(context.Global("Rplus_Bunker"), Values{});
					                          }
					                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Arena War##unlock")}))))
					                          {
						                          Call(context.Global("Rplus_ArenaWar"), Values{});
					                          }
					                          Call(Member(context.Global("ImGui"), "SameLine"), Values{});
					                          Member(context.Global("Rplus_Config"), "AW_tier") = First(Call(Member(context.Global("ImGui"), "Checkbox"), Values{Value("Tier 1000"), Member(context.Global("Rplus_Config"), "AW_tier")}));
					                          Call(Member(context.Global("ImGui"), "SameLine"), Values{});
					                          Member(context.Global("Rplus_Config"), "AW_stats") = First(Call(Member(context.Global("ImGui"), "Checkbox"), Values{Value("Update Stats"), Member(context.Global("Rplus_Config"), "AW_stats")}));
					                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Collectibles")}))))
					                          {
						                          Call(context.Global("Rplus_Collectibles"), Values{});
					                          }
					                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Tattoos")}))))
					                          {
						                          Call(context.Global("Rplus_Tattoos"), Values{});
					                          }
					                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Los Santos Customs")}))))
					                          {
						                          Call(context.Global("Rplus_LSCustoms"), Values{});
					                          }
					                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Los Santos Car Meet")}))))
					                          {
						                          Call(context.Global("Rplus_LSCM"), Values{});
					                          }
					                          Call(Member(context.Global("ImGui"), "SameLine"), Values{});
					                          Member(context.Global("Rplus_Config"), "LSCM_rep") = First(Call(Member(context.Global("ImGui"), "Checkbox"), Values{Value("Rep. Level 1000"), Member(context.Global("Rplus_Config"), "LSCM_rep")}));
					                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("LSCM Prize Ride Vehicle")}))))
					                          {
						                          if (Truthy(Value(!Truthy(First(Call(Member(context.Global("stats"), "get_bool"), Values{Value("MPX_CARMEET_PV_CLMED")}))))))
						                          {
							                          Call(Member(context.Global("stats"), "set_bool"), Values{Value("MPX_CARMEET_PV_CHLLGE_CMPLT"), Value(true)});
							                          Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - LSCM Prize Ride Vehicle"), Value("Success! Weekly Prize Ride ready to claim!")});
						                          }
						                          else
						                          {
							                          Call(Member(context.Global("notify"), "error"), Values{Value("TinkerScripts - LSCM Prize Ride Vehicle"), Value("You've already claimed this week's Prize Ride!")});
						                          }
					                          }
					                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Halloween Face Paints")}))))
					                          {
						                          Call(Member(context.Global("stats"), "set_packed_bool_range"), Values{Value(4270), Value(4299), Value(true)});
						                          Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Halloween Face Paints"), Value("All Halloween Face Paints Unlocked!"), Value(3000)});
					                          }
					                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Mansion Trophies")}))))
					                          {
						                          Call(context.Global("Rplus_MansionTrophies"), Values{});
					                          }
					                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("All Clothing")}))))
					                          {
						                          Call(context.Global("Rplus_Cloooathes"), Values{});
					                          }
					                          return {};
				                          })});
				Call(*CustomChild, Values{Value("Career Progresss"), Value("##careerprogressschild"), Value(360), Value(100), Value(0), MakeFunction([=, &context](const Values& args) -> Values {
					                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Complete All Challenges")}))))
					                          {
						                          Call(context.Global("Rplus_CareerProgress"), Values{});
					                          }
					                          if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
					                          {
						                          Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Completes All Challenges.")});
					                          }
					                          return {};
				                          })});
				Call(Member(context.Global("ImGui"), "EndGroup"), Values{});
				Call(Member(context.Global("ImGui"), "SameLine"), Values{});
				Call(*CustomChild, Values{Value("Character Stats"), Value("##characterstatschild"), Member(Member(context.Global("Rplus_Config"), "Child"), "Charstatx"), Member(Member(context.Global("Rplus_Config"), "Child"), "Charstaty"), Value(0), MakeFunction([=, &context](const Values& args) -> Values {
					                          Member(Member(context.Global("Rplus_Config"), "Child"), "Charstatx") = Value(360);
					                          Member(Member(context.Global("Rplus_Config"), "Child"), "Charstaty") = Value(336);
					                          Call(Member(context.Global("ImGui"), "Spacing"), Values{});
					                          Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(3.8)});
					                          if (Truthy(First(Call(Member(context.Global("ImGui"), "TreeNodeEx"), Values{Concat(Value("Rank Editor  ##"), *nomoretreeplease), Value(8219)}))))
					                          {
						                          Member(Member(context.Global("Rplus_Config"), "Child"), "Charstatx") = Value(400);
						                          Member(Member(context.Global("Rplus_Config"), "Child"), "Charstaty") = Value(440);
						                          Call(Member(context.Global("ImGui"), "BeginChild"), Values{Value("##setrankchild"), Value(370), Value(200), Value(1)});
						                          Call(Member(context.Global("ImGui"), "Text"), Values{Value("Player Rank")});
						                          Call(Member(context.Global("ImGui"), "SameLine"), Values{});
						                          Call(Member(context.Global("ImGui"), "TextDisabled"), Values{Concat(Value("|  Current Rank: "), First(Call(Member(context.Global("stats"), "get_int"), Values{Value("MPX_CHAR_RANK_FM")})))});
						                          Call(Member(context.Global("ImGui"), "Separator"), Values{});
						                          Call(Member(context.Global("ImGui"), "Spacing"), Values{});
						                          Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("rprank")})), "draw", Values{});
						                          Call(Member(context.Global("ImGui"), "SameLine"), Values{});
						                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Set Rank##player")}))))
						                          {
							                          Call(Member(context.Global("ImGui"), "OpenPopup"), Values{Value("##warnsetrank")});
						                          }
						                          if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginPopupModal"), Values{Value("##warnsetrank"), Value{}, Value(65)}))))
						                          {
							                          Call(Member(context.Global("ImGui"), "Text"), Values{Value("Are you sure?")});
							                          Call(Member(context.Global("ImGui"), "Spacing"), Values{});
							                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Yes")}))))
							                          {
								                          Call(Member(context.Global("stats"), "set_int"), Append(Values{Value("MPX_CHAR_SET_RP_GIFT_ADMIN")}, Call(Member(context.Global("Rplus_Ranks"), "QuadraPoly"), Append(Values{}, Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("rprank")})), "get_value", Values{})))));
								                          Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Player Rank"), Concat(Value("New Rank: "), Concat(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("rprank")})), "get_value", Values{})), Value("\nPlease change sessions to see the changes."))), Value(3000)});
								                          Call(Member(context.Global("ImGui"), "CloseCurrentPopup"), Values{});
							                          }
							                          Call(Member(context.Global("ImGui"), "SameLine"), Values{});
							                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("No")}))))
							                          {
								                          Call(Member(context.Global("ImGui"), "CloseCurrentPopup"), Values{});
							                          }
							                          Call(Member(context.Global("ImGui"), "EndPopup"), Values{});
						                          }
						                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Frozen Rank 1")}))))
						                          {
							                          Call(Member(context.Global("ImGui"), "OpenPopup"), Values{Value("##warnfrozenrank")});
						                          }
						                          if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginPopupModal"), Values{Value("##warnfrozenrank"), Value{}, Value(65)}))))
						                          {
							                          Call(Member(context.Global("ImGui"), "Text"), Values{Value("Are you sure you want to become a certified N00B?")});
							                          Call(Member(context.Global("ImGui"), "Spacing"), Values{});
							                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Yes")}))))
							                          {
								                          Call(context.Global("Rplus_FrozenRank"), Values{});
								                          Call(Member(context.Global("ImGui"), "CloseCurrentPopup"), Values{});
							                          }
							                          Call(Member(context.Global("ImGui"), "SameLine"), Values{});
							                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("No")}))))
							                          {
								                          Call(Member(context.Global("ImGui"), "CloseCurrentPopup"), Values{});
							                          }
							                          Call(Member(context.Global("ImGui"), "EndPopup"), Values{});
						                          }
						                          Call(Member(context.Global("ImGui"), "Spacing"), Values{});
						                          Call(Member(context.Global("ImGui"), "Text"), Values{Value("Crew Rank")});
						                          Call(Member(context.Global("ImGui"), "SameLine"), Values{});
						                          Call(Member(context.Global("ImGui"), "TextDisabled"), Values{Concat(Value("|  Current Active Crew Rank: "), First(Call(Member(context.Global("stats"), "get_int"), Values{Value("MPPLY_CURRENT_CREW_RANK")})))});
						                          Call(Member(context.Global("ImGui"), "Separator"), Values{});
						                          Call(Member(context.Global("ImGui"), "Spacing"), Values{});
						                          {
							                          const auto start_428 = Number(Value(0));
							                          const auto end_429 = Number(Value(4));
							                          const auto step_430 = Number(Value(1));
							                          for (double index_431 = start_428; step_430 > 0 ? index_431 <= end_429 : index_431 >= end_429; index_431 += step_430)
							                          {
								                          auto i_432 = MakeCell(Value(index_431));
								                          if (Truthy(NotEqual(First(Call(Member(context.Global("stats"), "get_int"), Values{Concat(Value("MPPLY_CREW_"), Concat(*i_432, Value("_ID")))})), Value(0))))
								                          {
									                          Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Concat(Value("crewrank"), *i_432)})), "draw", Values{});
									                          Call(Member(context.Global("ImGui"), "SameLine"), Values{});
									                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Concat(Value("Set Rank##crew"), *i_432)}))))
									                          {
										                          Call(Member(context.Global("stats"), "set_int"), Append(Values{Concat(Value("MPPLY_CREW_LOCAL_XP_"), *i_432)}, Call(Member(context.Global("Rplus_Ranks"), "QuadraPoly"), Append(Values{}, Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Concat(Value("crewrank"), *i_432)})), "get_value", Values{})))));
										                          Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Crew Rank"), Concat(Value("New Crew "), Concat(*i_432, Concat(Value(" Rank: "), First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Concat(Value("crewrank"), *i_432)})), "get_value", Values{}))))), Value(3000)});
									                          }
									                          if (Truthy(Equal(First(Call(Member(context.Global("stats"), "get_int"), Values{Concat(Value("MPPLY_CREW_"), Concat(*i_432, Value("_ID")))})), First(Call(Member(context.Global("stats"), "get_int"), Values{Value("MPX_CHAR_PREFERRED_CREW")})))))
									                          {
										                          Call(Member(context.Global("ImGui"), "SameLine"), Values{});
										                          Call(Member(context.Global("ImGui"), "TextDisabled"), Values{Value("Active Crew")});
									                          }
								                          }
							                          }
						                          }
						                          Call(Member(context.Global("ImGui"), "EndChild"), Values{});
					                          }
					                          Call(Member(context.Global("ImGui"), "Spacing"), Values{});
					                          Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(3.8)});
					                          if (Truthy(First(Call(Member(context.Global("ImGui"), "TreeNodeEx"), Values{Concat(Value("K/D Editor  ##"), *nomoretreeplease), Value(8219)}))))
					                          {
						                          Member(Member(context.Global("Rplus_Config"), "Child"), "Charstatx") = Value(400);
						                          Member(Member(context.Global("Rplus_Config"), "Child"), "Charstaty") = Value(440);
						                          Call(Member(context.Global("ImGui"), "BeginChild"), Values{Value("##editkdchild"), Value(370), Member(Member(context.Global("Rplus_Config"), "KD_editor"), "ChildRes"), Value(1)});
						                          Member(Member(context.Global("Rplus_Config"), "KD_editor"), "ChildRes") = Value(264);
						                          Call(Member(context.Global("ImGui"), "AlignTextToFramePadding"), Values{});
						                          Call(Member(context.Global("ImGui"), "Text"), Append(Values{}, Call(Member(context.Global("string"), "format"), Values{Value("K/D Ratio: %.2f"), Divide(First(Call(Member(context.Global("stats"), "get_int"), Values{Value("MPPLY_KILLS_PLAYERS")})), First(Call(Member(context.Global("stats"), "get_int"), Values{Value("MPPLY_DEATHS_PLAYER")})))})));
						                          if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
						                          {
							                          Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Not always accurate to ratio displayed in-game.")});
						                          }
						                          Call(Member(context.Global("ImGui"), "SameLine"), Values{});
						                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Switch Mode")}))))
						                          {
							                          Member(Member(context.Global("Rplus_Config"), "KD_editor"), "Slider") = Value(!Truthy(Member(Member(context.Global("Rplus_Config"), "KD_editor"), "Slider")));
						                          }
						                          Call(Member(context.Global("ImGui"), "Separator"), Values{});
						                          Call(Member(context.Global("ImGui"), "PushItemWidth"), Values{Value(180)});
						                          if (Truthy(Member(Member(context.Global("Rplus_Config"), "KD_editor"), "Slider")))
						                          {
							                          auto values_433 = Append(Values{}, Call(Member(context.Global("ImGui"), "SliderInt"), Values{Value("Kills"), First(Call(Member(context.Global("stats"), "get_int"), Values{Value("MPPLY_KILLS_PLAYERS")})), Negate(Value(1000000000)), Value(1000000000)}));
							                          Member(context.Global("Rplus_Config"), "PlayerKills") = At(values_433, 0);
							                          context.Global("Killschanged") = At(values_433, 1);
							                          auto values_434 = Append(Values{}, Call(Member(context.Global("ImGui"), "SliderInt"), Values{Value("Deaths"), First(Call(Member(context.Global("stats"), "get_int"), Values{Value("MPPLY_DEATHS_PLAYER")})), Negate(Value(1000000000)), Value(1000000000)}));
							                          Member(context.Global("Rplus_Config"), "PlayerDeaths") = At(values_434, 0);
							                          context.Global("Deathschanged") = At(values_434, 1);
						                          }
						                          else
						                          {
							                          auto values_435 = Append(Values{}, Call(Member(context.Global("ImGui"), "InputInt"), Append(Values{Value("Kills")}, Call(Member(context.Global("stats"), "get_int"), Values{Value("MPPLY_KILLS_PLAYERS")}))));
							                          Member(context.Global("Rplus_Config"), "PlayerKills") = At(values_435, 0);
							                          context.Global("Killschanged") = At(values_435, 1);
							                          auto values_436 = Append(Values{}, Call(Member(context.Global("ImGui"), "InputInt"), Append(Values{Value("Deaths")}, Call(Member(context.Global("stats"), "get_int"), Values{Value("MPPLY_DEATHS_PLAYER")}))));
							                          Member(context.Global("Rplus_Config"), "PlayerDeaths") = At(values_436, 0);
							                          context.Global("Deathschanged") = At(values_436, 1);
						                          }
						                          if (Truthy(context.Global("Killschanged")))
						                          {
							                          Call(Member(context.Global("stats"), "set_int"), Values{Value("MPPLY_KILLS_PLAYERS"), Member(context.Global("Rplus_Config"), "PlayerKills")});
						                          }
						                          if (Truthy(context.Global("Deathschanged")))
						                          {
							                          Call(Member(context.Global("stats"), "set_int"), Values{Value("MPPLY_DEATHS_PLAYER"), Member(context.Global("Rplus_Config"), "PlayerDeaths")});
						                          }
						                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Concat(Value("Frozen "), Concat(Member(Member(context.Global("Rplus_Config"), "KD_editor"), "FrezenKD"), Value(" K/D")))}))))
						                          {
							                          Call(Member(context.Global("stats"), "set_int"), Values{Value("MPPLY_KILLS_PLAYERS"), Multiply(Member(Member(context.Global("Rplus_Config"), "KD_editor"), "FrezenKD"), Value(10000000))});
							                          Call(Member(context.Global("stats"), "set_int"), Values{Value("MPPLY_DEATHS_PLAYER"), Value(10000000)});
						                          }
						                          Call(Member(context.Global("ImGui"), "SameLine"), Values{});
						                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("-"), Value(25)}))))
						                          {
							                          Member(Member(context.Global("Rplus_Config"), "KD_editor"), "FrezenKD") = Subtract(Member(Member(context.Global("Rplus_Config"), "KD_editor"), "FrezenKD"), Value(1));
						                          }
						                          Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(4)});
						                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("+"), Value(25)}))))
						                          {
							                          Member(Member(context.Global("Rplus_Config"), "KD_editor"), "FrezenKD") = Add(Member(Member(context.Global("Rplus_Config"), "KD_editor"), "FrezenKD"), Value(1));
						                          }
						                          Call(Member(context.Global("ImGui"), "SameLine"), Values{});
						                          auto values_437 = Append(Values{}, Call(Member(context.Global("ImGui"), "Checkbox"), Values{Value("Negative"), Member(Member(context.Global("Rplus_Config"), "KD_editor"), "Negative")}));
						                          Member(Member(context.Global("Rplus_Config"), "KD_editor"), "Negative") = At(values_437, 0);
						                          context.Global("Negativekdchanged") = At(values_437, 1);
						                          if (Truthy(context.Global("Negativekdchanged")))
						                          {
							                          Member(Member(context.Global("Rplus_Config"), "KD_editor"), "FrezenKD") = Negate(Member(Member(context.Global("Rplus_Config"), "KD_editor"), "FrezenKD"));
						                          }
						                          if (Truthy(LessEqual(Member(Member(context.Global("Rplus_Config"), "KD_editor"), "FrezenKD"), Value(0))))
						                          {
							                          Member(Member(context.Global("Rplus_Config"), "KD_editor"), "FrezenKD") = Value(1);
						                          }
						                          else if (Truthy(GreaterEqual(Member(Member(context.Global("Rplus_Config"), "KD_editor"), "FrezenKD"), Value(10))))
						                          {
							                          Member(Member(context.Global("Rplus_Config"), "KD_editor"), "FrezenKD") = Value(9);
						                          }
						                          Call(Member(context.Global("ImGui"), "Spacing"), Values{});
						                          Call(Member(context.Global("ImGui"), "AlignTextToFramePadding"), Values{});
						                          Call(Member(context.Global("ImGui"), "Text"), Values{Value("Archenemy & Victim")});
						                          Call(Member(context.Global("ImGui"), "SameLine"), Values{});
						                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Switch Mode##enemyvictim")}))))
						                          {
							                          Member(Member(context.Global("Rplus_Config"), "KD_editor"), "ArchOrVictimNode") = Value(!Truthy(Member(Member(context.Global("Rplus_Config"), "KD_editor"), "ArchOrVictimNode")));
						                          }
						                          Call(Member(context.Global("ImGui"), "Separator"), Values{});
						                          Call(Member(context.Global("ImGui"), "Spacing"), Values{});
						                          if (Truthy(Value(!Truthy(Member(Member(context.Global("Rplus_Config"), "KD_editor"), "ArchOrVictimNode")))))
						                          {
							                          Member(Member(context.Global("Rplus_Config"), "KD_editor"), "ArchOrVictim") = First(Call(Member(context.Global("ImGui"), "InputText"), Append(Values{Value("Archenemy")}, Call(Member(context.Global("stats"), "get_string"), Values{Value("MPX_ARCHENEMY_NAME")}))));
							                          Member(Member(context.Global("Rplus_Config"), "KD_editor"), "KilledKills") = First(Call(Member(context.Global("ImGui"), "InputText"), Append(Values{Value("Killed By")}, Call(context.Global("tostring"), Append(Values{}, Call(Member(context.Global("stats"), "get_int"), Values{Value("MPX_ARCHENEMY_KILLS")}))))));
						                          }
						                          else
						                          {
							                          Member(Member(context.Global("Rplus_Config"), "KD_editor"), "ArchOrVictim") = First(Call(Member(context.Global("ImGui"), "InputText"), Append(Values{Value("Victim")}, Call(Member(context.Global("stats"), "get_string"), Values{Value("MPX_BIGGEST_VICTIM_NAME")}))));
							                          Member(Member(context.Global("Rplus_Config"), "KD_editor"), "KilledKills") = First(Call(Member(context.Global("ImGui"), "InputText"), Append(Values{Value("Kills##victim")}, Call(context.Global("tostring"), Append(Values{}, Call(Member(context.Global("stats"), "get_int"), Values{Value("MPX_BIGGEST_VICTIM_KILLS")}))))));
						                          }
						                          Call(Member(context.Global("ImGui"), "Spacing"), Values{});
						                          Call(Member(context.Global("ImGui"), "Spacing"), Values{});
						                          Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(3.8)});
						                          if (Truthy(First(Call(Member(context.Global("ImGui"), "TreeNodeEx"), Values{Concat(Value("Text Formatting  ##kdeditorformatting"), *nomoretreeplease), Value(8219)}))))
						                          {
							                          Member(Member(context.Global("Rplus_Config"), "KD_editor"), "ChildRes") = Add(Member(Member(context.Global("Rplus_Config"), "KD_editor"), "ChildRes"), Value(148));
							                          Call(Member(context.Global("ImGui"), "BeginGroup"), Values{});
							                          {
								                          Iterator iterator_438(Append(Values{}, Call(context.Global("ipairs"), Values{*ReN_Style})));
								                          for (auto row_439 = iterator_438.Next(); !row_439.empty() && !row_439[0].IsNil(); row_439 = iterator_438.Next())
								                          {
									                          auto i_440 = MakeCell(At(row_439, 0));
									                          auto textformat_441 = MakeCell(At(row_439, 1));
									                          if (Truthy(Equal(Modulo(*i_440, Value(2)), Value(0))))
									                          {
										                          Call(Member(context.Global("ImGui"), "SameLine"), Values{});
									                          }
									                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Member(*textformat_441, "tbLabel"), Value(100)}))))
									                          {
										                          Member(Member(context.Global("Rplus_Config"), "KD_editor"), "ArchOrVictim") = Concat(Member(Member(context.Global("Rplus_Config"), "KD_editor"), "ArchOrVictim"), Member(*textformat_441, "tStyle"));
									                          }
									                          if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
									                          {
										                          Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Member(*textformat_441, "tStyle")});
									                          }
								                          }
							                          }
							                          Call(Member(context.Global("ImGui"), "EndGroup"), Values{});
							                          Call(Member(context.Global("ImGui"), "SameLine"), Values{});
							                          Call(Member(context.Global("ImGui"), "BeginGroup"), Values{});
							                          Call(Member(context.Global("ImGui"), "NewLine"), Values{});
							                          Call(Member(context.Global("ImGui"), "Spacing"), Values{});
							                          Call(Member(context.Global("ImGui"), "Spacing"), Values{});
							                          Call(Member(context.Global("ImGui"), "Spacing"), Values{});
							                          Call(Member(context.Global("ImGui"), "Text"), Values{Value("Colors")});
							                          {
								                          Iterator iterator_442(Append(Values{}, Call(context.Global("ipairs"), Values{*ReN_Color})));
								                          for (auto row_443 = iterator_442.Next(); !row_443.empty() && !row_443[0].IsNil(); row_443 = iterator_442.Next())
								                          {
									                          auto i_444 = MakeCell(At(row_443, 0));
									                          auto textcolor_445 = MakeCell(At(row_443, 1));
									                          if (Truthy(NotEqual(Modulo(Subtract(*i_444, Value(1)), Value(4)), Value(0))))
									                          {
										                          Call(Member(context.Global("ImGui"), "SameLine"), Values{});
									                          }
									                          if (Truthy(First(Call(Member(context.Global("ImGui"), "ColorButton"), Values{Member(*textcolor_445, "bName"), Member(*textcolor_445, "bCol"), Value(64)}))))
									                          {
										                          Member(Member(context.Global("Rplus_Config"), "KD_editor"), "ArchOrVictim") = Concat(Member(Member(context.Global("Rplus_Config"), "KD_editor"), "ArchOrVictim"), Member(*textcolor_445, "tColf"));
									                          }
									                          if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
									                          {
										                          Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Concat(Member(*textcolor_445, "bTip"), Concat(Value(": "), Member(*textcolor_445, "tColf")))});
									                          }
								                          }
							                          }
							                          Call(Member(context.Global("ImGui"), "EndGroup"), Values{});
							                          Call(Member(context.Global("ImGui"), "Spacing"), Values{});
						                          }
						                          if (Truthy(Value(!Truthy(Member(Member(context.Global("Rplus_Config"), "KD_editor"), "ArchOrVictimNode")))))
						                          {
							                          Call(Member(context.Global("stats"), "set_string"), Values{Value("MPX_ARCHENEMY_NAME"), Member(Member(context.Global("Rplus_Config"), "KD_editor"), "ArchOrVictim")});
						                          }
						                          else
						                          {
							                          Call(Member(context.Global("stats"), "set_string"), Values{Value("MPX_BIGGEST_VICTIM_NAME"), Member(Member(context.Global("Rplus_Config"), "KD_editor"), "ArchOrVictim")});
						                          }
						                          Call(Member(context.Global("ImGui"), "PopItemWidth"), Values{});
						                          Call(Member(context.Global("ImGui"), "EndChild"), Values{});
					                          }
					                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Max Out All Skills")}))))
					                          {
						                          Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_SCRIPT_INCREASE_STAM"), Value(100)});
						                          Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_SCRIPT_INCREASE_STRN"), Value(100)});
						                          Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_SCRIPT_INCREASE_DRIV"), Value(100)});
						                          Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_SCRIPT_INCREASE_FLY"), Value(100)});
						                          Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_SCRIPT_INCREASE_STL"), Value(100)});
						                          Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_SCRIPT_INCREASE_LUNG"), Value(100)});
						                          Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_SCRIPT_INCREASE_SHO"), Value(100)});
						                          Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_CHAR_FM_HEALTH_1_UNLCK"), Negate(Value(1))});
						                          Call(Member(context.Global("stats"), "set_int"), Values{Value("MPX_CHAR_FM_HEALTH_2_UNLCK"), Negate(Value(1))});
						                          Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Character Skills"), Value("All Character Skill have been maxed out!"), Value(3000)});
					                          }
					                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Fast Run")}))))
					                          {
						                          Call(context.Global("Rplus_FastRun"), Values{});
					                          }
					                          return {};
				                          })});
				Call(*CustomChild, Values{Value("Awards"), Value("##awardschild"), Member(Member(context.Global("Rplus_Config"), "Child"), "Awardsx"), Member(Member(context.Global("Rplus_Config"), "Child"), "Awardsy"), Value(0), MakeFunction([=, &context](const Values& args) -> Values {
					                          Member(Member(context.Global("Rplus_Config"), "Child"), "Awardsx") = Value(360);
					                          Member(Member(context.Global("Rplus_Config"), "Child"), "Awardsy") = Value(100);
					                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Unlock All Awards")}))))
					                          {
						                          Call(Member(context.Global("ImGui"), "OpenPopup"), Values{Value("##warnallawards")});
					                          }
					                          if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
					                          {
						                          Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Unlocks All Awards.")});
					                          }
					                          if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginPopupModal"), Values{Value("##warnallawards"), Value{}, Value(65)}))))
					                          {
						                          Call(Member(context.Global("ImGui"), "Text"), Values{Value("Are you sure you want to unlock All Awards?")});
						                          Call(Member(context.Global("ImGui"), "Spacing"), Values{});
						                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Yes")}))))
						                          {
							                          {
								                          Iterator iterator_446(Append(Values{}, Call(context.Global("ipairs"), Values{context.Global("Rplus_Awards")})));
								                          for (auto row_447 = iterator_446.Next(); !row_447.empty() && !row_447[0].IsNil(); row_447 = iterator_446.Next())
								                          {
									                          auto __448 = MakeCell(At(row_447, 0));
									                          auto allAwards = MakeCell(At(row_447, 1));
									                          Call(Member(*allAwards, "Unlock"), Values{});
								                          }
							                          }
							                          Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - All Awards Only"), Value("All Awards Unlocked!"), Value(3000)});
							                          Call(Member(context.Global("ImGui"), "CloseCurrentPopup"), Values{});
						                          }
						                          Call(Member(context.Global("ImGui"), "SameLine"), Values{});
						                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("No")}))))
						                          {
							                          Call(Member(context.Global("ImGui"), "CloseCurrentPopup"), Values{});
						                          }
						                          Call(Member(context.Global("ImGui"), "EndPopup"), Values{});
					                          }
					                          Call(Member(context.Global("ImGui"), "SameLine"), Values{});
					                          Call(Member(context.Global("ImGui"), "Spacing"), Values{});
					                          Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(3.8)});
					                          if (Truthy(First(Call(Member(context.Global("ImGui"), "TreeNodeEx"), Values{Concat(Value("Categories  ##"), *nomoretreeplease), Value(8219)}))))
					                          {
						                          Member(Member(context.Global("Rplus_Config"), "Child"), "Awardsx") = Value(768);
						                          Member(Member(context.Global("Rplus_Config"), "Child"), "Awardsy") = Value(244);
						                          {
							                          Iterator iterator_449(Append(Values{}, Call(context.Global("ipairs"), Values{context.Global("Rplus_Awards")})));
							                          for (auto row_450 = iterator_449.Next(); !row_450.empty() && !row_450[0].IsNil(); row_450 = iterator_449.Next())
							                          {
								                          auto i_451 = MakeCell(At(row_450, 0));
								                          auto Awards = MakeCell(At(row_450, 1));
								                          if (Truthy(NotEqual(Modulo(Subtract(*i_451, Value(1)), Value(4)), Value(0))))
								                          {
									                          Call(Member(context.Global("ImGui"), "SameLine"), Values{});
								                          }
								                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Member(*Awards, "Category")}))))
								                          {
									                          Call(Member(*Awards, "Unlock"), Values{});
									                          Call(Member(context.Global("notify"), "success"), Append(Values{Value("TinkerScripts - All Awards Only")}, Call(Member(context.Global("string"), "format"), Values{Value("All \"%s\" Awards Unlocked!"), Member(*Awards, "Category")})));
								                          }
							                          }
						                          }
					                          }
					                          return {};
				                          })});
				Call(*CustomChild, Values{Value("Achievements"), Value("##achievementschild"), Value(360), Value(100), Value(0), MakeFunction([=, &context](const Values& args) -> Values {
					                          Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("achievementsonly")})), "draw", Values{});
					                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Unlock Achievements")}))))
					                          {
						                          if (Truthy(Value(!Truthy(Member(context.Global("Rplus_Config"), "AllAchievements")))))
						                          {
							                          if (Truthy(LessEqual(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("achievementsonly")})), "get_value", Values{})), Value(0))))
							                          {
								                          Call(Member(context.Global("notify"), "error"), Values{Value("TinkerScripts - All Achievements Only"), Value("Select something first!"), Value(3000)});
							                          }
							                          else
							                          {
								                          if (Truthy(Value(!Truthy(First(Call(Member(context.Global("PLAYER"), "HAS_ACHIEVEMENT_BEEN_PASSED"), Append(Values{}, Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("achievementsonly")})), "get_value", Values{}))))))))
								                          {
									                          Call(Member(context.Global("PLAYER"), "GIVE_ACHIEVEMENT_TO_PLAYER"), Append(Values{}, Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("achievementsonly")})), "get_value", Values{})));
									                          Call(Member(context.Global("notify"), "success"), Append(Values{Value("TinkerScripts - All Achievements Only")}, Call(Member(context.Global("string"), "format"), Values{Value("%s Unlocked!"), Member(Member(context.Global("Rplus_Achievements"), First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("achievementsonly")})), "get_value", Values{}))), Value(2))})));
								                          }
								                          else
								                          {
									                          Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - All Achievements Only"), Value("Achievement already unlocked."), Value(3000)});
								                          }
							                          }
						                          }
						                          else
						                          {
							                          Call(Member(context.Global("ImGui"), "OpenPopup"), Values{Value("##warnallachievements")});
						                          }
					                          }
					                          if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginPopupModal"), Values{Value("##warnallachievements"), Value{}, Value(65)}))))
					                          {
						                          Call(Member(context.Global("ImGui"), "Text"), Values{Value("Are you sure you want to unlock All Achievements?")});
						                          Call(Member(context.Global("ImGui"), "Spacing"), Values{});
						                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Yes")}))))
						                          {
							                          auto trackachunlock = MakeCell(Value(0));
							                          {
								                          Iterator iterator_452(Append(Values{}, Call(context.Global("ipairs"), Values{context.Global("Rplus_Achievements")})));
								                          for (auto row_453 = iterator_452.Next(); !row_453.empty() && !row_453[0].IsNil(); row_453 = iterator_452.Next())
								                          {
									                          auto i_454 = MakeCell(At(row_453, 0));
									                          auto achievement = MakeCell(At(row_453, 1));
									                          if (Truthy(Value(!Truthy(First(Call(Member(context.Global("PLAYER"), "HAS_ACHIEVEMENT_BEEN_PASSED"), Values{*i_454}))))))
									                          {
										                          Call(Member(context.Global("PLAYER"), "GIVE_ACHIEVEMENT_TO_PLAYER"), Values{*i_454});
										                          Call(Member(context.Global("log"), "info"), Append(Values{}, Call(Member(context.Global("string"), "format"), Values{Value("%s%s\033[1;36m%s\033[m%s"), *cls, *lual, Member(*achievement, Value(2)), *cls})));
										                          *trackachunlock = Add(*trackachunlock, Value(1));
									                          }
								                          }
							                          }
							                          if (Truthy(Equal(*trackachunlock, Value(0))))
							                          {
								                          Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - All Achievements Only"), Value("No Achievements left to unlock!")});
							                          }
							                          else
							                          {
								                          Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - All Achievements Only"), Value("All Achievements Unlocked!")});
								                          Call(Member(context.Global("log"), "info"), Append(Values{}, Call(Member(context.Global("string"), "format"), Values{Value("%s%sTinkerScripts - All Achievements Only: \033[32mAll Achievements Unlocked!\033[m%s\n"), *cls, *lual, *cls})));
							                          }
							                          Call(Member(context.Global("ImGui"), "CloseCurrentPopup"), Values{});
						                          }
						                          Call(Member(context.Global("ImGui"), "SameLine"), Values{});
						                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("No")}))))
						                          {
							                          Call(Member(context.Global("ImGui"), "CloseCurrentPopup"), Values{});
						                          }
						                          Call(Member(context.Global("ImGui"), "EndPopup"), Values{});
					                          }
					                          Call(Member(context.Global("ImGui"), "SameLine"), Values{});
					                          Member(context.Global("Rplus_Config"), "AllAchievements") = First(Call(Member(context.Global("ImGui"), "Checkbox"), Values{Value("All Achievements"), Member(context.Global("Rplus_Config"), "AllAchievements")}));
					                          return {};
				                          })});
				Call(*CustomChild, Values{Value("Rockstar Dev Shirts"), Value("##devshirtschild"), Value(360), Value(100), Value(0), MakeFunction([=, &context](const Values& args) -> Values {
					                          if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Unlock All R* Dev Shirts")}))))
					                          {
						                          Call(context.Global("Rplus_DevShirt"), Values{});
					                          }
					                          return {};
				                          })});
				Call(Member(context.Global("ImGui"), "EndTabItem"), Values{});
			}
			if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginTabItem"), Values{Value("Tunables")}))))
			{
				Call(Member(context.Global("ImGui"), "Text"), Values{Value("Content Tunables")});
				Call(Member(context.Global("ImGui"), "Separator"), Values{});
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("Tunaggle_Independence")})), "draw", Values{});
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("Tunaggle_Halloween")})), "draw", Values{});
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("Tunaggle_Xmas")})), "draw", Values{});
				if (Truthy(First(Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("Tunaggle_Xmas")})), "get_value", Values{}))))
				{
					Call(Member(context.Global("ImGui"), "SameLine"), Values{});
					*xmas_snow = First(Call(Member(context.Global("ImGui"), "Checkbox"), Values{Value("Enable Snow"), *xmas_snow}));
					if (Truthy(*xmas_snow))
					{
						Call(Member(context.Global("tunables"), "set_bool"), Values{Negate(Value(1146554960)), Value(true)});
						Call(Member(context.Global("tunables"), "set_bool"), Values{Negate(Value(1195222077)), Value(false)});
					}
					else
					{
						Call(Member(context.Global("tunables"), "set_bool"), Values{Negate(Value(1146554960)), Value(false)});
						Call(Member(context.Global("tunables"), "set_bool"), Values{Negate(Value(1195222077)), Value(true)});
					}
				}
				Call(Member(context.Global("ImGui"), "EndTabItem"), Values{});
			}
			if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginTabItem"), Values{Value("Money")}))))
			{
				Call(Member(context.Global("ImGui"), "Text"), Values{Value("Money")});
				Call(Member(context.Global("ImGui"), "Separator"), Values{});
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Good Behavior Bonus")}))))
				{
					Invoke(First(Call(context.Global("ScriptGlobal"), Values{Value(2697091)})), "set_int", Values{Value(2000)});
					Invoke(First(Call(context.Global("ScriptGlobal"), Values{Value(2697090)})), "set_int", Values{Value(1)});
				}
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Casino Membership Bonus")}))))
				{
					Invoke(First(Call(context.Global("ScriptGlobal"), Values{Value(1973325)})), "set_int", Values{Value(1)});
				}
				if (Truthy(Value(!Truthy(Member(*SindyBar, "Loop")))))
				{
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Sindy Looper")}))))
					{
						if (Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("ob_jukebox")}))))
						{
							Member(*SindyBar, "Loop") = Value(true);
							Call(Member(context.Global("script"), "run_in_callback"), Values{MakeFunction([=, &context](const Values& args) -> Values {
								while (Truthy(Value(true)))
								{
									Call(Member(context.Global("script"), "yield"), Values{Member(*SindyBar, "Delay")});
									if (Truthy(Member(*SindyBar, "Loop")))
									{
										Invoke(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Member(*PropGlob, "mcbar")})), "at", Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_ID"), Values{})), Member(*PropGlob, "ArraySize")})), "set_int", Values{Member(*SindyBar, "Money")});
										Call(Member(context.Global("script"), "yield"), Values{Value(200)});
										Call(*LocalSetBit, Values{First(Call(context.Global("ScriptLocal"), Values{Value("am_mp_property_int"), Value(230)})), Value(1), Value(18)});
									}
									else
									{
										Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Sindy Looper"), Value("Loop Canceled."), Value(3000)});
										return Values{};
									}
								}
								return {};
							})});
						}
						else
						{
							Call(Member(context.Global("notify"), "error"), Values{Value("TinkerScripts - Sindy Looper"), Value("You must be inside your Clubhouse to Collect Bar Earnings!"), Value(3000)});
						}
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
					{
						Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Not to be confused with Cyndi Lauper.")});
					}
				}
				else
				{
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Cancel")}))))
					{
						Member(*SindyBar, "Loop") = Value(false);
					}
				}
				Call(Member(context.Global("ImGui"), "SameLine"), Values{});
				Member(*SindyBar, "ManualBarMoney") = First(Call(Member(context.Global("ImGui"), "Checkbox"), Values{Value("Manual Options"), Member(*SindyBar, "ManualBarMoney")}));
				if (Truthy(Member(*SindyBar, "ManualBarMoney")))
				{
					if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginChild"), Values{Value("##sindylooperchild"), Value(282), Value(132), Value(1)}))))
					{
						Call(Member(context.Global("ImGui"), "SetNextItemWidth"), Values{Value(128)});
						Member(*SindyBar, "Money") = First(Call(Member(context.Global("ImGui"), "InputInt"), Values{Value("Earnings"), Member(*SindyBar, "Money")}));
						Call(Member(context.Global("ImGui"), "SameLine"), Values{});
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Set"), Value(41)}))))
						{
							Invoke(First(Invoke(First(Call(context.Global("ScriptGlobal"), Values{Member(*PropGlob, "mcbar")})), "at", Values{First(Call(Member(context.Global("PLAYER"), "PLAYER_ID"), Values{})), Member(*PropGlob, "ArraySize")})), "set_int", Values{Member(*SindyBar, "Money")});
						}
						Call(Member(context.Global("ImGui"), "SetNextItemWidth"), Values{Value(108)});
						auto values_455 = Append(Values{}, Call(Member(context.Global("ImGui"), "InputInt"), Values{Value("Loop Delay"), Member(*SindyBar, "Delay")}));
						Member(*SindyBar, "Delay") = At(values_455, 0);
						context.Global("MCBD_changed") = At(values_455, 1);
						Call(Member(context.Global("ImGui"), "Spacing"), Values{});
						if (Truthy(Greater(Member(*SindyBar, "Money"), Value(100000))))
						{
							Member(*SindyBar, "Money") = Value(100000);
						}
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Teleport to Clubhouse")}))))
						{
							Call(*TpToBlip, Values{Value(false), Member(Member(Member(context.Global("Blips"), "Property"), "BIKER_CLUBHOUSE"), "ID"), Value(false)});
						}
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Collect Bar Earnings")}))))
						{
							if (Truthy(First(Call(Member(context.Global("scripts"), "is_active"), Values{Value("ob_jukebox")}))))
							{
								Call(*LocalSetBit, Values{First(Call(context.Global("ScriptLocal"), Values{Value("am_mp_property_int"), Value(230)})), Value(1), Value(18)});
							}
							else
							{
								Call(Member(context.Global("notify"), "error"), Values{Value("TinkerScripts - Sindy Looper"), Value("You must be inside your Clubhouse to Collect Bar Earnings!"), Value(3000)});
							}
						}
					}
					Call(Member(context.Global("ImGui"), "EndChild"), Values{});
				}
				Call(Member(context.Global("ImGui"), "EndTabItem"), Values{});
			}
			if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginTabItem"), Values{Value("Human Canvas")}))))
			{
				Call(Member(context.Global("ImGui"), "Text"), Values{Value("Tattoos")});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
				{
					Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Allows you to stack tattoos.\nFind a new session to see changes")});
				}
				Call(Member(context.Global("ImGui"), "Separator"), Values{});
				Call(Member(context.Global("ImGui"), "BeginGroup"), Values{});
				if (Truthy(Equal(First(Call(context.Global("next"), Values{context.Global("TattooZone")})), Value{})))
				{
					{
						Iterator iterator_456(Append(Values{}, Call(context.Global("ipairs"), Append(Values{}, Call(context.Global("HumanCanvas"), Values{})))));
						for (auto row_457 = iterator_456.Next(); !row_457.empty() && !row_457[0].IsNil(); row_457 = iterator_456.Next())
						{
							auto __458 = MakeCell(At(row_457, 0));
							auto TatZone = MakeCell(At(row_457, 1));
							Member(context.Global("TattooZone"), Member(*TatZone, "BPart")) = MakeTable({{"selectedTatName", Value("Select...")}, {"searchTat", Value("")}});
						}
					}
				}
				{
					Iterator iterator_459(Append(Values{}, Call(context.Global("ipairs"), Append(Values{}, Call(context.Global("HumanCanvas"), Values{})))));
					for (auto row_460 = iterator_459.Next(); !row_460.empty() && !row_460[0].IsNil(); row_460 = iterator_459.Next())
					{
						auto __461 = MakeCell(At(row_460, 0));
						auto TatZone_462 = MakeCell(At(row_460, 1));
						auto ZoneName = MakeCell(Member(*TatZone_462, "BPart"));
						auto Tattoo = MakeCell(Member(context.Global("TattooZone"), *ZoneName));
						auto TatPopup = MakeCell(Concat(Value("##tattoo_popup"), *ZoneName));
						Call(Member(context.Global("ImGui"), "BeginGroup"), Values{});
						Call(Member(context.Global("ImGui"), "Dummy"), Values{Value(75), Value(0)});
						Call(Member(context.Global("ImGui"), "AlignTextToFramePadding"), Values{});
						Call(Member(context.Global("ImGui"), "Text"), Values{Concat(*ZoneName, Value(":"))});
						Call(Member(context.Global("ImGui"), "EndGroup"), Values{});
						Call(Member(context.Global("ImGui"), "SameLine"), Values{});
						Call(Member(context.Global("ImGui"), "BeginGroup"), Values{});
						Call(Member(context.Global("ImGui"), "Dummy"), Values{Value(0), Value(0)});
						Call(Member(context.Global("ImGui"), "SetNextItemWidth"), Values{Value(300)});
						Call(Member(context.Global("ImGui"), "BeginCombo"), Values{Concat(Value("##tattoos"), *ZoneName), Member(*Tattoo, "selectedTatName")});
						if (Truthy(And(First(Call(Member(context.Global("ImGui"), "IsItemActive"), Values{})), [&] {
							    return Value(!Truthy(First(Call(Member(context.Global("ImGui"), "IsPopupOpen"), Values{*TatPopup}))));
						    })))
						{
							Call(Member(context.Global("ImGui"), "OpenPopup"), Values{*TatPopup});
							Member(*Tattoo, "searchTat") = Value("");
						}
						if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginPopup"), Values{*TatPopup, Add(Member(context.Global("ImGuiWindowFlags"), "NoResize"), Member(context.Global("ImGuiWindowFlags"), "NoMove"))}))))
						{
							Call(Member(context.Global("ImGui"), "Text"), Values{Value("Search:")});
							Call(Member(context.Global("ImGui"), "SameLine"), Values{});
							Call(Member(context.Global("ImGui"), "SetNextItemWidth"), Values{Value(250)});
							Member(*Tattoo, "searchTat") = First(Call(Member(context.Global("ImGui"), "InputText"), Values{Concat(Value("##searchtattoo"), *ZoneName), Member(*Tattoo, "searchTat")}));
							auto searchLower = MakeCell(First(Call(Member(context.Global("string"), "lower"), Values{Member(*Tattoo, "searchTat")})));
							{
								Iterator iterator_463(Append(Values{}, Call(context.Global("ipairs"), Values{Member(*TatZone_462, "Tattoos")})));
								for (auto row_464 = iterator_463.Next(); !row_464.empty() && !row_464[0].IsNil(); row_464 = iterator_463.Next())
								{
									auto i_465 = MakeCell(At(row_464, 0));
									auto tat = MakeCell(At(row_464, 1));
									auto tattooLower = MakeCell(First(Call(Member(context.Global("string"), "lower"), Values{Concat(Member(*tat, "tatName"), Value(" "))})));
									if (Truthy(First(Invoke(*tattooLower, "find", Values{*searchLower}))))
									{
										if (Truthy(First(Call(Member(context.Global("ImGui"), "Selectable"), Values{Member(*tat, "tatName")}))))
										{
											Member(*Tattoo, "selectedTatName") = Member(*tat, "tatName");
											Member(*Tattoo, "selectedTattoo") = Member(*tat, "tatStat");
											Member(*Tattoo, "selectedTatVal") = Member(*tat, "tatVal");
										}
									}
								}
							}
							Call(Member(context.Global("ImGui"), "EndPopup"), Values{});
						}
						Call(Member(context.Global("ImGui"), "SameLine"), Values{});
						Call(Member(context.Global("ImGui"), "BeginDisabled"), Values{Equal(Member(*Tattoo, "selectedTatName"), Value("Select..."))});
						if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Concat(Value("Apply##"), *ZoneName)}))))
						{
							Call(Member(context.Global("stats"), "set_masked_bool"), Values{Member(*Tattoo, "selectedTattoo"), Member(*Tattoo, "selectedTatVal"), Value(true)});
							Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Human Canvas"), Concat(Value("Applied: "), Member(*Tattoo, "selectedTatName"))});
						}
						Call(Member(context.Global("ImGui"), "SameLine"), Values{});
						if (Truthy(NotEqual(Member(*Tattoo, "selectedTattoo"), Value{})))
						{
							if (Truthy(NotEqual(First(Call(Member(context.Global("stats"), "get_masked_bool"), Values{Member(*Tattoo, "selectedTattoo"), Member(*Tattoo, "selectedTatVal")})), Value(false))))
							{
								if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Concat(Value("Remove##"), *ZoneName)}))))
								{
									Call(Member(context.Global("stats"), "set_masked_bool"), Values{Member(*Tattoo, "selectedTattoo"), Member(*Tattoo, "selectedTatVal"), Value(false)});
									Call(Member(context.Global("notify"), "info"), Values{Value("TinkerScripts - Human Canvas"), Concat(Value("Removed: "), Member(*Tattoo, "selectedTatName"))});
									Member(*Tattoo, "selectedTatName") = Value("Select...");
								}
							}
						}
						Call(Member(context.Global("ImGui"), "EndDisabled"), Values{});
						Call(Member(context.Global("ImGui"), "EndGroup"), Values{});
					}
				}
				Call(Member(context.Global("ImGui"), "EndGroup"), Values{});
				Call(Member(context.Global("ImGui"), "BeginGroup"), Values{});
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Import Tattoos"), Value(154)}))))
				{
					if (Truthy(Value(!Truthy(Member(context.Global("Rplus_Config"), "FiletoChar")))))
					{
						{
							Iterator iterator_466(Append(Values{}, Invoke(First(Call(Member(context.Global("ImGui"), "GetClipboardText"), Values{})), "gmatch", Values{Value("[^\r\n]+")})));
							for (auto row_467 = iterator_466.Next(); !row_467.empty() && !row_467[0].IsNil(); row_467 = iterator_466.Next())
							{
								auto stat = MakeCell(At(row_467, 0));
								if (Truthy(First(Invoke(*stat, "match", Values{Value("MPX_TATTOO_FM_CURRENT_(%d+) = (%-?%d+)")}))))
								{
									auto values_468 = Append(Values{}, Invoke(*stat, "match", Values{Value("^(.-) = (.-)$")}));
									auto tatstat = MakeCell(At(values_468, 0));
									auto tatvalue = MakeCell(At(values_468, 1));
									Call(Member(context.Global("stats"), "set_int"), Append(Values{*tatstat}, Call(context.Global("tonumber"), Values{*tatvalue})));
								}
								else
								{
									Call(Member(context.Global("log"), "error"), Append(Values{}, Call(Member(context.Global("string"), "format"), Values{Value("%s%sTinkerScripts - Human Canvas: \033[0;31mWrong Tattoo format!\033[m Make sure you copied the stats correctly."), *cls, *lual})));
									Call(Member(context.Global("notify"), "error"), Values{Value("TinkerScripts - Human Canvas"), Value("Wrong Tattoo format! Make sure you copied the stats correctly."), Value(3000)});
									break;
								}
								Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Human Canvas"), Value("Success! Tattoo Combo applied correctly.\nPlease change sessions to see the changes."), Value(3000)});
								Call(Member(context.Global("STATS"), "STAT_SAVE"), Values{Value(0), Value(0), Value(3), Value(0)});
							}
						}
					}
					else
					{
						{
							Iterator iterator_469(Append(Values{}, Invoke(First(Call(*TS_PATH_FMCharTatCombo, Values{})), "gmatch", Values{Value("[^\r\n]+")})));
							for (auto row_470 = iterator_469.Next(); !row_470.empty() && !row_470[0].IsNil(); row_470 = iterator_469.Next())
							{
								auto stat_471 = MakeCell(At(row_470, 0));
								if (Truthy(First(Invoke(*stat_471, "match", Values{Value("MPX_TATTOO_FM_CURRENT_(%d+) = (%-?%d+)")}))))
								{
									auto values_472 = Append(Values{}, Invoke(*stat_471, "match", Values{Value("^(.-) = (.-)$")}));
									auto tatstat_473 = MakeCell(At(values_472, 0));
									auto tatvalue_474 = MakeCell(At(values_472, 1));
									Call(Member(context.Global("stats"), "set_int"), Append(Values{*tatstat_473}, Call(context.Global("tonumber"), Values{*tatvalue_474})));
								}
								else
								{
									Call(Member(context.Global("log"), "error"), Append(Values{}, Call(Member(context.Global("string"), "format"), Values{Value("%s%sTinkerScripts - Human Canvas: \033[0;31mWrong Tattoo File format!\n\033[mValid format: MPX_TATTOO_FM_CURRENT_X = Value"), *cls, *lual})));
									Call(Member(context.Global("notify"), "error"), Values{Value("TinkerScripts - Human Canvas"), Value("Wrong Tattoo File format! Valid format: MPX_TATTOO_FM_CURRENT_X = Value"), Value(3000)});
									break;
								}
								Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Human Canvas"), Value("Success! Tattoo Combo applied correctly.\nPlease change sessions to see the changes."), Value(3000)});
								Call(Member(context.Global("STATS"), "STAT_SAVE"), Values{Value(0), Value(0), Value(3), Value(0)});
							}
						}
					}
				}
				Call(Member(context.Global("ImGui"), "SameLine"), Values{});
				Member(context.Global("Rplus_Config"), "FiletoChar") = First(Call(Member(context.Global("ImGui"), "Checkbox"), Values{Value("Load from file"), Member(context.Global("Rplus_Config"), "FiletoChar")}));
				if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
				{
					Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Toggle this if you want to load tattoo stats from a file.")});
				}
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Export Tattoos"), Value(154)}))))
				{
					auto ExpTatCombo = MakeCell(MakeTable({}));
					{
						const auto start_475 = Number(Value(0));
						const auto end_476 = Number(Value(53));
						const auto step_477 = Number(Value(1));
						for (double index_478 = start_475; step_477 > 0 ? index_478 <= end_476 : index_478 >= end_476; index_478 += step_477)
						{
							auto i_479 = MakeCell(Value(index_478));
							auto tatCombo = MakeCell(First(Call(Member(context.Global("stats"), "get_int"), Values{Concat(Value("MPX_TATTOO_FM_CURRENT_"), *i_479)})));
							Call(Member(context.Global("table"), "insert"), Append(Values{*ExpTatCombo}, Call(Member(context.Global("string"), "format"), Values{Value("MPX_TATTOO_FM_CURRENT_%d = %d"), *i_479, *tatCombo})));
						}
					}
					if (Truthy(Member(context.Global("Rplus_Config"), "ComboToFile")))
					{
						Call(Member(context.Global("FileMgr"), "WriteFileContent"), Values{Concat(*TinkerPath, Value("/TinkerScripts/include/HumanCanvas/Exported_TattooCombo.txt")), First(Call(Member(context.Global("table"), "concat"), Values{*ExpTatCombo, Value("\n")})), Value(false)});
						Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Human Canvas"), Value("Success! Tattoos Combo has been exported to: Exported_TattooCombo.txt"), Value(3000)});
					}
					else
					{
						Call(Member(context.Global("ImGui"), "SetClipboardText"), Append(Values{}, Call(Member(context.Global("table"), "concat"), Values{*ExpTatCombo, Value("\n")})));
						Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Human Canvas"), Value("Success! Tattoos Combo has been exported to Clipboard"), Value(3000)});
					}
				}
				if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
				{
					Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Exports your current Tattoo Combo to Clipboard")});
				}
				Call(Member(context.Global("ImGui"), "SameLine"), Values{});
				Member(context.Global("Rplus_Config"), "ComboToFile") = First(Call(Member(context.Global("ImGui"), "Checkbox"), Values{Value("Export to file"), Member(context.Global("Rplus_Config"), "ComboToFile")}));
				if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
				{
					Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Toggle this if you want to write your current tattoo stats to a file.")});
				}
				if (Truthy(Member(context.Global("Rplus_Config"), "ComboToFile")))
				{
					Call(Member(context.Global("ImGui"), "SameLine"), Values{});
					Call(Member(context.Global("ImGui"), "SetNextItemWidth"), Values{Value(360)});
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Copy Path")}))))
					{
						Call(Member(context.Global("ImGui"), "SetClipboardText"), Values{Value("%AppData%/YimMenuV2/scripts/TinkerScripts/include/HumanCanvas/Exported_TattooCombo.txt")});
						Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Human Canvas"), Value("Path to Exported Tattoos has been copied to clipboard!"), Value(3000)});
					}
					if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
					{
						Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Copies Exported Tattoos \"File\" path to clipboard.")});
					}
				}
				Call(Member(context.Global("ImGui"), "EndGroup"), Values{});
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Apply All"), Value(89)}))))
				{
					Call(Member(context.Global("ImGui"), "OpenPopup"), Values{Value("##warnfullbodytat")});
				}
				if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
				{
					Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Applies All Tattoos.\nThey won't display all at once. Actual combos look better.")});
				}
				if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginPopupModal"), Values{Value("##warnfullbodytat"), Value{}, Value(65)}))))
				{
					Call(Member(context.Global("ImGui"), "Text"), Values{Value("Are you sure you want to apply all tattoos?")});
					Call(Member(context.Global("ImGui"), "Spacing"), Values{});
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Yes")}))))
					{
						{
							const auto start_480 = Number(Value(0));
							const auto end_481 = Number(Value(53));
							const auto step_482 = Number(Value(1));
							for (double index_483 = start_480; step_482 > 0 ? index_483 <= end_481 : index_483 >= end_481; index_483 += step_482)
							{
								auto i_484 = MakeCell(Value(index_483));
								Call(Member(context.Global("stats"), "set_int"), Values{Concat(Value("MPX_TATTOO_FM_CURRENT_"), *i_484), Negate(Value(1))});
							}
						}
						Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Human Canvas"), Value("Success! All tattoos Applied."), Value(3000)});
						Call(Member(context.Global("ImGui"), "CloseCurrentPopup"), Values{});
					}
					Call(Member(context.Global("ImGui"), "SameLine"), Values{});
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("No")}))))
					{
						Call(Member(context.Global("ImGui"), "CloseCurrentPopup"), Values{});
					}
					Call(Member(context.Global("ImGui"), "EndPopup"), Values{});
				}
				Call(Member(context.Global("ImGui"), "SameLine"), Values{});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Remove all")}))))
				{
					Call(Member(context.Global("ImGui"), "OpenPopup"), Values{Value("##warnremovetats")});
				}
				if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
				{
					Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Lucky Diamond who?")});
				}
				if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginPopupModal"), Values{Value("##warnremovetats"), Value{}, Value(65)}))))
				{
					Call(Member(context.Global("ImGui"), "Text"), Values{Value("Are you sure you want to remove all your tattoos?")});
					Call(Member(context.Global("ImGui"), "Spacing"), Values{});
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Yes")}))))
					{
						{
							const auto start_485 = Number(Value(0));
							const auto end_486 = Number(Value(53));
							const auto step_487 = Number(Value(1));
							for (double index_488 = start_485; step_487 > 0 ? index_488 <= end_486 : index_488 >= end_486; index_488 += step_487)
							{
								auto i_489 = MakeCell(Value(index_488));
								Call(Member(context.Global("stats"), "set_int"), Values{Concat(Value("MPX_TATTOO_FM_CURRENT_"), *i_489), Value(0)});
							}
						}
						Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Human Canvas"), Value("Success! All tattoos removed."), Value(3000)});
						Call(Member(context.Global("ImGui"), "CloseCurrentPopup"), Values{});
					}
					Call(Member(context.Global("ImGui"), "SameLine"), Values{});
					if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("No")}))))
					{
						Call(Member(context.Global("ImGui"), "CloseCurrentPopup"), Values{});
					}
					Call(Member(context.Global("ImGui"), "EndPopup"), Values{});
				}
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "Text"), Values{Value("Face Paints")});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
				{
					Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Allows you to stack face paints.\nSelected face paint will be applied automatically.\nIf any of these don't get applied, buy any of them first.")});
				}
				Call(Member(context.Global("ImGui"), "Separator"), Values{});
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("humcanv_fp1list")})), "draw", Values{});
				Call(Member(context.Global("ImGui"), "SameLine"), Values{});
				Call(Member(context.Global("ImGui"), "TextDisabled"), Values{Value("(?)")});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
				{
					Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Eyes Makeup: Smoky Black - Devastated.\nFace Paints start with \"Kiss My Axe\".")});
				}
				Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("humcanv_fp2list")})), "draw", Values{});
				Call(Member(context.Global("ImGui"), "SameLine"), Values{});
				Call(Member(context.Global("ImGui"), "TextDisabled"), Values{Value("(?)")});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
				{
					Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Blusher (Female Only): Full - Eighties.\nFace Paints start with \"Waves Left\".")});
				}
				Call(Member(context.Global("ImGui"), "SetNextItemWidth"), Values{Value(100)});
				auto values_490 = Append(Values{}, Call(Member(context.Global("ImGui"), "SliderFloat"), Values{Value("Face Paint Opacity##fppaintopacity"), Member(context.Global("Rplus_Config"), "FacePaint_Opacity"), Value(0.0), Value(1.0)}));
				Member(context.Global("Rplus_Config"), "FacePaint_Opacity") = At(values_490, 0);
				context.Global("fpchange") = At(values_490, 1);
				if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
				{
					Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Looks cool ¯\\_(°-°)_/¯")});
				}
				if (Truthy(context.Global("fpchange")))
				{
					Call(Member(context.Global("stats"), "set_float"), Values{Value("MPX_HEADBLEND_OVERLAY_MAKEUP_PC"), Member(context.Global("Rplus_Config"), "FacePaint_Opacity")});
				}
				Call(Member(context.Global("ImGui"), "SetNextItemWidth"), Values{Value(100)});
				auto values_491 = Append(Values{}, Call(Member(context.Global("ImGui"), "SliderFloat"), Values{Value("Makeup Opacity##fppaintopacity2"), Member(context.Global("Rplus_Config"), "FacePaint_Opacity2"), Value(0.0), Value(1.0)}));
				Member(context.Global("Rplus_Config"), "FacePaint_Opacity2") = At(values_491, 0);
				context.Global("fpchange2") = At(values_491, 1);
				if (Truthy(context.Global("fpchange2")))
				{
					Call(Member(context.Global("stats"), "set_float"), Values{Value("MPX_HEADBLEND_OVERLAY_BLUSHER"), Member(context.Global("Rplus_Config"), "FacePaint_Opacity2")});
				}
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(3.8)});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "TreeNodeEx"), Values{Concat(Value("Face Paint/Makeup Color ##fpmakeupcol"), *nomoretreeplease), Value(8219)}))))
				{
					Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(14)});
					Call(Member(context.Global("ImGui"), "Text"), Values{Concat(Value("Current: "), First(Call(Member(context.Global("stats"), "get_int"), Values{Member(context.Global("HC_MakeupColor"), "Stat2")})))});
					{
						Iterator iterator_492(Append(Values{}, Call(context.Global("ipairs"), Values{Member(context.Global("HC_MakeupColor"), "Colors")})));
						for (auto row_493 = iterator_492.Next(); !row_493.empty() && !row_493[0].IsNil(); row_493 = iterator_492.Next())
						{
							auto i_494 = MakeCell(At(row_493, 0));
							auto makeup = MakeCell(At(row_493, 1));
							if (Truthy(NotEqual(Modulo(Subtract(*i_494, Value(1)), Value(16)), Value(0))))
							{
								Call(Member(context.Global("ImGui"), "SameLine"), Values{});
							}
							if (Truthy(First(Call(Member(context.Global("ImGui"), "ColorButton"), Values{Concat(Value("##fpmakeupcol"), Member(*makeup, "statVal")), Member(*makeup, "bCol"), Value(64)}))))
							{
								Call(Member(context.Global("stats"), "set_int"), Values{Member(context.Global("HC_MakeupColor"), "Stat"), Member(*makeup, "statVal")});
								Call(Member(context.Global("stats"), "set_int"), Values{Member(context.Global("HC_MakeupColor"), "Stat2"), *i_494});
								Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Human Canvas"), Concat(Value("Success! Makeup Color Applied: "), *i_494), Value(3000)});
							}
							if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
							{
								Call(Member(context.Global("ImGui"), "SetTooltip"), Append(Values{}, Call(Member(context.Global("string"), "format"), Values{Value("Color (%d of 64)"), *i_494})));
							}
						}
					}
				}
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "Text"), Values{Value("Hair")});
				Call(Member(context.Global("ImGui"), "Separator"), Values{});
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "BeginGroup"), Values{});
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(3.8)});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "TreeNodeEx"), Values{Concat(Value("Hair Color  ##haircol"), *nomoretreeplease), Value(8219)}))))
				{
					Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(14)});
					Call(Member(context.Global("ImGui"), "Text"), Values{Concat(Value("Current: "), First(Call(Member(context.Global("stats"), "get_int"), Values{Member(Member(context.Global("HC_HairColor"), "HighlightOnOff"), "Stat")})))});
					if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
					{
						Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("This is you real hair color value.\nHovering over the colors below will display the value from Barber Shops.")});
					}
					Call(Member(context.Global("ImGui"), "SameLine"), Values{});
					Member(context.Global("HC_HairColor"), "HiddenColToggle") = First(Call(Member(context.Global("ImGui"), "Checkbox"), Values{Value("Hidden Colors"), Member(context.Global("HC_HairColor"), "HiddenColToggle")}));
					Call(Member(context.Global("ImGui"), "SameLine"), Values{});
					Member(Member(context.Global("HC_HairColor"), "HighlightOnOff"), "Toggle") = First(Call(Member(context.Global("ImGui"), "Checkbox"), Values{Value("Highlight"), Member(Member(context.Global("HC_HairColor"), "HighlightOnOff"), "Toggle")}));
					{
						Iterator iterator_495(Append(Values{}, Call(context.Global("ipairs"), Values{Member(context.Global("HC_HairColor"), "Regular")})));
						for (auto row_496 = iterator_495.Next(); !row_496.empty() && !row_496[0].IsNil(); row_496 = iterator_495.Next())
						{
							auto i_497 = MakeCell(At(row_496, 0));
							auto HairB = MakeCell(At(row_496, 1));
							if (Truthy(NotEqual(Modulo(Subtract(*i_497, Value(1)), Value(12)), Value(0))))
							{
								Call(Member(context.Global("ImGui"), "SameLine"), Values{});
							}
							if (Truthy(First(Call(Member(context.Global("ImGui"), "ColorButton"), Values{Concat(Value("##haircol"), Member(*HairB, "statVal")), Member(*HairB, "bCol"), Value(64)}))))
							{
								Call(Member(context.Global("stats"), "set_int"), Values{Member(Member(context.Global("HC_HairColor"), "HighlightOnOff"), "Stat"), Member(*HairB, "statVal")});
								if (Truthy(Value(!Truthy(Member(context.Global("HC_HairColor"), "HiddenColToggle")))))
								{
									Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Human Canvas"), Concat(Value("Success! Hair Color Applied: "), *i_497), Value(3000)});
								}
								else
								{
									Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Human Canvas"), Concat(Value("Success! Hair Color Applied: "), First(Call(Member(context.Global("stats"), "get_int"), Values{Member(Member(context.Global("HC_HairColor"), "HighlightOnOff"), "Stat")}))), Value(3000)});
								}
							}
							if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
							{
								if (Truthy(Value(!Truthy(Member(context.Global("HC_HairColor"), "HiddenColToggle")))))
								{
									Call(Member(context.Global("ImGui"), "SetTooltip"), Append(Values{}, Call(Member(context.Global("string"), "format"), Values{Value("Color (%d of 48)"), *i_497})));
								}
								else
								{
									Call(Member(context.Global("ImGui"), "SetTooltip"), Append(Values{}, Call(Member(context.Global("string"), "format"), Values{Value("Color %d"), Member(*HairB, "statVal")})));
								}
							}
						}
					}
				}
				Call(Member(context.Global("ImGui"), "EndGroup"), Values{});
				Call(Member(context.Global("ImGui"), "BeginGroup"), Values{});
				if (Truthy(Member(context.Global("HC_HairColor"), "HiddenColToggle")))
				{
					{
						Iterator iterator_498(Append(Values{}, Call(context.Global("ipairs"), Values{Member(context.Global("HC_HairColor"), "Hidden")})));
						for (auto row_499 = iterator_498.Next(); !row_499.empty() && !row_499[0].IsNil(); row_499 = iterator_498.Next())
						{
							auto i_500 = MakeCell(At(row_499, 0));
							auto HairB_501 = MakeCell(At(row_499, 1));
							if (Truthy(NotEqual(Modulo(Subtract(*i_500, Value(1)), Value(12)), Value(0))))
							{
								Call(Member(context.Global("ImGui"), "SameLine"), Values{});
							}
							if (Truthy(First(Call(Member(context.Global("ImGui"), "ColorButton"), Values{Concat(Value("##haircol"), Member(*HairB_501, "statVal")), Member(*HairB_501, "bCol"), Value(64)}))))
							{
								Call(Member(context.Global("stats"), "set_int"), Values{Member(Member(context.Global("HC_HairColor"), "HighlightOnOff"), "Stat"), Member(*HairB_501, "statVal")});
								Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Human Canvas"), Concat(Value("Success! Hidden Hair Color Applied: "), First(Call(Member(context.Global("stats"), "get_int"), Values{Member(Member(context.Global("HC_HairColor"), "HighlightOnOff"), "Stat")}))), Value(3000)});
							}
							if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
							{
								Call(Member(context.Global("ImGui"), "SetTooltip"), Append(Values{}, Call(Member(context.Global("string"), "format"), Values{Value("Color %d"), Member(*HairB_501, "statVal")})));
							}
						}
					}
				}
				if (Truthy(Member(Member(context.Global("HC_HairColor"), "HighlightOnOff"), "Toggle")))
				{
					Member(Member(context.Global("HC_HairColor"), "HighlightOnOff"), "Stat") = Value("MPX_SEC_HAIR_TINT");
				}
				else
				{
					Member(Member(context.Global("HC_HairColor"), "HighlightOnOff"), "Stat") = Value("MPX_HAIR_TINT");
				}
				Call(Member(context.Global("ImGui"), "EndGroup"), Values{});
				Call(Member(context.Global("ImGui"), "EndTabItem"), Values{});
			}
			Call(Member(context.Global("ImGui"), "EndTabBar"), Values{});
			return {};
		})});
		auto Stngsplus_Config = MakeCell(MakeTable({{"CloseBCSonUnload", Or(Member(context.Global("TS_Settings"), "CloseBCSonUnload"), [&] {
			                                             return Value(true);
		                                             })},
		    {"LoadSettings", Or(Member(context.Global("TS_Settings"), "LoadSettings"), [&] {
			     return Value(false);
		     })}}));
		// CloseBCSonUnload preserves the feature callback and its captured state.
		auto CloseBCSonUnload = MakeCell();
		*CloseBCSonUnload = MakeFunction([=, &context](const Values& args) -> Values {
			if (Truthy(Member(*Stngsplus_Config, "CloseBCSonUnload")))
			{
				Call(Member(context.Global("FileMgr"), "WriteFileContent"), Values{Concat(*TinkerPath, Value("/TinkerScripts/include/Settings/BlockCloudSaves/NoSave.txt")), Value("Close"), Value(false)});
			}
			return Values{Member(*Stngsplus_Config, "CloseBCSonUnload")};
			return {};
		});
		// SaveSettings preserves the feature callback and its captured state.
		auto SaveSettings = MakeCell();
		*SaveSettings = MakeFunction([=, &context](const Values&) -> Values {
			SaveNativeSettings(context, *NoClipWindowTop, *Stngsplus_Config);
			return {};
		});
		// LoadSavedSettings preserves the feature callback and its captured state.
		auto LoadSavedSettings = MakeCell();
		*LoadSavedSettings = MakeFunction([=, &context](const Values& args) -> Values {
			Call(context.Global("require"), Values{Value("TS_Settings")});
			Member(*Stngsplus_Config, "LoadSettings") = Member(context.Global("TS_Settings"), "LoadSettings");
			Member(*Stngsplus_Config, "CloseBCSonUnload") = Member(context.Global("TS_Settings"), "CloseBCSonUnload");
			*NoClipWindowTop = Member(context.Global("TS_Settings"), "NoClipWindowTop");
			Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("noclip_onscreen")})), "set_value", Values{Member(context.Global("TS_Settings"), "NoClipState")});
			Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("HGWH_restock")})), "set_value", Values{Member(context.Global("TS_Settings"), "HGWHrestock")});
			Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("HGWH_maxgoods")})), "set_value", Values{Member(context.Global("TS_Settings"), "HGWHmaxgoods")});
			Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("HG_setgood")})), "set_value", Values{Member(context.Global("TS_Settings"), "HGsetgood")});
			Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("WH_setgood")})), "set_value", Values{Member(context.Global("TS_Settings"), "WHsetgood")});
			Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("MC_resup")})), "set_value", Values{Member(context.Global("TS_Settings"), "MCresup")});
			Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("MC_restock")})), "set_value", Values{Member(context.Global("TS_Settings"), "MCrestock")});
			Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("NC_goods")})), "set_value", Values{Member(context.Global("TS_Settings"), "NCgoods")});
			Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("HG_goodtype")})), "set_value", Values{Member(context.Global("TS_Settings"), "HGgoodtype")});
			Invoke(First(Call(Member(context.Global("commandmgr"), "get_command"), Values{Value("WH_goodtype")})), "set_value", Values{Member(context.Global("TS_Settings"), "WHgoodtype")});
			return {};
		});
		Invoke(*TS_SettingsPlus, "imgui", Values{MakeFunction([=, &context](const Values& args) -> Values {
			if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginChild"), Values{Value("##setttingschild"), Value(271), Value(132), Value(1)}))))
			{
				Call(*CenteredText, Values{Value("Submenu Settings"), Value(false), Value(true)});
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Save Current Settings")}))))
				{
					Call(*SaveSettings, Values{});
				}
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Load Saved Settings")}))))
				{
					Call(*LoadSavedSettings, Values{});
				}
				Member(*Stngsplus_Config, "LoadSettings") = First(Call(Member(context.Global("ImGui"), "Checkbox"), Values{Value("Load Saved Settings on Startup"), Member(*Stngsplus_Config, "LoadSettings")}));
			}
			Call(Member(context.Global("ImGui"), "EndChild"), Values{});
			if (Truthy(First(Call(Member(context.Global("ImGui"), "BeginChild"), Values{Value("##cloudsaveschild"), Value(296), Value(196), Value(1)}))))
			{
				Call(*CenteredText, Values{Value("Block Cloud Saves"), Value(false), Value(true)});
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "Text"), Values{Value("Cloud Save State: ")});
				if (Truthy(Equal(First(Call(*TS_PATH_NoCloudSave, Values{})), Value(""))))
				{
					Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(0)});
					Call(Member(context.Global("ImGui"), "TextColored"), Values{Value(0), Value(1), Value(1), Value(1), Value("Ready")});
				}
				else if (Truthy(Equal(First(Call(*TS_PATH_NoCloudSave, Values{})), Value("Error"))))
				{
					Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(0)});
					Call(Member(context.Global("ImGui"), "TextColored"), Values{Value(1), Value(0), Value(0), Value(1), Value("ERROR")});
				}
				else if (Truthy(Equal(First(Call(*TS_PATH_NoCloudSave, Values{})), Value("Disabled"))))
				{
					Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(0)});
					Call(Member(context.Global("ImGui"), "TextColored"), Values{Value(1), Value(0), Value(0), Value(1), Value("OFF")});
				}
				else if (Truthy(Equal(First(Call(*TS_PATH_NoCloudSave, Values{})), Value("Enabled"))))
				{
					Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(0)});
					Call(Member(context.Global("ImGui"), "TextColored"), Values{Value(0), Value(1), Value(0), Value(1), Value("ON")});
				}
				else if (Truthy(Or(Or(Or(Equal(First(Call(*TS_PATH_NoCloudSave, Values{})), Value("Closed")), [&] {
					         return Equal(First(Call(*TS_PATH_NoCloudSave, Values{})), Value("Close"));
				         }),
				                       [&] {
					                       return Equal(First(Call(*TS_PATH_NoCloudSave, Values{})), Value("Disable"));
				                       }),
				             [&] {
					             return Equal(First(Call(*TS_PATH_NoCloudSave, Values{})), Value("Enable"));
				             })))
				{
					Call(Member(context.Global("ImGui"), "SameLine"), Values{Value(0), Value(0)});
					Call(Member(context.Global("ImGui"), "TextDisabled"), Values{Value("WAITING")});
				}
				Call(Member(context.Global("ImGui"), "SameLine"), Values{});
				if (Truthy(NotEqual(First(Call(*TS_PATH_NoCloudSave, Values{})), Value(""))))
				{
					Call(Member(context.Global("ImGui"), "TextDisabled"), Values{Value("(?)")});
				}
				if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
				{
					Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("ON: Cloud Saves ENABLED\nOFF: Cloud Saves DISABLED\n\nWAITING: the cloud-save tool is not running")});
				}
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Disable Cloud Saves")}))))
				{
					Call(Member(context.Global("FileMgr"), "WriteFileContent"), Values{Concat(*TinkerPath, Value("/TinkerScripts/include/Settings/BlockCloudSaves/NoSave.txt")), Value("Disable"), Value(false)});
				}
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Enable Cloud Saves")}))))
				{
					Call(Member(context.Global("FileMgr"), "WriteFileContent"), Values{Concat(*TinkerPath, Value("/TinkerScripts/include/Settings/BlockCloudSaves/NoSave.txt")), Value("Enable"), Value(false)});
				}
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				Call(Member(context.Global("ImGui"), "Spacing"), Values{});
				if (Truthy(First(Call(Member(context.Global("ImGui"), "Button"), Values{Value("Start Cloud Save Tool")}))))
				{
					Call(Member(context.Global("CloudSave"), "Start"), Values{});
					Call(Member(context.Global("notify"), "success"), Values{Value("TinkerScripts - Block Cloud Saves"), Value("Cloud-save tool requested. Approve the Windows prompt to start it."), Value(3000)});
				}
				Member(*Stngsplus_Config, "CloseBCSonUnload") = First(Call(Member(context.Global("ImGui"), "Checkbox"), Values{Value("Close Tool on Menu Unload"), Member(*Stngsplus_Config, "CloseBCSonUnload")}));
				if (Truthy(First(Call(Member(context.Global("ImGui"), "IsItemHovered"), Values{}))))
				{
					Call(Member(context.Global("ImGui"), "SetTooltip"), Values{Value("Should the cloud-save tool close itself on menu unload? (Enabled by default)\nThis will remove the firewall rule added by the tool, so you don't have problems joining Online in your next game session.")});
				}
			}
			Call(Member(context.Global("ImGui"), "EndChild"), Values{});
			return {};
		})});
		if (Truthy(Member(*Stngsplus_Config, "LoadSettings")))
		{
			Call(*LoadSavedSettings, Values{});
		}
		Call(Member(context.Global("log"), "info"), Append(Values{}, Call(Member(context.Global("string"), "format"), Values{Value("%s%sTinkerScripts%s"), *cls, *lual, *lual_s})));
		Call(Member(context.Global("event"), "register_handler"), Values{Member(context.Global("menu_event"), "Unload"), MakeFunction([=, &context](const Values& args) -> Values {
			                                                                 Call(*CloseBCSonUnload, Values{});
			                                                                 return {};
		                                                                 })});
		return {};
	}
}

#ifndef TINKER_SCRIPT_TESTING

    // Typed game-native bindings.
    // Direct game-native bindings used by the compiled TinkerScripts features.
	#include "common.hpp"
	#include "TinkerScriptData.hpp"
	#include "game/gta/Natives.hpp"
	#include "core/util/Joaat.hpp"
namespace YimMenu::TinkerScript
{
	std::uint32_t Hash(const Value& value)
	{
		return std::holds_alternative<std::string>(value.data) ? Joaat(Text(value)) : UnsignedBits(value);
	}
	static Value Vector(const rage::scrVector& value)
	{
		return MakeTable({{"x", value.x}, {"y", value.y}, {"z", value.z}});
	}
	void BindGameNatives(Context& context)
	{
		context.Bind("ScriptNatives", "START_NEW_SCRIPT", [](const Values& args) -> Values {
			auto text_0 = At(args, 0).IsNil() ? std::string{} : Text(At(args, 0));
			return {NativeInvoker::Invoke<static_cast<int>(NativeIndex::START_NEW_SCRIPT), int, false>(At(args, 0).IsNil() ? nullptr : text_0.c_str(), Integer(At(args, 1)))};
		});
		context.Bind("ScriptNatives", "VDIST", [](const Values& args) -> Values {
			return {NativeInvoker::Invoke<static_cast<int>(NativeIndex::VDIST), float, false>(static_cast<float>(Number(At(args, 0))), static_cast<float>(Number(At(args, 1))), static_cast<float>(Number(At(args, 2))), static_cast<float>(Number(At(args, 3))), static_cast<float>(Number(At(args, 4))), static_cast<float>(Number(At(args, 5))))};
		});
		context.Bind("CAMERA", "DO_SCREEN_FADE_IN", [](const Values& args) -> Values {
			CAMERA::DO_SCREEN_FADE_IN(Integer(At(args, 0)));
			return {};
		});
		context.Bind("CAMERA", "GET_FOLLOW_PED_CAM_VIEW_MODE", [](const Values& args) -> Values {
			return {CAMERA::GET_FOLLOW_PED_CAM_VIEW_MODE()};
		});
		context.Bind("CAMERA", "INVALIDATE_IDLE_CAM", [](const Values& args) -> Values {
			CAMERA::INVALIDATE_IDLE_CAM();
			return {};
		});
		context.Bind("CAMERA", "IS_GAMEPLAY_CAM_RENDERING", [](const Values& args) -> Values {
			return {static_cast<bool>(CAMERA::IS_GAMEPLAY_CAM_RENDERING())};
		});
		context.Bind("CAMERA", "IS_SCREEN_FADED_OUT", [](const Values& args) -> Values {
			return {static_cast<bool>(CAMERA::IS_SCREEN_FADED_OUT())};
		});
		context.Bind("ENTITY", "GET_ENTITY_COORDS", [](const Values& args) -> Values {
			return {Vector(ENTITY::GET_ENTITY_COORDS(static_cast<Entity>(Number(At(args, 0))), static_cast<BOOL>(At(args, 1).IsNil() ? 0 : Number(At(args, 1)))))};
		});
		context.Bind("ENTITY", "GET_ENTITY_FORWARD_VECTOR", [](const Values& args) -> Values {
			return {Vector(ENTITY::GET_ENTITY_FORWARD_VECTOR(static_cast<Entity>(Number(At(args, 0)))))};
		});
		context.Bind("ENTITY", "GET_ENTITY_HEADING", [](const Values& args) -> Values {
			return {ENTITY::GET_ENTITY_HEADING(static_cast<Entity>(Number(At(args, 0))))};
		});
		context.Bind("ENTITY", "GET_ENTITY_HEIGHT_ABOVE_GROUND", [](const Values& args) -> Values {
			return {ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(static_cast<Entity>(Number(At(args, 0))))};
		});
		context.Bind("ENTITY", "GET_ENTITY_MODEL", [](const Values& args) -> Values {
			return {static_cast<std::int32_t>(ENTITY::GET_ENTITY_MODEL(static_cast<Entity>(Number(At(args, 0)))))};
		});
		context.Bind("ENTITY", "IS_ENTITY_A_MISSION_ENTITY", [](const Values& args) -> Values {
			return {static_cast<bool>(ENTITY::IS_ENTITY_A_MISSION_ENTITY(static_cast<Entity>(Number(At(args, 0)))))};
		});
		context.Bind("ENTITY", "SET_ENTITY_AS_MISSION_ENTITY", [](const Values& args) -> Values {
			ENTITY::SET_ENTITY_AS_MISSION_ENTITY(static_cast<Entity>(Number(At(args, 0))), static_cast<BOOL>(At(args, 1).IsNil() ? 0 : Number(At(args, 1))), static_cast<BOOL>(At(args, 2).IsNil() ? 0 : Number(At(args, 2))));
			return {};
		});
		context.Bind("ENTITY", "SET_ENTITY_HEADING", [](const Values& args) -> Values {
			ENTITY::SET_ENTITY_HEADING(static_cast<Entity>(Number(At(args, 0))), static_cast<float>(Number(At(args, 1))));
			return {};
		});
		context.Bind("GRAPHICS", "BEGIN_SCALEFORM_MOVIE_METHOD", [](const Values& args) -> Values {
			auto text_1 = At(args, 1).IsNil() ? std::string{} : Text(At(args, 1));
			return {static_cast<bool>(GRAPHICS::BEGIN_SCALEFORM_MOVIE_METHOD(Integer(At(args, 0)), At(args, 1).IsNil() ? nullptr : text_1.c_str()))};
		});
		context.Bind("GRAPHICS", "DRAW_RECT", [](const Values& args) -> Values {
			GRAPHICS::DRAW_RECT(static_cast<float>(Number(At(args, 0))), static_cast<float>(Number(At(args, 1))), static_cast<float>(Number(At(args, 2))), static_cast<float>(Number(At(args, 3))), Integer(At(args, 4)), Integer(At(args, 5)), Integer(At(args, 6)), Integer(At(args, 7)), static_cast<BOOL>(At(args, 8).IsNil() ? 0 : Number(At(args, 8))));
			return {};
		});
		context.Bind("GRAPHICS", "DRAW_SCALEFORM_MOVIE_FULLSCREEN", [](const Values& args) -> Values {
			GRAPHICS::DRAW_SCALEFORM_MOVIE_FULLSCREEN(Integer(At(args, 0)), Integer(At(args, 1)), Integer(At(args, 2)), Integer(At(args, 3)), Integer(At(args, 4)), Integer(At(args, 5)));
			return {};
		});
		context.Bind("GRAPHICS", "DRAW_SPRITE", [](const Values& args) -> Values {
			auto text_0 = At(args, 0).IsNil() ? std::string{} : Text(At(args, 0));
			auto text_1 = At(args, 1).IsNil() ? std::string{} : Text(At(args, 1));
			GRAPHICS::DRAW_SPRITE(At(args, 0).IsNil() ? nullptr : text_0.c_str(), At(args, 1).IsNil() ? nullptr : text_1.c_str(), static_cast<float>(Number(At(args, 2))), static_cast<float>(Number(At(args, 3))), static_cast<float>(Number(At(args, 4))), static_cast<float>(Number(At(args, 5))), static_cast<float>(Number(At(args, 6))), Integer(At(args, 7)), Integer(At(args, 8)), Integer(At(args, 9)), Integer(At(args, 10)), static_cast<BOOL>(At(args, 11).IsNil() ? 0 : Number(At(args, 11))), static_cast<Any>(Number(At(args, 12))));
			return {};
		});
		context.Bind("GRAPHICS", "END_SCALEFORM_MOVIE_METHOD", [](const Values& args) -> Values {
			GRAPHICS::END_SCALEFORM_MOVIE_METHOD();
			return {};
		});
		context.Bind("GRAPHICS", "HAS_SCALEFORM_MOVIE_LOADED", [](const Values& args) -> Values {
			return {static_cast<bool>(GRAPHICS::HAS_SCALEFORM_MOVIE_LOADED(Integer(At(args, 0))))};
		});
		context.Bind("GRAPHICS", "REQUEST_SCALEFORM_MOVIE", [](const Values& args) -> Values {
			auto text_0 = At(args, 0).IsNil() ? std::string{} : Text(At(args, 0));
			return {GRAPHICS::REQUEST_SCALEFORM_MOVIE(At(args, 0).IsNil() ? nullptr : text_0.c_str())};
		});
		context.Bind("GRAPHICS", "REQUEST_STREAMED_TEXTURE_DICT", [](const Values& args) -> Values {
			auto text_0 = At(args, 0).IsNil() ? std::string{} : Text(At(args, 0));
			GRAPHICS::REQUEST_STREAMED_TEXTURE_DICT(At(args, 0).IsNil() ? nullptr : text_0.c_str(), static_cast<BOOL>(At(args, 1).IsNil() ? 0 : Number(At(args, 1))));
			return {};
		});
		context.Bind("GRAPHICS", "SCALEFORM_MOVIE_METHOD_ADD_PARAM_PLAYER_NAME_STRING", [](const Values& args) -> Values {
			auto text_0 = At(args, 0).IsNil() ? std::string{} : Text(At(args, 0));
			GRAPHICS::SCALEFORM_MOVIE_METHOD_ADD_PARAM_PLAYER_NAME_STRING(At(args, 0).IsNil() ? nullptr : text_0.c_str());
			return {};
		});
		context.Bind("GRAPHICS", "SET_ARTIFICIAL_LIGHTS_STATE", [](const Values& args) -> Values {
			GRAPHICS::SET_ARTIFICIAL_LIGHTS_STATE(static_cast<BOOL>(At(args, 0).IsNil() ? 0 : Number(At(args, 0))));
			return {};
		});
		context.Bind("GRAPHICS", "SET_ARTIFICIAL_VEHICLE_LIGHTS_STATE", [](const Values& args) -> Values {
			GRAPHICS::SET_ARTIFICIAL_VEHICLE_LIGHTS_STATE(static_cast<BOOL>(At(args, 0).IsNil() ? 0 : Number(At(args, 0))));
			return {};
		});
		context.Bind("GRAPHICS", "START_NETWORKED_PARTICLE_FX_NON_LOOPED_ON_PED_BONE", [](const Values& args) -> Values {
			auto text_0 = At(args, 0).IsNil() ? std::string{} : Text(At(args, 0));
			return {static_cast<bool>(GRAPHICS::START_NETWORKED_PARTICLE_FX_NON_LOOPED_ON_PED_BONE(At(args, 0).IsNil() ? nullptr : text_0.c_str(), static_cast<Ped>(Number(At(args, 1))), static_cast<float>(Number(At(args, 2))), static_cast<float>(Number(At(args, 3))), static_cast<float>(Number(At(args, 4))), static_cast<float>(Number(At(args, 5))), static_cast<float>(Number(At(args, 6))), static_cast<float>(Number(At(args, 7))), Integer(At(args, 8)), static_cast<float>(Number(At(args, 9))), static_cast<BOOL>(At(args, 10).IsNil() ? 0 : Number(At(args, 10))), static_cast<BOOL>(At(args, 11).IsNil() ? 0 : Number(At(args, 11))), static_cast<BOOL>(At(args, 12).IsNil() ? 0 : Number(At(args, 12)))))};
		});
		context.Bind("GRAPHICS", "START_PARTICLE_FX_NON_LOOPED_ON_PED_BONE", [](const Values& args) -> Values {
			auto text_0 = At(args, 0).IsNil() ? std::string{} : Text(At(args, 0));
			return {static_cast<bool>(GRAPHICS::START_PARTICLE_FX_NON_LOOPED_ON_PED_BONE(At(args, 0).IsNil() ? nullptr : text_0.c_str(), static_cast<Ped>(Number(At(args, 1))), static_cast<float>(Number(At(args, 2))), static_cast<float>(Number(At(args, 3))), static_cast<float>(Number(At(args, 4))), static_cast<float>(Number(At(args, 5))), static_cast<float>(Number(At(args, 6))), static_cast<float>(Number(At(args, 7))), Integer(At(args, 8)), static_cast<float>(Number(At(args, 9))), static_cast<BOOL>(At(args, 10).IsNil() ? 0 : Number(At(args, 10))), static_cast<BOOL>(At(args, 11).IsNil() ? 0 : Number(At(args, 11))), static_cast<BOOL>(At(args, 12).IsNil() ? 0 : Number(At(args, 12)))))};
		});
		context.Bind("GRAPHICS", "USE_PARTICLE_FX_ASSET", [](const Values& args) -> Values {
			auto text_0 = At(args, 0).IsNil() ? std::string{} : Text(At(args, 0));
			GRAPHICS::USE_PARTICLE_FX_ASSET(At(args, 0).IsNil() ? nullptr : text_0.c_str());
			return {};
		});
		context.Bind("GTA", "CELL_HORIZONTAL_MODE_TOGGLE", [](const Values& args) -> Values {
			GTA::CELL_HORIZONTAL_MODE_TOGGLE(static_cast<BOOL>(At(args, 0).IsNil() ? 0 : Number(At(args, 0))));
			return {};
		});
		context.Bind("GTA", "CELL_SET_INPUT", [](const Values& args) -> Values {
			GTA::CELL_SET_INPUT(Integer(At(args, 0)));
			return {};
		});
		context.Bind("HUD", "ADD_BLIP_FOR_ENTITY", [](const Values& args) -> Values {
			return {HUD::ADD_BLIP_FOR_ENTITY(static_cast<Entity>(Number(At(args, 0))))};
		});
		context.Bind("HUD", "ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME", [](const Values& args) -> Values {
			auto text_0 = At(args, 0).IsNil() ? std::string{} : Text(At(args, 0));
			HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(At(args, 0).IsNil() ? nullptr : text_0.c_str());
			return {};
		});
		context.Bind("HUD", "BEGIN_TEXT_COMMAND_DISPLAY_HELP", [](const Values& args) -> Values {
			auto text_0 = At(args, 0).IsNil() ? std::string{} : Text(At(args, 0));
			HUD::BEGIN_TEXT_COMMAND_DISPLAY_HELP(At(args, 0).IsNil() ? nullptr : text_0.c_str());
			return {};
		});
		context.Bind("HUD", "BEGIN_TEXT_COMMAND_DISPLAY_TEXT", [](const Values& args) -> Values {
			auto text_0 = At(args, 0).IsNil() ? std::string{} : Text(At(args, 0));
			HUD::BEGIN_TEXT_COMMAND_DISPLAY_TEXT(At(args, 0).IsNil() ? nullptr : text_0.c_str());
			return {};
		});
		context.Bind("HUD", "BEGIN_TEXT_COMMAND_THEFEED_POST", [](const Values& args) -> Values {
			auto text_0 = At(args, 0).IsNil() ? std::string{} : Text(At(args, 0));
			HUD::BEGIN_TEXT_COMMAND_THEFEED_POST(At(args, 0).IsNil() ? nullptr : text_0.c_str());
			return {};
		});
		context.Bind("HUD", "DOES_BLIP_EXIST", [](const Values& args) -> Values {
			return {static_cast<bool>(HUD::DOES_BLIP_EXIST(static_cast<Blip>(Number(At(args, 0)))))};
		});
		context.Bind("HUD", "END_TEXT_COMMAND_DISPLAY_HELP", [](const Values& args) -> Values {
			HUD::END_TEXT_COMMAND_DISPLAY_HELP(Integer(At(args, 0)), static_cast<BOOL>(At(args, 1).IsNil() ? 0 : Number(At(args, 1))), static_cast<BOOL>(At(args, 2).IsNil() ? 0 : Number(At(args, 2))), Integer(At(args, 3)));
			return {};
		});
		context.Bind("HUD", "END_TEXT_COMMAND_DISPLAY_TEXT", [](const Values& args) -> Values {
			HUD::END_TEXT_COMMAND_DISPLAY_TEXT(static_cast<float>(Number(At(args, 0))), static_cast<float>(Number(At(args, 1))), Integer(At(args, 2)));
			return {};
		});
		context.Bind("HUD", "END_TEXT_COMMAND_THEFEED_POST_TICKER", [](const Values& args) -> Values {
			return {HUD::END_TEXT_COMMAND_THEFEED_POST_TICKER(static_cast<BOOL>(At(args, 0).IsNil() ? 0 : Number(At(args, 0))), static_cast<BOOL>(At(args, 1).IsNil() ? 0 : Number(At(args, 1))))};
		});
		context.Bind("HUD", "GET_BLIP_INFO_ID_COORD", [](const Values& args) -> Values {
			return {Vector(HUD::GET_BLIP_INFO_ID_COORD(static_cast<Blip>(Number(At(args, 0)))))};
		});
		context.Bind("HUD", "GET_BLIP_INFO_ID_ENTITY_INDEX", [](const Values& args) -> Values {
			return {HUD::GET_BLIP_INFO_ID_ENTITY_INDEX(static_cast<Blip>(Number(At(args, 0))))};
		});
		context.Bind("HUD", "GET_FIRST_BLIP_INFO_ID", [](const Values& args) -> Values {
			return {HUD::GET_FIRST_BLIP_INFO_ID(Integer(At(args, 0)))};
		});
		context.Bind("HUD", "IS_HELP_MESSAGE_BEING_DISPLAYED", [](const Values& args) -> Values {
			return {static_cast<bool>(HUD::IS_HELP_MESSAGE_BEING_DISPLAYED())};
		});
		context.Bind("HUD", "IS_WARNING_MESSAGE_READY_FOR_CONTROL", [](const Values& args) -> Values {
			return {static_cast<bool>(HUD::IS_WARNING_MESSAGE_READY_FOR_CONTROL())};
		});
		context.Bind("HUD", "SET_BLIP_COLOUR", [](const Values& args) -> Values {
			HUD::SET_BLIP_COLOUR(static_cast<Blip>(Number(At(args, 0))), Integer(At(args, 1)));
			return {};
		});
		context.Bind("HUD", "SET_BLIP_PRIORITY", [](const Values& args) -> Values {
			HUD::SET_BLIP_PRIORITY(static_cast<Blip>(Number(At(args, 0))), Integer(At(args, 1)));
			return {};
		});
		context.Bind("HUD", "SET_BLIP_SCALE", [](const Values& args) -> Values {
			HUD::SET_BLIP_SCALE(static_cast<Blip>(Number(At(args, 0))), static_cast<float>(Number(At(args, 1))));
			return {};
		});
		context.Bind("HUD", "SET_BLIP_SPRITE", [](const Values& args) -> Values {
			HUD::SET_BLIP_SPRITE(static_cast<Blip>(Number(At(args, 0))), Integer(At(args, 1)));
			return {};
		});
		context.Bind("HUD", "SET_TEXT_LEADING", [](const Values& args) -> Values {
			HUD::SET_TEXT_LEADING(Integer(At(args, 0)));
			return {};
		});
		context.Bind("HUD", "SET_TEXT_SCALE", [](const Values& args) -> Values {
			HUD::SET_TEXT_SCALE(static_cast<float>(Number(At(args, 0))), static_cast<float>(Number(At(args, 1))));
			return {};
		});
		context.Bind("HUD", "THEFEED_SET_BACKGROUND_COLOR_FOR_NEXT_POST", [](const Values& args) -> Values {
			HUD::THEFEED_SET_BACKGROUND_COLOR_FOR_NEXT_POST(Integer(At(args, 0)));
			return {};
		});
		context.Bind("MISC", "DISPLAY_ONSCREEN_KEYBOARD", [](const Values& args) -> Values {
			auto text_1 = At(args, 1).IsNil() ? std::string{} : Text(At(args, 1));
			auto text_2 = At(args, 2).IsNil() ? std::string{} : Text(At(args, 2));
			auto text_3 = At(args, 3).IsNil() ? std::string{} : Text(At(args, 3));
			auto text_4 = At(args, 4).IsNil() ? std::string{} : Text(At(args, 4));
			auto text_5 = At(args, 5).IsNil() ? std::string{} : Text(At(args, 5));
			auto text_6 = At(args, 6).IsNil() ? std::string{} : Text(At(args, 6));
			MISC::DISPLAY_ONSCREEN_KEYBOARD(Integer(At(args, 0)), At(args, 1).IsNil() ? nullptr : text_1.c_str(), At(args, 2).IsNil() ? nullptr : text_2.c_str(), At(args, 3).IsNil() ? nullptr : text_3.c_str(), At(args, 4).IsNil() ? nullptr : text_4.c_str(), At(args, 5).IsNil() ? nullptr : text_5.c_str(), At(args, 6).IsNil() ? nullptr : text_6.c_str(), Integer(At(args, 7)));
			return {};
		});
		context.Bind("MISC", "GET_GAME_TIMER", [](const Values& args) -> Values {
			return {MISC::GET_GAME_TIMER()};
		});
		context.Bind("MISC", "GET_ONSCREEN_KEYBOARD_RESULT", [](const Values& args) -> Values {
			return {MISC::GET_ONSCREEN_KEYBOARD_RESULT()};
		});
		context.Bind("MISC", "UPDATE_ONSCREEN_KEYBOARD", [](const Values& args) -> Values {
			return {MISC::UPDATE_ONSCREEN_KEYBOARD()};
		});
		context.Bind("NETSHOPPING", "NET_GAMESERVER_GET_PRICE", [](const Values& args) -> Values {
			return {NETSHOPPING::NET_GAMESERVER_GET_PRICE(Hash(At(args, 0)), Hash(At(args, 1)), static_cast<BOOL>(At(args, 2).IsNil() ? 0 : Number(At(args, 2))))};
		});
		context.Bind("NETWORK", "NETWORK_IS_ACTIVITY_SESSION", [](const Values& args) -> Values {
			return {static_cast<bool>(NETWORK::NETWORK_IS_ACTIVITY_SESSION())};
		});
		context.Bind("NETWORK", "NETWORK_IS_IN_TRANSITION", [](const Values& args) -> Values {
			return {static_cast<bool>(NETWORK::NETWORK_IS_IN_TRANSITION())};
		});
		context.Bind("NETWORK", "NETWORK_IS_SESSION_STARTED", [](const Values& args) -> Values {
			return {static_cast<bool>(NETWORK::NETWORK_IS_SESSION_STARTED())};
		});
		context.Bind("OBJECT", "DOOR_SYSTEM_SET_DOOR_STATE", [](const Values& args) -> Values {
			OBJECT::DOOR_SYSTEM_SET_DOOR_STATE(Hash(At(args, 0)), Integer(At(args, 1)), static_cast<BOOL>(At(args, 2).IsNil() ? 0 : Number(At(args, 2))), static_cast<BOOL>(At(args, 3).IsNil() ? 0 : Number(At(args, 3))));
			return {};
		});
		context.Bind("PAD", "IS_CONTROL_JUST_PRESSED", [](const Values& args) -> Values {
			return {static_cast<bool>(PAD::IS_CONTROL_JUST_PRESSED(Integer(At(args, 0)), Integer(At(args, 1))))};
		});
		context.Bind("PAD", "IS_CONTROL_JUST_RELEASED", [](const Values& args) -> Values {
			return {static_cast<bool>(PAD::IS_CONTROL_JUST_RELEASED(Integer(At(args, 0)), Integer(At(args, 1))))};
		});
		context.Bind("PAD", "IS_CONTROL_PRESSED", [](const Values& args) -> Values {
			return {static_cast<bool>(PAD::IS_CONTROL_PRESSED(Integer(At(args, 0)), Integer(At(args, 1))))};
		});
		context.Bind("PAD", "IS_DISABLED_CONTROL_PRESSED", [](const Values& args) -> Values {
			return {static_cast<bool>(PAD::IS_DISABLED_CONTROL_PRESSED(Integer(At(args, 0)), Integer(At(args, 1))))};
		});
		context.Bind("PAD", "SET_CONTROL_VALUE_NEXT_FRAME", [](const Values& args) -> Values {
			return {static_cast<bool>(PAD::SET_CONTROL_VALUE_NEXT_FRAME(Integer(At(args, 0)), Integer(At(args, 1)), static_cast<float>(Number(At(args, 2)))))};
		});
		context.Bind("PED", "GET_PED_CONFIG_FLAG", [](const Values& args) -> Values {
			return {static_cast<bool>(PED::GET_PED_CONFIG_FLAG(static_cast<Ped>(Number(At(args, 0))), Integer(At(args, 1)), static_cast<BOOL>(At(args, 2).IsNil() ? 0 : Number(At(args, 2)))))};
		});
		context.Bind("PED", "GET_VEHICLE_PED_IS_IN", [](const Values& args) -> Values {
			return {PED::GET_VEHICLE_PED_IS_IN(static_cast<Ped>(Number(At(args, 0))), static_cast<BOOL>(At(args, 1).IsNil() ? 0 : Number(At(args, 1))))};
		});
		context.Bind("PED", "IS_PED_IN_ANY_VEHICLE", [](const Values& args) -> Values {
			return {static_cast<bool>(PED::IS_PED_IN_ANY_VEHICLE(static_cast<Ped>(Number(At(args, 0))), static_cast<BOOL>(At(args, 1).IsNil() ? 0 : Number(At(args, 1)))))};
		});
		context.Bind("PED", "IS_PED_SITTING_IN_ANY_VEHICLE", [](const Values& args) -> Values {
			return {static_cast<bool>(PED::IS_PED_SITTING_IN_ANY_VEHICLE(static_cast<Ped>(Number(At(args, 0)))))};
		});
		context.Bind("PED", "SET_PED_CONFIG_FLAG", [](const Values& args) -> Values {
			PED::SET_PED_CONFIG_FLAG(static_cast<Ped>(Number(At(args, 0))), Integer(At(args, 1)), static_cast<BOOL>(At(args, 2).IsNil() ? 0 : Number(At(args, 2))));
			return {};
		});
		context.Bind("PED", "SET_PED_COORDS_KEEP_VEHICLE", [](const Values& args) -> Values {
			PED::SET_PED_COORDS_KEEP_VEHICLE(static_cast<Ped>(Number(At(args, 0))), static_cast<float>(Number(At(args, 1))), static_cast<float>(Number(At(args, 2))), static_cast<float>(Number(At(args, 3))));
			return {};
		});
		context.Bind("PED", "SET_PED_INTO_VEHICLE", [](const Values& args) -> Values {
			PED::SET_PED_INTO_VEHICLE(static_cast<Ped>(Number(At(args, 0))), static_cast<Vehicle>(Number(At(args, 1))), Integer(At(args, 2)));
			return {};
		});
		context.Bind("PED", "SET_PED_TO_RAGDOLL", [](const Values& args) -> Values {
			return {static_cast<bool>(PED::SET_PED_TO_RAGDOLL(static_cast<Ped>(Number(At(args, 0))), Integer(At(args, 1)), Integer(At(args, 2)), Integer(At(args, 3)), static_cast<BOOL>(At(args, 4).IsNil() ? 0 : Number(At(args, 4))), static_cast<BOOL>(At(args, 5).IsNil() ? 0 : Number(At(args, 5))), static_cast<BOOL>(At(args, 6).IsNil() ? 0 : Number(At(args, 6)))))};
		});
		context.Bind("PLAYER", "GIVE_ACHIEVEMENT_TO_PLAYER", [](const Values& args) -> Values {
			return {static_cast<bool>(PLAYER::GIVE_ACHIEVEMENT_TO_PLAYER(Integer(At(args, 0))))};
		});
		context.Bind("PLAYER", "HAS_ACHIEVEMENT_BEEN_PASSED", [](const Values& args) -> Values {
			return {static_cast<bool>(PLAYER::HAS_ACHIEVEMENT_BEEN_PASSED(Integer(At(args, 0))))};
		});
		context.Bind("PLAYER", "PLAYER_ID", [](const Values& args) -> Values {
			return {PLAYER::PLAYER_ID()};
		});
		context.Bind("PLAYER", "PLAYER_PED_ID", [](const Values& args) -> Values {
			return {PLAYER::PLAYER_PED_ID()};
		});
		context.Bind("SCRIPT", "HAS_SCRIPT_LOADED", [](const Values& args) -> Values {
			auto text_0 = At(args, 0).IsNil() ? std::string{} : Text(At(args, 0));
			return {static_cast<bool>(SCRIPT::HAS_SCRIPT_LOADED(At(args, 0).IsNil() ? nullptr : text_0.c_str()))};
		});
		context.Bind("SCRIPT", "REQUEST_SCRIPT", [](const Values& args) -> Values {
			auto text_0 = At(args, 0).IsNil() ? std::string{} : Text(At(args, 0));
			SCRIPT::REQUEST_SCRIPT(At(args, 0).IsNil() ? nullptr : text_0.c_str());
			return {};
		});
		context.Bind("SCRIPT", "SET_SCRIPT_AS_NO_LONGER_NEEDED", [](const Values& args) -> Values {
			auto text_0 = At(args, 0).IsNil() ? std::string{} : Text(At(args, 0));
			SCRIPT::SET_SCRIPT_AS_NO_LONGER_NEEDED(At(args, 0).IsNil() ? nullptr : text_0.c_str());
			return {};
		});
		context.Bind("SCRIPT", "TERMINATE_THIS_THREAD", [](const Values& args) -> Values {
			SCRIPT::TERMINATE_THIS_THREAD();
			return {};
		});
		context.Bind("STATS", "STAT_SAVE", [](const Values& args) -> Values {
			return {static_cast<bool>(STATS::STAT_SAVE(Integer(At(args, 0)), static_cast<BOOL>(At(args, 1).IsNil() ? 0 : Number(At(args, 1))), Integer(At(args, 2)), static_cast<BOOL>(At(args, 3).IsNil() ? 0 : Number(At(args, 3)))))};
		});
		context.Bind("STREAMING", "HAS_ANIM_DICT_LOADED", [](const Values& args) -> Values {
			auto text_0 = At(args, 0).IsNil() ? std::string{} : Text(At(args, 0));
			return {static_cast<bool>(STREAMING::HAS_ANIM_DICT_LOADED(At(args, 0).IsNil() ? nullptr : text_0.c_str()))};
		});
		context.Bind("STREAMING", "IS_IPL_ACTIVE", [](const Values& args) -> Values {
			auto text_0 = At(args, 0).IsNil() ? std::string{} : Text(At(args, 0));
			return {static_cast<bool>(STREAMING::IS_IPL_ACTIVE(At(args, 0).IsNil() ? nullptr : text_0.c_str()))};
		});
		context.Bind("STREAMING", "IS_PLAYER_SWITCH_IN_PROGRESS", [](const Values& args) -> Values {
			return {static_cast<bool>(STREAMING::IS_PLAYER_SWITCH_IN_PROGRESS())};
		});
		context.Bind("STREAMING", "REMOVE_NAMED_PTFX_ASSET", [](const Values& args) -> Values {
			auto text_0 = At(args, 0).IsNil() ? std::string{} : Text(At(args, 0));
			STREAMING::REMOVE_NAMED_PTFX_ASSET(At(args, 0).IsNil() ? nullptr : text_0.c_str());
			return {};
		});
		context.Bind("STREAMING", "REMOVE_PTFX_ASSET", [](const Values& args) -> Values {
			STREAMING::REMOVE_PTFX_ASSET();
			return {};
		});
		context.Bind("STREAMING", "REQUEST_ANIM_DICT", [](const Values& args) -> Values {
			auto text_0 = At(args, 0).IsNil() ? std::string{} : Text(At(args, 0));
			STREAMING::REQUEST_ANIM_DICT(At(args, 0).IsNil() ? nullptr : text_0.c_str());
			return {};
		});
		context.Bind("STREAMING", "REQUEST_NAMED_PTFX_ASSET", [](const Values& args) -> Values {
			auto text_0 = At(args, 0).IsNil() ? std::string{} : Text(At(args, 0));
			STREAMING::REQUEST_NAMED_PTFX_ASSET(At(args, 0).IsNil() ? nullptr : text_0.c_str());
			return {};
		});
		context.Bind("TASK", "CLEAR_PED_TASKS", [](const Values& args) -> Values {
			TASK::CLEAR_PED_TASKS(static_cast<Ped>(Number(At(args, 0))));
			return {};
		});
		context.Bind("TASK", "CLEAR_PED_TASKS_IMMEDIATELY", [](const Values& args) -> Values {
			TASK::CLEAR_PED_TASKS_IMMEDIATELY(static_cast<Ped>(Number(At(args, 0))));
			return {};
		});
		context.Bind("TASK", "SET_DRIVE_TASK_DRIVING_STYLE", [](const Values& args) -> Values {
			TASK::SET_DRIVE_TASK_DRIVING_STYLE(static_cast<Ped>(Number(At(args, 0))), Integer(At(args, 1)));
			return {};
		});
		context.Bind("TASK", "STOP_ANIM_TASK", [](const Values& args) -> Values {
			auto text_1 = At(args, 1).IsNil() ? std::string{} : Text(At(args, 1));
			auto text_2 = At(args, 2).IsNil() ? std::string{} : Text(At(args, 2));
			TASK::STOP_ANIM_TASK(static_cast<Entity>(Number(At(args, 0))), At(args, 1).IsNil() ? nullptr : text_1.c_str(), At(args, 2).IsNil() ? nullptr : text_2.c_str(), static_cast<float>(Number(At(args, 3))));
			return {};
		});
		context.Bind("TASK", "TASK_PLAY_ANIM", [](const Values& args) -> Values {
			auto text_1 = At(args, 1).IsNil() ? std::string{} : Text(At(args, 1));
			auto text_2 = At(args, 2).IsNil() ? std::string{} : Text(At(args, 2));
			TASK::TASK_PLAY_ANIM(static_cast<Ped>(Number(At(args, 0))), At(args, 1).IsNil() ? nullptr : text_1.c_str(), At(args, 2).IsNil() ? nullptr : text_2.c_str(), static_cast<float>(Number(At(args, 3))), static_cast<float>(Number(At(args, 4))), Integer(At(args, 5)), Integer(At(args, 6)), static_cast<float>(Number(At(args, 7))), static_cast<BOOL>(At(args, 8).IsNil() ? 0 : Number(At(args, 8))), static_cast<BOOL>(At(args, 9).IsNil() ? 0 : Number(At(args, 9))), static_cast<BOOL>(At(args, 10).IsNil() ? 0 : Number(At(args, 10))));
			return {};
		});
		context.Bind("TASK", "TASK_VEHICLE_DRIVE_TO_COORD", [](const Values& args) -> Values {
			TASK::TASK_VEHICLE_DRIVE_TO_COORD(static_cast<Ped>(Number(At(args, 0))), static_cast<Vehicle>(Number(At(args, 1))), static_cast<float>(Number(At(args, 2))), static_cast<float>(Number(At(args, 3))), static_cast<float>(Number(At(args, 4))), static_cast<float>(Number(At(args, 5))), static_cast<Any>(Number(At(args, 6))), Hash(At(args, 7)), Integer(At(args, 8)), static_cast<float>(Number(At(args, 9))), static_cast<float>(Number(At(args, 10))));
			return {};
		});
		context.Bind("TASK", "TASK_VEHICLE_DRIVE_WANDER", [](const Values& args) -> Values {
			TASK::TASK_VEHICLE_DRIVE_WANDER(static_cast<Ped>(Number(At(args, 0))), static_cast<Vehicle>(Number(At(args, 1))), static_cast<float>(Number(At(args, 2))), Integer(At(args, 3)));
			return {};
		});
		context.Bind("VEHICLE", "BRING_VEHICLE_TO_HALT", [](const Values& args) -> Values {
			VEHICLE::BRING_VEHICLE_TO_HALT(static_cast<Vehicle>(Number(At(args, 0))), static_cast<float>(Number(At(args, 1))), Integer(At(args, 2)), static_cast<BOOL>(At(args, 3).IsNil() ? 0 : Number(At(args, 3))));
			return {};
		});
		context.Bind("VEHICLE", "GET_PED_IN_VEHICLE_SEAT", [](const Values& args) -> Values {
			return {VEHICLE::GET_PED_IN_VEHICLE_SEAT(static_cast<Vehicle>(Number(At(args, 0))), Integer(At(args, 1)), static_cast<BOOL>(At(args, 2).IsNil() ? 0 : Number(At(args, 2))))};
		});
		context.Bind("VEHICLE", "GET_RANDOM_VEHICLE_IN_SPHERE", [](const Values& args) -> Values {
			return {VEHICLE::GET_RANDOM_VEHICLE_IN_SPHERE(static_cast<float>(Number(At(args, 0))), static_cast<float>(Number(At(args, 1))), static_cast<float>(Number(At(args, 2))), static_cast<float>(Number(At(args, 3))), Hash(At(args, 4)), Integer(At(args, 5)))};
		});
		context.Bind("VEHICLE", "IS_VEHICLE_STOPPED_AT_TRAFFIC_LIGHTS", [](const Values& args) -> Values {
			return {static_cast<bool>(VEHICLE::IS_VEHICLE_STOPPED_AT_TRAFFIC_LIGHTS(static_cast<Vehicle>(Number(At(args, 0)))))};
		});
		context.Bind("VEHICLE", "SET_VEHICLE_CUSTOM_PRIMARY_COLOUR", [](const Values& args) -> Values {
			VEHICLE::SET_VEHICLE_CUSTOM_PRIMARY_COLOUR(static_cast<Vehicle>(Number(At(args, 0))), Integer(At(args, 1)), Integer(At(args, 2)), Integer(At(args, 3)));
			return {};
		});
		context.Bind("VEHICLE", "SET_VEHICLE_CUSTOM_SECONDARY_COLOUR", [](const Values& args) -> Values {
			VEHICLE::SET_VEHICLE_CUSTOM_SECONDARY_COLOUR(static_cast<Vehicle>(Number(At(args, 0))), Integer(At(args, 1)), Integer(At(args, 2)), Integer(At(args, 3)));
			return {};
		});
		context.Bind("VEHICLE", "SET_VEHICLE_ENGINE_ON", [](const Values& args) -> Values {
			VEHICLE::SET_VEHICLE_ENGINE_ON(static_cast<Vehicle>(Number(At(args, 0))), static_cast<BOOL>(At(args, 1).IsNil() ? 0 : Number(At(args, 1))), static_cast<BOOL>(At(args, 2).IsNil() ? 0 : Number(At(args, 2))), static_cast<BOOL>(At(args, 3).IsNil() ? 0 : Number(At(args, 3))));
			return {};
		});
		context.Bind("VEHICLE", "SET_VEHICLE_EXTRA_COLOURS", [](const Values& args) -> Values {
			VEHICLE::SET_VEHICLE_EXTRA_COLOURS(static_cast<Vehicle>(Number(At(args, 0))), Integer(At(args, 1)), Integer(At(args, 2)));
			return {};
		});
		context.Bind("VEHICLE", "SET_VEHICLE_MAX_SPEED", [](const Values& args) -> Values {
			VEHICLE::SET_VEHICLE_MAX_SPEED(static_cast<Vehicle>(Number(At(args, 0))), static_cast<float>(Number(At(args, 1))));
			return {};
		});
		context.Bind("VEHICLE", "SET_VEHICLE_MOD", [](const Values& args) -> Values {
			VEHICLE::SET_VEHICLE_MOD(static_cast<Vehicle>(Number(At(args, 0))), Integer(At(args, 1)), Integer(At(args, 2)), static_cast<BOOL>(At(args, 3).IsNil() ? 0 : Number(At(args, 3))));
			return {};
		});
		context.Bind("VEHICLE", "SET_VEHICLE_MOD_KIT", [](const Values& args) -> Values {
			VEHICLE::SET_VEHICLE_MOD_KIT(static_cast<Vehicle>(Number(At(args, 0))), Integer(At(args, 1)));
			return {};
		});
		context.Bind("VEHICLE", "SET_VEHICLE_NEON_COLOUR", [](const Values& args) -> Values {
			VEHICLE::SET_VEHICLE_NEON_COLOUR(static_cast<Vehicle>(Number(At(args, 0))), Integer(At(args, 1)), Integer(At(args, 2)), Integer(At(args, 3)));
			return {};
		});
		context.Bind("VEHICLE", "SET_VEHICLE_NEON_ENABLED", [](const Values& args) -> Values {
			VEHICLE::SET_VEHICLE_NEON_ENABLED(static_cast<Vehicle>(Number(At(args, 0))), Integer(At(args, 1)), static_cast<BOOL>(At(args, 2).IsNil() ? 0 : Number(At(args, 2))));
			return {};
		});
		context.Bind("VEHICLE", "SET_VEHICLE_NUMBER_PLATE_TEXT_INDEX", [](const Values& args) -> Values {
			VEHICLE::SET_VEHICLE_NUMBER_PLATE_TEXT_INDEX(static_cast<Vehicle>(Number(At(args, 0))), Integer(At(args, 1)));
			return {};
		});
		context.Bind("VEHICLE", "SET_VEHICLE_TYRE_SMOKE_COLOR", [](const Values& args) -> Values {
			VEHICLE::SET_VEHICLE_TYRE_SMOKE_COLOR(static_cast<Vehicle>(Number(At(args, 0))), Integer(At(args, 1)), Integer(At(args, 2)), Integer(At(args, 3)));
			return {};
		});
		context.Bind("VEHICLE", "SET_VEHICLE_UNDRIVEABLE", [](const Values& args) -> Values {
			VEHICLE::SET_VEHICLE_UNDRIVEABLE(static_cast<Vehicle>(Number(At(args, 0))), static_cast<BOOL>(At(args, 1).IsNil() ? 0 : Number(At(args, 1))));
			return {};
		});
		context.Bind("VEHICLE", "SET_VEHICLE_WHEEL_TYPE", [](const Values& args) -> Values {
			VEHICLE::SET_VEHICLE_WHEEL_TYPE(static_cast<Vehicle>(Number(At(args, 0))), Integer(At(args, 1)));
			return {};
		});
		context.Bind("VEHICLE", "SET_VEHICLE_XENON_LIGHT_COLOR_INDEX", [](const Values& args) -> Values {
			VEHICLE::SET_VEHICLE_XENON_LIGHT_COLOR_INDEX(static_cast<Vehicle>(Number(At(args, 0))), Integer(At(args, 1)));
			return {};
		});
		context.Bind("VEHICLE", "TOGGLE_VEHICLE_MOD", [](const Values& args) -> Values {
			VEHICLE::TOGGLE_VEHICLE_MOD(static_cast<Vehicle>(Number(At(args, 0))), Integer(At(args, 1)), static_cast<BOOL>(At(args, 2).IsNil() ? 0 : Number(At(args, 2))));
			return {};
		});
		context.Bind("WEAPON", "GET_SELECTED_PED_WEAPON", [](const Values& args) -> Values {
			return {static_cast<std::int32_t>(WEAPON::GET_SELECTED_PED_WEAPON(static_cast<Ped>(Number(At(args, 0)))))};
		});
		context.Bind("WEAPON", "GIVE_WEAPON_TO_PED", [](const Values& args) -> Values {
			WEAPON::GIVE_WEAPON_TO_PED(static_cast<Ped>(Number(At(args, 0))), Hash(At(args, 1)), Integer(At(args, 2)), static_cast<BOOL>(At(args, 3).IsNil() ? 0 : Number(At(args, 3))), static_cast<BOOL>(At(args, 4).IsNil() ? 0 : Number(At(args, 4))));
			return {};
		});
		context.Bind("WEAPON", "HAS_PED_GOT_WEAPON", [](const Values& args) -> Values {
			return {static_cast<bool>(WEAPON::HAS_PED_GOT_WEAPON(static_cast<Ped>(Number(At(args, 0))), Hash(At(args, 1)), static_cast<BOOL>(At(args, 2).IsNil() ? 0 : Number(At(args, 2)))))};
		});
		context.Bind("WEAPON", "SET_CURRENT_PED_WEAPON", [](const Values& args) -> Values {
			WEAPON::SET_CURRENT_PED_WEAPON(static_cast<Ped>(Number(At(args, 0))), Hash(At(args, 1)), static_cast<BOOL>(At(args, 2).IsNil() ? 0 : Number(At(args, 2))));
			return {};
		});
	}
}

    // Native statistics, tunables and transactions.
    // Statistics and tunables use the menu's existing native implementations.
	#include "common.hpp"
	#include "TinkerScriptData.hpp"
	#include "game/gta/Stats.hpp"
	#include "game/backend/Tunables.hpp"
	#include "game/backend/AnticheatBypass.hpp"
	#include "game/gta/Natives.hpp"
	#include "game/gta/Scripts.hpp"
namespace YimMenu::TinkerScript
{
	void BindServices(Context& context)
	{
		context.Bind("stats", "get_bool", [](const Values& args) -> Values {
			return {YimMenu::Stats::GetBool(Text(args[0]))};
		});
		context.Bind("stats", "get_int", [](const Values& args) -> Values {
			return {YimMenu::Stats::GetInt(Text(args[0]))};
		});
		context.Bind("stats", "get_masked_bool", [](const Values& args) -> Values {
			return {YimMenu::Stats::GetMaskedBool(Text(args[0]), Integer(args[1]))};
		});
		context.Bind("stats", "get_masked_int", [](const Values& args) -> Values {
			return {YimMenu::Stats::GetMaskedInt(Text(args[0]), Integer(args[1]), Integer(args[2]))};
		});
		context.Bind("stats", "get_packed_bool", [](const Values& args) -> Values {
			return {YimMenu::Stats::GetPackedBool(Integer(args[0]))};
		});
		context.Bind("stats", "get_packed_int", [](const Values& args) -> Values {
			return {YimMenu::Stats::GetPackedInt(Integer(args[0]))};
		});
		context.Bind("stats", "get_string", [](const Values& args) -> Values {
			return {YimMenu::Stats::GetString(Text(args[0]))};
		});
		context.Bind("stats", "set_bool", [](const Values& args) -> Values {
			YimMenu::Stats::SetBool(Text(args[0]), Truthy(args[1]));
			return {};
		});
		context.Bind("stats", "set_float", [](const Values& args) -> Values {
			YimMenu::Stats::SetFloat(Text(args[0]), static_cast<float>(Number(args[1])));
			return {};
		});
		context.Bind("stats", "set_int", [](const Values& args) -> Values {
			YimMenu::Stats::SetInt(Text(args[0]), Integer(args[1]));
			return {};
		});
		context.Bind("stats", "set_masked_bool", [](const Values& args) -> Values {
			YimMenu::Stats::SetMaskedBool(Text(args[0]), Integer(args[1]), Truthy(args[2]));
			return {};
		});
		context.Bind("stats", "set_masked_int", [](const Values& args) -> Values {
			YimMenu::Stats::SetMaskedInt(Text(args[0]), Integer(args[1]), Integer(args[2]), Integer(args[3]));
			return {};
		});
		context.Bind("stats", "set_packed_bool", [](const Values& args) -> Values {
			YimMenu::Stats::SetPackedBool(Integer(args[0]), Truthy(args[1]));
			return {};
		});
		context.Bind("stats", "set_packed_bool_range", [](const Values& args) -> Values {
			YimMenu::Stats::SetPackedBoolRange(Integer(args[0]), Integer(args[1]), Truthy(args[2]));
			return {};
		});
		context.Bind("stats", "set_packed_int", [](const Values& args) -> Values {
			YimMenu::Stats::SetPackedInt(Integer(args[0]), Integer(args[1]));
			return {};
		});
		context.Bind("stats", "set_string", [](const Values& args) -> Values {
			YimMenu::Stats::SetString(Text(args[0]), Text(args[1]).c_str());
			return {};
		});
		context.Bind("tunables", "get_int", [](const Values& args) -> Values {
			auto tunable = YimMenu::Tunable(Hash(args[0]));
			if (!tunable.IsReady())
				return {0};
			return {tunable.Get<int>()};
		});
		context.Bind("tunables", "set_bool", [](const Values& args) -> Values {
			auto tunable = YimMenu::Tunable(Hash(args[0]));
			if (!tunable.IsReady())
				return {};
			tunable.Set<int>(Integer(args[1]));
			return {};
		});
		context.Bind("tunables", "set_float", [](const Values& args) -> Values {
			auto tunable = YimMenu::Tunable(Hash(args[0]));
			if (!tunable.IsReady())
				return {};
			tunable.Set<float>(static_cast<float>(Number(args[1])));
			return {};
		});
		context.Bind("tunables", "set_int", [](const Values& args) -> Values {
			auto tunable = YimMenu::Tunable(Hash(args[0]));
			if (!tunable.IsReady())
				return {};
			tunable.Set<int>(Integer(args[1]));
			return {};
		});

		context.Bind("transactions", "can_use_transactions", [](const Values&) -> Values {
			return {!AnticheatBypass::IsFSLProvidingLocalSaves() && NETSHOPPING::NET_GAMESERVER_CATALOG_IS_VALID()};
		});
		context.Bind("transactions", "run_service", [](const Values& args) -> Values {
			auto category = Hash(args[0]), action = Hash(args[1]), item = Hash(args[2]);
			int value = Integer(args[3]);
			const std::uint32_t excluded[] = {"SERVICE_EARN_DEBUG"_J, "SERVICE_EARN_JBONUS"_J, "SERVICE_EARN_JBONUS_MODEL"_J, "SERVICE_EARN_JBONUS_NO_DEATH"_J, "SERVICE_EARN_JBONUS_NO_FALL"_J, "SERVICE_EARN_JBONUS_NOT_SEEN"_J, "SERVICE_EARN_JBONUS_SE"_J, "SERVICE_EARN_EVOLVE"_J, "SERVICE_EARN_IMPULSE"_J};
			if (std::ranges::find(excluded, item) != std::end(excluded))
				throw std::runtime_error("Unsupported transaction service");
			if (category != "CATEGORY_SERVICE"_J && category != "CATEGORY_SERVICE_WITH_LIMIT"_J && category != "CATEGORY_SERVICE_WITH_THRESHOLD"_J)
				throw std::runtime_error("Invalid transaction category");
			if (action != "NET_SHOP_ACTION_EARN"_J && action != "NET_SHOP_ACTION_SPEND"_J)
				throw std::runtime_error("Invalid transaction action");
			auto* thread = Scripts::FindScriptThread("shop_controller"_J);
			if (!thread)
				return {false};
			bool success = false;
			Scripts::RunAsScript(thread, [&] {
				int id = 0;
				if (NETSHOPPING::NET_GAMESERVER_BASKET_IS_ACTIVE())
					NETSHOPPING::NET_GAMESERVER_BASKET_END();
				success = NETSHOPPING::NET_GAMESERVER_BEGIN_SERVICE(&id, category, item, action, value, 4) && NETSHOPPING::NET_GAMESERVER_CHECKOUT_START(id);
				if (!success)
					NETSHOPPING::NET_GAMESERVER_BASKET_END();
			});
			return {success};
		});
	}
}


    // The elevated Windows Firewall helper.
    // The elevated helper uses Windows Firewall directly and installs no batch files.
	#include "common.hpp"
	#include "TinkerScriptData.hpp"
	#include "core/filemgr/FileMgr.hpp"
	#include <netfw.h>
	#include <shellapi.h>
	#include <shlobj.h>

namespace YimMenu::TinkerScript
{
	// Keep COM ownership independent of WRL so the existing cross-compiler job can build this code.
	template<class T>
	class ComObject
	{
		T* object = nullptr;

	public:
		~ComObject()
		{
			if (object)
				object->Release();
		}
		T* operator->() const
		{
			return object;
		}
		T* Get() const
		{
			return object;
		}
		T** Put()
		{
			return &object;
		}
	};
	static constexpr GUID PolicyClass = {0xe2b3c97f, 0x6ae1, 0x41ac, {0x81, 0x7a, 0xf6, 0xf9, 0x21, 0x66, 0xd7, 0xdd}};
	static constexpr GUID PolicyInterface = {0x98325047, 0xc671, 0x4174, {0x8d, 0x81, 0xde, 0xfc, 0xd3, 0xf0, 0x31, 0x86}};
	static constexpr GUID RuleClass = {0x2c5bc43e, 0x3369, 0x4c33, {0xab, 0x0c, 0xbe, 0x94, 0x69, 0x67, 0x7a, 0xf4}};
	static constexpr GUID RuleInterface = {0xaf230d27, 0xbaba, 0x4e42, {0xac, 0xed, 0xf5, 0x24, 0xf2, 0x2c, 0xfc, 0xe2}};
	static constexpr wchar_t RuleName[] = L"Disable GTA Online Cloud Saves";
	static HANDLE helperProcess = nullptr;
	static std::filesystem::path helperState;
	static std::filesystem::path StatePath()
	{
		// Preserve the game's data path when elevation uses another Windows account.
		return helperState.empty() ? FileMgr::GetProjectFolder("./TinkerScripts").Path() / "NoSave.txt" : helperState;
	}

	static void WriteState(std::string_view state)
	{
		std::ofstream output(StatePath(), std::ios::binary | std::ios::trunc);
		output << state;
		if (!output)
			throw std::runtime_error("Unable to write cloud-save state");
	}

	static bool ChangeRule(bool disable)
	{
		ComObject<INetFwPolicy2> policy;
		if (FAILED(CoCreateInstance(PolicyClass, nullptr, CLSCTX_INPROC_SERVER, PolicyInterface, reinterpret_cast<void**>(policy.Put()))))
			return false;
		ComObject<INetFwRules> rules;
		if (FAILED(policy->get_Rules(rules.Put())))
			return false;
		BSTR name = SysAllocString(RuleName);
		// Replacing this rule retains the original tool's fixed outbound IP list.
		auto removed = rules->Remove(name);
		if (!disable)
		{
			SysFreeString(name);
			return SUCCEEDED(removed) || removed == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
		}
		ComObject<INetFwRule> rule;
		HRESULT result = CoCreateInstance(RuleClass, nullptr, CLSCTX_INPROC_SERVER, RuleInterface, reinterpret_cast<void**>(rule.Put()));
		if (SUCCEEDED(result))
			result = rule->put_Name(name);
		SysFreeString(name);
		BSTR addresses = SysAllocString(L"192.81.241.170,192.81.241.171,193.82.242.172");
		if (SUCCEEDED(result))
			result = rule->put_RemoteAddresses(addresses);
		SysFreeString(addresses);
		if (SUCCEEDED(result))
			result = rule->put_Direction(NET_FW_RULE_DIR_OUT);
		if (SUCCEEDED(result))
			result = rule->put_Protocol(NET_FW_IP_PROTOCOL_ANY);
		if (SUCCEEDED(result))
			result = rule->put_Action(NET_FW_ACTION_BLOCK);
		if (SUCCEEDED(result))
			result = rule->put_Profiles(NET_FW_PROFILE2_ALL);
		if (SUCCEEDED(result))
			result = rule->put_Enabled(VARIANT_TRUE);
		if (SUCCEEDED(result))
			result = rules->Add(rule.Get());
		return SUCCEEDED(result);
	}

	void StartCloudSaveTool()
	{
		if (helperProcess && WaitForSingleObject(helperProcess, 0) == WAIT_TIMEOUT)
			return;
		if (helperProcess)
		{
			CloseHandle(helperProcess);
			helperProcess = nullptr;
		}
		std::wstring path(32768, L'\0');
		auto length = GetModuleFileNameW(g_DllInstance, path.data(), static_cast<DWORD>(path.size()));
		if (!length || length >= path.size())
			throw std::runtime_error("Unable to find the menu DLL");
		path.resize(length);
		auto parameters = L"\"" + path + L"\",TinkerCloudSaveHelperW " + std::to_wstring(GetCurrentProcessId()) + L" \"" + StatePath().wstring() + L"\"";
		SHELLEXECUTEINFOW launch{sizeof(launch)};
		launch.fMask = SEE_MASK_NOCLOSEPROCESS;
		launch.lpVerb = L"runas";
		launch.lpFile = L"rundll32.exe";
		launch.lpParameters = parameters.c_str();
		launch.nShow = SW_HIDE;
		if (!ShellExecuteExW(&launch))
			throw std::runtime_error("The cloud-save tool was not started");
		helperProcess = launch.hProcess;
	}

	void RequestCloudSave(std::string command)
	{
		if (command != "Enable" && command != "Disable" && command != "Close")
			throw std::runtime_error("Invalid cloud-save request");
		WriteState(command);
		// Requests remain visible until the helper confirms the firewall operation.
	}

	void CloseCloudSaves()
	{
		if (helperProcess)
		{
			CloseHandle(helperProcess);
			helperProcess = nullptr;
		}
	}

	static void RunCloudSaveHelper(DWORD parentId, const std::filesystem::path& state)
	{
		helperState = state;
		auto initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
		if (FAILED(initialized))
			return;
		HANDLE parent = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, parentId);
		wchar_t image[32768]{};
		DWORD length = static_cast<DWORD>(std::size(image));
		if (!parent || !QueryFullProcessImageNameW(parent, 0, image, &length) || _wcsicmp(std::filesystem::path(image).filename().c_str(), L"GTA5_Enhanced.exe") != 0)
		{
			if (parent)
				CloseHandle(parent);
			CoUninitialize();
			return;
		}
		try
		{
			// Each helper owns the same rule as the original tool and removes it on exit.
			if (!ChangeRule(false))
				throw std::runtime_error("Unable to prepare the cloud-save rule");
			WriteState("");
			while (parent && WaitForSingleObject(parent, 200) == WAIT_TIMEOUT)
			{
				std::ifstream input(StatePath(), std::ios::binary);
				std::string state(std::istreambuf_iterator<char>(input), {});
				if (state == "Close")
					break;
				if (state == "Disable")
					WriteState(ChangeRule(true) ? "Disabled" : "Error");
				else if (state == "Enable")
					WriteState(ChangeRule(false) ? "Enabled" : "Error");
			}
			WriteState(ChangeRule(false) ? "Closed" : "Error");
		}
		catch (...)
		{
			ChangeRule(false);
			try
			{
				WriteState("Error");
			}
			catch (...)
			{
			}
		}
		if (parent)
			CloseHandle(parent);
		CoUninitialize();
	}
}

// rundll32 supplies an elevated process for the firewall work, without a separate executable.
extern "C" __declspec(dllexport) void CALLBACK TinkerCloudSaveHelperW(HWND, HINSTANCE, LPWSTR command, int)
{
	int count = 0;
	auto arguments = std::wstring(L"TinkerCloudSaveHelperW ") + command;
	auto argv = CommandLineToArgvW(arguments.c_str(), &count);
	if (!argv)
		return;
	if (count == 3)
	{
		wchar_t* end = nullptr;
		auto process = std::wcstoul(argv[1], &end, 10);
		std::filesystem::path state(argv[2]);
		if (process && end && *end == '\0' && state.is_absolute() && state.filename() == L"NoSave.txt" && state.parent_path().filename() == L"TinkerScripts")
			YimMenu::TinkerScript::RunCloudSaveHelper(static_cast<DWORD>(process), state);
	}
	LocalFree(argv);
}


    // Registration, worker fibers and menu lifetime.
    // Native menu integration for TinkerScripts v1.0.1 by ImagineNothing.
    // Copyright (C) 2026 ImagineNothing; GPL-3.0-or-later.
	#include "common.hpp"
	#include "TinkerScript.hpp"
	#include "TinkerScriptData.hpp"
	#include "core/backend/FiberPool.hpp"
	#include "core/backend/ScriptMgr.hpp"
	#include "core/commands/Commands.hpp"
	#include "core/commands/BoolCommand.hpp"
	#include "core/commands/LoopedCommand.hpp"
	#include "core/commands/ListCommand.hpp"
	#include "core/commands/IntCommand.hpp"
	#include "core/commands/FloatCommand.hpp"
	#include "core/filemgr/FileMgr.hpp"
	#include "core/frontend/Notifications.hpp"
	#include "core/frontend/manager/UIManager.hpp"
	#include "core/frontend/manager/Submenu.hpp"
	#include "core/renderer/Renderer.hpp"
	#include "core/memory/ModuleMgr.hpp"
	#include "core/memory/Pattern.hpp"
	#include "core/memory/PatternScanner.hpp"
	#include "game/frontend/items/Items.hpp"
	#include "game/gta/Natives.hpp"
	#include "game/gta/Stats.hpp"
	#include "game/gta/ScriptGlobal.hpp"
	#include "game/gta/ScriptLocal.hpp"
	#include "game/gta/Pools.hpp"
	#include "game/gta/Network.hpp"
	#include "game/backend/Self.hpp"
	#include "game/backend/Tunables.hpp"
	#include "game/backend/AnticheatBypass.hpp"
	#include "game/pointers/Pointers.hpp"
	#include <fstream>

namespace YimMenu::TinkerScript
{
	void BindGameNatives(Context& context);
	void BindRenderApi(Context& context);
	void BindServices(Context& context);
	void BindCloudSaves(Context& context);
	void CloseCloudSaves();
	static Context context;
	static std::atomic<bool> ready = false;
	static std::vector<std::unique_ptr<Command>> commands;
	static std::vector<std::shared_ptr<Submenu>> submenus;
	static std::vector<Value> alwaysDraw, unloadCallbacks;
	static std::string menuIcon;

	// UI callbacks and game fibers share state; release the lock while a fiber yields.
	static void Execute(const Value& callback, const Values& args = {})
	{
		if (callback.IsNil())
			return;
		std::lock_guard lock(context.mutex);
		try
		{
			Call(callback, args);
		}
		catch (const std::exception& error)
		{
			LOG(WARNING) << "TinkerScripts: " << error.what();
		}
	}

	static void Queue(Value callback, Values args = {})
	{
		if (callback.IsNil())
			return;
		FiberPool::Push([callback = std::move(callback), args = std::move(args)] {
			if (ready)
				Execute(callback, args);
		});
	}

	class TinkerToggle : public BoolCommand
	{
		Value enable, disable;
		void OnEnable() override
		{
			Execute(enable);
		}
		void OnDisable() override
		{
			Execute(disable);
		}

	public:
		explicit TinkerToggle(const Values& args) :
		    BoolCommand(Text(args[0]), Text(args[1]), Text(args[2]), Truthy(At(args, 3))),
		    enable(At(args, 4)),
		    disable(At(args, 5))
		{
		}
	};

	class TinkerLoop : public LoopedCommand
	{
		Value tick, enable, disable;
		std::shared_ptr<std::atomic<bool>> pending = std::make_shared<std::atomic<bool>>(false);
		void OnTick() override
		{
			// A yielding tick gets its own worker fiber and does not stall other commands.
			if (pending->exchange(true))
				return;
			FiberPool::Push([callback = tick, pending = pending] {
				if (ready)
					Execute(callback);
				pending->store(false);
			});
		}
		void OnEnable() override
		{
			Execute(enable);
		}
		void OnDisable() override
		{
			Execute(disable);
		}

	public:
		explicit TinkerLoop(const Values& args) :
		    LoopedCommand(Text(args[0]), Text(args[1]), Text(args[2])),
		    tick(At(args, 3)),
		    enable(At(args, 4)),
		    disable(At(args, 5))
		{
		}
	};

	class TinkerList : public ListCommand
	{
		std::vector<std::string> labels;
		Value changed;
		void OnChange() override
		{
			Queue(changed, {GetState()});
		}

	public:
		explicit TinkerList(const Values& args) :
		    ListCommand(Text(args[0]), Text(args[1]), Text(args[2]), {}, At(args, 4).IsNil() ? 0 : Integer(args[4])),
		    changed(At(args, 5))
		{
			const auto count = Integer(Length(args[3]));
			labels.reserve(count);
			for (int i = 1; i <= count; ++i)
				labels.push_back(Text(Member(Member(args[3], i), 2)));
			std::vector<std::pair<int, const char*>> entries;
			for (int i = 1; i <= count; ++i)
				entries.emplace_back(Integer(Member(Member(args[3], i), 1)), labels[i - 1].c_str());
			SetList(std::move(entries));
		}
	};

	// Handles resolve through the existing command registry instead of owning a second value.
	static Value CommandHandle(joaat_t hash)
	{
		auto* command = Commands::GetCommand(hash);
		if (!command)
			return {};
		auto handle = MakeTable();
		Member(handle, "get_value") = MakeFunction([hash](const Values&) -> Values {
			auto* command = Commands::GetCommand(hash);
			if (auto* list = dynamic_cast<ListCommand*>(command))
				return {list->GetState()};
			if (auto* integer = dynamic_cast<IntCommand*>(command))
				return {integer->GetState()};
			if (auto* number = dynamic_cast<FloatCommand*>(command))
				return {number->GetState()};
			if (auto* toggle = dynamic_cast<BoolCommand*>(command))
				return {toggle->GetState()};
			return {};
		});
		Member(handle, "set_value") = MakeFunction([hash](const Values& args) -> Values {
			auto* command = Commands::GetCommand(hash);
			if (auto* list = dynamic_cast<ListCommand*>(command))
				list->SetState(Integer(args[1]));
			else if (auto* integer = dynamic_cast<IntCommand*>(command))
				integer->SetState(Integer(args[1]));
			else if (auto* number = dynamic_cast<FloatCommand*>(command))
				number->SetState(static_cast<float>(Number(args[1])));
			else if (auto* toggle = dynamic_cast<BoolCommand*>(command))
				toggle->SetState(Truthy(args[1]));
			return {};
		});
		Member(handle, "draw") = MakeFunction([hash](const Values&) -> Values {
			auto* command = Commands::GetCommand(hash);
			if (dynamic_cast<ListCommand*>(command))
				ListCommandItem(hash).Draw();
			else if (dynamic_cast<IntCommand*>(command))
				IntCommandItem(hash).Draw();
			else if (dynamic_cast<FloatCommand*>(command))
				FloatCommandItem(hash).Draw();
			else if (dynamic_cast<BoolCommand*>(command))
				BoolCommandItem(hash).Draw();
			else
				CommandItem(hash).Draw();
			return {};
		});
		Member(handle, "enable") = MakeFunction([hash](const Values&) -> Values {
			Commands::GetCommand<BoolCommand>(hash)->SetState(true);
			return {};
		});
		Member(handle, "disable") = MakeFunction([hash](const Values&) -> Values {
			Commands::GetCommand<BoolCommand>(hash)->SetState(false);
			return {};
		});
		return handle;
	}

	static Value VectorValue(const rage::fvector3& vector)
	{
		return MakeTable({{"x", vector.x}, {"y", vector.y}, {"z", vector.z}});
	}
	static rage::fvector3 ReadVector(const Value& vector)
	{
		return {static_cast<float>(Number(Member(vector, "x"))), static_cast<float>(Number(Member(vector, "y"))), static_cast<float>(Number(Member(vector, "z")))};
	}

	// Global and local accesses are checked each time because scripts can stop during a yield.
	template<class T>
	static Value ScriptAddress(T address)
	{
		auto value = MakeTable();
		Member(value, "at") = MakeFunction([address](const Values& args) -> Values {
			return {ScriptAddress(args.size() > 2 && Number(args[2]) != 0 ? address.At(static_cast<std::ptrdiff_t>(Number(args[1])), static_cast<std::size_t>(Number(args[2]))) : address.At(static_cast<std::ptrdiff_t>(Number(args[1]))))};
		});
		Member(value, "get_int") = MakeFunction([address](const Values&) mutable -> Values {
			return {address.CanAccess() ? *address.template As<int*>() : 0};
		});
		Member(value, "get_float") = MakeFunction([address](const Values&) mutable -> Values {
			return {address.CanAccess() ? *address.template As<float*>() : 0.0f};
		});
		Member(value, "get_string") = MakeFunction([address](const Values&) mutable -> Values {
			return {address.CanAccess() ? address.template As<char*>() : nullptr};
		});
		Member(value, "get_vector3") = MakeFunction([address](const Values&) mutable -> Values {
			return {VectorValue(address.CanAccess() ? rage::fvector3(*address.template As<rage::scrVector*>()) : rage::fvector3{})};
		});
		Member(value, "set_int") = MakeFunction([address](const Values& args) mutable -> Values {
			if (address.CanAccess())
				*address.template As<int*>() = Integer(args[1]);
			return {};
		});
		Member(value, "set_float") = MakeFunction([address](const Values& args) mutable -> Values {
			if (address.CanAccess())
				*address.template As<float*>() = static_cast<float>(Number(args[1]));
			return {};
		});
		Member(value, "set_vector3") = MakeFunction([address](const Values& args) mutable -> Values {
			if (address.CanAccess())
				*address.template As<rage::scrVector*>() = rage::scrVector(ReadVector(args[1]));
			return {};
		});
		Member(value, "set_string") = MakeFunction([address](const Values& args) mutable -> Values {
			if (address.CanAccess())
			{
				auto text = Text(args[1]);
				auto capacity = args.size() > 2 ? static_cast<std::size_t>(Number(args[2])) : text.size() + 1;
				if (capacity)
				{
					auto count = std::min(text.size(), capacity - 1);
					auto* destination = address.template As<char*>();
					std::memcpy(destination, text.data(), count);
					destination[count] = '\0';
				}
			}
			return {};
		});
		return value;
	}

	static Value EntityValue(int handle)
	{
		auto value = MakeTable();
		Member(value, "is_valid") = MakeFunction([handle](const Values&) -> Values {
			return {YimMenu::Entity(handle).IsValid()};
		});
		Member(value, "is_dead") = MakeFunction([handle](const Values&) -> Values {
			return {YimMenu::Entity(handle).IsDead()};
		});
		Member(value, "is_enemy") = MakeFunction([handle](const Values&) -> Values {
			return {YimMenu::Ped(handle).IsEnemy()};
		});
		Member(value, "get_model") = MakeFunction([handle](const Values&) -> Values {
			return {static_cast<std::int32_t>(YimMenu::Entity(handle).GetModel())};
		});
		Member(value, "get_position") = MakeFunction([handle](const Values&) -> Values {
			return {VectorValue(YimMenu::Entity(handle).GetPosition())};
		});
		Member(value, "get_speed") = MakeFunction([handle](const Values&) -> Values {
			return {YimMenu::Entity(handle).GetSpeed()};
		});
		Member(value, "delete") = MakeFunction([handle](const Values&) -> Values {
			YimMenu::Entity(handle).Delete();
			return {};
		});
		Member(value, "kill") = MakeFunction([handle](const Values&) -> Values {
			YimMenu::Entity(handle).Kill();
			return {};
		});
		Member(value, "set_invincible") = MakeFunction([handle](const Values& args) -> Values {
			YimMenu::Entity(handle).SetInvincible(Truthy(args[1]));
			return {};
		});
		Member(value, "set_plate_text") = MakeFunction([handle](const Values& args) -> Values {
			YimMenu::Vehicle(handle).SetPlateText(Text(args[1]));
			return {};
		});
		Member(value, "is_seat_free") = MakeFunction([handle](const Values& args) -> Values {
			return {YimMenu::Vehicle(handle).IsSeatFree(Integer(args[1]))};
		});
		return value;
	}

	static Value PointerValue(std::uintptr_t address)
	{
		if (!address)
			return {};
		auto pointer = MakeTable();
		Member(pointer, "add") = MakeFunction([address](const Values& args) -> Values {
			return {PointerValue(address + static_cast<std::ptrdiff_t>(Number(args[1])))};
		});
		Member(pointer, "deref") = MakeFunction([address](const Values&) -> Values {
			return {PointerValue(*reinterpret_cast<std::uintptr_t*>(address))};
		});
		Member(pointer, "get_string") = MakeFunction([address](const Values&) -> Values {
			return {reinterpret_cast<const char*>(address)};
		});
		Member(pointer, "set_string") = MakeFunction([address](const Values& args) -> Values {
			auto text = Text(args[1]);
			std::memcpy(reinterpret_cast<void*>(address), text.c_str(), text.size() + 1);
			return {};
		});
		return pointer;
	}

	class TinkerPattern : public IPattern
	{
		SimplePattern pattern;
		PatternHash hash;

	public:
		explicit TinkerPattern(std::string_view signature) :
		    pattern(signature)
		{
			for (auto& byte : pattern.m_Bytes)
				hash = hash.Update(byte ? static_cast<char>(*byte) : '?');
			hash = hash.Update(pattern.m_Bytes.size());
		}
		const std::string_view Name() const override
		{
			return "TinkerScripts outfit names";
		}
		std::span<const std::optional<std::uint8_t>> Signature() const override
		{
			return pattern.m_Bytes;
		}
		const PatternHash Hash() const override
		{
			return hash;
		}
	};

	static void BindMenu()
	{
		context.Bind("menu", "set_menu_icon", [](const Values& args) -> Values {
			menuIcon = Text(args[0]);
			return {};
		});
		context.Bind("menu", "get_submenu", [](const Values& args) -> Values {
			auto submenu = std::make_shared<Submenu>(Text(args[0]), menuIcon);
			submenus.push_back(submenu);
			UIManager::AddSubmenu(std::shared_ptr<Submenu>(submenu));
			auto value = MakeTable();
			Member(value, "add_category") = MakeFunction([submenu](const Values& args) -> Values {
				auto category = std::make_shared<Category>(Text(args[1]));
				submenu->AddCategory(std::shared_ptr<Category>(category));
				auto value = MakeTable();
				Member(value, "imgui") = MakeFunction([category](const Values& args) -> Values {
					auto callback = args[1];
					category->AddItem(std::make_shared<ImGuiItem>([callback] {
						if (ready)
							Execute(callback);
					}));
					return {};
				});
				return {value};
			});
			return {value};
		});
		context.Bind("menu", "add_always_draw_imgui", [](const Values& args) -> Values {
			alwaysDraw.push_back(args[0]);
			return {};
		});
		context.Bind("event", "register_handler", [](const Values& args) -> Values {
			unloadCallbacks.push_back(args[1]);
			return {};
		});
		context.Global("menu_event") = MakeTable({{"Unload", 0}});
		context.Bind("commandmgr", "get_command", [](const Values& args) -> Values {
			return {CommandHandle(Hash(args[0]))};
		});
		for (auto kind : {"add_bool_command", "add_looped_command", "add_list_command"})
			context.Bind("commandmgr", kind, [kind](const Values& args) -> Values {
				auto hash = Hash(args[0]);
				if (Commands::GetCommand(hash))
					throw std::runtime_error("Duplicate command: " + Text(args[0]));
				if (std::string_view(kind) == "add_bool_command")
					commands.push_back(std::make_unique<TinkerToggle>(args));
				else if (std::string_view(kind) == "add_looped_command")
					commands.push_back(std::make_unique<TinkerLoop>(args));
				else
					commands.push_back(std::make_unique<TinkerList>(args));
				return {CommandHandle(hash)};
			});
	}

	static void BindGame()
	{
		context.Global("natives") = MakeTable({{"load_natives", MakeFunction([](const Values&) -> Values {
			                                        return {};
		                                        })}});
		context.Global("require") = MakeFunction([](const Values&) -> Values {
			return {};
		});
		context.Global("package") = MakeTable({{"loaded", MakeTable()}});
		context.Bind("util", "get_game_version", [](const Values&) -> Values {
			return {Pointers.OnlineVersion, Pointers.GameVersion};
		});
		context.Bind("util", "time", [](const Values&) -> Values {
			return {std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()};
		});
		context.Bind("util", "joaat", [](const Values& args) -> Values {
			return {static_cast<std::int32_t>(Hash(args[0]))};
		});
		for (auto name : {"Entity", "Ped", "Vehicle"})
			context.Global(name) = MakeFunction([](const Values& args) -> Values {
				return {EntityValue(Integer(args[0]))};
			});
		context.Global("Vector3") = MakeFunction([](const Values& args) -> Values {
			return {MakeTable({{"x", At(args, 0).IsNil() ? Value(0) : args[0]}, {"y", At(args, 1).IsNil() ? Value(0) : args[1]}, {"z", At(args, 2).IsNil() ? Value(0) : args[2]}})};
		});
		context.Global("ScriptGlobal") = MakeFunction([](const Values& args) -> Values {
			return {ScriptAddress(YimMenu::ScriptGlobal(static_cast<std::size_t>(Number(args[0]))))};
		});
		context.Global("ScriptLocal") = MakeFunction([](const Values& args) -> Values {
			auto* thread = Scripts::FindScriptThread(Hash(args[0]));
			if (!thread || !thread->m_Stack)
				return {};
			return {ScriptAddress(YimMenu::ScriptLocal(thread, static_cast<std::size_t>(Number(args[1]))))};
		});
		context.Global("Global") = context.Global("ScriptGlobal");
		context.Global("Local") = context.Global("ScriptLocal");
		context.Bind("script", "run_in_callback", [](const Values& args) -> Values {
			Queue(args[0]);
			return {};
		});
		context.Bind("script", "yield", [](const Values& args) -> Values {
			context.mutex.unlock();
			ScriptMgr::Yield(std::chrono::milliseconds(At(args, 0).IsNil() ? 0 : Integer(args[0])));
			context.mutex.lock();
			return {};
		});
		context.Bind("scripts", "is_active", [](const Values& args) -> Values {
			return {Scripts::IsScriptActive(Hash(args[0]))};
		});
		context.Bind("scripts", "run_as_script", [](const Values& args) -> Values {
			auto* thread = Scripts::FindScriptThread(Hash(args[0]));
			if (thread)
				Scripts::RunAsScript(thread, [&] {
					Call(args[1], {});
				});
			return {};
		});
		context.Bind("network", "is_session_started", [](const Values&) -> Values {
			return {static_cast<bool>(NETWORK::NETWORK_IS_SESSION_STARTED())};
		});
		context.Bind("network", "join_session", [](const Values& args) -> Values {
			Network::LaunchJoinType(static_cast<Network::JoinType>(Integer(args[0])));
			return {};
		});
		context.Global("session_types") = MakeTable({{"invite_only", static_cast<int>(Network::JoinType::INVITE_ONLY)}});
		context.Bind("players", "get_local", [](const Values&) -> Values {
			auto value = MakeTable();
			Member(value, "get_ped") = MakeFunction([](const Values&) -> Values {
				return {EntityValue(Self::GetPed().GetHandle())};
			});
			Member(value, "set_wanted_level") = MakeFunction([](const Values& args) -> Values {
				Self::GetPlayer().SetWantedLevel(Integer(args[1]));
				return {};
			});
			return {value};
		});
		context.Bind("entities", "get_all_peds_as_handles", [](const Values&) -> Values {
			auto result = MakeTable();
			int i = 0;
			if (GetPedPool())
				for (auto ped : Pools::GetPeds())
					if (ped)
						Member(result, ++i) = ped.GetHandle();
			return {result};
		});
		context.Bind("entities", "get_all_objects_as_handles", [](const Values&) -> Values {
			auto result = MakeTable();
			int i = 0;
			if (GetObjectPool())
				for (auto entity : Pools::GetObjects())
					if (entity)
						Member(result, ++i) = entity.GetHandle();
			return {result};
		});
		context.Bind("memory", "scan_pattern", [](const Values& args) -> Values {
			auto* module = ModuleMgr.Get("GTA5_Enhanced.exe"_J);
			if (!module)
				return {};
			TinkerPattern pattern(Text(args[0]));
			std::uintptr_t address = 0;
			PatternScanner scanner(module);
			scanner.Add(pattern, [&](PointerCalculator pointer) {
				address = pointer.As<std::uintptr_t>();
			});
			scanner.Scan();
			return {PointerValue(address)};
		});
		for (auto level : {"info", "warn", "error"})
			context.Bind("log", level, [](const Values& args) -> Values {
				LOG(INFO) << Text(args[0]);
				return {};
			});
		for (auto level : {"info", "warn", "error", "success"})
			context.Bind("notify", level, [level](const Values& args) -> Values {
				auto type = std::string_view(level) == "error" ? NotificationType::Error : std::string_view(level) == "success" ? NotificationType::Success :
				                                                                                                                  NotificationType::Info;
				Notifications::Show(Text(args[0]), Text(args[1]), type);
				return {};
			});
	}

	// Start the compiled feature set on the game's scheduler, without creating a Lua state.
	void Run()
	{
		try
		{
			std::lock_guard lock(context.mutex);
			InitializeStandardLibrary(context);
			BindGameNatives(context);
			BindRenderApi(context);
			BindGame();
			BindMenu();
			BindServices(context);
			BindCloudSaves(context);
			InitializeBlips(context);
			InitializeUnlocks(context);
			InitializeTunables(context);
			InitializeTattoos(context);
			InitializeSettings(context);
			InitializeFeatures(context);
			ready = true;
			for (auto& command : commands)
				if (auto* toggle = dynamic_cast<BoolCommand*>(command.get()); toggle && toggle->GetState())
					toggle->Initialize();
			Renderer::AddRendererCallback([] {
				if (ready)
					for (auto& callback : alwaysDraw)
						Execute(callback);
			},
			    -8);
		}
		catch (const std::exception& error)
		{
			Notifications::Show("TinkerScripts", error.what(), NotificationType::Error);
			LOG(WARNING) << error.what();
		}
	}

	void Shutdown()
	{
		ready = false;
		std::lock_guard lock(context.mutex);
		for (auto& callback : unloadCallbacks)
			Execute(callback);
		CloseCloudSaves();
		for (auto& command : commands)
			Commands::RemoveCommand(command.get());
		commands.clear();
		for (auto& submenu : submenus)
			UIManager::RemoveSubmenu(submenu);
		submenus.clear();
		alwaysDraw.clear();
		unloadCallbacks.clear();
		context.globals.clear();
	}
}

#endif
