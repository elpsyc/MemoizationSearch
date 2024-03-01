# MemoizationSearch

## 简介
记忆化搜索（Memoization）是一种优化技术，用于提高递归算法的效率。它通过存储先前计算的结果来避免重复计算，从而在递归算法中实现更高的效率。

在递归算法中，可能会出现大量的重复计算，这是因为在递归调用过程中，同样的输入参数可能会导致重复的计算路径。记忆化搜索通过将中间结果保存起来，在需要时直接查表获取结果，从而避免了重复计算。

记忆化搜索通常通过使用数组、哈希表或其他数据结构来存储已经计算过的结果。当递归函数被调用时，首先检查结果是否已经存在于存储中，如果存在则直接返回结果，否则进行计算并将结果存储起来以备后续使用。

记忆化搜索的优点包括：

提高效率：避免了重复计算，减少了算法的时间复杂度，从而提高了算法的执行效率。
简化递归算法：使递归算法更易于理解和实现，因为不需要担心重复计算的问题。
记忆化搜索常用于解决动态规划、图搜索等问题，在这些问题中，递归算法往往会出现大量的重复计算。通过应用记忆化搜索技术，可以将算法的时间复杂度从指数级别降低到多项式级别，从而大大提高了算法的实用性和效率。
Memorization search is an optimization technique used to improve the efficiency of recursive algorithms. It avoids duplicate calculations by storing the results of previous calculations, thus achieving higher efficiency in recursive algorithms.


In recursive algorithms, there may be a large number of duplicate calculations, as the same input parameters may lead to duplicate calculation paths during recursive calls. Memory based search saves intermediate results and directly looks them up in a table when needed, thus avoiding duplicate calculations.


Memory based search typically stores computed results by using arrays, hash tables, or other data structures. When a recursive function is called, the first step is to check if the result already exists in the storage. If it exists, the result is returned directly. Otherwise, the calculation is performed and the result is stored for future use.


The advantages of memory based search include:


Improving efficiency: avoids duplicate calculations, reduces the time complexity of the algorithm, and thus improves the execution efficiency of the algorithm.

Simplify recursive algorithms: make recursive algorithms easier to understand and implement, as there is no need to worry about duplicate calculations.

Memory based search is commonly used to solve problems such as dynamic programming and graph search, where recursive algorithms often result in a large number of repeated computations. By applying memory search technology, the time complexity of the algorithm can be reduced from exponential level to polynomial level, greatly improving the practicality and efficiency of the algorithm.

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
