> [!WARNING]
> This project is still in active development. File a bug report if you meet
> any!\
> 此项目仍在开发阶段，如果遇到问题请发送 Issue

[中文](./README_zh.md)

<div align=center>
  <img src=./resources/icon.webp width=300 />
<h1>Breeze Shell</h1>
<h5>Bring fluency & delication back to Windows</h5>
<div>
  <img widtb=500 src=./resources/preview1.webp />
</div>
</div>

Breeze is an **alternative context menu** for Windows 10 and Windows 11.

## Fluent

Breeze is designed with animations in mind.

<img src=https://github.com/user-attachments/assets/304fdd08-ef67-4cdb-94cc-83b47d41eb36 height=300 />

## Extensible

Empowered by the embedded JavaScript script api, Breeze enables you to extend
the functionalities of your context menu in a few lines of code.

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

[See full bindings →](./src/shell/script/binding_types.d.ts)

## Configurable
Breeze shell exposed a bunch of configurations ranging from item height to background radius method. Customize them as you need.

[Configuration Document →](./CONFIG.md)

## Lightweight & Fast

Breeze uses breeze-ui, which is implemented to be a cross-platform, simple,
animation-friendly and fast ui library for modern C++, with the support of both
NanoVG and ThorVG render context. This allowed Breeze to have a delicated user
interface in ~2MiB.

# Try it out!

Download `inject.exe` and `shell.dll` into the same directory. Run `inject.exe`
and click "注入一次" for testing.

![image](https://github.com/user-attachments/assets/474e1fbb-b94c-49c2-afb1-18c84f9f2639)


# Building

Breeze uses xmake. You'd have to install xmake in your computer first. Then,
type `xmake` in the project dir and follow the instructions. Both clang-cl and
MSVC 2019+ can build this project.

# Developing

After building successfully once, you can oprn the project dir in VSCode for
development. Install clangd plugin for full intellisense.

# Credits
[@lipraty](https://github.com/lipraty) - Icon Design
