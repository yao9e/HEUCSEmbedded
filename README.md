# HEU 6系 嵌入式课设（Arduino 及仿真部分）
2023年12月6日20:27:27

哈尔滨工程大学（哈工程）计算机科学与技术学院大四嵌入式作业。使用 FreeRTOS 作为 Arduino UNO 开发版的操作系统，修改了 FreeRTOS 内核设置。
使用了协作式CPU调度方案，实现三线程协作式调度，可用 vTaskDelay （或 delay ） 函数。
Release 版本经过测试，可直接运行，运行环境见后。

## 项目结构
``` 
│ .gitignore
│ platformio.ini    --> platformio 配置文件，
│                       当安装 PlatformIO 插件后，可自动按照此文件
│                       安装依赖
│
├─.vscode           --> 存放vscode的插件信息，由 PlatformIO 和 
│   extensions.json     VS Code 自动生成;
│   settings.json
│
├─include           --> PlatformIO 自动生成，用于存放头文件
│   README
│
├─lib               --> 存放本项目使用的、且未能自动下载的依赖库，
│   README              内有笔者自己找的 DHT11 依赖库，
│                       请在 FreeRTOS 使用此库控制DHT;
│
│
├─proteus           --> 存放 Proteus 仿真文件，使用 Proteus 8
│   FinalWork.pdsprj    打开，请注意在仿真之前载入本项目生成的
│                       elf 文件;
│
├─src               --> Arduino 程序源码所在文件夹
│   main.cpp        --> Arduino 程序源码
│
└─test              --> PlatformIO 项目测试文件夹
    main.cpp.backup     笔者用来存放代码副本了
    main.cpp.backup2
    README
```

## 使用方法
本嵌入式下位机采用 PlatformIO 开发，请按照任课教师发的 `嵌入式技术课程设计Proteus实验指导书（带书签）.pdf 中的` 的 `3.3 使用 PlatformIO IDE 生成程序代码` 配置环境，或自行在网络上查找如何使用 PlatformIO 开发 Arduino 程序。

1. 下载本项目，并解压;
2. 在按照实验指导手册完成环境配置后，在 VS Code 的 PlatformIO 中打开本项目根目录（即存放有 `README.md` 文件的那一级目录;
3. 若请等待 PlatformIO 加载，若网络环境良好，PlatformIO 会自动在 5min 内项目加载与依赖库安装;
4. 按照`main.cpp`中的说明修改内核配置后，编译本项目（左下角或右上角的 √ Build 符号）;
5. 使用 Proteus 8 打开 位于 proteus 文件夹中的 Proteus 工程文件，文件名为 `FinalWork.pdsprj`;
6. 将 PlatformIO 生成的 ELF 或 HEX 文件载入控制器中（双击仿真文件中的最大的那个方形元件，在 Program Flie 中填入 `..\.pio\build\uno\firmware.elf`;
7. 运行仿真即可，可使用 XCOM 进行串口通讯，Proteus 仿真文件默认使用 COM1 串口;

main.cpp 中的重要提示信息：
``` C++
/*
 ! ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!!! Attention !!!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 !
 ! 请在编译此项目前，将 FreeRTOS 配置文件中的 configUSE_PREEMPTION 定义为 0 ，以便开启协同式调度
 !      关于此调度方式的详情可在 https://www.freertos.org/zh-cn-cmn-s/a00110.html 中找到。
 ! FreeRTOS的配置文件是 .pio\libdeps\uno\FreeRTOS\src\FreeRTOSConfig.h （需要完成依赖安装） 
 !
 ! Proteus 中，单片机的ELF文件路径为 ..\.pio\build\uno\firmware.elf
 !
 ! ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!!! Attention !!!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
```
## 测试环境
Release 版本已经过测试，测试环境如下：
- Proteus 8.10 SP3
- PlatformIO IDE v3.3.1
- VS Code v1.84.2 (user setup)
- Windows 10 19045.3693

## 已知 bug 信息
1. 在项目代码基础上增添功能时，有概率遇到无法仿真问题，包括但不限无法分配内存，内存溢出，请各位使用者自行解决。
2. 在本项目基础上增添信号量或消息队列功能，有概率导致无法仿真，请自行解决

# 可用于 Arduino 的 FreeRTOS 移植版提示信息
**Errors**
- Stack Overflow: If any stack (for the loop() or) for any Task overflows, there will be a slow LED blink, with 4 second cycle.
- Heap Overflow: If any Task tries to allocate memory and that allocation fails, there will be a fast LED blink, with 100 millisecond cycle.

可用于 Arduino 的 FreeRTOS 移植版仓库地址：https://github.com/feilipu/Arduino_FreeRTOS_Library/
你可在其中找到更多关于 FreeRTOS 的信息。

# Contact
若你在使用过程中遇到任何问题，请使用 Issue 提出，或给 yao9e@outlook.com 邮箱发送邮件。