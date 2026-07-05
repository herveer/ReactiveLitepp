#pragma once
#include "Event.h"
#include "Property.h"
#include <nameof.hpp>
#include <any>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace ReactiveLitepp
{
	struct PropertyChangeArgs {
	public:
		PropertyChangeArgs(std::string propertyName) : _propertyName(propertyName) {}
		std::string PropertyName() const { return _propertyName; }

	private:
		std::string _propertyName = "";
	};

	struct PropertyChangingArgs : public PropertyChangeArgs {
		PropertyChangingArgs(std::string propertyName) : PropertyChangeArgs(propertyName) {}
	};

	struct PropertyChangedArgs : public PropertyChangeArgs {
		PropertyChangedArgs(std::string propertyName) : PropertyChangeArgs(propertyName) {}
	};

	// Outcome of a by-name SetProperty() call.
	enum class SetPropertyResult {
		Success,       // property found, value type matched, its setter was invoked
		NotFound,      // no registered property has that name
		TypeMismatch,  // property found, but the supplied value's type does not match
		ReadOnly       // property found, but it is read-only (has no setter)
	};

	class ObservableObject {
	public:
		Event<ObservableObject&, PropertyChangingArgs> PropertyChanging;
		Event<ObservableObject&, PropertyChangedArgs> PropertyChanged;

		// Retrieves the current value of a registered property by name.
		//
		// Returns an empty optional when no property with that name has been
		// registered, or when the requested type T does not exactly match the
		// property's declared value type.
		template<typename T>
		std::optional<T> GetProperty(std::string_view propertyName) const {
			auto it = _propertyAccessors.find(std::string(propertyName));
			if (it == _propertyAccessors.end())
				return std::nullopt;

			std::any value = it->second.get();
			if (const T* typed = std::any_cast<T>(&value))
				return *typed;

			return std::nullopt;
		}

		// Retrieves the current value of a registered property by name, type-erased.
		//
		// Returns an empty std::any (has_value() == false) when no property with
		// that name has been registered. The caller can inspect the contained type
		// via std::any_cast<T> or any.type().
		std::any GetProperty(std::string_view propertyName) const {
			auto it = _propertyAccessors.find(std::string(propertyName));
			if (it == _propertyAccessors.end())
				return {};

			return it->second.get();
		}

		// Sets a registered property by name, running its setter (so validation,
		// clamping and change notifications behave exactly as SetPropertyValueAndNotify)
		// without the caller needing the member pointer.
		//
		// The value must hold exactly the property's declared value type. The result
		// distinguishes success from a missing name, a type mismatch, or a read-only
		// property.
		SetPropertyResult SetProperty(std::string_view propertyName, const std::any& value) {
			auto it = _propertyAccessors.find(std::string(propertyName));
			if (it == _propertyAccessors.end())
				return SetPropertyResult::NotFound;

			if (!it->second.set)
				return SetPropertyResult::ReadOnly;

			return it->second.set(value) ? SetPropertyResult::Success
			                             : SetPropertyResult::TypeMismatch;
		}

	protected:
		void NotifyPropertyChanging(std::string_view propertyName) {
			PropertyChangingArgs args = PropertyChangingArgs(std::string(propertyName));
			PropertyChanging.Notify(*this, args);
		}

		void NotifyPropertyChanged(std::string_view propertyName) {
			PropertyChangedArgs args = PropertyChangedArgs(std::string(propertyName));
			PropertyChanged.Notify(*this, args);
		}

		template <auto V>
		auto NotifyPropertyChanging() -> std::enable_if_t<std::is_member_pointer_v<decltype(V)>>
		{
			PropertyChangingArgs args = PropertyChangingArgs(std::string(nameof::nameof_member<V>()));
			PropertyChanging.Notify(*this, args);
			return;
		}

		template <auto V>
		auto NotifyPropertyChanged() -> std::enable_if_t<std::is_member_pointer_v<decltype(V)>>
		{
			PropertyChangedArgs args = PropertyChangedArgs(std::string(nameof::nameof_member<V>()));
			PropertyChanged.Notify(*this, args);
			return;
		}

		template<typename T>
		struct member_pointer_traits;

		template<typename Class, typename MemberType>
		struct member_pointer_traits<MemberType Class::*> {
			using type = MemberType;
			using class_type = Class;
		};

		template<auto Member>
		using property_value_t =
			typename member_pointer_traits<decltype(Member)>::type::value_type;

		template<auto Member>
		using property_class_t =
			typename member_pointer_traits<decltype(Member)>::class_type;

		// Registers a Property member so it can be looked up (and, when writable,
		// assigned) at runtime by name via GetProperty()/SetProperty(). The name is
		// derived from the member at compile time; the accessors read/write the
		// property's live value on each call. A read-only property (no Set) records
		// only a getter, so SetProperty() reports it as SetPropertyResult::ReadOnly.
		//
		// Call this from the derived class constructor, e.g.
		//     RegisterProperty<&Person::FirstName>();
		template<auto Member>
		void RegisterProperty() {
			using PropertyType = typename member_pointer_traits<decltype(Member)>::type;
			using ValueType = property_value_t<Member>;
			using ClassType = property_class_t<Member>;

			ClassType* self = static_cast<ClassType*>(this);

			PropertyAccessor accessor;
			accessor.get = [self]() -> std::any {
				return std::any(std::in_place_type<ValueType>, (self->*Member).Get());
			};

			if constexpr (requires(PropertyType& p, const ValueType& v) { p.Set(v); }) {
				accessor.set = [self](const std::any& value) -> bool {
					const ValueType* typed = std::any_cast<ValueType>(&value);
					if (!typed)
						return false;  // type mismatch
					(self->*Member).Set(*typed);
					return true;
				};
			}

			_propertyAccessors[std::string(nameof::nameof_member<Member>())] = std::move(accessor);
		}

		// Convenience helper to register several properties at once, e.g.
		//     RegisterProperties<&Person::FirstName, &Person::LastName, &Person::Age>();
		template<auto... Members>
		void RegisterProperties() {
			(RegisterProperty<Members>(), ...);
		}

		// Builds a Property and registers it for name-based lookup in one step,
		// so a property becomes discoverable through GetProperty() simply by
		// being declared. Registration is independent of the getter/setter, so
		// this works for plain backing fields as well as custom validation,
		// clamping, or computed logic. The value type is deduced from the member.
		//
		//     Property<double> Salary = MakeProperty<&Person::Salary>(
		//         [this] { return _salary; },
		//         [this](double& v) {
		//             if (v >= 0) SetPropertyValueAndNotify<&Person::Salary>(_salary, v);
		//         });
		template<auto Member>
		Property<property_value_t<Member>> MakeProperty(
			Getter<property_value_t<Member>> get,
			Setter<property_value_t<Member>> set)
		{
			RegisterProperty<Member>();
			return Property<property_value_t<Member>>(std::move(get), std::move(set));
		}

		// Read-only counterpart of MakeProperty. Registers a getter-only property
		// (e.g. a value computed from other fields) for GetProperty() lookup.
		//
		//     ReadonlyProperty<std::string> FullName = MakeReadonlyProperty<&Person::FullName>(
		//         [this] { return _first + " " + _last; });
		template<auto Member>
		ReadonlyProperty<property_value_t<Member>> MakeReadonlyProperty(
			Getter<property_value_t<Member>> get)
		{
			RegisterProperty<Member>();
			return ReadonlyProperty<property_value_t<Member>>(std::move(get));
		}

		template<auto Member>
		bool SetPropertyValueAndNotify(property_value_t<Member>& field, const property_value_t<Member>& value)
		{
			if (field == value)
				return false;

			NotifyPropertyChanging<Member>();
			field = value;
			NotifyPropertyChanged<Member>();
			return true;
		}

	private:
		// Type-erased read/write access to one registered property.
		// 'set' is empty for read-only properties; it returns false on a type mismatch.
		struct PropertyAccessor {
			std::function<std::any()> get;
			std::function<bool(const std::any&)> set;
		};

		// Maps a property name to its type-erased accessors.
		std::unordered_map<std::string, PropertyAccessor> _propertyAccessors;
	};
}