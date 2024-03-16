# MemoizationSearch

## 简介
## 记忆化搜索模板
记忆化搜索：一种高效的搜索模式
在计算机科学中，记忆化搜索是一种优化技术，它通过存储昂贵的函数调用结果并在后续调用中重用它们，从而提高算法的运行速度。这种方法是动态规划和分治策略的一个重要组成部分，可以显著提高这些算法的性能。

## 记忆化搜索的工作原理

记忆化搜索的基本思想是将已解决的子问题的解决方案存储在一个表中，然后在需要时查找这些解决方案，而不是重新计算它们。这种方法在处理具有重叠子问题的问题时特别有效，因为它可以避免不必要的重复计算。

例如，考虑斐波那契数列的计算。传统的递归方法会导致大量的重复计算，因为每个数字的计算都依赖于前两个数字的计算。通过使用记忆化搜索，我们可以将每个数字的计算结果存储在一个表中，然后在计算新数字时直接查找这些结果，从而避免了重复计算。

## 记忆化搜索的优点
记忆化搜索的主要优点是它可以显著提高算法的运行速度。通过避免不必要的重复计算，记忆化搜索可以将指数级的时间复杂度降低到多项式级别。此外，记忆化搜索还可以提高算法的空间效率，因为它只需要存储已解决的子问题的解决方案，而不是所有可能的子问题。

## 记忆化搜索的应用
记忆化搜索在许多领域都有广泛的应用，包括计算机图形学、人工智能、生物信息学和网络优化。在这些领域，记忆化搜索被用来解决各种复杂的优化问题，如路径规划、序列对齐和图像重建。

```c++
#include <iostream>
#include"MemoizationSearch.h"
//斐波那契数列的缓存搜索
__int64 Fibonacci(int n) {
	if (n <= 1) return n;
	//利用nonstd::makecached函数生成一个缓存搜索的函数
	auto& fib = nonstd::makecached(Fibonacci);
	return fib(n - 1) + fib(n - 2);
}
int main(){
	std::cout << Fibonacci(10);
	return 0;
}
```
#### 基础组件
`CacheItem` 类：用于存储缓存项，包含一个值和一个过期时间。`IsValid` 方法用于检查缓存项是否仍然有效。

`SimpleBasicCache` 类：一个基本的线程安全缓存实现，使用 `concurrent_unordered_map` 来存储缓存项。它提供了异步添加缓存项 (`AddAysncCache`)、删除缓存项 (`erase`) 和查找缓存项 (`find`) 的方法。

`CachedFunction` 类：用于缓存函数调用结果的主要类。它接收一个 `std::function` 和一个可选的缓存有效时间。`operator()` 被重载以接收任意数量的参数，并根据这些参数来查找或生成缓存项。如果缓存命中，则返回缓存的结果；否则，计算新结果、缓存并返回它。

#### 工厂组件
`CachedFunctionFactory` 类：一个工厂类，用于创建和管理 `CachedFunction` 实例。它使用一个静态的 `concurrent_map` 来存储所有的缓存函数实例。`GetCachedFunction` 方法检查是否已有相应的缓存函数实例，如果没有则创建一个新的实例并返回。

`makecached` 函数模板：一个辅助函数，用于简化 `CachedFunction` 实例的创建。它接收一个函数（或者可调用对象）和一个缓存有效时间，然后返回一个对应的 `CachedFunction` 引用。这个函数主要是调用 `CachedFunctionFactory` 来实现的。

#### 关键概念和实现细节
模板和类型推导：`CachedFunction` 和 `makecached` 都使用了模板编程和类型推导来处理不同的函数签名和调用参数。

线程安全：`SimpleBasicCache` 类使用了 `std::shared_mutex` 来实现线程安全，确保并发访问缓存时的数据一致性。

异步和期货：`AddAysncCache` 方法使用 `std::async` 来异步添加缓存项，以提高性能并避免阻塞主线程。

函数缓存的使用场景：这种缓存机制特别适合用于计算代价高昂且结果可预测的函数，比如递归计算、数据库查询或者文件I/O操作。
### 编译环境
Visual Studio 2019以上IDE
项目使用C++20标准 

### 注意
1. 凡是带有下划线的函数都是内部函数,不要建议直接调用 调用前请阅读源代码以避免理解错误


### 3.免责声明
该开源项目（以下简称“本项目”）是由开发者无偿提供的，并基于开放源代码许可协议发布。本项目仅供参考和学习使用，使用者应该自行承担风险。

本项目没有任何明示或暗示的保证，包括但不限于适销性、特定用途适用性和非侵权性。开发者不保证本项目的功能符合您的需求，也不保证本项目的操作不会中断或错误。

在任何情况下，开发者都不承担由使用本项目而导致的任何直接、间接、偶然、特殊或后果性损失，包括但不限于商业利润的损失，无论这些损失是由合同、侵权行为还是其他原因造成的，即使开发者已被告知此类损失的可能性。

使用本项目即表示您已经阅读并同意遵守此免责声明。如果您不同意此免责声明，请不要使用本项目。开发者保留随时更改此免责声明的权利，恕不另行通知。
# MemoizationSearch
## Introduction
Memoization Search Template Memoization Search: An Efficient Search Mode In computer science, memoization search is an optimization technique that improves the running speed of algorithms by storing the results of expensive function calls and reusing them in subsequent calls. This method is an important component of dynamic programming and divide-and-conquer strategies, which can significantly improve the performance of these algorithms.

Working Principle of Memoization Search The basic idea of memoization search is to store the solutions of solved sub-problems in a table, and then look up these solutions when needed, instead of recalculating them. This method is particularly effective when dealing with problems with overlapping sub-problems, as it can avoid unnecessary repeated calculations.

For example, consider the calculation of the Fibonacci sequence. The traditional recursive method leads to a large amount of repeated calculation, because the calculation of each number depends on the calculation of the previous two numbers. By using memoization search, we can store the calculation results of each number in a table, and then directly look up these results when calculating new numbers, thus avoiding repeated calculation.

Advantages of Memoization Search The main advantage of memoization search is that it can significantly improve the running speed of algorithms. By avoiding unnecessary repeated calculations, memoization search can reduce the time complexity from exponential level to polynomial level. In addition, memoization search can also improve the space efficiency of algorithms, because it only needs to store the solutions of solved sub-problems, not all possible sub-problems.

Applications of Memoization Search Memoization search has a wide range of applications in many fields, including computer graphics, artificial intelligence, bioinformatics, and network optimization. In these fields, memoization search is used to solve various complex optimization problems, such as path planning, sequence alignment, and image reconstruction.

## Compilation Environment
Visual Studio 2019 or above IDE The project uses the C++20 standard

## Note
All functions with underscores are internal functions, it is not recommended to call them directly. Please read the source code before calling to avoid misunderstanding.
## Disclaimer
This open source project (hereinafter referred to as “this project”) is provided free of charge by the developer and is published based on the open source code license agreement. This project is for reference and learning purposes only, and users should bear the risk themselves.

This project does not have any express or implied warranties, including but not limited to merchantability, fitness for a particular purpose, and non-infringement. The developer does not guarantee that the functions of this project meet your needs, nor does it guarantee that the operation of this project will not be interrupted or error-free.

In any case, the developer does not assume any direct, indirect, incidental, special or consequential damages caused by the use of this project, including but not limited to the loss of commercial profits, whether these damages are caused by contract, tort or other reasons, even if the developer has been informed of the possibility of such damages.

Using this project means that you have read and agree to abide by this disclaimer. If you do not agree with this disclaimer, please do not use this project. The developer reserves the right to change this disclaimer at any time without further notice.
