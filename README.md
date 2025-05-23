# x64_tracer
x64dbg 条件分支记录器 [插件]

x64dbg conditional branches logger [Plugin]

这只是一个正在进行的工作，所以不要期待太多。

This is just a work in progress so don't expect too much.

请测试它，如果发现bug请报告。

Please test it and report if you find bugs.

## 使用方法 / Usage

我是这样使用它的：

I use it like this :

首先你需要2个断点来进行追踪，起始点和结束点。

First you need 2 break points to trace between, Start and End.

### 步骤 / Steps

1 - 将目标文件放入调试器中。
    Throw your target in the debugger.

2 - 起始点应该断下。
    The Start point should break.

3 - 启动插件。
    Start the plugin.

4 - 输入你感兴趣的模块名称，插件会尝试检测当前RIP所在的模块名称。
    Enter the name of the module you are interested in, the plugin will try to detect the name where RIP is now.

5 - 输入目标VA，即日志记录应该停止的点，这是你上面设置的结束点。
    Enter the target VA, i.e the point where logging should stop, It's your End point from above.

在此模块中将进行单步执行，但如果RIP跳出此模块，那么在这些外部模块中将进行步过执行，
除非有回调到目标模块，那么将在目标模块中进行单步执行。

There will be single stepping into this module but if RIP goes out of this module then there will be stepping over
in those external modules unless there is a call back into the that target module then there will be a single step into the target module.

6 - 步进将继续直到我们到达第二个点。
    stepping will continue until we hit the 2nd point.

7 - 插件将显示一个消息框告诉我们已经结束追踪。
    The plugin will show a message box telling we have ended tracing.

8 - 现在你可以将结果保存到日志文件中，如下图所示。
    now you can save the result to a log file which looks like this in the image below.

9 - 你可以使用任何差异比较系统来比较两次追踪的结果，这里我使用了Notepad++的插件。
    you can use any diffing system to compare the results between 2 traces, here I used a plugin for Notepad++.
