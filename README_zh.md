<div align=center>
  <img src=./resources/icon.webp width=200/>
<h1>Breeze Shell</h1>
<h5>为 Windows 重新带来流畅与精致体验</h5>
<div>
  <img widtb=500 src=./resources/preview1.webp />
</div>
</div>

Breeze 是专为 Windows 10/11 设计的现代化**次世代右键菜单解决方案**。

## 丝滑流畅

Breeze 以交互动画为核心设计理念。

<img src=https://github.com/user-attachments/assets/304fdd08-ef67-4cdb-94cc-83b47d41eb36 height=300 />

## 无限扩展

通过嵌入式 JavaScript 脚本 API，您可以用寥寥数行代码为右键菜单增添全新功能。

```javascript
shell.menu_controller.add_menu_listener((e) => {
  e.menu.add({
    type: "button",
    name: "Shuffle Buttons",
    action: () => {
      for (const item of menus) {
        item.set_position(Math.floor(menus.length * Math.random()));
      }
    },
  }, 0);
});
```

[查看完整 API 文档 →](./src/shell/script/binding_types.d.ts)

## 轻量高速

Breeze 基于自研 breeze-ui 框架实现，这是一个跨平台、简洁优雅、动画友好的现代 C++ UI 库，支持 NanoVG 和 ThorVG 双渲染后端。这使得 Breeze 能在约 2MB 的体积下实现精致的视觉体验。

# 立即体验

从 Releases 下载，然后解压压缩包并运行 `breeze.exe`

# 构建指南

本项目使用 xmake 构建系统。请先安装 xmake，在项目根目录执行 `xmake` 命令并按提示操作。支持 clang-cl 和 MSVC 2019+ 编译器。

# 开发指南

首次构建成功后，可使用 VSCode 打开项目进行开发。建议安装 clangd 插件以获得完整的代码智能提示。
