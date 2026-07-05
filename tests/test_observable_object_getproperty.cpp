#include <catch2/catch_test_macros.hpp>
#include <ReactiveLitepp/Property.h>
#include <ReactiveLitepp/ObservableObject.h>
#include <optional>
#include <string>
#include <vector>

using namespace ReactiveLitepp;

class Widget : public ObservableObject {
public:
    Property<int> Age = Property<int>(
        [this]() { return _age; },
        [this](int& value) {
            SetPropertyValueAndNotify<&Widget::Age>(_age, value);
        }
    );

    Property<std::string> Name = Property<std::string>(
        [this]() { return _name; },
        [this](std::string& value) {
            SetPropertyValueAndNotify<&Widget::Name>(_name, value);
        }
    );

    Property<std::vector<int>> Numbers = Property<std::vector<int>>(
        [this]() { return _numbers; },
        [this](std::vector<int>& value) {
            SetPropertyValueAndNotify<&Widget::Numbers>(_numbers, value);
        }
    );

    // Deliberately NOT registered, to prove unregistered properties are invisible.
    Property<double> Balance = Property<double>(
        [this]() { return _balance; },
        [this](double& value) {
            SetPropertyValueAndNotify<&Widget::Balance>(_balance, value);
        }
    );

    Widget() : _age(0), _name(""), _balance(0.0) {
        RegisterProperties<&Widget::Age, &Widget::Name, &Widget::Numbers>();
    }

private:
    int _age;
    std::string _name;
    std::vector<int> _numbers;
    double _balance;
};

TEST_CASE("GetProperty returns value for a registered property with matching type", "[observable][getproperty]") {
    Widget w;
    w.Age = 42;
    w.Name = "Alice";
    w.Numbers = std::vector<int>{ 1, 2, 3 };

    std::optional<int> age = w.GetProperty<int>("Age");
    REQUIRE(age.has_value());
    REQUIRE(age.value() == 42);

    std::optional<std::string> name = w.GetProperty<std::string>("Name");
    REQUIRE(name.has_value());
    REQUIRE(name.value() == "Alice");

    std::optional<std::vector<int>> numbers = w.GetProperty<std::vector<int>>("Numbers");
    REQUIRE(numbers.has_value());
    REQUIRE(numbers.value() == std::vector<int>{ 1, 2, 3 });
}

TEST_CASE("GetProperty reflects the live value", "[observable][getproperty]") {
    Widget w;

    w.Age = 10;
    REQUIRE(w.GetProperty<int>("Age") == 10);

    w.Age = 20;
    REQUIRE(w.GetProperty<int>("Age") == 20);
}

TEST_CASE("GetProperty returns empty optional for an unknown property name", "[observable][getproperty]") {
    Widget w;

    std::optional<int> result = w.GetProperty<int>("DoesNotExist");
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("GetProperty returns empty optional for an unregistered property", "[observable][getproperty]") {
    Widget w;
    w.Balance = 99.5;

    // Balance exists as a Property but was never registered.
    std::optional<double> result = w.GetProperty<double>("Balance");
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("GetProperty returns empty optional when the requested type does not match", "[observable][getproperty]") {
    Widget w;
    w.Age = 42;

    // "Age" is an int; asking for a mismatching type yields no value.
    REQUIRE_FALSE(w.GetProperty<double>("Age").has_value());
    REQUIRE_FALSE(w.GetProperty<std::string>("Age").has_value());
    REQUIRE_FALSE(w.GetProperty<long>("Age").has_value());
}

TEST_CASE("GetProperty works through an ObservableObject reference", "[observable][getproperty]") {
    Widget w;
    w.Name = "Bob";

    ObservableObject& base = w;
    std::optional<std::string> name = base.GetProperty<std::string>("Name");
    REQUIRE(name.has_value());
    REQUIRE(name.value() == "Bob");
}

TEST_CASE("GetProperty is usable from a PropertyChanged handler", "[observable][getproperty]") {
    Widget w;

    std::optional<int> observed;
    auto sub = w.PropertyChanged.Subscribe(
        [&](ObservableObject& obj, PropertyChangedArgs args) {
            observed = obj.GetProperty<int>(args.PropertyName());
        });

    w.Age = 7;

    REQUIRE(observed.has_value());
    REQUIRE(observed.value() == 7);
}
