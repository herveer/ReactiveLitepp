#include <catch2/catch_test_macros.hpp>
#include <ReactiveLitepp/Property.h>
#include <ReactiveLitepp/ObservableObject.h>
#include <algorithm>
#include <optional>
#include <string>
#include <vector>

using namespace ReactiveLitepp;

// A type whose properties are built with the MakeProperty/MakeReadonlyProperty
// factories, so every one is auto-registered for GetProperty() lookup with no
// separate RegisterProperties call.
class Account : public ObservableObject {
public:
    // 1) Simple backing field.
    Property<std::string> Owner = MakeProperty<&Account::Owner>(
        [this] { return _owner; },
        [this](std::string& v) { SetPropertyValueAndNotify<&Account::Owner>(_owner, v); });

    // 2) Custom setter: reject negative deposits.
    Property<double> Balance = MakeProperty<&Account::Balance>(
        [this] { return _balance; },
        [this](double& v) { if (v >= 0) SetPropertyValueAndNotify<&Account::Balance>(_balance, v); });

    // 3) Custom setter: clamp to [0, 100].
    Property<int> HealthPercent = MakeProperty<&Account::HealthPercent>(
        [this] { return _health; },
        [this](int& v) {
            int c = std::clamp(v, 0, 100);
            SetPropertyValueAndNotify<&Account::HealthPercent>(_health, c);
        });

    // 4) Read-only, computed from other fields.
    ReadonlyProperty<std::string> Summary = MakeReadonlyProperty<&Account::Summary>(
        [this] { return _owner + ":" + std::to_string(_health); });

    Account() : _owner("none"), _balance(0.0), _health(0) {}

private:
    std::string _owner;
    double _balance;
    int _health;
};

TEST_CASE("MakeProperty auto-registers a simple property", "[observable][makeproperty]") {
    Account a;
    a.Owner = "Ada";

    auto owner = a.GetProperty<std::string>("Owner");
    REQUIRE(owner.has_value());
    REQUIRE(owner.value() == "Ada");
}

TEST_CASE("MakeProperty keeps custom setter logic while registering", "[observable][makeproperty]") {
    Account a;

    a.Balance = -100.0;   // rejected by the custom setter
    REQUIRE(a.GetProperty<double>("Balance") == 0.0);

    a.Balance = 250.0;    // accepted
    REQUIRE(a.GetProperty<double>("Balance") == 250.0);

    a.HealthPercent = 250;  // clamped to 100
    REQUIRE(a.GetProperty<int>("HealthPercent") == 100);

    a.HealthPercent = -5;   // clamped to 0
    REQUIRE(a.GetProperty<int>("HealthPercent") == 0);
}

TEST_CASE("MakeReadonlyProperty registers a computed getter-only property", "[observable][makeproperty]") {
    Account a;
    a.Owner = "Grace";
    a.HealthPercent = 80;

    auto summary = a.GetProperty<std::string>("Summary");
    REQUIRE(summary.has_value());
    REQUIRE(summary.value() == "Grace:80");

    // Computed value is live: it reflects later changes to the source fields.
    a.HealthPercent = 55;
    REQUIRE(a.GetProperty<std::string>("Summary") == std::string("Grace:55"));
}

TEST_CASE("MakeProperty preserves change notifications", "[observable][makeproperty]") {
    Account a;

    std::vector<std::string> changed;
    auto sub = a.PropertyChanged.Subscribe([&](ObservableObject&, PropertyChangedArgs args) {
        changed.push_back(args.PropertyName());
    });

    a.Owner = "Linus";
    a.Balance = -1.0;   // rejected -> no notification
    a.Balance = 10.0;   // accepted -> notification

    REQUIRE(changed.size() == 2);
    REQUIRE(changed[0] == "Owner");
    REQUIRE(changed[1] == "Balance");
}

TEST_CASE("Factory-built properties honor type and name mismatches", "[observable][makeproperty]") {
    Account a;
    a.Balance = 42.0;

    REQUIRE_FALSE(a.GetProperty<int>("Balance").has_value());     // wrong type
    REQUIRE_FALSE(a.GetProperty<double>("Missing").has_value());  // unknown name
}

TEST_CASE("GetProperty (type-erased) returns the value as std::any", "[observable][getproperty][any]") {
    Account a;
    a.Owner = "Edsger";

    std::any owner = a.GetProperty("Owner");
    REQUIRE(owner.has_value());
    REQUIRE(owner.type() == typeid(std::string));
    REQUIRE(std::any_cast<std::string>(owner) == "Edsger");

    // Unknown name -> empty any.
    std::any missing = a.GetProperty("Missing");
    REQUIRE_FALSE(missing.has_value());

    // Read-only computed property is retrievable this way too.
    a.HealthPercent = 20;
    std::any summary = a.GetProperty("Summary");
    REQUIRE(std::any_cast<std::string>(summary) == "Edsger:20");
}

TEST_CASE("SetProperty by name sets the value and notifies", "[observable][setproperty]") {
    Account a;

    std::string changed;
    auto sub = a.PropertyChanged.Subscribe([&](ObservableObject&, PropertyChangedArgs args) {
        changed = args.PropertyName();
    });

    SetPropertyResult r = a.SetProperty("Owner", std::any(std::string("Bjarne")));

    REQUIRE(r == SetPropertyResult::Success);
    REQUIRE(a.Owner.Get() == "Bjarne");
    REQUIRE(changed == "Owner");
}

TEST_CASE("SetProperty by name runs the property's own setter logic", "[observable][setproperty]") {
    Account a;

    // Custom setter clamps to [0,100]; SetProperty routes through it.
    REQUIRE(a.SetProperty("HealthPercent", std::any(250)) == SetPropertyResult::Success);
    REQUIRE(a.GetProperty<int>("HealthPercent") == 100);

    // Success means "setter was invoked", not "value changed": a rejected
    // negative deposit still reports Success but leaves the value untouched.
    a.Balance = 500.0;
    REQUIRE(a.SetProperty("Balance", std::any(-1.0)) == SetPropertyResult::Success);
    REQUIRE(a.GetProperty<double>("Balance") == 500.0);
}

TEST_CASE("SetProperty reports NotFound / TypeMismatch / ReadOnly", "[observable][setproperty]") {
    Account a;
    a.Owner = "Ada";

    // Unknown name.
    REQUIRE(a.SetProperty("Missing", std::any(1)) == SetPropertyResult::NotFound);

    // Owner is std::string; supplying an int is a type mismatch and changes nothing.
    REQUIRE(a.SetProperty("Owner", std::any(42)) == SetPropertyResult::TypeMismatch);
    REQUIRE(a.Owner.Get() == "Ada");

    // Summary is a read-only computed property.
    REQUIRE(a.SetProperty("Summary", std::any(std::string("x"))) == SetPropertyResult::ReadOnly);
}

TEST_CASE("SetProperty type mismatch does not notify", "[observable][setproperty]") {
    Account a;

    int notifications = 0;
    auto sub = a.PropertyChanged.Subscribe([&](ObservableObject&, PropertyChangedArgs) {
        notifications++;
    });

    a.SetProperty("Owner", std::any(3.14));  // wrong type -> setter never runs
    REQUIRE(notifications == 0);
}
