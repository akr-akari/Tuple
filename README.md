# **akr::Tuple**

## **Contents**

  - [1. Require](#1-require)
  - [2. Usage](#2-usage)
  - [3. Operators](#3-operators)
  - [4. Methods](#4-methods)

## **1. Require**
* ### `C++20`

## **2. Usage**
```c++
#include "tuple.hh"

auto t1 = akr::Tuple();

auto t2 = akr::Tuple<>::Create();

auto t3 = akr::Tuple(true, 'A', 123, 3.14, "ABC", nullptr);

auto t4 = akr::Tuple<>::Create(true, 'A', 123, 3.14, "ABC", nullptr);
```

## **3. Operators**
* ### **`=`**
```c++
auto t1 = akr::Tuple(1, 2, 3);

t1 = akr::Tuple(4, 5, 6);
```

* ### **`&`**
```c++
auto t1 = akr::Tuple(1) & 2 & 3;
```

* ### **`+`**
```c++
auto t1 = akr::Tuple(1) + akr::Tuple(2) + akr::Tuple(3);
```

## **4. Methods**
* ### **`auto Concat<T>(T&&... values)`**
```c++
auto t1 = akr::Tuple<>::Concat(akr::Tuple(), true, 'A', 123, akr::Tuple(3.14, "ABC"), nullptr);
```

* ### **`auto Create<T>(T&&... values)`**
```c++
auto t1 = akr::Tuple<>::Create();

auto t2 = akr::Tuple<>::Create(true, 'A', 123, 3.14, "ABC", nullptr);
```

* ### **`void ForEach<F>(const F& func) const?`**
```c++
auto t1 = akr::Tuple(true, 'A', 123, 3.14, "ABC", nullptr);

t1.ForEach(   [](auto&& e) { std::cout << std::boolalpha << e << '\n'; });
```

* ### **`void IndexBy<F>(std::size_t index, const F& func) const?`**
```c++
auto t1 = akr::Tuple(true, 'A', 123, 3.14, "ABC", nullptr);

t1.IndexBy(0, [](auto&& e) { std::cout << std::boolalpha << e << '\n'; });

try
{
    t1.IndexBy(6, [](auto&& e) {} );
}
catch (const std::out_of_range& e)
{
    std::cout << e.what() << '\n';
}
```

* ### **`auto IndexOf<I>() const? noexcept -> const? auto&`**
```c++
auto t1 = akr::Tuple(true, 'A', 123, 3.14, "ABC", nullptr);

t1.IndexOf<0>() = false;
```
