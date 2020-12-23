﻿# cpp-frp
Static functional reactive programming in C++

cpp-frp is a modern multi-threaded lock-free type-safe header-only library written in standard C++14.

The library contains basic `transform`, `map` and `filter` functionality as shown below:

```C++
auto base = source(5);
auto exponent = source(2);

auto squared = transform(
	[](auto base, auto exponent) { return pow(base, exponent); },
	std::ref(base), std::ref(exponent));

auto random_sequence = transform([](auto i) {
		auto v = std::vector<int>(std::size_t(i));
		std::generate(v.begin(), v.end(), std::rand);
		return v;
	}, std::ref(squared));

auto filtered = filter([](auto i) { return i % 2; }, std::ref(random_sequence));

auto strings = map([](auto i) { return std::to_string(i); }, std::ref(filtered));

auto print = sink(std::ref(strings));

// read the content of print
auto values(*print);
for (auto value : *values) {
	std::cout << value << " ";
}
std::cout << std::endl;

// Update base and exponent repositories with new values.
// changes will propagate through the graph where necessary.
base = 6;
exponent = 3;
```

The above example executes the lambda expressions on the current thread, to set an executor to use:

```C++

executor_type executor;

auto receiver = transform(execute_on(executor, [](auto i){}), std::ref(provider));
```
```map```, ```filter``` and ```map_cache``` also supports multiple dependencies and expansion of a specific index:
```C++
auto greeting(source<std::string>());
auto names(source<std::vector<std::string>>());
auto message(source<std::string>());
auto messages(sink(map<1>([](const auto &greeting, const auto &name, const auto &message) {
		std::stringstream ss;
		ss << greeting << ' ' << name << ' ' << message;
		return ss.str();
	}, std::ref(greeting), std::ref(names), std::ref(message))));

greeting = "Hello";
names = { "Frodo", "Samwise", "Merry", "Pippin", "Aragorn", "Boromir", "Legolas", "Gimli", "Gandalf" };
message = "of the Fellowship.";

for (const auto &value : **messages) {
	std::cout << value << std::endl;
}

greeting = "Hallå";
message = "av ringens brödraskap.";
for (const auto &value : **messages) {
	std::cout << value << std::endl;
}
```
## Build and installation instructions
This is a header-only library. Just add ```cpp-frp/include``` as an include directory.
Tested compilers include
 - ```Visual Studio 2015```
 - ```GCC 5, 6```
 - ```clang 3.8, 3.9```
Note that ```GCC 4``` is not supported.

```cmake``` is used for building and running the tests.

 - ```cmake -G``` to list the available generators.
 - ```cmake .``` to set up projects.
  * With Visual Studio, after execution, ```cpp-frp.sln``` can be found in the root directory.
 - ```cmake --build .``` to set up and build the project.
 - ```ctest -V .``` to run the tests.
 - ```cmake -DENABLE_COVERAGE:BOOL=true .``` to enable test coverage statistics. Available with ```GCC``` and ```clang```.
  * ```./test-coverage.sh``` to generate test coverage statistics. Requires ```lcov```
  * ```gnome-open test-coverage/index.html``` or equivalent to open the generated test coverage data.

## Type requirements
### Value types
The requirements for value types are as follows:

 - Must be *move constructible*
  * If used with ```map_cache``` input type must be *copy constructible*
  * If used with ```filter``` type must be *copy constructible*
 - Unless a custom *comparator* is used with ```transform```, ```filter```, ```map```, ```map_cache``` or ```source```:
  * Implement the equality comparator ```auto T::operator==(const T &) const``` or equivalent.

The *comparator* is used to suppress redundant updates while traversing the graph.

### Function types
Functions must implement the ```operator()``` with the argument types relevant. Lambda expressions with ```auto``` type deductions are allowed as seen above. ```std::bind```, function pointers etc works as well.

Functions can return ```void``` for ```transform```, the function will be executed on any dependency changes but the resulting repository becomes a leaf-node and can not be used by any other repository.

#### Thread safety
Functions are expected to have **no side-effects** and only operate on their input arguments. That said, in order to interface with other code that might be a too strict requirement. This library uses atomics to synchronize logic, which means that in theory, at any point, there might be multiple invocations of your function running in parallell. The values returned by functions are guaranteed to arrive in the correct order, the invocation of functions are not.

There are two ways to manage functions with side effects:
 - Specify a single threaded executor with ```execute_on(...)```. This way the execution of your function will be serialized.
 - Use atomics or locks to protect your data. This way you can write thread-safe code while still leveraging multi-thread performance.

### Executor types
Executor types must have a signature equivalent to:
```C++
template<typename F>
auto operator()(F &&f);
```
The executor is expected to call the ```operator()``` of the given instance with no arguments.

This is not an official Google product. This is purely a project made by a Google employee.
