1. [安装环境](#安装go语言环境)
2. [hello world](#go的hello_world)
3. [值](#值)
4. [变量](#变量)
5. [常量](#常量)
6. [For循环](#for循环)
7. [if_else分支](#if_else分支)
8. [分支结构](#分支结构)
9. [数组](#数组)
10. [切片](#切片)
11. [关联数组](#关联数组)
12. [Range遍历](#range遍历)
13. [函数](#函数)
14. [多返回值](#多返回值)
15. [变参函数](#变参函数)
16. [闭包](#闭包)
17. [递归](#递归)
18. [指针](#指针)
19. [结构体](#结构体)
20. [方法](#方法)
21. [接口](#接口)
22. [错误处理](#错误处理)
23. [协程](#协程)
24. [通道](#通道)
25. [通道缓冲](#通道缓冲)
26. [通道同步](#通道同步)
27. [通道方向](#通道方向)
28. [通道选择器](#通道选择器)
29. [超时处理](#超时处理)
30. [非阻塞通道操作](#非阻塞通道操作)
31. [通道的关闭](#通道的关闭)
32. [通道遍历](#通道遍历)
33. [定时器](#定时器)
34. [打点器](#打点器)
35. [工作池](#工作池)
36. [速率限制](#速率限制)
37. [原子计数器](#原子计数器)
38. [互斥锁](#互斥锁)
39. [GO状态协程](#go状态协程)
40. [排序](#排序)
41. [使用函数自定义排序](#使用函数自定义排序)
42. [Panic](#panic)
43. [Defer](#defer)
44. [组合函数](#组合函数)
45. [字符串函数](#字符串函数)
46. [字符串实例化](#字符串实例化)
47. [正则表达式](#正则表达式)
48. [JSON](#json)
49. [时间](#时间)
50. [时间戳](#时间戳)
51. [时间的格式化和解析](#时间的格式化和解析)
52. [随机数](#随机数)
53. [数字解析](#数字解析)
54. [URL解析](#url解析)
55. [SHA1散列](#sha1散列)
56. [Base64编码](#base64编码)
57. [读文件](#读文件)
58. [写文件](#写文件)
59. [行过滤器](#行过滤器)
60. [命令行参数](#命令行参数)
61. [命令行标志](#命令行标志)
62. [环境变量](#环境变量)
63. [生成进程](#生成进程)
64. [执行进程](#执行进程)
65. [信号](#信号)
66. [退出](#退出)


### 安装go语言环境

```
sudo apt install golang-go

go version 
#检查版本
```

### go的hello_world

```
vim hello-world.go
```

输入：

```
package main
import "fmt"
func main() {
    fmt.Println("hello world")
}
```

保存：

```
go run hello-world.go
hello world
```

编译：
```
$go build hello-world.go
$ls
hello-world hello-world.go

$./hello-world
hello world
```
### 值
```
package main

import "fmt"

func main() {

    // 字符串可以通过 `+` 连接。
    fmt.Println("go" + "lang")

    // 整数和浮点数
    fmt.Println("1+1 =", 1+1)
    fmt.Println("7.0/3.0 =", 7.0/3.0)

    // 布尔型，还有你想要的逻辑运算符。
    fmt.Println(true && false)
    fmt.Println(true || false)
    fmt.Println(!true)
}

```

结果：
```
golang
1+1 = 2
7.0/3.0 = 2.3333333333333335
false
true
false
```



### 变量

```
package main

import "fmt"

func main() {

    // `var` 声明 1 个或者多个变量。
    var a string = "initial"
    fmt.Println(a)

    // 你可以申明一次性声明多个变量。
    var b, c int = 1, 2
    fmt.Println(b, c)

    // Go 将自动推断已经初始化的变量类型。
    var d = true
    fmt.Println(d)

    // 声明变量且没有给出对应的初始值时，变量将会初始化为
    // _零值_ 。例如，一个 `int` 的零值是 `0`。
    var e int
    fmt.Println(e)

    // `:=` 语句是申明并初始化变量的简写，例如
    // 这个例子中的 `var f string = "short"`。
    f := "short"
    fmt.Println(f)
}

```

结果：
```
initial
1 2
true
0
short
```


### 常量

```
package main

import "fmt"
import "math"

// `const` 用于声明一个常量。
const s string = "constant"

func main() {
    fmt.Println(s)

    // `const` 语句可以出现在任何 `var` 语句可以出现
    // 的地方
    const n = 500000000

    // 常数表达式可以执行任意精度的运算
    const d = 3e20 / n
    fmt.Println(d)

    // 数值型常量是没有确定的类型的，直到它们被给定了一个
    // 类型，比如说一次显示的类型转化。
    fmt.Println(int64(d))

    // 当上下文需要时，一个数可以被给定一个类型，比如
    // 变量赋值或者函数调用。举个例子，这里的 `math.Sin`
    // 函数需要一个 `float64` 的参数。
    fmt.Println(math.Sin(n))

}

```

结果:
```
constant
6e+11
600000000000
-0.28470407323754404
```

### for循环
```
// `for` 是 Go 中唯一的循环结构。这里有 `for` 循环
// 的三个基本使用方式。

package main

import "fmt"

func main() {

    // 最常用的方式，带单个循环条件。
    i := 1
    for i <= 3 {
        fmt.Println(i)
        i = i + 1
    }

    // 经典的初始化/条件/后续形式 `for` 循环。
    for j := 7; j <= 9; j++ {
        fmt.Println(j)
    }

    // 不带条件的 `for` 循环将一直执行，直到在循环体内使用
    // 了 `break` 或者 `return` 来跳出循环。
    for {
        fmt.Println("loop")
        break
    }
}
```

结果:
```
1
2
3
7
8
9
loop
```


### if_else分支
```
// `if` 和 `else` 分支结构在 Go 中当然是直接了当的了。

package main

import "fmt"

func main() {

    // 这里是一个基本的例子。
    if 7%2 == 0 {
        fmt.Println("7 is even")
    } else {
        fmt.Println("7 is odd")
    }

    // 你可以不要 `else` 只用 `if` 语句。
    if 8%4 == 0 {
        fmt.Println("8 is divisible by 4")
    }

    // 在条件语句之前可以有一个语句；任何在这里声明的变量
    // 都可以在所有的条件分支中使用。
    if num := 9; num < 0 {
        fmt.Println(num, "is negative")
    } else if num < 10 {
        fmt.Println(num, "has 1 digit")
    } else {
        fmt.Println(num, "has multiple digits")
    }
}

// 注意，在 Go 中，你可以不使用圆括号，但是花括号是需
// 要的。

```

结果:
```
7 is odd
8 is divisible by 4
9 has 1 digit
```


### 分支结构
```
// _switch_ ，方便的条件分支语句。

package main

import "fmt"
import "time"

func main() {

    // 一个基本的 `switch`。
    i := 2
    fmt.Print("write ", i, " as ")
    switch i {
    case 1:
        fmt.Println("one")
    case 2:
        fmt.Println("two")
    case 3:
        fmt.Println("three")
    }

    // 在一个 `case` 语句中，你可以使用逗号来分隔多个表
    // 达式。在这个例子中，我们很好的使用了可选的
    // `default` 分支。
    switch time.Now().Weekday() {
    case time.Saturday, time.Sunday:
        fmt.Println("it's the weekend")
    default:
        fmt.Println("it's a weekday")
    }

    // 不带表达式的 `switch` 是实现 if/else 逻辑的另一种
    // 方式。这里展示了 `case` 表达式是如何使用非常量的。
    t := time.Now()
    switch {
    case t.Hour() < 12:
        fmt.Println("it's before noon")
    default:
        fmt.Println("it's after noon")
    }
}

// todo: type switches
```

结果:
```
write 2 as two
it's the weekend
it's before noon
```

### 数组
```
// 在 Go 中，_数组_ 是一个固定长度的数列。
package main

import "fmt"

func main() {

    // 这里我们创建了一个数组 `a` 来存放刚好 5 个 `int`。
    // 元素的类型和长度都是数组类型的一部分。数组默认是
    // 零值的，对于 `int` 数组来说也就是 `0`。
    var a [5]int
    fmt.Println("emp:", a)

    // 我们可以使用 `array[index] = value` 语法来设置数组
    // 指定位置的值，或者用 `array[index]` 得到值。
    a[4] = 100
    fmt.Println("set:", a)
    fmt.Println("get:", a[4])

    // 使用内置函数 `len` 返回数组的长度
    fmt.Println("len:", len(a))

    // 使用这个语法在一行内初始化一个数组
    b := [5]int{1, 2, 3, 4, 5}
    fmt.Println("dcl:", b)

    // 数组的存储类型是单一的，但是你可以组合这些数据
    // 来构造多维的数据结构。
    var twoD [2][3]int
    for i := 0; i < 2; i++ {
        for j := 0; j < 3; j++ {
            twoD[i][j] = i + j
        }
    }
    fmt.Println("2d: ", twoD)
}
```

结果:
```
emp: [0 0 0 0 0]
set: [0 0 0 0 100]
get: 100
len: 5
dcl: [1 2 3 4 5]
2d:  [[0 1 2] [1 2 3]]

```

### 切片
```
package main

import "fmt"

func main() {

    // 不想数组，slice 的类型仅有它所包含的元素决定（不像
    // 数组中还需要元素的个数）。要创建一个长度非零的空
    // slice，需要使用内建的方法 `make`。这里我们创建了一
    // 个长度为3的 `string` 类型 slice（初始化为零值）。
    s := make([]string, 3)
    fmt.Println("emp:", s)

    // 我们可以和数组一起设置和得到值
    s[0] = "a"
    s[1] = "b"
    s[2] = "c"
    fmt.Println("set:", s)
    fmt.Println("get:", s[2])

    // 如你所料，`len` 返回 slice 的长度
    fmt.Println("len:", len(s))

    // 作为基本操作的补充，slice 支持比数组更多的操作。
    // 其中一个是内建的 `append`，它返回一个包含了一个
    // 或者多个新值的 slice。注意我们接受返回由 append
    // 返回的新的 slice 值。
    s = append(s, "d")
    s = append(s, "e", "f")
    fmt.Println("apd:", s)

    // Slice 也可以被 `copy`。这里我们创建一个空的和 `s` 有
    // 相同长度的 slice `c`，并且将 `s` 复制给 `c`。
    c := make([]string, len(s))
    copy(c, s)
    fmt.Println("cpy:", c)

    // Slice 支持通过 `slice[low:high]` 语法进行“切片”操
    // 作。例如，这里得到一个包含元素 `s[2]`, `s[3]`,
    // `s[4]` 的 slice。
    l := s[2:5]
    fmt.Println("sl1:", l)

    // 这个 slice 从 `s[0]` 到（但是不包含）`s[5]`。
    l = s[:5]
    fmt.Println("sl2:", l)

    // 这个 slice 从（包含）`s[2]` 到 slice 的后一个值。
    l = s[2:]
    fmt.Println("sl3:", l)

    // 我们可以在一行代码中申明并初始化一个 slice 变量。
    t := []string{"g", "h", "i"}
    fmt.Println("dcl:", t)

    // Slice 可以组成多维数据结构。内部的 slice 长度可以不
    // 同，这和多位数组不同。
    twoD := make([][]int, 3)
    for i := 0; i < 3; i++ {
        innerLen := i + 1
        twoD[i] = make([]int, innerLen)
        for j := 0; j < innerLen; j++ {
            twoD[i][j] = i + j
        }
    }
    fmt.Println("2d: ", twoD)
}

```

结果:
```
emp: [  ]
set: [a b c]
get: c
len: 3
apd: [a b c d e f]
cpy: [a b c d e f]
sl1: [c d e]
sl2: [a b c d e]
sl3: [c d e f]
dcl: [g h i]
2d:  [[0] [1 2] [2 3 4]]

```

### 关联数组
```
// _map_ 是 Go 内置[关联数据类型](http://zh.wikipedia.org/wiki/关联数组)（
// 在一些其他的语言中称为_哈希_ 或者_字典_ ）。

package main

import "fmt"

func main() {

    // 要创建一个空 map，需要使用内建的 `make`:
    // `make(map[key-type]val-type)`.
    m := make(map[string]int)

    // 使用典型的 `make[key] = val` 语法来设置键值对。
    m["k1"] = 7
    m["k2"] = 13

    // 使用例如 `Println` 来打印一个 map 将会输出所有的
    // 键值对。
    fmt.Println("map:", m)

    // 使用 `name[key]` 来获取一个键的值
    v1 := m["k1"]
    fmt.Println("v1: ", v1)

    // 当对一个 map 调用内建的 `len` 时，返回的是键值对
    // 数目
    fmt.Println("len:", len(m))

    // 内建的 `delete` 可以从一个 map 中移除键值对
    delete(m, "k2")
    fmt.Println("map:", m)

    // 当从一个 map 中取值时，可选的第二返回值指示这个键
    // 是在这个 map 中。这可以用来消除键不存在和键有零值，
    // 像 `0` 或者 `""` 而产生的歧义。
    _, prs := m["k2"]
    fmt.Println("prs:", prs)

    // 你也可以通过这个语法在同一行申明和初始化一个新的
    // map。
    n := map[string]int{"foo": 1, "bar": 2}
    fmt.Println("map:", n)
}
```

结果:
```
map: map[k1:7 k2:13]
v1:  7
len: 2
map: map[k1:7]
prs: false
map: map[foo:1 bar:2]
```


### range遍历
```
// _range_ 迭代各种各样的数据结构。让我们来看看如何在我们
// 已经学过的数据结构上使用 `rang` 吧。

package main

import "fmt"

func main() {

    // 这里我们使用 `range` 来统计一个 slice 的元素个数。
    // 数组也可以采用这种方法。
    nums := []int{2, 3, 4}
    sum := 0
    for _, num := range nums {
        sum += num
    }
    fmt.Println("sum:", sum)

    // `range` 在数组和 slice 中都同样提供每个项的索引和
    // 值。上面我们不需要索引，所以我们使用 _空值定义符_
    // `_` 来忽略它。有时候我们实际上是需要这个索引的。
    for i, num := range nums {
        if num == 3 {
            fmt.Println("index:", i)
        }
    }

    // `range` 在 map 中迭代键值对。
    kvs := map[string]string{"a": "apple", "b": "banana"}
    for k, v := range kvs {
        fmt.Printf("%s -> %s\n", k, v)
    }

    // `range` 在字符串中迭代 unicode 编码。第一个返回值是
    // `rune` 的起始字节位置，然后第二个是 `rune` 自己。
    for i, c := range "go" {
        fmt.Println(i, c)
    }
}
```
结果:
```
sum: 9
index: 1
a -> apple
b -> banana
0 103
1 111
```


### 函数
```
// _函数_ 是 Go 的中心。我们将通过一些不同的例子来
// 进行学习。

package main

import "fmt"

// 这里是一个函数，接受两个 `int` 并且以 `int` 返回它
// 们的和
func plus(a int, b int) int {

    // Go 需要明确的返回值，例如，它不会自动返回最
    // 后一个表达式的值
    return a + b
}

func main() {

    // 正如你期望的那样，通过 `name(args)` 来调用一
    // 个函数，
    res := plus(1, 2)
    fmt.Println("1+2 =", res)
}

// todo: coalesced parameter types

```

结果:
```
1+2 = 3
```

### 多返回值
```
package main

import "fmt"

// `(int, int)` 在这个函数中标志着这个函数返回 2 个 `int`。
func vals() (int, int) {
    return 3, 7
}

func main() {

    // 这里我们通过_多赋值_ 操作来使用这两个不同的返回值。
    a, b := vals()
    fmt.Println(a)
    fmt.Println(b)

    // 如果你仅仅想返回值的一部分的话，你可以使用空白定
    // 义符 `_`。
    _, c := vals()
    fmt.Println(c)
}
```

结果:
```
3
7
7
```

### 变参函数
```
package main

import "fmt"

// 这个函数使用任意数目的 `int` 作为参数。
func sum(nums ...int) {
    fmt.Print(nums, " ")
    total := 0
    for _, num := range nums {
        total += num
    }
    fmt.Println(total)
}

func main() {

    // 变参函数使用常规的调用方式，除了参数比较特殊。
    sum(1, 2)
    sum(1, 2, 3)

    // 如果你的 slice 已经有了多个值，想把它们作为变参
    // 使用，你要这样调用 `func(slice...)`。
    nums := []int{1, 2, 3, 4}
    sum(nums...)
}

```

结果:
```
[1 2] 3
[1 2 3] 6
[1 2 3 4] 10
```

### 闭包
```
package main

import "fmt"

// 这个 `intSeq` 函数返回另一个在 `intSeq` 函数体内定义的
// 匿名函数。这个返回的函数使用闭包的方式 _隐藏_ 变量 `i`。
func intSeq() func() int {
    i := 0
    return func() int {
        i += 1
        return i
    }
}

func main() {

    // 我们调用 `intSeq` 函数，将返回值（也是一个函数）赋给
    // `nextInt`。这个函数的值包含了自己的值 `i`，这样在每
    // 次调用 `nextInt` 是都会更新 `i` 的值。
    nextInt := intSeq()

    // 通过多次调用 `nextInt` 来看看闭包的效果。
    fmt.Println(nextInt())
    fmt.Println(nextInt())
    fmt.Println(nextInt())

    // 为了确认这个状态对于这个特定的函数是唯一的，我们
    // 重新创建并测试一下。
    newInts := intSeq()
    fmt.Println(newInts())
}

```


结果:
```
1
2
3
1
```


### 递归
```
package main

import "fmt"

// `face` 函数在到达 `face(0)` 前一直调用自身。
func fact(n int) int {
    if n == 0 {
        return 1
    }
    return n * fact(n-1)
}

func main() {
    fmt.Println(fact(7))
}
```

结果:
```
5040
```

### 指针
```
package main

import "fmt"

// 我们将通过两个函数：`zeroval` 和 `zeroptr` 来比较指针和
// 值类型的不同。`zeroval` 有一个 `int` 型参数，所以使用值
// 传递。`zeroval` 将从调用它的那个函数中得到一个 `ival`
// 形参的拷贝。
func zeroval(ival int) {
    ival = 0
}

// `zeroptr` 有一和上面不同的 `*int` 参数，意味着它用了一
// 个 `int`指针。函数体内的 `*iptr` 接着_解引用_ 这个指针，
// 从它内存地址得到这个地址对应的当前值。对一个解引用的指
// 针赋值将会改变这个指针引用的真实地址的值。
func zeroptr(iptr *int) {
    *iptr = 0
}

func main() {
    i := 1
    fmt.Println("initial:", i)

    zeroval(i)
    fmt.Println("zeroval:", i)

    // 通过 `&i` 语法来取得 `i` 的内存地址，例如一个变量
    // `i` 的指针。
    zeroptr(&i)
    fmt.Println("zeroptr:", i)

    // 指针也是可以被打印的。
    fmt.Println("pointer:", &i)
}

```
结果:
```
initial: 1
zeroval: 1
zeroptr: 0
pointer: 0x42131100

```

### 结构体
```
package main

import "fmt"

// 这里的 `person` 结构体包含了 `name` 和 `age` 两个字段。
type person struct {
    name string
    age  int
}

func main() {

    // 使用这个语法创建了一个新的结构体元素。
    fmt.Println(person{"Bob", 20})

    // 你可以在初始化一个结构体元素时指定字段名字。
    fmt.Println(person{name: "Alice", age: 30})

    // 省略的字段将被初始化为零值。
    fmt.Println(person{name: "Fred"})

    // `&` 前缀生成一个结构体指针。
    fmt.Println(&person{name: "Ann", age: 40})

    // 使用点来访问结构体字段。
    s := person{name: "Sean", age: 50}
    fmt.Println(s.name)

    // 也可以对结构体指针使用`.` - 指针会被自动解引用。
    sp := &s
    fmt.Println(sp.age)

    // 结构体是可变的。
    sp.age = 51
    fmt.Println(sp.age)
}

```
结果:
```
{Bob 20}
{Alice 30}
{Fred 0}
&{Ann 40}
Sean
50
51
```

### 方法
```
// Go 支持在结构体类型中定义_方法_ 。

package main

import "fmt"

type rect struct {
    width, height int
}

// 这里的 `area` 方法有一个_接收器类型_ `rect`。
func (r *rect) area() int {
    r.width = 100
    return r.width * r.height
}

// 可以为值类型或者指针类型的接收器定义方法。这里是一个
// 值类型接收器的例子。
func (r rect) perim() int {
    r.width = 100
    return 2*r.width + 2*r.height
}

func main() {
    r := rect{width: 10, height: 5}

    // 这里我们调用上面为结构体定义的两个方法。
    fmt.Println("perim:", r.perim())
    fmt.Println(r.width)
    fmt.Println("area: ", r.area())
    fmt.Println(r.width)
    
    r.width=10
    // Go 自动处理方法调用时的值和指针之间的转化。你可以使
    // 用指针来调用方法来避免在方法调用时产生一个拷贝，或者
    // 让方法能够改变接受的数据。
    rp := &r
    fmt.Println("perim:", rp.perim())
    fmt.Println(rp.width)
    fmt.Println("area: ", rp.area())
    fmt.Println(rp.width)
}
```

结果：
```
perim: 210
10
area:  500
100
perim: 210
10
area:  500
100
```

### 接口
```
// _接口_ 是方法特征的命名集合。

package main

import "fmt"
import "math"

// 这里是一个几何体的基本接口。
type geometry interface {
    area() float64
    perim() float64
}

// 在我们的例子中，我们将让 `square` 和 `circle` 实现
// 这个接口
type square struct {
    width, height float64
}
type circle struct {
    radius float64
}

// 要在 Go 中实现一个接口，我们只需要实现接口中的所有
// 方法。这里我们让 `square` 实现了 `geometry` 接口。
func (s square) area() float64 {
    return s.width * s.height
}
func (s square) perim() float64 {
    return 2*s.width + 2*s.height
}

// `circle` 的实现。
func (c circle) area() float64 {
    return math.Pi * c.radius * c.radius
}
func (c circle) perim() float64 {
    return 2 * math.Pi * c.radius
}

// 如果一个变量的是接口类型，那么我们可以调用这个被命名的
// 接口中的方法。这里有一个一通用的 `measure` 函数，利用这个
// 特性，它可以用在任何 `geometry` 上。
func measure(g geometry) {
    fmt.Println(g)
    fmt.Println(g.area())
    fmt.Println(g.perim())
}

func main() {
    s := square{width: 3, height: 4}
    c := circle{radius: 5}

    // 结构体类型 `circle` 和 `square` 都实现了 `geometry`
    // 接口，所以我们可以使用它们的实例作为 `measure` 的参数。
    measure(s)
    measure(c)
}

```

运行程序:
```
{3 4}
12
14
{5}
78.53981633974483
31.41592653589793
```
[接口](http://jordanorelli.tumblr.com/post/32665860244/how-to-use-interfaces-in-go)。

### 错误处理
```
package main

import "errors"
import "fmt"

// 按照惯例，错误通常是最后一个返回值并且是 `error` 类
// 型，一个内建的接口。
func f1(arg int) (int, error) {
    if arg == 42 {

        // `errors.New` 构造一个使用给定的错误信息的基本
        // `error` 值。
        return -1, errors.New("can't work with 42")

    }

    // 返回错误值为 nil 代表没有错误。
    return arg + 3, nil
}

// 通过实现 `Error` 方法来自定义 `error` 类型是可以得。
// 这里使用自定义错误类型来表示上面的参数错误。
type argError struct {
    arg  int
    prob string
}

func (e *argError) Error() string {
    return fmt.Sprintf("%d - %s", e.arg, e.prob)
}

func f2(arg int) (int, error) {
    if arg == 42 {

        // 在这个例子中，我们使用 `&argError` 语法来建立一个
        // 新的结构体，并提供了 `arg` 和 `prob` 这个两个字段
        // 的值。
        return -1, &argError{arg, "can't work with it"}
    }
    return arg + 3, nil
}

func main() {

    // 下面的两个循环测试了各个返回错误的函数。注意在 `if`
    // 行内的错误检查代码，在 Go 中是一个普遍的用法。
    for _, i := range []int{7, 42} {
        if r, e := f1(i); e != nil {
            fmt.Println("f1 failed:", e)
        } else {
            fmt.Println("f1 worked:", r)
        }
    }
    for _, i := range []int{7, 42} {
        if r, e := f2(i); e != nil {
            fmt.Println("f2 failed:", e)
        } else {
            fmt.Println("f2 worked:", r)
        }
    }

    // 你如果想在程序中使用一个自定义错误类型中的数据，你
    // 需要通过类型断言来得到这个错误类型的实例。
    _, e := f2(42)
    if ae, ok := e.(*argError); ok {
        fmt.Println(ae.arg)
        fmt.Println(ae.prob)
    }
}
```
结果:

```
f1 worked: 10
f1 failed: can't work with 42
f2 worked: 10
f2 failed: 42 - can't work with it
42
can't work with it

```


### 协程
```
package main

import "fmt"

func f(from string) {
    for i := 0; i < 3; i++ {
        fmt.Println(from, ":", i)
    }
}

func main() {

    // 假设我们有一个函数叫做 `f(s)`。我们使用一般的方式
    // 调并同时运行。
    f("direct")

    // 使用 `go f(s)` 在一个 Go 协程中调用这个函数。
    // 这个新的 Go 协程将会并行的执行这个函数调用。
    go f("goroutine")

    // 你也可以为匿名函数启动一个 Go 协程。
    go func(msg string) {
        fmt.Println(msg)
    }("going")

    // 现在这两个 Go 协程在独立的 Go 协程中异步的运行，所以
    // 我们需要等它们执行结束。这里的 `Scanln` 代码需要我们
    // 在程序退出前按下任意键结束。
    var input string
    fmt.Scanln(&input)
    fmt.Println("done")
}

```


结果:
```
direct : 0
direct : 1
direct : 2
goroutine : 0
going
goroutine : 1
goroutine : 2
<enter>
done
```



### 通道
```
// _通道_ 是连接多个 Go 协程的管道。你可以从一个 Go 协程
// 将值发送到通道，然后在别的 Go 协程中接收。

package main

import "fmt"

func main() {

    // 使用 `make(chan val-type)` 创建一个新的通道。
    // 通道类型就是他们需要传递值的类型。
    messages := make(chan string)

    // 使用 `channel <-` 语法 _发送_ 一个新的值到通道中。这里
    // 我们在一个新的 Go 协程中发送 `"ping"` 到上面创建的
    // `messages` 通道中。
    go func() { messages <- "ping" }()

    // 使用 `<-channel` 语法从通道中 _接收_ 一个值。这里
    // 将接收我们在上面发送的 `"ping"` 消息并打印出来。
    msg := <-messages
    fmt.Println(msg)
}


```
结果:
```
ping
```


### 通道缓冲
```
// 默认通道是 _无缓冲_ 的，这意味着只有在对应的接收（`<- chan`）
// 通道准备好接收时，才允许进行发送（`chan <-`）。_可缓存通道_
// 允许在没有对应接收方的情况下，缓存限定数量的值。

package main

import "fmt"

func main() {

    // 这里我们 `make` 了一个通道，最多允许缓存 2 个值。
    messages := make(chan string, 2)

    // 因为这个通道是有缓冲区的，即使没有一个对应的并发接收
    // 方，我们仍然可以发送这些值。
    messages <- "buffered"
    messages <- "channel"

    // 然后我们可以像前面一样接收这两个值。
    fmt.Println(<-messages)
    fmt.Println(<-messages)
}

```

结果:
```
buffered
channel
```

### 通道同步
```
// 我们可以使用通道来同步 Go 协程间的执行状态。这里是一个
// 使用阻塞的接受方式来等待一个 Go 协程的运行结束。

package main

import "fmt"
import "time"

// 这是一个我们将要在 Go 协程中运行的函数。`done` 通道
// 将被用于通知其他 Go 协程这个函数已经工作完毕。
func worker(done chan bool) {
    fmt.Print("working...")
    time.Sleep(time.Second)
    fmt.Println("done")

    // 发送一个值来通知我们已经完工啦。
    done <- true
}

func main() {

    // 运行一个 worker Go协程，并给予用于通知的通道。
    done := make(chan bool, 1)
    go worker(done)

    // 程序将在接收到通道中 worker 发出的通知前一直阻塞。
    <-done
}

```

结果:
```
working...done

# 如果你把 `<- done` 这行代码从程序中移除，程序甚至会在 `worker`
# 还没开始运行时就结束了。

```

### 通道方向
```
// 当使用通道作为函数的参数时，你可以指定这个通道是不是
// 只用来发送或者接收值。这个特性提升了程序的类型安全性。

package main

import "fmt"

// `ping` 函数定义了一个只允许发送数据的通道。尝试使用这个通
// 道来接收数据将会得到一个编译时错误。
func ping(pings chan<- string, msg string) {
    pings <- msg
}

// `pong` 函数允许通道（`pings`）来接收数据，另一通道
// （`pongs`）来发送数据。
func pong(pings <-chan string, pongs chan<- string) {
    msg := <-pings
    pongs <- msg
}

func main() {
    pings := make(chan string, 1)
    pongs := make(chan string, 1)
    ping(pings, "passed message")
    pong(pings, pongs)
    fmt.Println(<-pongs)
}
```
结果:
```
passed message

```


### 通道选择器
```
// Go 的_通道选择器_ 让你可以同时等待多个通道操作。
// Go 协程和通道以及选择器的结合是 Go 的一个强大特性。

package main

import "time"
import "fmt"

func main() {

    // 在我们的例子中，我们将从两个通道中选择。
    c1 := make(chan string)
    c2 := make(chan string)

    // 各个通道将在若干时间后接收一个值，这个用来模拟例如
    // 并行的 Go 协程中阻塞的 RPC 操作
    go func() {
        time.Sleep(time.Second * 1)
        c1 <- "one"
    }()
    go func() {
        time.Sleep(time.Second * 2)
        c2 <- "two"
    }()

    // 我们使用 `select` 关键字来同时等待这两个值，并打
    // 印各自接收到的值。
    for i := 0; i < 2; i++ {
        select {
        case msg1 := <-c1:
            fmt.Println("received", msg1)
        case msg2 := <-c2:
            fmt.Println("received", msg2)
        }
    }
}

```
结果:
```
$ time go run select.go 
received one
received two

# 注意从第一次和第二次 `Sleeps` 并发执行，总共仅运行了
# 两秒左右。
real    0m2.245s
user	0m0.311s
sys	    0m0.115s
```


### 超时处理

```
// _超时_ 对于一个连接外部资源，或者其它一些需要花费执行时间
// 的操作的程序而言是很重要的。得益于通道和 `select`，在 Go
// 中实现超时操作是简洁而优雅的。

package main

import "time"
import "fmt"

func main() {

    // 在我们的例子中，假如我们执行一个外部调用，并在 2 秒后
    // 通过通道 `c1` 返回它的执行结果。
    c1 := make(chan string, 1)
    go func() {
        time.Sleep(time.Second * 2)
        c1 <- "result 1"
    }()

    // 这里是使用 `select` 实现一个超时操作。
    // `res := <- c1` 等待结果，`<-Time.After` 等待超时
    // 时间 1 秒后发送的值。由于 `select` 默认处理第一个
    // 已准备好的接收操作，如果这个操作超过了允许的 1 秒
    // 的话，将会执行超时 case。
    select {
    case res := <-c1:
        fmt.Println(res)
    case <-time.After(time.Second * 1):
        fmt.Println("timeout 1")
    }

    // 如果我允许一个长一点的超时时间 3 秒，将会成功的从 `c2`
    // 接收到值，并且打印出结果。
    c2 := make(chan string, 1)
    go func() {
        time.Sleep(time.Second * 2)
        c2 <- "result 2"
    }()
    select {
    case res := <-c2:
        fmt.Println(res)
    case <-time.After(time.Second * 3):
        fmt.Println("timeout 2")
    }
}

// todo: cancellation?

```

结果:
```
timeout 1
result 2
```
### 非阻塞通道操作
```
// 常规的通过通道发送和接收数据是阻塞的。然而，我们可以
// 使用带一个 `default` 子句的 `select` 来实现_非阻塞_ 的
// 发送、接收，甚至是非阻塞的多路 `select`。

package main

import "fmt"

func main() {
    messages := make(chan string)
    signals := make(chan bool)

    // 这里是一个非阻塞接收的例子。如果在 `messages` 中
    // 存在，然后 `select` 将这个值带入 `<-messages` `case`
    // 中。如果不是，就直接到 `default` 分支中。
    select {
    case msg := <-messages:
        fmt.Println("received message", msg)
    default:
        fmt.Println("no message received")
    }

    // 一个非阻塞发送的实现方法和上面一样。
    msg := "hi"
    select {
    case messages <- msg:
        fmt.Println("sent message", msg)
    default:
        fmt.Println("no message sent")
    }

    // 我们可以在 `default` 前使用多个 `case` 子句来实现
    // 一个多路的非阻塞的选择器。这里我们视图在 `messages`
    // 和 `signals` 上同时使用非阻塞的接受操作。
    select {
    case msg := <-messages:
        fmt.Println("received message", msg)
    case sig := <-signals:
        fmt.Println("received signal", sig)
    default:
        fmt.Println("no activity")
    }
}
```
结果:

```
no message received
no message sent
no activity

```


### 通道的关闭
```
// _关闭_ 一个通道意味着不能再向这个通道发送值了。这个特性可以
// 用来给这个通道的接收方传达工作已将完成的信息。

package main

import "fmt"

// 在这个例子中，我们将使用一个 `jobs` 通道来传递 `main()` 中 Go
// 协程任务执行的结束信息到一个工作 Go 协程中。当我们没有多余的
// 任务给这个工作 Go 协程时，我们将 `close` 这个 `jobs` 通道。
func main() {
    jobs := make(chan int, 5)
    done := make(chan bool)

    // 这是工作 Go 协程。使用 `j, more := <- jobs` 循环的从
    // `jobs` 接收数据。在接收的这个特殊的二值形式的值中，
    // 如果 `jobs` 已经关闭了，并且通道中所有的值都已经接收
    // 完毕，那么 `more` 的值将是 `false`。当我们完成所有
    // 的任务时，将使用这个特性通过 `done` 通道去进行通知。
    go func() {
        for {
            j, more := <-jobs
            if more {
                fmt.Println("received job", j)
            } else {
                fmt.Println("received all jobs")
                done <- true
                return
            }
        }
    }()

    // 这里使用 `jobs` 发送 3 个任务到工作函数中，然后
    // 关闭 `jobs`。
    for j := 1; j <= 3; j++ {
        jobs <- j
        fmt.Println("sent job", j)
    }
    close(jobs)
    fmt.Println("sent all jobs")

    // 我们使用前面学到的[通道同步](../channel-synchronization/)
    // 方法等待任务结束。
    <-done
}
```


结果:
```
sent job 1
received job 1
sent job 2
received job 2
sent job 3
received job 3
sent all jobs
received all jobs
```


### 通道遍历
```
// 在[前面](../range/)的例子中，我们讲过 `for` 和 `range`
// 为基本的数据结构提供了迭代的功能。我们也可以使用这个语法
// 来遍历从通道中取得的值。

package main

import "fmt"

func main() {

    // 我们将遍历在 `queue` 通道中的两个值。
    queue := make(chan string, 2)
    queue <- "one"
    queue <- "two"
    close(queue)

    // 这个 `range` 迭代从 `queue` 中得到的每个值。因为我们
    // 在前面 `close` 了这个通道，这个迭代会在接收完 2 个值
    // 之后结束。如果我们没有 `close` 它，我们将在这个循环中
    // 继续阻塞执行，等待接收第三个值
    for elem := range queue {
        fmt.Println(elem)
    }
}

```
结果:
```
one
two
```


### 定时器
```
package main

import "time"
import "fmt"

func main() {

    // 定时器表示在未来某一时刻的独立事件。你告诉定时器
    // 需要等待的时间，然后它将提供一个用于通知的通道。
    // 这里的定时器将等待 2 秒。
    timer1 := time.NewTimer(time.Second * 2)

    // `<-timer1.C` 直到这个定时器的通道 `C` 明确的发送了
    // 定时器失效的值之前，将一直阻塞。
    <-timer1.C
    fmt.Println("Timer 1 expired")

    // 如果你需要的仅仅是单纯的等待，你需要使用 `time.Sleep`。
    // 定时器是有用原因之一就是你可以在定时器失效之前，取消这个
    // 定时器。这是一个例子
    timer2 := time.NewTimer(time.Second)
    go func() {
        <-timer2.C
        fmt.Println("Timer 2 expired")
    }()
    stop2 := timer2.Stop()
    if stop2 {
        fmt.Println("Timer 2 stopped")
    }
}


```


结果:
```
Timer 1 expired
Timer 2 stopped

```

### 打点器
```
// [定时器](../timers/) 是当你想要在未来某一刻执行一次时
// 使用的 - _打点器_ 则是当你想要在固定的时间间隔重复执行
// 准备的。这里是一个打点器的例子，它将定时的执行，直到我
// 们将它停止。

package main

import "time"
import "fmt"

func main() {

    // 打点器和定时器的机制有点相似：一个通道用来发送数据。
    // 这里我们在这个通道上使用内置的 `range` 来迭代值每隔
    // 500ms 发送一次的值。
    ticker := time.NewTicker(time.Millisecond * 500)
    go func() {
        for t := range ticker.C {
            fmt.Println("Tick at", t)
        }
    }()

    // 打点器可以和定时器一样被停止。一旦一个打点停止了，
    // 将不能再从它的通道中接收到值。我们将在运行后 1500ms
    // 停止这个打点器。
    time.Sleep(time.Millisecond * 1500)
    ticker.Stop()
    fmt.Println("Ticker stopped")
}

```

结果:
```
Tick at 2012-09-23 11:29:56.487625 -0700 PDT
Tick at 2012-09-23 11:29:56.988063 -0700 PDT
Tick at 2012-09-23 11:29:57.488076 -0700 PDT
Ticker stopped


```

### 工作池
```
// 在这个例子中，我们将看到如何使用 Go  协程和通道
// 实现一个_工作池_ 。

package main

import "fmt"
import "time"

// 这是我们将要在多个并发实例中支持的任务了。这些执行者
// 将从 `jobs` 通道接收任务，并且通过 `results` 发送对应
// 的结果。我们将让每个任务间隔 1s 来模仿一个耗时的任务。
func worker(id int, jobs <-chan int, results chan<- int) {
    for j := range jobs {
        fmt.Println("worker", id, "processing job", j)
        time.Sleep(time.Second)
        results <- j * 2
    }
}

func main() {

    // 为了使用 worker 工作池并且收集他们的结果，我们需要
    // 2 个通道。
    jobs := make(chan int, 100)
    results := make(chan int, 100)

    // 这里启动了 3 个 worker，初始是阻塞的，因为
    // 还没有传递任务。
    for w := 1; w <= 3; w++ {
        go worker(w, jobs, results)
    }

    // 这里我们发送 9 个 `jobs`，然后 `close` 这些通道
    // 来表示这些就是所有的任务了。

    for j := 1; j <= 9; j++ {
        jobs <- j
    }
    close(jobs)

    // 最后，我们收集所有这些任务的返回值。
    for a := 1; a <= 9; a++ {
        <-results
    }
}

```

结果:
```
$time go run worker-pools.go 
worker 1 processing job 1
worker 2 processing job 2
worker 3 processing job 3
worker 1 processing job 4
worker 2 processing job 5
worker 3 processing job 6
worker 1 processing job 7
worker 2 processing job 8
worker 3 processing job 9

real    0m3.149s
```

### 速率限制
```
// [_速率限制(英)_](http://en.wikipedia.org/wiki/Rate_limiting) 是
// 一个重要的控制服务资源利用和质量的途径。Go 通过 Go 协程、通
// 道和[打点器](../tickers/)优美的支持了速率限制。

package main

import "time"
import "fmt"

func main() {

    // 首先我们将看一下基本的速率限制。假设我们想限制我们
    // 接收请求的处理，我们将这些请求发送给一个相同的通道。
    requests := make(chan int, 5)
    for i := 1; i <= 5; i++ {
        requests <- i
    }
    close(requests)

    // 这个 `limiter` 通道将每 200ms 接收一个值。这个是
    // 速率限制任务中的管理器。
    limiter := time.Tick(time.Millisecond * 200)

    // 通过在每次请求前阻塞 `limiter` 通道的一个接收，我们限制
    // 自己每 200ms 执行一次请求。
    for req := range requests {
        <-limiter
        fmt.Println("request", req, time.Now())
    }

    // 有时候我们想临时进行速率限制，并且不影响整体的速率控制
    // 我们可以通过[通道缓冲](channel-buffering.html)来实现。
    // 这个 `burstyLimiter` 通道用来进行 3 次临时的脉冲型速率限制。
    burstyLimiter := make(chan time.Time, 3)

    // 想将通道填充需要临时改变次的值，做好准备。
    for i := 0; i < 3; i++ {
        burstyLimiter <- time.Now()
    }

    // 每 200 ms 我们将添加一个新的值到 `burstyLimiter`中，
    // 直到达到 3 个的限制。
    go func() {
        for t := range time.Tick(time.Millisecond * 200) {
            burstyLimiter <- t
        }
    }()

    // 现在模拟超过 5 个的接入请求。它们中刚开始的 3 个将
    // 由于受 `burstyLimiter` 的“脉冲”影响。
    burstyRequests := make(chan int, 5)
    for i := 1; i <= 5; i++ {
        burstyRequests <- i
    }
    close(burstyRequests)
    for req := range burstyRequests {
        <-burstyLimiter
        fmt.Println("request", req, time.Now())
    }
}

```
结果:
```
request 1 2012-10-19 00:38:18.687438 +0000 UTC
request 2 2012-10-19 00:38:18.887471 +0000 UTC
request 3 2012-10-19 00:38:19.087238 +0000 UTC
request 4 2012-10-19 00:38:19.287338 +0000 UTC
request 5 2012-10-19 00:38:19.487331 +0000 UTC

# 第二批请求，我们直接连续处理了 3 次，这是由于这个“脉冲”
# 速率控制，然后大约每 200ms 处理其余的 2 个。
request 1 2012-10-19 00:38:20.487578 +0000 UTC
request 2 2012-10-19 00:38:20.487645 +0000 UTC
request 3 2012-10-19 00:38:20.487676 +0000 UTC
request 4 2012-10-19 00:38:20.687483 +0000 UTC
request 5 2012-10-19 00:38:20.887542 +0000 UTC

```


### 原子计数器
```
// Go 中最主要的状态管理方式是通过通道间的沟通来完成的，我们
// 在[工作池](../worker-pools/)的例子中碰到过，但是还是有一
// 些其他的方法来管理状态的。这里我们将看看如何使用 `sync/atomic`
// 包在多个 Go 协程中进行 _原子计数_ 。

package main

import "fmt"
import "time"
import "sync/atomic"
import "runtime"

func main() {

    // 我们将使用一个无符号整形数来表示（永远是正整数）这个计数器。
    var ops uint64 = 0

    // 为了模拟并发更新，我们启动 50 个 Go 协程，对计数
    // 器每隔 1ms （译者注：应为非准确时间）进行一次加一操作。
    for i := 0; i < 50; i++ {
        go func() {
            for {
                // 使用 `AddUint64` 来让计数器自动增加，使用
                // `&` 语法来给出 `ops` 的内存地址。
                atomic.AddUint64(&ops, 1)

                // 允许其它 Go 协程的执行
                runtime.Gosched()
            }
        }()
    }

    // 等待一秒，让 ops 的自加操作执行一会。
    time.Sleep(time.Second)

    // 为了在计数器还在被其它 Go 协程更新时，安全的使用它，
    // 我们通过 `LoadUint64` 将当前值得拷贝提取到 `opsFinal`
    // 中。和上面一样，我们需要给这个函数所取值的内存地址 `&ops`
    opsFinal := atomic.LoadUint64(&ops)
    fmt.Println("ops:", opsFinal)
}

```
结果:
```
ops:40200
```

### 互斥锁
```
// 在前面的例子中，我们看到了如何使用原子操作来管理简单的计数器。
// 对于更加复杂的情况，我们可以使用一个_[互斥锁](http://zh.wikipedia.org/wiki/%E4%BA%92%E6%96%A5%E9%94%81)_
// 来在 Go 协程间安全的访问数据。

package main

import (
    "fmt"
    "math/rand"
    "runtime"
    "sync"
    "sync/atomic"
    "time"
)

func main() {

    // 在我们的例子中，`state` 是一个 map。
    var state = make(map[int]int)

    // 这里的 `mutex` 将同步对 `state` 的访问。
    var mutex = &sync.Mutex{}

    // we'll see later, `ops` will count how many
    // operations we perform against the state.
    // 为了比较基于互斥锁的处理方式和我们后面将要看到的其他
    // 方式，`ops` 将记录我们对 state 的操作次数。
    var ops int64 = 0

    // 这里我们运行 100 个 Go 协程来重复读取 state。
    for r := 0; r < 100; r++ {
        go func() {
            total := 0
            for {

                // 每次循环读取，我们使用一个键来进行访问，
                // `Lock()` 这个 `mutex` 来确保对 `state` 的
                // 独占访问，读取选定的键的值，`Unlock()` 这个
                // mutex，并且 `ops` 值加 1。
                key := rand.Intn(5)
                mutex.Lock()
                total += state[key]
                mutex.Unlock()
                atomic.AddInt64(&ops, 1)

                // 为了确保这个 Go 协程不会再调度中饿死，我们
                // 在每次操作后明确的使用 `runtime.Gosched()`
                // 进行释放。这个释放一般是自动处理的，像例如
                // 每个通道操作后或者 `time.Sleep` 的阻塞调用后
                // 相似，但是在这个例子中我们需要手动的处理。
                runtime.Gosched()
            }
        }()
    }

    // 同样的，我们运行 10 个 Go 协程来模拟写入操作，使用
    // 和读取相同的模式。
    for w := 0; w < 10; w++ {
        go func() {
            for {
                key := rand.Intn(5)
                val := rand.Intn(100)
                mutex.Lock()
                state[key] = val
                mutex.Unlock()
                atomic.AddInt64(&ops, 1)
                runtime.Gosched()
            }
        }()
    }

    // 让这 10 个 Go 协程对 `state` 和 `mutex` 的操作
    // 运行 1 s。
    time.Sleep(time.Second)

    // 获取并输出最终的操作计数。
    opsFinal := atomic.LoadInt64(&ops)
    fmt.Println("ops:", opsFinal)

    // 对 `state` 使用一个最终的锁，显示它是如何结束的。
    mutex.Lock()
    fmt.Println("state:", state)
    mutex.Unlock()
}

```

结果:
```
ops: 3598302
state: map[1:38 4:98 2:23 3:85 0:44]

```

### go状态协程
```
// 在前面的例子中，我们用互斥锁进行了明确的锁定来让共享的
// state 跨多个 Go 协程同步访问。另一个选择是使用内置的 Go
// 协程和通道的的同步特性来达到同样的效果。这个基于通道的方
// 法和 Go 通过通信以及    每个 Go 协程间通过通讯来共享内存，确
// 保每块数据有单独的 Go 协程所有的思路是一致的。

package main

import (
    "fmt"
    "math/rand"
    "sync/atomic"
    "time"
)

// 在这个例子中，state 将被一个单独的 Go 协程拥有。这就
// 能够保证数据在并行读取时不会混乱。为了对 state 进行
// 读取或者写入，其他的 Go 协程将发送一条数据到拥有的 Go
// 协程中，然后接收对应的回复。结构体 `readOp` 和 `writeOp`
// 封装这些请求，并且是拥有 Go 协程响应的一个方式。
type readOp struct {
    key  int
    resp chan int
}
type writeOp struct {
    key  int
    val  int
    resp chan bool
}

func main() {

    // 和前面一样，我们将计算我们执行操作的次数。
    var ops int64

    // `reads` 和 `writes` 通道分别将被其他 Go 协程用来发
    // 布读和写请求。
    reads := make(chan *readOp)
    writes := make(chan *writeOp)

    // 这个就是拥有 `state` 的那个 Go 协程，和前面例子中的
    // map一样，不过这里是被这个状态协程私有的。这个 Go 协程
    // 反复响应到达的请求。先响应到达的请求，然后返回一个值到
    // 响应通道 `resp` 来表示操作成功（或者是 `reads` 中请求的值）
    go func() {
        var state = make(map[int]int)
        for {
            select {
            case read := <-reads:
                read.resp <- state[read.key]
            case write := <-writes:
                state[write.key] = write.val
                write.resp <- true
            }
        }
    }()

    // 启动 100 个 Go 协程通过 `reads` 通道发起对 state 所有者
    // Go 协程的读取请求。每个读取请求需要构造一个 `readOp`，
    // 发送它到 `reads` 通道中，并通过给定的 `resp` 通道接收
    // 结果。
    for r := 0; r < 100; r++ {
        go func() {
            for {
                read := &readOp{
                    key:  rand.Intn(5),
                    resp: make(chan int)}
                reads <- read
                <-read.resp
                atomic.AddInt64(&ops, 1)
            }
        }()
    }

    // 用相同的方法启动 10 个写操作。
    for w := 0; w < 10; w++ {
        go func() {
            for {
                write := &writeOp{
                    key:  rand.Intn(5),
                    val:  rand.Intn(100),
                    resp: make(chan bool)}
                writes <- write
                <-write.resp
                atomic.AddInt64(&ops, 1)
            }
        }()
    }

    // 让 Go 协程们跑 1s。
    time.Sleep(time.Second)

    // 最后，获取并报告 `ops` 值。
    opsFinal := atomic.LoadInt64(&ops)
    fmt.Println("ops:", opsFinal)
}

```

结果:
```
ops:807434
```

### 排序
```
// Go 的 `sort` 包实现了内置和用户自定义数据类型的排序
// 功能。我们首先关注内置数据类型的排序。

package main

import "fmt"
import "sort"

func main() {

    // 排序方法是正对内置数据类型的；这里是一个字符串的例子。
    // 注意排序是原地更新的，所以他会改变给定的序列并且不返回
    // 一个新值。
    strs := []string{"c", "a", "b"}
    sort.Strings(strs)
    fmt.Println("Strings:", strs)

    // 一个 `int` 排序的例子。
    ints := []int{7, 2, 4}
    sort.Ints(ints)
    fmt.Println("Ints:   ", ints)

    // 我们也可以使用 `sort` 来检查一个序列是不是已经
    // 是排好序的。
    s := sort.IntsAreSorted(ints)
    fmt.Println("Sorted: ", s)
}

```

结果:
```
Strings: [a b c]
Ints:    [2 4 7]
Sorted:  true
```

### 使用函数自定义排序
```
// 有时候我们想使用和集合的自然排序不同的方法对集合进行排序。
// 例如，我们想按照字母的长度而不是首字母顺序对字符串排序。
// 这里是一个 Go 自定义排序的例子。

package main

import "sort"
import "fmt"

// 为了在 Go 中使用自定义函数进行排序，我们需要一个对应的
// 类型。这里我们创建一个为内置 `[]string` 类型的别名的
// `ByLength` 类型，
type ByLength []string

// 我们在类型中实现了 `sort.Interface` 的 `Len`，`Less`
// 和 `Swap` 方法，这样我们就可以使用 `sort` 包的通用
// `Sort` 方法了，`Len` 和 `Swap` 通常在各个类型中都差
// 不多，`Less` 将控制实际的自定义排序逻辑。在我们的例
// 子中，我们想按字符串长度增加的顺序来排序，所以这里
// 使用了 `len(s[i])` 和 `len(s[j])`。
func (s ByLength) Len() int {
    return len(s)
}
func (s ByLength) Swap(i, j int) {
    s[i], s[j] = s[j], s[i]
}
func (s ByLength) Less(i, j int) bool {
    return len(s[i]) < len(s[j])
}

// 一切都准备好了，我们现在可以通过将原始的 `fruits` 切片转型成 `ByLength` 来实现我们的自定排序了。然后对这个转型的切片使用 `sort.Sort` 方法。
func main() {
    fruits := []string{"peach", "banana", "kiwi"}
    sort.Sort(ByLength(fruits))
    fmt.Println(fruits)
}


```

结果:
```
[kiwi peach banana]
```

### panic
```
// `panic` 意味着有些出乎意料的错误发生。通常我们用它
// 来表示程序正常运行中不应该出现的，后者我么没有处理
// 好的错误。

package main

import "os"

func main() {

    // 我们将在真个网站中使用 panic 来检查预期外的错误。这个
    // 是唯一一个为 panic 准备的例子。
    panic("a problem")

    // panic 的一个基本用法就是在一个函数返回了错误值但是我们并不知道（或
    // 者不想）处理时终止运行。这里是一个在创建一个新文件时返回异常错误时的
    // `panic` 用法。
    _, err := os.Create("/tmp/file")
    if err != nil {
        panic(err)
    }
}

```

结果:
```
panic: a problem

goroutine 1 [running]:
main.main()
    /.../panic.go:12 +0x47
...
exit status 2

```


### defer
```
// _Defer_ 被用来确保一个函数调用在程序执行结束前执行。同
// 样用来执行一些清理工作。 `defer` 用在像其他语言中的
// `ensure` 和 `finally`用到的地方。

package main

import "fmt"
import "os"

// 假设我们想要创建一个文件，向它进行写操作，然后在结束
// 时关闭它。这里展示了如何通过 `defer` 来做到这一切。
func main() {

    // 在 `closeFile` 后得到一个文件对象，我们使用 defer
    // 通过 `closeFile` 来关闭这个文件。这会在封闭函数
    // （`main`）结束时执行，就是 `writeFile` 结束后。
    f := createFile("/tmp/defer.txt")
    defer closeFile(f)
    writeFile(f)
}

func createFile(p string) *os.File {
    fmt.Println("creating")
    f, err := os.Create(p)
    if err != nil {
        panic(err)
    }
    return f
}

func writeFile(f *os.File) {
    fmt.Println("writing")
    fmt.Fprintln(f, "data")

}

func closeFile(f *os.File) {
    fmt.Println("closing")
    f.Close()
}

```

结果:
```
creating
writing
closing
```

### 组合函数
```
// 我们经常需要程序在数据集上执行操作，比如选择满足给定条件
// 的所有项，或者将所有的项通过一个自定义函数映射到一个新的
// 集合上。

// 在某些语言中，会习惯使用[泛型](http://zh.wikipedia.org/wiki/%E6%B3%9B%E5%9E%8B)。
// Go 不支持泛型，在 Go 中，当你的程序或者数据类型需要
// 时，通常是通过组合的方式来提供操作函数。

// 这是一些 `strings` 切片的组合函数示例。你可以使用这些例
// 子来构建自己的函数。注意有时候，直接使用内联组合操作代
// 码会更清晰，而不是创建并调用一个帮助函数。

package main

import "strings"
import "fmt"

// 返回目标字符串 `t` 出现的的第一个索引位置，或者在没有匹
// 配值时返回 -1。
func Index(vs []string, t string) int {
    for i, v := range vs {
        if v == t {
            return i
        }
    }
    return -1
}

// 如果目标字符串 `t` 在这个切片中则返回 `true`。
func Include(vs []string, t string) bool {
    return Index(vs, t) >= 0
}

// 如果这些切片中的字符串有一个满足条件 `f` 则返回
// `true`。
func Any(vs []string, f func(string) bool) bool {
    for _, v := range vs {
        if f(v) {
            return true
        }
    }
    return false
}

// 如果切片中的所有字符串都满足条件 `f` 则返回 `true`。
func All(vs []string, f func(string) bool) bool {
    for _, v := range vs {
        if !f(v) {
            return false
        }
    }
    return true
}

// 返回一个包含所有切片中满足条件 `f` 的字符串的新切片。
func Filter(vs []string, f func(string) bool) []string {
    vsf := make([]string, 0)
    for _, v := range vs {
        if f(v) {
            vsf = append(vsf, v)
        }
    }
    return vsf
}

// 返回一个对原始切片中所有字符串执行函数 `f` 后的新切片。
func Map(vs []string, f func(string) string) []string {
    vsm := make([]string, len(vs))
    for i, v := range vs {
        vsm[i] = f(v)
    }
    return vsm
}

func main() {

    // 这里试试这些组合函数。
    var strs = []string{"peach", "apple", "pear", "plum"}

    fmt.Println(Index(strs, "pear"))

    fmt.Println(Include(strs, "grape"))

    fmt.Println(Any(strs, func(v string) bool {
        return strings.HasPrefix(v, "p")
    }))

    fmt.Println(All(strs, func(v string) bool {
        return strings.HasPrefix(v, "p")
    }))

    fmt.Println(Filter(strs, func(v string) bool {
        return strings.Contains(v, "e")
    }))

    // 上面的例子都是用的匿名函数，但是你也可以使用类型正确
    // 的命名函数
    fmt.Println(Map(strs, strings.ToUpper))

}

```


结果:
```
2
false
true
false
[peach apple pear]
[PEACH APPLE PEAR PLUM]
```

### 字符串函数
```
// 标准库的 `strings` 包提供了很多有用的字符串相关的函数。
// 这里是一些用来让你对这个包有个初步了解的例子。

package main

import s "strings"
import "fmt"

// 我们给 `fmt.Println` 一个短名字的别名，我们随后将会经常
// 用到。
var p = fmt.Println

func main() {

    // 这是一些 `strings` 中的函数例子。注意他们都是包中的
    // 函数，不是字符串对象自身的方法，这意味着我们需要考
    // 虑在调用时传递字符作为第一个参数进行传递。
    p("Contains:  ", s.Contains("test", "es"))
    p("Count:     ", s.Count("test", "t"))
    p("HasPrefix: ", s.HasPrefix("test", "te"))
    p("HasSuffix: ", s.HasSuffix("test", "st"))
    p("Index:     ", s.Index("test", "e"))
    p("Join:      ", s.Join([]string{"a", "b"}, "-"))
    p("Repeat:    ", s.Repeat("a", 5))
    p("Replace:   ", s.Replace("foo", "o", "0", -1))
    p("Replace:   ", s.Replace("foo", "o", "0", 1))
    p("Split:     ", s.Split("a-b-c-d-e", "-"))
    p("ToLower:   ", s.ToLower("TEST"))
    p("ToUpper:   ", s.ToUpper("test"))
    p()

    // 你可以在 [`strings`](http://golang.org/pkg/strings/)
    // 包文档中找到更多的函数

    // 虽然不是 `strings` 的一部分，但是仍然值得一提的是获
    // 取字符串长度和通过索引获取一个字符的机制。
    p("Len: ", len("hello"))
    p("Char:", "hello"[1])
}
```
结果:
```
Contains:   true
Count:      2
HasPrefix:  true
HasSuffix:  true
Index:      1
Join:       a-b
Repeat:     aaaaa
Replace:    f00
Replace:    f0o
Split:      [a b c d e]
toLower:    test
ToUpper:    TEST

Len:  5
Char: 101

```

### 字符串实例化
```
// Go 在传统的`printf` 中对字符串格式化提供了优异的支持。
// 这里是一些基本的字符串格式化的人物的例子。

package main

import "fmt"
import "os"

type point struct {
    x, y int
}

func main() {

    // Go 为常规 Go 值的格式化设计提供了多种打印方式。例
    // 如，这里打印了 `point` 结构体的一个实例。
    p := point{1, 2}
    fmt.Printf("%v\n", p)

    // 如果值是一个结构体，`%+v` 的格式化输出内容将包括
    // 结构体的字段名。
    fmt.Printf("%+v\n", p)

    // `%#v` 形式则输出这个值的 Go 语法表示。例如，值的
    // 运行源代码片段。
    fmt.Printf("%#v\n", p)

    // 需要打印值的类型，使用 `%T`。
    fmt.Printf("%T\n", p)

    // 格式化布尔值是简单的。
    fmt.Printf("%t\n", true)

    // 格式化整形数有多种方式，使用 `%d`进行标准的十进
    // 制格式化。
    fmt.Printf("%d\n", 123)

    // 这个输出二进制表示形式。
    fmt.Printf("%b\n", 14)

    // 这个输出给定整数的对应字符。
    fmt.Printf("%c\n", 33)

    // `%x` 提供十六进制编码。
    fmt.Printf("%x\n", 456)

    // 对于浮点型同样有很多的格式化选项。使用 `%f` 进
    // 行最基本的十进制格式化。
    fmt.Printf("%f\n", 78.9)

    // `%e` 和 `%E` 将浮点型格式化为（稍微有一点不
    // 同的）科学技科学记数法表示形式。
    fmt.Printf("%e\n", 123400000.0)
    fmt.Printf("%E\n", 123400000.0)

    // 使用 `%s` 进行基本的字符串输出。
    fmt.Printf("%s\n", "\"string\"")

    // 像 Go 源代码中那样带有双引号的输出，使用 `%q`。
    fmt.Printf("%q\n", "\"string\"")

    // 和上面的整形数一样，`%x` 输出使用 base-16 编码的字
    // 符串，每个字节使用 2 个字符表示。
    fmt.Printf("%x\n", "hex this")

    // 要输出一个指针的值，使用 `%p`。
    fmt.Printf("%p\n", &p)

    // 当输出数字的时候，你将经常想要控制输出结果的宽度和
    // 精度，可以使用在 `%` 后面使用数字来控制输出宽度。
    // 默认结果使用右对齐并且通过空格来填充空白部分。
    fmt.Printf("|%6d|%6d|\n", 12, 345)

    // 你也可以指定浮点型的输出宽度，同时也可以通过 宽度.
    // 精度 的语法来指定输出的精度。
    fmt.Printf("|%6.2f|%6.2f|\n", 1.2, 3.45)

    // 要左对齐，使用 `-` 标志。
    fmt.Printf("|%-6.2f|%-6.2f|\n", 1.2, 3.45)

    // 你也许也想控制字符串输出时的宽度，特别是要确保他们在
    // 类表格输出时的对齐。这是基本的右对齐宽度表示。
    fmt.Printf("|%6s|%6s|\n", "foo", "b")

    // 要左对齐，和数字一样，使用 `-` 标志。
    fmt.Printf("|%-6s|%-6s|\n", "foo", "b")

    // 到目前为止，我们已经看过 `Printf`了，它通过 `os.Stdout`
    // 输出格式化的字符串。`Sprintf` 则格式化并返回一个字
    // 符串而不带任何输出。
    s := fmt.Sprintf("a %s", "string")
    fmt.Println(s)

    // 你可以使用 `Fprintf` 来格式化并输出到 `io.Writers`
    // 而不是 `os.Stdout`。
    fmt.Fprintf(os.Stderr, "an %s\n", "error")
}

```
结果:
```
{1 2}
{x:1 y:2}
main.point{x:1, y:2}
main.point
true
123
1110
!
1c8
78.900000
1.234000e+08
1.234000E+08
"string"
"\"string\""
6865782074686973
0x42135100
|    12|   345|
|  1.20|  3.45|
|1.20  |3.45  |
|   foo|     b|
|foo   |b     |
a string
an error

```

### 正则表达事
```
// Go 提供内置的[正则表达式](http://zh.wikipedia.org/wiki/%E6%AD%A3%E5%88%99%E8%A1%A8%E8%BE%BE%E5%BC%8F)。
// 这里是 Go 中基本的正则相关功能的例子。

package main

import "bytes"
import "fmt"
import "regexp"

func main() {

    // 这个测试一个字符串是否符合一个表达式。
    match, _ := regexp.MatchString("p([a-z]+)ch", "peach")
    fmt.Println(match)

    // 上面我们是直接使用字符串，但是对于一些其他的正则任
    // 务，你需要 `Compile` 一个优化的 `Regexp` 结构体。
    r, _ := regexp.Compile("p([a-z]+)ch")

    // 这个结构体有很多方法。这里是类似我们前面看到的一个
    // 匹配测试。
    fmt.Println(r.MatchString("peach"))

    // 这是查找匹配字符串的。
    fmt.Println(r.FindString("peach punch"))

    // 这个也是查找第一次匹配的字符串的，但是返回的匹配开
    // 始和结束位置索引，而不是匹配的内容。
    fmt.Println(r.FindStringIndex("peach punch"))

    // `Submatch` 返回完全匹配和局部匹配的字符串。例如，
    // 这里会返回 `p([a-z]+)ch` 和 `([a-z]+) 的信息。
    fmt.Println(r.FindStringSubmatch("peach punch"))

    // 类似的，这个会返回完全匹配和局部匹配的索引位置。
    fmt.Println(r.FindStringSubmatchIndex("peach punch"))

    // 带 `All` 的这个函数返回所有的匹配项，而不仅仅是首
    // 次匹配项。例如查找匹配表达式的所有项。
    fmt.Println(r.FindAllString("peach punch pinch", -1))

    // `All` 同样可以对应到上面的所有函数。
    fmt.Println(r.FindAllStringSubmatchIndex(
        "peach punch pinch", -1))

    // 这个函数提供一个正整数来限制匹配次数。
    fmt.Println(r.FindAllString("peach punch pinch", 2))

    // 上面的例子中，我们使用了字符串作为参数，并使用了
    // 如 `MatchString` 这样的方法。我们也可以提供 `[]byte`
    // 参数并将 `String` 从函数命中去掉。
    fmt.Println(r.Match([]byte("peach")))

    // 创建正则表示式常量时，可以使用 `Compile` 的变体
    // `MustCompile` 。因为 `Compile` 返回两个值，不能用于常量。
    r = regexp.MustCompile("p([a-z]+)ch")
    fmt.Println(r)

    // `regexp` 包也可以用来替换部分字符串为其他值。
    fmt.Println(r.ReplaceAllString("a peach", "<fruit>"))

    // `Func` 变量允许传递匹配内容到一个给定的函数中，
    in := []byte("a peach")
    out := r.ReplaceAllFunc(in, bytes.ToUpper)
    fmt.Println(string(out))
}
```

结果:
```
true
true
peach
[0 5]
[peach ea]
[0 5 1 3]
[peach punch pinch]
[[0 5 1 3] [6 11 7 9] [12 17 13 15]]
[peach punch]
true
p([a-z]+)ch
a <fruit>
a PEACH

# 完整的 Go 正则表达式参考，请查阅 [`regexp`](http://golang.org/pkg/regexp/) 包文档。

```

### json
```
// Go 提供内置的 JSON 编解码支持，包括内置或者自定义类
// 型与 JSON 数据之间的转化。

package main

import "encoding/json"
import "fmt"
import "os"

// 下面我们将使用这两个结构体来演示自定义类型的编码和解
// 码。
type Response1 struct {
    Page   int
    Fruits []string
}
type Response2 struct {
    Page   int      `json:"page"`
    Fruits []string `json:"fruits"`
}

func main() {

    // 首先我们来看一下基本数据类型到 JSON 字符串的编码
    // 过程。这里是一些原子值的例子。
    bolB, _ := json.Marshal(true)
    fmt.Println(string(bolB))

    intB, _ := json.Marshal(1)
    fmt.Println(string(intB))

    fltB, _ := json.Marshal(2.34)
    fmt.Println(string(fltB))

    strB, _ := json.Marshal("gopher")
    fmt.Println(string(strB))

    // 这里是一些切片和 map 编码成 JSON 数组和对象的例子。
    slcD := []string{"apple", "peach", "pear"}
    slcB, _ := json.Marshal(slcD)
    fmt.Println(string(slcB))

    mapD := map[string]int{"apple": 5, "lettuce": 7}
    mapB, _ := json.Marshal(mapD)
    fmt.Println(string(mapB))

    // JSON 包可以自动的编码你的自定义类型。编码仅输出可
    // 导出的字段，并且默认使用他们的名字作为 JSON 数据的
    // 键。
    res1D := &Response1{
        Page:   1,
        Fruits: []string{"apple", "peach", "pear"}}
    res1B, _ := json.Marshal(res1D)
    fmt.Println(string(res1B))

    // 你可以给结构字段声明标签来自定义编码的 JSON 数据键
    // 名称。在上面 `Response2` 的定义可以作为这个标签这个
    // 的一个例子。
    res2D := &Response2{
        Page:   1,
        Fruits: []string{"apple", "peach", "pear"}}
    res2B, _ := json.Marshal(res2D)
    fmt.Println(string(res2B))

    // 现在来看看解码 JSON 数据为 Go 值的过程。这里
    // 是一个普通数据结构的解码例子。
    byt := []byte(`{"num":6.13,"strs":["a","b"]}`)

    // 我们需要提供一个 JSON 包可以存放解码数据的变量。这里
    // 的 `map[string]interface{}` 将保存一个 string 为键，
    // 值为任意值的map。
    var dat map[string]interface{}

    // 这里就是实际的解码和相关的错误检查。
    if err := json.Unmarshal(byt, &dat); err != nil {
        panic(err)
    }
    fmt.Println(dat)

    // 为了使用解码 map 中的值，我们需要将他们进行适当的类
    // 型转换。例如这里我们将 `num` 的值转换成 `float64`
    // 类型。
    num := dat["num"].(float64)
    fmt.Println(num)

    // 访问嵌套的值需要一系列的转化。
    strs := dat["strs"].([]interface{})
    str1 := strs[0].(string)
    fmt.Println(str1)

    // 我们也可以解码 JSON 值到自定义类型。这个功能的好处就
    // 是可以为我们的程序带来额外的类型安全加强，并且消除在
    // 访问数据时的类型断言。
    str := `{"page": 1, "fruits": ["apple", "peach"]}`
    res := &Response2{}
    json.Unmarshal([]byte(str), &res)
    fmt.Println(res)
    fmt.Println(res.Fruits[0])

    // 在上面的例子中，我们经常使用 byte 和 string 作为使用
    // 标准输出时数据和 JSON 表示之间的中间值。我们也可以和
    // `os.Stdout` 一样，直接将 JSON 编码直接输出至 `os.Writer`
    // 流中，或者作为 HTTP 响应体。
    enc := json.NewEncoder(os.Stdout)
    d := map[string]int{"apple": 5, "lettuce": 7}
    enc.Encode(d)
}

```


结果:
```
true
1
2.34
"gopher"
["apple","peach","pear"]
{"apple":5,"lettuce":7}
{"Page":1,"Fruits":["apple","peach","pear"]}
{"page":1,"fruits":["apple","peach","pear"]}
map[num:6.13 strs:[a b]]
6.13
a
&{1 [apple peach]}
apple
{"apple":5,"lettuce":7}
```
[JSON 和 Go](http://blog.golang.org/2011/01/json-and-go.html)
[JSON 包文档](http://golang.org/pkg/encoding/json/)


### 时间
```
// Go 对时间和时间段提供了大量的支持；这里是一些例子。

package main

import "fmt"
import "time"

func main() {
    p := fmt.Println

    // 得到当前时间。
    now := time.Now()
    p(now)

    // 通过提供年月日等信息，你可以构建一个 `time`。时间总
    // 是关联着位置信息，例如时区。
    then := time.Date(
        2009, 11, 17, 20, 34, 58, 651387237, time.UTC)
    p(then)

    // 你可以提取出时间的各个组成部分。
    p(then.Year())
    p(then.Month())
    p(then.Day())
    p(then.Hour())
    p(then.Minute())
    p(then.Second())
    p(then.Nanosecond())
    p(then.Location())

    // 输出是星期一到日的 `Weekday` 也是支持的。
    p(then.Weekday())

    // 这些方法来比较两个时间，分别测试一下是否是之前，
    // 之后或者是同一时刻，精确到秒。
    p(then.Before(now))
    p(then.After(now))
    p(then.Equal(now))

    // 方法 `Sub` 返回一个 `Duration` 来表示两个时间点的间
    // 隔时间。
    diff := now.Sub(then)
    p(diff)

    // 我们计算出不同单位下的时间长度值。
    p(diff.Hours())
    p(diff.Minutes())
    p(diff.Seconds())
    p(diff.Nanoseconds())

    // 你可以使用 `Add` 将时间后移一个时间间隔，或者使
    // 用一个 `-` 来将时间前移一个时间间隔。
    p(then.Add(diff))
    p(then.Add(-diff))
}

```
结果:
```
2012-10-31 15:50:13.793654 +0000 UTC
2009-11-17 20:34:58.651387237 +0000 UTC
2009
November
17
20
34
58
651387237
UTC
Tuesday
true
false
false
25891h15m15.142266763s
25891.25420618521
1.5534752523711128e+06
9.320851514226677e+07
93208515142266763
2012-10-31 15:50:13.793654 +0000 UTC
2006-12-05 01:19:43.509120474 +0000 UTC

```

### 时间戳
```
// 一般程序会有获取 [Unix 时间](http://zh.wikipedia.org/wiki/UNIX%E6%97%B6%E9%97%B4)
// 的秒数，毫秒数，或者微秒数的需要。来看看如何用 Go 来实现。

package main

import "fmt"
import "time"

func main() {

    // 分别使用带 `Unix` 或者 `UnixNano` 的 `time.Now`
    // 来获取从自[协调世界时](http://zh.wikipedia.org/wiki/%E5%8D%94%E8%AA%BF%E4%B8%96%E7%95%8C%E6%99%82)
    // 起到现在的秒数或者纳秒数。
    now := time.Now()
    secs := now.Unix()
    nanos := now.UnixNano()
    fmt.Println(now)

    // 注意 `UnixMillis` 是不存在的，所以要得到毫秒数的话，
    // 你要自己手动的从纳秒转化一下。
    millis := nanos / 1000000
    fmt.Println(secs)
    fmt.Println(millis)
    fmt.Println(nanos)

    // 你也可以将协调世界时起的整数秒或者纳秒转化到相应的时间。
    fmt.Println(time.Unix(secs, 0))
    fmt.Println(time.Unix(0, nanos))
}


```
结果:
```
2012-10-31 16:13:58.292387 +0000 UTC
1351700038
1351700038292
1351700038292387000
2012-10-31 16:13:58 +0000 UTC
2012-10-31 16:13:58.292387 +0000 UTC

```

### 时间的格式化和解析
```
// Go 支持通过基于描述模板的时间格式化和解析。

package main

import "fmt"
import "time"

func main() {
    p := fmt.Println

    // 这里是一个基本的按照 RFC3339 进行格式化的例子，使用
    // 对应模式常量。
    t := time.Now()
    p(t.Format(time.RFC3339))

    // 时间解析使用同 `Format` 相同的形式值。
    t1, e := time.Parse(
        time.RFC3339,
        "2012-11-01T22:08:41+00:00")
    p(t1)

    // `Format` 和 `Parse` 使用基于例子的形式来决定日期格式，
    // 一般你只要使用 `time` 包中提供的模式常量就行了，但是你
    // 也可以实现自定义模式。模式必须使用时间 `Mon Jan 2 15:04:05 MST 2006`
    // 来指定给定时间/字符串的格式化/解析方式。时间一定要按照
    // 如下所示：2006为年，15 为小时，Monday 代表星期几，等等。
    p(t.Format("3:04PM"))
    p(t.Format("Mon Jan _2 15:04:05 2006"))
    p(t.Format("2006-01-02T15:04:05.999999-07:00"))
    form := "3 04 PM"
    t2, e := time.Parse(form, "8 41 PM")
    p(t2)

    // 对于纯数字表示的时间，你也可以使用标准的格式化字
    // 符串来提出出时间值得组成。
    fmt.Printf("%d-%02d-%02dT%02d:%02d:%02d-00:00\n",
        t.Year(), t.Month(), t.Day(),
        t.Hour(), t.Minute(), t.Second())

    // `Parse` 函数在输入的时间格式不正确是会返回一个
    // 错误。
    ansic := "Mon Jan _2 15:04:05 2006"
    _, e = time.Parse(ansic, "8:41PM")
    p(e)
}


```
结果:
```
2014-04-15T18:00:15-07:00
2012-11-01 22:08:41 +0000 +0000
6:00PM
Tue Apr 15 18:00:15 2014
2014-04-15T18:00:15.161182-07:00
0000-01-01 20:41:00 +0000 UTC
2014-04-15T18:00:15-00:00
parsing time "8:41PM" as "Mon Jan _2 15:04:05 2006": ...

```

### 随机数
```
// Go 的 `math/rand` 包提供了[伪随机数生成器（英）](http://en.wikipedia.org/wiki/Pseudorandom_number_generator)。

package main

import "fmt"
import "math/rand"

func main() {

    // 例如，`rand.Intn` 返回一个随机的整数 n，`0 <= n <= 100`。
    fmt.Print(rand.Intn(100), ",")
    fmt.Print(rand.Intn(100))
    fmt.Println()

    // `rand.Float64` 返回一个64位浮点数 `f`，
    // `0.0 <= f <= 1.0`。
    fmt.Println(rand.Float64())

    // 这个技巧可以用来生成其他范围的随机浮点数，例如
    // `5.0 <= f <= 10.0`
    fmt.Print((rand.Float64()*5)+5, ",")
    fmt.Print((rand.Float64() * 5) + 5)
    fmt.Println()

    // 要让伪随机数生成器有确定性，可以给它一个明确的种子。
    s1 := rand.NewSource(42)
    r1 := rand.New(s1)

    // 调用上面返回的 `rand.Source` 的函数和调用 `rand` 包中函数
    // 是相同的。
    fmt.Print(r1.Intn(100), ",")
    fmt.Print(r1.Intn(100))
    fmt.Println()

    // 如果使用相同的种子生成的随机数生成器，将会产生相同的随机
    // 数序列。
    s2 := rand.NewSource(42)
    r2 := rand.New(s2)
    fmt.Print(r2.Intn(100), ",")
    fmt.Print(r2.Intn(100))
    fmt.Println()
}

```

结果:
```
81,87
0.6645600532184904
7.123187485356329,8.434115364335547
5,87
5,87
```

[`math/rand`](http://golang.org/pkg/math/rand/) 

### 数字解析
```
// 从字符串中解析数字在很多程序中是一个基础常见的任务，在
// Go 中是这样处理的。

package main

// 内置的 `strconv` 包提供了数字解析功能。
import "strconv"
import "fmt"

func main() {

    // 使用 `ParseFloat` 解析浮点数，这里的 `64` 表示表示解
    // 析的数的位数。
    f, _ := strconv.ParseFloat("1.234", 64)
    fmt.Println(f)

    // 在使用 `ParseInt` 解析整形数时，例子中的参数 `0` 表
    // 示自动推断字符串所表示的数字的进制。`64` 表示返回的
    // 整形数是以 64 位存储的。
    i, _ := strconv.ParseInt("123", 0, 64)
    fmt.Println(i)

    // `ParseInt` 会自动识别出十六进制数。
    d, _ := strconv.ParseInt("0x1c8", 0, 64)
    fmt.Println(d)

    // `ParseUint` 也是可用的。
    u, _ := strconv.ParseUint("789", 0, 64)
    fmt.Println(u)

    // `Atoi` 是一个基础的 10 进制整型数转换函数。
    k, _ := strconv.Atoi("135")
    fmt.Println(k)

    // 在输入错误时，解析函数会返回一个错误。
    _, e := strconv.Atoi("wat")
    fmt.Println(e)
}


```

结果:
```
1.234
123
456
789
135
strconv.ParseInt: parsing "wat": invalid syntax
```

### url解析
```
// URL 提供了一个[统一资源定位方式](http://adam.heroku.com/past/2010/3/30/urls_are_the_uniform_way_to_locate_resources/)。
// 这里了解一下 Go 中是如何解析 URL 的。


package main

import "fmt"
import "net/url"
import "strings"

func main() {

    // 我们将解析这个 URL 示例，它包含了一个 scheme，
    // 认证信息，主机名，端口，路径，查询参数和片段。
    s := "postgres://user:pass@host.com:5432/path?k=v#f"

    // 解析这个 URL 并确保解析没有出错。
    u, err := url.Parse(s)
    if err != nil {
        panic(err)
    }

    // 直接访问 scheme。
    fmt.Println(u.Scheme)

    // `User` 包含了所有的认证信息，这里调用 `Username`
    // 和 `Password` 来获取独立值。
    fmt.Println(u.User)
    fmt.Println(u.User.Username())
    p, _ := u.User.Password()
    fmt.Println(p)

    // `Host` 同时包括主机名和端口信息，如过端口存在的话，
    // 使用 `strings.Split()` 从 `Host` 中手动提取端口。
    fmt.Println(u.Host)
    h := strings.Split(u.Host, ":")
    fmt.Println(h[0])
    fmt.Println(h[1])

    // 这里我们提出路径和查询片段信息。
    fmt.Println(u.Path)
    fmt.Println(u.Fragment)

    // 要得到字符串中的 `k=v` 这种格式的查询参数，可以使
    // 用 `RawQuery` 函数。你也可以将查询参数解析为一个
    // map。已解析的查询参数 map 以查询字符串为键，对应
    // 值字符串切片为值，所以如何只想得到一个键对应的第
    // 一个值，将索引位置设置为 `[0]` 就行了。
    fmt.Println(u.RawQuery)
    m, _ := url.ParseQuery(u.RawQuery)
    fmt.Println(m)
    fmt.Println(m["k"][0])
}

```

结果:
```
postgres
user:pass
user
pass
host.com:5432
host.com
5432
/path
f
k=v
map[k:[v]]
v
```


### sha1散列
```
// [_SHA1 散列_](http://en.wikipedia.org/wiki/SHA-1)经常用
// 生成二进制文件或者文本块的短标识。例如，[git 版本控制系统](http://git-scm.com/)
// 大量的使用 SHA1 来标识受版本控制的文件和目录。这里是 Go
// 中如何进行 SHA1 散列计算的例子。

package main

// Go 在多个 `crypto/*` 包中实现了一系列散列函数。
import "crypto/sha1"
import "fmt"

func main() {
    s := "sha1 this string"

    // 产生一个散列值得方式是 `sha1.New()`，`sha1.Write(bytes)`，
    // 然后 `sha1.Sum([]byte{})`。这里我们从一个新的散列开始。
    h := sha1.New()

    // 写入要处理的字节。如果是一个字符串，需要使用
    // `[]byte(s)` 来强制转换成字节数组。
    h.Write([]byte(s))

    // 这个用来得到最终的散列值的字符切片。`Sum` 的参数可以
    // 用来都现有的字符切片追加额外的字节切片：一般不需要要。
    bs := h.Sum(nil)

    // SHA1 值经常以 16 进制输出，例如在 git commit 中。使用
    // `%x` 来将散列结果格式化为 16 进制字符串。
    fmt.Println(s)
    fmt.Printf("%x\n", bs)
}
```

结果:
```
sha1 this string
cf23df2207d99a74fbe169e3eba035e633b65d94

```
[哈希强度](http://en.wikipedia.org/wiki/Cryptographic_hash_function)。


### base64编码
```
// Go 提供内建的 [base64 编解码](http://zh.wikipedia.org/wiki/Base64)支持。

package main

// 这个语法引入了 `encoding/base64` 包并使用名称 `b64`
// 代替默认的 `base64`。这样可以节省点空间。
import b64 "encoding/base64"
import "fmt"

func main() {

    // 这是将要编解码的字符串。
    data := "abc123!?$*&()'-=@~"

    // Go 同时支持标准的和 URL 兼容的 base64 格式。编码需要
    // 使用 `[]byte` 类型的参数，所以要将字符串转成此类型。
    sEnc := b64.StdEncoding.EncodeToString([]byte(data))
    fmt.Println(sEnc)

    // 解码可能会返回错误，如果不确定输入信息格式是否正确，
    // 那么，你就需要进行错误检查了。
    sDec, _ := b64.StdEncoding.DecodeString(sEnc)
    fmt.Println(string(sDec))
    fmt.Println()

    // 使用 URL 兼容的 base64 格式进行编解码。
    uEnc := b64.URLEncoding.EncodeToString([]byte(data))
    fmt.Println(uEnc)
    uDec, _ := b64.URLEncoding.DecodeString(uEnc)
    fmt.Println(string(uDec))
}

```

结果:
```
YWJjMTIzIT8kKiYoKSctPUB+
abc123!?$*&()'-=@~

YWJjMTIzIT8kKiYoKSctPUB-
abc123!?$*&()'-=@~

```

### 读文件
```
// 读写文件在很多程序中都是必须的基本任务。首先我们看看一
// 些读文件的例子。

package main

import (
    "bufio"
    "fmt"
    "io"
    "io/ioutil"
    "os"
)

// 读取文件需要经常进行错误检查，这个帮助方法可以精简下面
// 的错误检查过程。
func check(e error) {
    if e != nil {
        panic(e)
    }
}

func main() {

    // 也许大部分基本的文件读取任务是将文件内容读取到
    // 内存中。
    dat, err := ioutil.ReadFile("/tmp/dat")
    check(err)
    fmt.Print(string(dat))

    // 你经常会想对于一个文件是怎么读并且读取到哪一部分
    // 进行更多的控制。对于这个任务，从使用 `os.Open`
    // 打开一个文件获取一个 `os.File` 值开始。
    f, err := os.Open("/tmp/dat")
    check(err)

    // 从文件开始位置读取一些字节。这里最多读取 5 个字
    // 节，并且这也是我们实际读取的字节数。
    b1 := make([]byte, 5)
    n1, err := f.Read(b1)
    check(err)
    fmt.Printf("%d bytes: %s\n", n1, string(b1))

    // 你也可以 `Seek` 到一个文件中已知的位置并从这个位置开
    // 始进行读取。
    o2, err := f.Seek(6, 0)
    check(err)
    b2 := make([]byte, 2)
    n2, err := f.Read(b2)
    check(err)
    fmt.Printf("%d bytes @ %d: %s\n", n2, o2, string(b2))

    // `io` 包提供了一些可以帮助我们进行文件读取的函数。
    // 例如，上面的读取可以使用 `ReadAtLeast` 得到一个更
    // 健壮的实现。
    o3, err := f.Seek(6, 0)
    check(err)
    b3 := make([]byte, 2)
    n3, err := io.ReadAtLeast(f, b3, 2)
    check(err)
    fmt.Printf("%d bytes @ %d: %s\n", n3, o3, string(b3))

    // 没有内置的回转支持，但是使用 `Seek(0, 0)` 实现。
    _, err = f.Seek(0, 0)
    check(err)

    // `bufio` 包实现了带缓冲的读取，这不仅对有很多小的读
    // 取操作的能提升性能，也提供了很多附加的读取函数。
    r4 := bufio.NewReader(f)
    b4, err := r4.Peek(5)
    check(err)
    fmt.Printf("5 bytes: %s\n", string(b4))

    // 任务结束后要关闭这个文件（通常这个操作应该在 `Open`
    // 操作后立即使用 `defer` 来完成）。
    f.Close()

}

```
结果:
```
$ echo "hello" > /tmp/dat
$ echo "go" >>   /tmp/dat
$ go run reading-files.go 
hello
go
5 bytes: hello
2 bytes @ 6: go
2 bytes @ 6: go
5 bytes: hello

```

### 写文件
```
// Go 写文件和我们前面看过的读操作有着相似的方式。

package main

import (
    "bufio"
    "fmt"
    "io/ioutil"
    "os"
)

func check(e error) {
    if e != nil {
        panic(e)
    }
}

func main() {

    // 开始，这里是展示如写入一个字符串（或者只是一些
    // 字节）到一个文件。
    d1 := []byte("hello\ngo\n")
    err := ioutil.WriteFile("/tmp/dat1", d1, 0644)
    check(err)

    // 对于更细粒度的写入，先打开一个文件。
    f, err := os.Create("/tmp/dat2")
    check(err)

    // 打开文件后，习惯立即使用 defer 调用文件的 `Close`
    // 操作。
    defer f.Close()

    // 你可以写入你想写入的字节切片
    d2 := []byte{115, 111, 109, 101, 10}
    n2, err := f.Write(d2)
    check(err)
    fmt.Printf("wrote %d bytes\n", n2)

    // `WriteString` 也是可用的。
    n3, err := f.WriteString("writes\n")
    fmt.Printf("wrote %d bytes\n", n3)

    // 调用 `Sync` 来将缓冲区的信息写入磁盘。
    f.Sync()

    // `bufio` 提供了和我们前面看到的带缓冲的读取器一
    // 样的带缓冲的写入器。
    w := bufio.NewWriter(f)
    n4, err := w.WriteString("buffered\n")
    fmt.Printf("wrote %d bytes\n", n4)

    // 使用 `Flush` 来确保所有缓存的操作已写入底层写入器。
    w.Flush()

}

```

结果:
```
# 运行这端文件写入代码。
$ go run writing-files.go 
wrote 5 bytes
wrote 7 bytes
wrote 9 bytes

# 然后检查写入文件的内容。
$ cat /tmp/dat1
hello
go
$ cat /tmp/dat2
some
writes
buffered

# 下面我们将看一些文件 I/O 的想法，就像我们已经看过的 
# `stdin` 和 `stdout` 流。
```


### 行过滤器
```
// 一个_行过滤器_ 在读取标准输入流的输入，处理该输入，然后
// 将得到一些的结果输出到标准输出的程序中是常见的一个功能。
// `grep` 和 `sed` 是常见的行过滤器。

// 这里是一个使用 Go 编写的行过滤器示例，它将所有的输入文字
// 转化为大写的版本。你可以使用这个模式来写一个你自己的 Go
// 行过滤器。
package main

import (
    "bufio"
    "fmt"
    "os"
    "strings"
)

func main() {

    // 对 `os.Stdin` 使用一个带缓冲的 scanner，让我们可以
    // 直接使用方便的 `Scan` 方法来直接读取一行，每次调用
    // 该方法可以让 scanner 读取下一行。
    scanner := bufio.NewScanner(os.Stdin)

    for scanner.Scan() {
        // `Text` 返回当前的 token，现在是输入的下一行。
        ucl := strings.ToUpper(scanner.Text())

        // 写出大写的行。
        fmt.Println(ucl)
    }

    // 检查 `Scan` 的错误。文件结束符是可以接受的，并且
    // 不会被 `Scan` 当作一个错误。
    if err := scanner.Err(); err != nil {
        fmt.Fprintln(os.Stderr, "error:", err)
        os.Exit(1)
    }
}
```

结果:
```
# 试一下我们的行过滤器，首先创建多一个有小写行的文件。
$ echo 'hello'   > /tmp/lines
$ echo 'filter' >> /tmp/lines

# 然后使用行过滤器来得到大些的行。
$ cat /tmp/lines | go run line-filters.go
HELLO
FILTER
```

### 命令行参数

[_命令行参数_](http://en.wikipedia.org/wiki/Command-line_interface#Arguments)

```
package main

import "os"
import "fmt"

func main() {

    // `os.Args` 提供原始命令行参数访问功能。注意，切片中
    // 的第一个参数是该程序的路径，并且 `os.Args[1:]`保存
    // 所有程序的的参数。
    argsWithProg := os.Args
    argsWithoutProg := os.Args[1:]

    // 你可以使用标准的索引位置方式取得单个参数的值。
    arg := os.Args[3]

    fmt.Println(argsWithProg)
    fmt.Println(argsWithoutProg)
    fmt.Println(arg)
}
```

运行程序:
```
$ go build command-line-arguments.go
$ ./command-line-arguments a b c d
[./command-line-arguments a b c d]
[a b c d]
c
```

### 命令行标志
```
package main

// Go 提供了一个 `flag` 包，支持基本的命令行标志解析。
// 我们将用这个包来实现我们的命令行程序示例。
import "flag"
import "fmt"

func main() {

    // 基本的标记声明仅支持字符串、整数和布尔值选项。
    // 这里我们声明一个默认值为 `"foo"` 的字符串标志 `word`
    // 并带有一个简短的描述。这里的 `flag.String` 函数返回一个字
    // 符串指针（不是一个字符串值），在下面我们会看到是如何
    // 使用这个指针的。
    wordPtr := flag.String("word", "foo", "a string")

    // 使用和声明 `word` 标志相同的方法来声明 `numb` 和 `fork` 标志。
    numbPtr := flag.Int("numb", 42, "an int")
    boolPtr := flag.Bool("fork", false, "a bool")

    // 用程序中已有的参数来声明一个标志也是可以的。注
    // 意在标志声明函数中需要使用该参数的指针。
    var svar string
    flag.StringVar(&svar, "svar", "bar", "a string var")

    // 所有标志都声明完成以后，调用 `flag.Parse()` 来执行
    // 命令行解析。
    flag.Parse()

    // 这里我们将仅输出解析的选项以及后面的位置参数。注意，
    // 我们需要使用类似 `*wordPtr` 这样的语法来对指针解引用，从而
    // 得到选项的实际值。
    fmt.Println("word:", *wordPtr)
    fmt.Println("numb:", *numbPtr)
    fmt.Println("fork:", *boolPtr)
    fmt.Println("svar:", svar)
    fmt.Println("tail:", flag.Args())
}
```

结果:
```
# 测试这个程序前，最好将这个程序编译成二进制文件，然后再运
# 行这个程序。
$ go build command-line-flags.go

word: opt
numb: 7
fork: true
svar: flag
tail: []

# 注意到，如果你省略一个标志，那么这个标志的值自动的设
# 定为他的默认值。
$ ./command-line-flags -word=opt
word: opt
numb: 42
fork: false
svar: bar
tail: []

# 位置参数可以出现在任何标志后面。
$ ./command-line-flags -word=opt a1 a2 a3
word: opt
...
tail: [a1 a2 a3]

# 注意，`flag` 包需要所有的标志出现位置参数之前（
# 否则，这个标志将会被解析为位置参数）。
$ ./command-line-flags -word=opt a1 a2 a3 -numb=7
word: opt
numb: 42
fork: false
svar: bar
trailing: [a1 a2 a3 -numb=7]

# 使用 `-h` 或者 `--help` 标志来得到自动生成的这个命
# 令行程序的帮助文本。
$ ./command-line-flags -h
Usage of ./command-line-flags:
  -fork=false: a bool
  -numb=42: an int
  -svar="bar": a string var
  -word="foo": a string

# 如果你提供一个没有使用 `flag` 包指定的标志，程序会输出一
# 个错误信息，并再次显示帮助文本。
$ ./command-line-flags -wat
flag provided but not defined: -wat
Usage of ./command-line-flags:
...

# 后面，我们将会看一下环境变量，另一个用于参数化程序的基本方式。

```

### 环境变量
```
// [_环境变量_](http://zh.wikipedia.org/wiki/%E7%8E%AF%E5%A2%83%E5%8F%98%E9%87%8F)
// 是一个在[为 Unix 程序传递配置信息](http://www.12factor.net/config)的普遍方式。
// 让我们来看看如何设置，获取并列举环境变量。

package main

import "os"
import "strings"
import "fmt"

func main() {

    // 使用 `os.Setenv` 来设置一个键值队。使用 `os.Getenv`
    // 获取一个键对应的值。如果键不存在，将会返回一个空字符
    // 串。
    os.Setenv("FOO", "1")
    fmt.Println("FOO:", os.Getenv("FOO"))
    fmt.Println("BAR:", os.Getenv("BAR"))

    // 使用 `os.Environ` 来列出所有环境变量键值队。这个函数
    // 会返回一个 `KEY=value` 形式的字符串切片。你可以使用
    // `strings.Split` 来得到键和值。这里我们打印所有的键。
    fmt.Println()
    for _, e := range os.Environ() {
        pair := strings.Split(e, "=")
        fmt.Println(pair[0])
    }
}

```

运行结果:
```
# 运行这个程序，显示我们在程序中设置的 `FOO` 的值，然而
# 没有设置的 `BAR` 是空的。
$ go run environment-variables.go
FOO: 1
BAR: 

# 键的列表是由你的电脑情况而定的。
TERM_PROGRAM
PATH
SHELL
...

# 如果我们在运行前设置了 `BAR` 的值，那么运行程序将会获
# 取到这个值。
$ BAR=2 go run environment-variables.go
FOO: 1
BAR: 2
...
```

### 生成进程
```
// 有时，我们的 Go 程序需要生成其他的，非 Go 进程。例如，这个
// 网站的语法高亮是通过在 Go 程序中生成一个 [`pygmentize`](http://pygments.org/)
// 来[实现的](https://github.com/everyx/gobyexample/blob/master/tools/generate.go)。
// 让我们看一些关于 Go 生成进程的例子。

package main

import "fmt"
import "io/ioutil"
import "os/exec"

func main() {

    // 我们将从一个简单的命令开始，没有参数或者输入，仅打印
    // 一些信息到标准输出流。`exec.Command` 函数帮助我们创
    // 建一个表示这个外部进程的对象。
    dateCmd := exec.Command("date")

    // `.Output` 是另一个帮助我们处理运行一个命令的常见情况
    // 的函数，它等待命令运行完成，并收集命令的输出。如果没
    // 有出错，`dateOut` 将获取到日期信息的字节。
    dateOut, err := dateCmd.Output()
    if err != nil {
        panic(err)
    }
    fmt.Println("> date")
    fmt.Println(string(dateOut))

    // 下面我们将看看一个稍复杂的例子，我们将从外部进程的
    // `stdin` 输入数据并从 `stdout` 收集结果。
    grepCmd := exec.Command("grep", "hello")

    // 这里我们明确的获取输入/输出管道，运行这个进程，写入
    // 一些输入信息，读取输出的结果，最后等待程序运行结束。
    grepIn, _ := grepCmd.StdinPipe()
    grepOut, _ := grepCmd.StdoutPipe()
    grepCmd.Start()
    grepIn.Write([]byte("hello grep\ngoodbye grep"))
    grepIn.Close()
    grepBytes, _ := ioutil.ReadAll(grepOut)
    grepCmd.Wait()

    // 上面的例子中，我们忽略了错误检测，但是你可以使用
    // `if err != nil` 的方式来进行错误检查，我们也只收集
    // `StdoutPipe` 的结果，但是你可以使用相同的方法收集
    // `StderrPipe` 的结果。
    fmt.Println("> grep hello")
    fmt.Println(string(grepBytes))

    // 注意，当我们需要提供一个明确的命令和参数数组来生成命
    // 令，和能够只需要提供一行命令行字符串相比，你想使用通
    // 过一个字符串生成一个完整的命令，那么你可以使用 `bash`
    // 命令的 `-c` 选项：
    lsCmd := exec.Command("bash", "-c", "ls -a -l -h")
    lsOut, err := lsCmd.Output()
    if err != nil {
        panic(err)
    }
    fmt.Println("> ls -a -l -h")
    fmt.Println(string(lsOut))
}


```

结果：
```
# 生成的程序返回和我们直接通过命令行运行这些程序的输出是相同的。
$ go run spawning-processes.go 
> date
Wed Oct 10 09:53:11 PDT 2012

> grep hello
hello grep

> ls -a -l -h
drwxr-xr-x  4 mark 136B Oct 3 16:29 .
drwxr-xr-x 91 mark 3.0K Oct 3 12:50 ..
-rw-r--r--  1 mark 1.3K Oct 3 16:28 spawning-processes.go

```

### 执行进程

```
// 在前面的例子中，我们了解了[生成外部进程](../spawning-processes/)
// 的知识，当我们需要访问外部进程时时需要这样做，但是有时候，我们只想
// 用其他的（也许是非 Go 程序）来完全替代当前的 Go 进程。这时候，我们
// 可以使用经典的 <a href="http://en.wikipedia.org/wiki/Exec_(operating_system)"><code>exec</code></a>
// 方法的 Go 实现。

package main

import "syscall"
import "os"
import "os/exec"

func main() {

    // 在我们的例子中，我们将执行 `ls` 命令。Go 需要提供我
    // 们需要执行的可执行文件的绝对路径，所以我们将使用
    // `exec.LookPath` 来得到它（大概是 `/bin/ls`）。
    binary, lookErr := exec.LookPath("ls")
    if lookErr != nil {
        panic(lookErr)
    }

    // `Exec` 需要的参数是切片的形式的（不是放在一起的一个大字
    // 符串）。我们给 `ls` 一些基本的参数。注意，第一个参数需要
    // 是程序名。
    args := []string{"ls", "-a", "-l", "-h"}

    // `Exec` 同样需要使用[环境变量](environment-variables.html)。
    // 这里我们仅提供当前的环境变量。
    env := os.Environ()

    // 这里是 `os.Exec` 调用。如果这个调用成功，那么我们的
    // 进程将在这里被替换成 `/bin/ls -a -l -h` 进程。如果存
    // 在错误，那么我们将会得到一个返回值。
    execErr := syscall.Exec(binary, args, env)
    if execErr != nil {
        panic(execErr)
    }
}

```

结果：
```
$ go run execing-processes.go
total 16
drwxr-xr-x  4 mark 136B Oct 3 16:29 .
drwxr-xr-x 91 mark 3.0K Oct 3 12:50 ..
-rw-r--r--  1 mark 1.3K Oct 3 16:28 execing-processes.go

# 注意 Go 并不提供一个经典的 Unix `fork` 函数。通常这不
# 是个问题，因为运行 Go 协程，生成进程和执行进程覆盖了
# fork 的大多数使用用场景。
```


### 信号
```
// 有时候，我们希望 Go 能智能的处理 <a href="http://zh.wikipedia.org/wiki/%E4%BF%A1%E5%8F%B7_(%E8%AE%A1%E7%AE%97%E6%9C%BA%E7%A7%91%E5%AD%A6)">Unix 信号</a>。
// 例如，我们希望当服务器接收到一个 `SIGTERM` 信号时能够
// 自动关机，或者一个命令行工具在接收到一个 `SIGINT` 信号
// 时停止处理输入信息。这里讲的就就是在 Go 中如何通过通道
// 来处理信号。

package main

import "fmt"
import "os"
import "os/signal"
import "syscall"

func main() {

    // Go 通过向一个通道发送 `os.Signal` 值来进行信号通知。我们
    // 将创建一个通道来接收这些通知（同时还创建一个用于在程序可
    // 以结束时进行通知的通道）。
    sigs := make(chan os.Signal, 1)
    done := make(chan bool, 1)

    // `signal.Notify` 注册这个给定的通道用于接收特定信号。
    signal.Notify(sigs, syscall.SIGINT, syscall.SIGTERM)

    // 这个 Go 协程执行一个阻塞的信号接收操作。当它得到一个
    // 值时，它将打印这个值，然后通知程序可以退出。
    go func() {
        sig := <-sigs
        fmt.Println()
        fmt.Println(sig)
        done <- true
    }()

    // 程序将在这里进行等待，直到它得到了期望的信号（也就
    // 是上面的 Go 协程发送的 `done` 值）然后退出。
    fmt.Println("awaiting signal")
    <-done
    fmt.Println("exiting")
}

```

结果：
```
# 当我们运行这个程序时，它将一直等待一个信号。使用 `ctrl-C`
# （终端显示为 `^C`），我们可以发送一个 `SIGINT` 信号，这会
# 使程序打印 `interrupt` 然后退出。
$ go run signals.go
awaiting signal
^C
interrupt
exiting

```

### 退出
```
// 使用 `os.Exit` 来立即进行带给定状态的退出。

package main

import "fmt"
import "os"

func main() {

    // 当使用 `os.Exit` 时 `defer` 将_不会_ 执行，所以这里的 `fmt.Println`
    // 将永远不会被调用。
    defer fmt.Println("!")

    // 退出并且退出状态为 3。
    os.Exit(3)
}

// 注意，不像例如 C 语言，Go 不使用在 `main` 中返回一个整
// 数来指明退出状态。如果你想以非零状态退出，那么你就要
// 使用 `os.Exit`。

```


结果:
```
# 如果你使用 `go run` 来运行 `exit.go`，那么退出状态将会被 `go`
# 捕获并打印。
$ go run exit.go
exit status 3

# 使用编译并执行一个二进制文件的方式，你可以在终端中查看退出状态。
$ go build exit.go
$ ./exit
$ echo $?
3

# 注意我们程序中的 `!` 永远不会被打印出来。
```
