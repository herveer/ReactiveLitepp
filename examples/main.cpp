#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <nameof.hpp>
#include <ReactiveLitepp/Event.h>
#include <ReactiveLitepp/Property.h>
#include <ReactiveLitepp/ObservableObject.h>
#include <ReactiveLitepp/ObservableCollection.h>

using namespace ReactiveLitepp;

// ============================================================================
// SECTION 1: Events - Publish/Subscribe Pattern
// ============================================================================

void DemonstrateEvents() {
	std::cout << "\n" << std::string(80, '=') << "\n";
	std::cout << "SECTION 1: Events - Publish/Subscribe Pattern\n";
	std::cout << std::string(80, '=') << "\n\n";

	// 1.1 Simple Event with single parameter
	std::cout << "--- 1.1 Simple Event ---\n";
	Event<std::string> messageEvent;

	auto sub1 = messageEvent.Subscribe([](const std::string& msg) {
		std::cout << "  Subscriber 1 received: " << msg << "\n";
		});

	messageEvent.Notify("Hello, World!");

	// 1.2 Event with multiple parameters
	std::cout << "\n--- 1.2 Event with Multiple Parameters ---\n";
	Event<std::string, int, double> dataEvent;

	auto sub2 = dataEvent.Subscribe([](const std::string& name, int count, double value) {
		std::cout << "  Data: " << name << ", Count: " << count << ", Value: " << value << "\n";
		});

	dataEvent.Notify("Temperature", 5, 23.7);

	// 1.3 Multiple subscribers
	std::cout << "\n--- 1.3 Multiple Subscribers ---\n";
	Event<int> numberEvent;

	auto subA = numberEvent.Subscribe([](int n) {
		std::cout << "  Subscriber A: Square = " << (n * n) << "\n";
		});

	auto subB = numberEvent.Subscribe([](int n) {
		std::cout << "  Subscriber B: Double = " << (n * 2) << "\n";
		});

	numberEvent.Notify(5);

	// 1.4 Unsubscribe
	std::cout << "\n--- 1.4 Unsubscribe ---\n";
	std::cout << "  Before unsubscribe:\n";
	numberEvent.Notify(3);

	subA.Unsubscribe();
	std::cout << "  After unsubscribing A:\n";
	numberEvent.Notify(3);

	// 1.5 Scoped Subscription (RAII)
	std::cout << "\n--- 1.5 Scoped Subscription (RAII) ---\n";
	{
		auto scopedSub = numberEvent.SubscribeScoped([](int n) {
			std::cout << "  Scoped subscriber: Triple = " << (n * 3) << "\n";
			});

		std::cout << "  Inside scope:\n";
		numberEvent.Notify(4);
	} // scopedSub automatically unsubscribes here

	std::cout << "  Outside scope (scoped sub auto-unsubscribed):\n";
	numberEvent.Notify(4);
}

// ============================================================================
// SECTION 2: Properties - Reactive Value Wrappers
// ============================================================================

void DemonstrateProperties() {
	std::cout << "\n" << std::string(80, '=') << "\n";
	std::cout << "SECTION 2: Properties - Reactive Value Wrappers\n";
	std::cout << std::string(80, '=') << "\n\n";

	// 2.1 Property with custom getter/setter (backing field required)
	std::cout << "--- 2.1 Property with Custom Getter/Setter ---\n";

	// All properties require a backing field
	int ageValue = 25;
	std::string nameValue = "Alice";

	Property<int> age(
		[&]() { return ageValue; },
		[&](int& value) { ageValue = value; }
	);

	Property<std::string> name(
		[&]() { return nameValue; },
		[&](std::string& value) { nameValue = value; }
	);

	std::cout << "  Name: " << name << ", Age: " << age << "\n";

	// Using Set/Get methods
	age.Set(30);
	std::cout << "  After Set(30): Age = " << age.Get() << "\n";

	// Using assignment operator
	name = "Bob";
	std::cout << "  After assignment: Name = " << name << "\n";

	// Using in expressions (implicit conversion)
	int doubled = age * 2;
	std::cout << "  Age * 2 = " << doubled << "\n";

	// 2.2 Property with custom logic in setter
	std::cout << "\n--- 2.2 Property with Custom Logic ---\n";
	double celsius = 0.0;

	Property<double> temperature(
		[&]() { return celsius; },
		[&](double& value) {
			std::cout << "  Temperature changing: " << celsius << "C -> " << value << "C\n";
			celsius = value;
		}
	);

	temperature = 25.0;
	temperature = 30.5;
	std::cout << "  Current temperature: " << temperature << "C\n";

	// 2.3 Property with validation
	std::cout << "\n--- 2.3 Property with Validation ---\n";
	int score = 0;

	Property<int> validatedScore(
		[&]() { return score; },
		[&](int& value) {
			if (value < 0) {
				std::cout << "  [X] Validation failed: Score cannot be negative!\n";
				return;  // Don't update
			}
			if (value > 100) {
				std::cout << "  [!] Clamping score to 100\n";
				score = 100;
				return;
			}
			score = value;
		}
	);

	validatedScore = 75;
	std::cout << "  Score: " << validatedScore << "\n";
	validatedScore = -10;  // Will be rejected
	std::cout << "  Score after rejected update: " << validatedScore << "\n";
	validatedScore = 150;  // Will be clamped
	std::cout << "  Score after clamped update: " << validatedScore << "\n";

	// 2.4 Property with transformation
	std::cout << "\n--- 2.4 Property with Transformation ---\n";
	int percentageValue = 0;

	Property<int> percentage(
		[&]() { return percentageValue; },
		[&](int& newValue) {
			// Clamp between 0 and 100
			percentageValue = std::max(0, std::min(100, newValue));
			std::cout << "  Percentage set to: " << percentageValue << "%\n";
		}
	);

	percentage = 50;
	percentage = 150;  // Auto-clamped
	percentage = -20;  // Auto-clamped

	// 2.5 Read-only property
	std::cout << "\n--- 2.5 Read-only Property ---\n";
	int readOnlyValue = 42;
	ReadonlyProperty<int> readOnly(
		[&]() { return readOnlyValue; }
	);

	std::cout << "  Read-only value: " << readOnly << "\n";
	readOnlyValue = 100;
	std::cout << "  After backing value update: " << readOnly << "\n";
}

// ============================================================================
// SECTION 3: ObservableObject - Property Change Notifications
// ============================================================================

class Person : public ObservableObject {
public:
	// All properties require backing fields
	std::string _firstName;

	// Simple backing-field properties: MakeProperty both builds and registers
	// them, so they are queryable via GetProperty() without any extra step.
	Property<std::string> FirstName = MakeProperty<&Person::FirstName>(
		[this]() { return _firstName; },
		[this](std::string& value) {
			SetPropertyValueAndNotify<&Person::FirstName>(_firstName, value);
		}
	);

	Property<std::string> LastName = MakeProperty<&Person::LastName>(
		[this]() { return _lastName; },
		[this](std::string& value) {
			SetPropertyValueAndNotify<&Person::LastName>(_lastName, value);
		}
	);

	Property<int> Age = MakeProperty<&Person::Age>(
		[this]() { return _age; },
		[this](int& value) {
			SetPropertyValueAndNotify<&Person::Age>(_age, value);
		}
	);

	// Custom setter logic (validation) — registration is independent of it.
	Property<std::string> Email = MakeProperty<&Person::Email>(
		[this]() { return _email; },
		[this](std::string& value) {
			if (value.find('@') == std::string::npos) {
				std::cout << "  [X] Invalid email format!\n";
				return;
			}
			SetPropertyValueAndNotify<&Person::Email>(_email, value);
		}
	);

	// Custom setter logic (reject negative).
	Property<double> Salary = MakeProperty<&Person::Salary>(
		[this]() { return _salary; },
		[this](double& newValue) {
			if (newValue < 0) return;  // Reject negative
			SetPropertyValueAndNotify<&Person::Salary>(_salary, newValue);
		}
	);

	// Read-only, computed from other fields — getter only, still discoverable.
	ReadonlyProperty<std::string> FullName = MakeReadonlyProperty<&Person::FullName>(
		[this]() { return _firstName + " " + _lastName; }
	);

	// Constructor to initialize backing fields
	Person() : _firstName("John"), _lastName("Doe"), _age(30), _salary(0.0) {}

	// Using SetPropertyValueAndNotify (recommended for simple properties)
	void SetAge(int newAge) {
		if (SetPropertyValueAndNotify<&Person::Age>(_age, newAge)) {
			std::cout << "  [OK] Age updated to " << _age << "\n";
		}
		else {
			std::cout << "  = Age unchanged (same value)\n";
		}
	}

	void SetFirstName(const std::string& name) {
		SetPropertyValueAndNotify<&Person::FirstName>(_firstName, name);
	}

	std::string GetFullName() const {
		return _firstName + " " + _lastName;
	}

private:
	std::string _lastName;
	int _age;
	std::string _email = "john.doe@example.com";
	double _salary;
};

void DemonstrateObservableObject() {
	std::cout << "\n" << std::string(80, '=') << "\n";
	std::cout << "SECTION 3: ObservableObject - Property Change Notifications\n";
	std::cout << std::string(80, '=') << "\n\n";

	Person person;

	// 3.1 Subscribe to property changes
	std::cout << "--- 3.1 Property Change Events ---\n";

	auto changingSub = person.PropertyChanging.Subscribe(
		[](ObservableObject& obj, PropertyChangingArgs args) {
			std::cout << "  [CHANGING] PropertyChanging: " << args.PropertyName() << " is about to change\n";
		}
	);

	auto changedSub = person.PropertyChanged.Subscribe(
		[&person](ObservableObject& obj, PropertyChangedArgs args) {
			auto& p = static_cast<Person&>(obj);
			std::cout << "  [CHANGED] PropertyChanged: " << args.PropertyName() << " changed\n";
			if (args.PropertyName() == "Email") {
				std::cout << "     New email: " << p.Email << "\n";
			}
		}
	);

	// 3.2 Trigger property changes
	std::cout << "\n--- 3.2 Changing Properties ---\n";
	person.Email = "jane.smith@example.com";
	person.Salary = 75000.0;

	// 3.3 SetPropertyValueAndNotify only fires events when value actually changes
	std::cout << "\n--- 3.3 SetPropertyValueAndNotify (Smart Change Detection) ---\n";
	person.SetAge(30);  // Same value - no events
	person.SetAge(31);  // Different value - events

	// 3.4 Multiple property changes
	std::cout << "\n--- 3.4 Multiple Property Changes ---\n";
	person.SetFirstName("Jane");
	person.LastName = "Smith";
	person.Salary = 80000.0;

	// 3.5 Retrieve any property's value by name (no knowledge of the concrete type)
	std::cout << "\n--- 3.5 GetProperty by Name ---\n";
	if (auto age = person.GetProperty<int>("Age")) {
		std::cout << "  GetProperty<int>(\"Age\") = " << *age << "\n";
	}
	// A read-only, computed property is retrievable the same way.
	if (auto fullName = person.GetProperty<std::string>("FullName")) {
		std::cout << "  GetProperty<std::string>(\"FullName\") = " << *fullName << "\n";
	}
	// Wrong type or unknown name yields an empty optional.
	if (!person.GetProperty<double>("Age")) {
		std::cout << "  GetProperty<double>(\"Age\") = <empty> (type mismatch)\n";
	}
	if (!person.GetProperty<int>("Unknown")) {
		std::cout << "  GetProperty<int>(\"Unknown\") = <empty> (no such property)\n";
	}

	// Type-erased retrieval: get the value as std::any without naming the type.
	std::any ageAny = person.GetProperty("Age");
	if (ageAny.has_value()) {
		std::cout << "  GetProperty(\"Age\") -> std::any holding "
			<< ageAny.type().name() << " = " << std::any_cast<int>(ageAny) << "\n";
	}

	// Typical use: read the changed property straight from a PropertyChanged handler.
	auto byNameSub = person.PropertyChanged.Subscribe(
		[](ObservableObject& obj, PropertyChangedArgs args) {
			if (auto age = obj.GetProperty<int>(args.PropertyName())) {
				std::cout << "  [BY NAME] " << args.PropertyName() << " is now " << *age << "\n";
			}
		});
	person.Age = 41;

	// 3.6 Set any property by name, with a type-erased value.
	std::cout << "\n--- 3.6 SetProperty by Name ---\n";
	auto describe = [](SetPropertyResult r) {
		switch (r) {
		case SetPropertyResult::Success:      return "Success";
		case SetPropertyResult::NotFound:     return "NotFound";
		case SetPropertyResult::TypeMismatch: return "TypeMismatch";
		case SetPropertyResult::ReadOnly:     return "ReadOnly";
		}
		return "?";
	};
	std::cout << "  SetProperty(\"Age\", 50)        -> " << describe(person.SetProperty("Age", std::any(50))) << "\n";
	std::cout << "  Age is now " << person.Age << "\n";
	std::cout << "  SetProperty(\"Age\", 3.14)      -> " << describe(person.SetProperty("Age", std::any(3.14))) << " (type mismatch)\n";
	std::cout << "  SetProperty(\"Nope\", 1)        -> " << describe(person.SetProperty("Nope", std::any(1))) << "\n";
	std::cout << "  SetProperty(\"FullName\", ...)  -> " << describe(person.SetProperty("FullName", std::any(std::string("x")))) << " (computed/read-only)\n";

	std::cout << "\n  Final state: " << person.GetFullName()
		<< ", Age: " << person.Age
		<< ", Email: " << person.Email
		<< ", Salary: $" << person.Salary << "\n";
}

// ============================================================================
// SECTION 4: ObservableCollection - Collection Change Notifications
// ============================================================================

void DemonstrateObservableCollection() {
	std::cout << "\n" << std::string(80, '=') << "\n";
	std::cout << "SECTION 4: ObservableCollection - Collection Change Notifications\n";
	std::cout << std::string(80, '=') << "\n\n";

	// 4.1 Basic ObservableCollection usage
	std::cout << "--- 4.1 Basic ObservableCollection ---\n";
	ObservableCollection<std::string> items;

	// Subscribe to collection changes
	auto changingSub = items.CollectionChanging.Subscribe(
		[](ObservableCollection<std::string>& coll, ObservableCollection<std::string>::CollectionChangingArgs args) {
			std::cout << "  [CHANGING] About to change collection (Old count: " << args.OldCount 
				<< " -> New count: " << args.NewCount << ")\n";
		}
	);

	auto changedSub = items.CollectionChanged.Subscribe(
		[](ObservableCollection<std::string>& coll, ObservableCollection<std::string>::CollectionChangedArgs args) {
			std::cout << "  [CHANGED] Collection changed (Count: " << args.NewCount << ")\n";
		}
	);

	// 4.2 Adding items
	std::cout << "\n--- 4.2 Adding Items ---\n";
	items.push_back("Coffee");
	items.push_back("Tea");
	items.emplace_back("Juice");

	std::cout << "  Items in collection: ";
	for (const auto& item : items) {
		std::cout << item << " ";
	}
	std::cout << "\n";

	// 4.3 Read-only Count property
	std::cout << "\n--- 4.3 Read-only Count Property ---\n";
	std::cout << "  Count: " << items.Count << "\n";
	std::cout << "  Is empty: " << (items.empty() ? "Yes" : "No") << "\n";

	// 4.4 Removing items
	std::cout << "\n--- 4.4 Removing Items ---\n";
	items.erase(items.begin());
	std::cout << "  After erase, items: ";
	for (const auto& item : items) {
		std::cout << item << " ";
	}
	std::cout << "\n";

	// 4.5 Inserting items
	std::cout << "\n--- 4.5 Inserting Items ---\n";
	items.insert(items.begin() + 1, "Soda");
	std::cout << "  After insert, items: ";
	for (const auto& item : items) {
		std::cout << item << " ";
	}
	std::cout << "\n";

	// 4.6 Clearing the collection
	std::cout << "\n--- 4.6 Clearing Collection ---\n";
	items.clear();
	std::cout << "  After clear, count: " << items.Count << "\n";

	// 4.7 ReadonlyObservableCollection
	std::cout << "\n--- 4.7 ReadonlyObservableCollection ---\n";
	ObservableCollection<int> numbers;
	ReadonlyObservableCollection<int> readonlyView(numbers);

	std::cout << "  Creating a readonly view of the collection...\n";

	// Subscribe to events through the readonly view
	auto readonlyChangedSub = readonlyView.CollectionChanged().Subscribe(
		[](ObservableCollection<int>& coll, ObservableCollection<int>::CollectionChangedArgs args) {
			std::cout << "  [READONLY VIEW] Collection changed (Count: " << args.NewCount << ")\n";
		}
	);

	// Modify the underlying collection
	std::cout << "  Modifying underlying collection:\n";
	numbers.push_back(10);
	numbers.push_back(20);
	numbers.push_back(30);

	// Access through readonly view
	std::cout << "  Accessing through readonly view:\n";
	std::cout << "    Count: " << readonlyView.Count << "\n";
	std::cout << "    Items: ";
	for (auto num : readonlyView) {
		std::cout << num << " ";
	}
	std::cout << "\n";
	std::cout << "    First: " << readonlyView.front() << ", Last: " << readonlyView.back() << "\n";
	std::cout << "    Item at index 1: " << readonlyView[1] << "\n";

	// Demonstrate read-only nature (compile-time safety)
	std::cout << "  Note: ReadonlyObservableCollection prevents modifications\n";
	std::cout << "        (e.g., no push_back, clear, erase methods available)\n";
}

// ============================================================================
// SECTION 5: Real-World Example - Shopping Cart
// ============================================================================

class ShoppingCart : public ObservableObject {
public:
	Property<int> ItemCount = MakeProperty<&ShoppingCart::ItemCount>(
		[this]() { return _itemCount; },
		[this](int& value) {
			SetPropertyValueAndNotify<&ShoppingCart::ItemCount>(_itemCount, value);
		}
	);

	Property<double> TotalPrice = MakeProperty<&ShoppingCart::TotalPrice>(
		[this]() { return _totalPrice; },
		[this](double& value) {
			SetPropertyValueAndNotify<&ShoppingCart::TotalPrice>(_totalPrice, value);
		}
	);

	Property<bool> HasDiscount = MakeProperty<&ShoppingCart::HasDiscount>(
		[this]() { return _hasDiscount; },
		[this](bool& value) {
			if (SetPropertyValueAndNotify<&ShoppingCart::HasDiscount>(_hasDiscount, value)) {
				std::cout << "  " << (value ? "[OK] Discount code applied!" : "[X] Discount removed") << "\n";
			}
		}
	);

	ShoppingCart() : _itemCount(0), _totalPrice(0.0), _hasDiscount(false) {}

	void AddItem(const std::string& name, double price, int quantity = 1) {
		std::cout << "  Adding: " << quantity << "x " << name << " ($" << price << " each)\n";
		ItemCount = ItemCount + quantity;
		double itemTotal = price * quantity;
		double discount = _hasDiscount ? itemTotal * 0.1 : 0.0;
		double finalPrice = itemTotal - discount;
		TotalPrice = TotalPrice + finalPrice;

		if (discount > 0) {
			std::cout << "    [DISCOUNT] Discount applied: -$" << discount << "\n";
		}
	}

	void ApplyDiscount(bool apply) {
		HasDiscount = apply;
	}

	void Clear() {
		ItemCount = 0;
		TotalPrice = 0;
		HasDiscount = false;
		std::cout << "  [CLEAR] Cart cleared\n";
	}

private:
	int _itemCount;
	double _totalPrice;
	bool _hasDiscount;
};

void DemonstrateRealWorldExample() {
	std::cout << "\n" << std::string(80, '=') << "\n";
	std::cout << "SECTION 5: Real-World Example - Shopping Cart\n";
	std::cout << std::string(80, '=') << "\n\n";

	ShoppingCart cart;

	// Monitor cart changes
	cart.PropertyChanged.Subscribe([&cart](ObservableObject&, PropertyChangedArgs args) {
		std::cout << "  [UPDATE] Cart Update: ";
		if (args.PropertyName() == "ItemCount") {
			std::cout << "Items: " << cart.ItemCount;
		}
		else if (args.PropertyName() == "TotalPrice") {
			std::cout << std::fixed << std::setprecision(2) << "Total: $" << cart.TotalPrice;
		}
		else if (args.PropertyName() == "HasDiscount") {
			std::cout << "Discount: " << (cart.HasDiscount.Get() ? "Yes" : "No");
		}
		std::cout << "\n";
		});

	// Shopping scenario
	std::cout << "--- Shopping Scenario ---\n";
	cart.AddItem("Laptop", 999.99);
	cart.AddItem("Mouse", 29.99, 2);

	std::cout << "\n";
	cart.ApplyDiscount(true);
	cart.AddItem("Keyboard", 79.99);  // This will have discount applied

	std::cout << "\n--- Final Cart ---\n";
	std::cout << "  Items: " << cart.ItemCount << "\n";
	std::cout << "  Total: $" << std::fixed << std::setprecision(2) << cart.TotalPrice << "\n";
	std::cout << "  Discount Active: " << (cart.HasDiscount ? "Yes" : "No") << "\n";

	std::cout << "\n";
	cart.Clear();
}

// ============================================================================
// SECTION 6: Advanced Patterns
// ============================================================================

void DemonstrateAdvancedPatterns() {
	std::cout << "\n" << std::string(80, '=') << "\n";
	std::cout << "SECTION 6: Advanced Patterns\n";
	std::cout << std::string(80, '=') << "\n\n";

	// 6.1 Computed Properties
	std::cout << "--- 6.1 Computed Properties ---\n";

	double widthValue = 10.0;
	double heightValue = 5.0;

	Property<double> width(
		[&]() { return widthValue; },
		[&](double& value) { widthValue = value; }
	);

	Property<double> height(
		[&]() { return heightValue; },
		[&](double& value) { heightValue = value; }
	);

	// Area is computed from width and height
	double area = width * height;
	std::cout << "  Initial area: " << area << "\n";

	width = 20.0;
	area = width * height;  // Recompute
	std::cout << "  After width change: " << area << "\n";

	// 6.2 Property Dependency Chain
	std::cout << "\n--- 6.2 Property Dependency Chain ---\n";
	int baseValue = 10;

	Property<int> base(
		[&]() { return baseValue; },
		[&](int& v) { baseValue = v; std::cout << "  Base set to: " << v << "\n"; }
	);

	Property<int> derived(
		[&]() { return base.Get() * 2; },
		[&](int& v) { base.Set(v / 2); }
	);

	std::cout << "  Base: " << base << ", Derived: " << derived << "\n";
	base = 20;
	std::cout << "  After base=20: Base: " << base << ", Derived: " << derived << "\n";
	derived = 100;
	std::cout << "  After derived=100: Base: " << base << ", Derived: " << derived << "\n";

	// 6.3 Event Broadcasting
	std::cout << "\n--- 6.3 Event Broadcasting ---\n";
	Event<std::string> logger;

	// Multiple logging destinations
	auto consoleLogger = logger.Subscribe([](const std::string& msg) {
		std::cout << "  [CONSOLE] " << msg << "\n";
		});

	auto fileLogger = logger.Subscribe([](const std::string& msg) {
		std::cout << "  [FILE] (simulated) " << msg << "\n";
		});

	logger.Notify("Application started");
	logger.Notify("User logged in");
	logger.Notify("Data saved");
}

// ============================================================================
// MAIN - Run All Demonstrations
// ============================================================================

int main() {
	std::cout << "\n";
	std::cout << "+============================================================================+\n";
	std::cout << "|                  ReactiveLitepp Library - Complete Demo                   |\n";
	std::cout << "|                  Reactive Programming for Modern C++                       |\n";
	std::cout << "+============================================================================+\n";

	try {
		DemonstrateEvents();
		DemonstrateProperties();
		DemonstrateObservableObject();
		DemonstrateObservableCollection();
		DemonstrateRealWorldExample();
		DemonstrateAdvancedPatterns();

		std::cout << "\n" << std::string(80, '=') << "\n";
		std::cout << "[OK] All demonstrations completed successfully!\n";
		std::cout << std::string(80, '=') << "\n\n";

	}
	catch (const std::exception& e) {
		std::cerr << "[ERROR] Error: " << e.what() << "\n";
		return 1;
	}

	return 0;
}