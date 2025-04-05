# ConsoleNotepad

一个控制台文本编辑器. 使用 C++ 编写, 追求代码可读性、可拓展性, 并尽量使用现代 C++ 特性. 

~~不过考虑到 ddl 还是不得不写屎山~~

为 BIT 第三届十行代码大赛参赛作品. 95% 以上的代码均为原创, 只有极少数代码来源网络 / 文档 / AI. 

## 功能列表 (兼操作说明) 
1. 所见即所得的插入、删除、选择功能, 均支持鼠标、键盘操作, 且和各大 IDE 的操作逻辑基本保持一致
	1. 鼠标: 直接点击即可移动光标, 拖动即可选择
	2. 键盘: 
		1. 使用方向键移动光标, 到达屏幕边缘时会自动滚动屏幕以适配光标
		2. `Shift` + 方向键进行选择, 具体逻辑和各大 IDE 基本保持一致
		3. 支持 `Backspace` (退格) 键删除选中内容或光标前一格的字符, 也支持 `Delete` 键删除光标后一格 (或选中内容) 的字符
		4. 也支持使用 `Home` / `End` 键来直接移动到文件头 / 尾
2. 支持四向滚动屏幕, 且能自动滚动
	1. 使用鼠标滚轮即可上下滚动页面, 按住 `Ctrl` 来更精确地调整
	2. 也支持使用鼠标滚轮来左右滚动页面, 按 `Ctrl` 来精确滚动   (但 Win11 新版终端中不支持)
	3. 使用 `Ctrl` + `{[` / `]}` 即可向左 / 右滚动屏幕, 按住 Alt 来更精确地调整 (这里说的 `{[` 和 `]}` 指的是键盘上的按键) 
	4. 使用键盘移动光标时, 屏幕会自动滚动来使光标始终位于屏幕内, 进行插入 / 删除时也一样
	5. 使用鼠标时也会自动移动, 但没有那么智能 (比如鼠标滑到最右侧时不会自动往右滚屏, 往上 / 往下时同理)
	6. 使用 `Ctrl` + `上` / `下` 方向键也可以滚动屏幕, 一次一行
	7. 也支持使用 `PageUp` / `PageDown` 键来滚动页面, 一次一屏 (少一行, 也就是上一屏的最后一行 / 第一行会保留)
3. 支持复制 / 粘贴 (使用 Win11 的新终端时粘贴会被终端接管, 但仍能正常发挥作用, 只是速度慢一点) 
	- 使用 `Ctrl` + `C` / `V` 即可
	- 在 Win11 终端中可以使用 `Ctrl` + (任意一个其它按键) + `V` 来绕过终端自带的粘贴功能, 当然不绕过也行
4. 针对不同大小的窗口能自动适配, 即使在使用过程中拉动窗口大小也能即时适配
	- 使用老式终端时可能需要拉动左右边框才会自动适配, 单拉上下边框没用
	- 大部分控制台窗口没有动态适配 (即当窗口打开后, 拉动终端窗口大小, 控制台窗口不会自动改变大小, 但会在控制台无法容纳自身时自动关闭)
		- 但是有静态适配, 根据不同的终端大小自动调整控制台窗口大小, 并在空间不足以打开时弹出提示
5. 支持中文输入, 也支持中英文混杂排版, 加上制表符 (`Tab`) 也完全没问题
6. 双击 `Esc` 即可退出 (按一次 `Esc` 时会有提示) , 非常符合直觉, 哪怕是电脑小白应该也能成功退出
	- 在有弹出窗口时按第一次 `Esc` 只会关闭当前窗口, 但只要一直按 `Esc` 就能退出, 而且已经按过一次 `Esc` 时也能强制退出, 所以电脑小白应该也没问题......吧
7. `Ctrl` + `S` 可以保存
8. 支持打开已经存在的文件, 使用 `Ctrl` + `O` 即可
9. 支持新建文件, 使用 `Ctrl` + `N` 即可. 具体逻辑基本同主流编辑器一致, 在新建时无需给文件起名、选择路径等, 直到保存时才需要指定保存路径
10. 支持另存为, 使用 `Ctrl` + `Shift` + `S` 即可. 当原文件无法继续写入时也会自动触发
	- 支持覆盖保存, 此时会弹出控制台窗口询问是否覆盖
11. 打开 / 另存为都有对应的控制台内窗口, 并有对应提示 (失败了也有对应的提示) , 新窗口也能部分地动态适配控制台大小
    - 文件路径的输入有防呆设计, 可以正常识别多种输入方式, 同时支持按 Tab 自动补全
		- 按 Tab 可以补全未写完的路径, 在当前路径有效时会逐次循环遍历当前目录下的其它文件 / 目录, 在目录名后加 `\` 后再按 Tab 则会遍历子目录
		- 也支持带点的路径, 比如 `.\testText.txt` 或 `..\..\testText.txt`, 不加前缀默认当前目录, 补全时会补上前缀
		- 总的来说逻辑和一般终端的类似, 试一试就基本能知道
12. 支持撤销 / 重做, 按 `Ctrl` + `Z` / `Y` 即可 (任意文本输入框内都支持) , 最高支持步数、操作融合上限及策略均可配置
13. 当前文件未保存时就试图打开 / 新建新文件 / 退出时, 会用一个控制台窗口提示是否先保存
	- 弹出窗口也支持用鼠标进行选择！
14. 支持查找 / 替换, 使用 `Ctrl` + `F` / `H` 即可, 查找 / 替换时, 使用方向键在结果中切换. 替换时, 按`回车`替换当前项目, `Shift` + `回车` 替换全部
	- 替换支持撤回, 虽然只能挨个撤回
	- 查找 / 替换均支持自主选择是否区分大小写以及全字匹配, 只需在窗口中按 Tab 切换到对应选项卡, 然后使用方向键 (左 / 右) 切换即可 (也支持鼠标)
	- 查找 / 替换均支持正则表达式
		- 比如可以尝试打开本项目中的 Editor.ixx, 然后查找 `(wstring)(_view)?`, 替换为 `$2` (记得打开正则匹配), 即可将 `wstring` 删除, 并将 `wstring_view` 替换为 `_view`
15. 支持不同编码类型, 并会在编码类型错误时给出提示
	- 目前支持 UTF-8 和 GB 2312
	- 也可以强行用错误的编码方式打开, 但会有很多 bug (最明显的就是字符宽度错误以及连带的光标、制表符错误) , 而且这些 bug 不在修复范围内
	- 也支持自动识别编码方式
16. 支持自动保存, 会自动保存至另一个临时文件 (后缀名、默认名字均可配置), 且会在恰当的时机删除该临时文件
	- 默认 3 分钟保存一次 (可配置)
17. 能同时兼容新终端与老终端
	- Win11 下 `Win` + `R` 输入 `conhost` 即可调出老终端, 用管理员模式运行时貌似也会默认使用老终端
	- 加上没有使用一些新特性, 理论上在老版本 Windows 下也可以正常使用, 但未经测试
18. 支持通过命令行调用, 此时使用命令行参数可直接以指定编码打开文件
	1. "-o" / "--open" 表示打开, 后接要打开的文件路径
	2. "-e" / "--encoding" 表示编码, 后接编码名称
		- 有防呆设计, 比如 "utf-8", "UTF-8", "UTF 8", "utf8" 都能定位到 UTF-8
	3. "-h" / "--help" / "/?" 可以输出帮助, 会覆盖其它一切参数
	4. 只需要指定打开文件路径时, 可以直接作为参数附加, 无需带 "-o" / "--open"
	5. 示例   (程序名字取 ConsoleNotepad) : 
		1. ConsoleNotepad 1.txt
		2. ConsoleNotepad -o 1.txt -e UTF-8
		3. ConsoleNotepad --encoding "gb 2312" -o 1.txt
		4. ConsoleNotepad /?
19. 有简单友好地图形化设置界面, 按 `Ctrl` + `P` 即可打开.
	- 该界面完全支持鼠标操作. 可以直接点击来切换设置项, 也可以用滚轮上下滚动, 甚至可以直接通过点击左右两侧的箭头来切换选项 (其它地方其实也可以, 比如查找 / 替换里的就可以)
	- 设置内容会保存到当前目录下
20. 有历史记录窗口，按 `Ctrl` + `L` 即可打开，其中记录了所有还能撤销 / 重做的操作，可以直观、简单地撤销 / 重做
	- 该窗口也支持鼠标操作
21. 有行号显示, 支持自定义行号宽度 (设置为 0 即可关闭)
	- 行号是彩色的
## 题目要求及完成情况

所选题目为赛道一, 复杂版 (控制台编辑器) 

- [x] 你需要做到的有
	- [x] 作为一个文本编辑器应该有的基础编辑功能, 必须为 **WYSIWYG**  (What You See Is What You Get, 所视即所得) 的编辑方式.
	- [x] 足够简单易懂的**交互逻辑**, 不用查文档也能让新手知道怎么保存退出.
	- [x] 在一些场景下有基本的TUI界面, 例如查找替换窗口, 选择保存路径等.
- [ ] 你可以尝试的有
	- [x] 响应式的界面设计, 在不同尺寸下界面都能表现良好.
	- [ ] 跨平台兼容性, 也许你的代码在 Windows, Mac 和 Linux 上都能运行.  
	      *(其实做了个框架, 只是还没与对应 API 接轨, 而且编译用的 MSBuild 方案不太方便跨平台, 用的 C++ 标准太新很多编译器也不太支持)*
	- [x] 实现组件的封装, 让代码更简洁易懂, 同时具有后续的拓展能力.    
		  *(也许做了封装, 但代码很难说简洁易懂......尽管一开始确实是朝着那方面努力的)*   
		  ~~*(最后还是不要脸地勾上了)*~~
	- [x] 也许可以实现一些鼠标交互? 比如用鼠标定位光标位置.
	- [x] 实现多彩的界面设计, 让编辑器不那么单调.   
		  *(有吗? 如有)*     
		  ~~*(行号、标题、正文加起来三种颜色, 加上代码内对彩颜的支持比较好, 实际上想要什么颜色都行, 所以还是给勾上吧)*~~

部分 Windows 相关代码引用自 Microsoft Learn 中的文档. 

项目未使用任何第三方库, 只使用 Windows API 与 C++ 标准内容. 项目同样未使用任何现有代码, 所有代码均是新编写的. 

主要参考文献: cppreference, Microsoft Learn

## 编译环境搭建

**IDE**: Visual Studio 2022 17.13.5 x64    
**平台工具集**: Visual Studio 2022 (v143)    
**C++ 语言标准**: 最新  ( `/std:c++latest` )    
**操作系统**: Windows 11 22H2 22621.3737

## 问题清单
基本按时间排序, 越靠上的越古老

(加了勾选框却完全不勾.jpg)
- [ ] 调整窗口大小的过程中编辑器会鬼畜 (特别是快速拉动时) , 不过这个不涉及核心所以不急着修 (谁没事干天天拉编辑器窗口大小玩) 
- [ ] 代码有点屎山了 (长期优化计划) 
- [ ] 性能优化, 目前的 FormattedString 完全没有缓存, 每次都要重新算, 是可以把 C++ 写成 Python 的优化水平 () 

## 更新计划
理论上会按照顺序更新, 但实际上很可能随缘更 () 

1. 增加文件名、光标位置等的显示
2. 优化滚动时的性能 (由于加了行号显示, 目前在快速滚动时会有很明显的性能问题)
3. 优化查找 / 替换, 使其能只使用 \n 匹配 \r
4. 增加更多的高亮提示等 (比如查找 / 替换时高亮所有选择结果) , 这一条涉及的底层比较多所以放后面了
5. 添加十六进制查看 / 编辑功能