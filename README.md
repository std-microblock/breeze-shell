> [!WARNING]
> This project is still in active development. File a bug report if you meet
> any!\
> 此项目仍在开发阶段，如果遇到问题请发送 Issue
>
> Both English and Chinese issues are accepted.
> Issue 中使用中文或英文均可

[中文](./README_zh.md) [Donate Me](./DONATE.md) [Discord](https://discord.gg/MgpHk8pa3d)

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

<img src=https://github.com/user-attachments/assets/1d0e8b5d-c808-4d3d-8004-0a2490775d96 />

All animations are configurable and can be scaled and disabled as you want.
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

Send pull requests to [this repo](https://github.com/breeze-shell/plugins) to add your script to the plugin market!

## Configurable
Breeze shell exposed a bunch of configurations ranging from item height to background radius method. Customize them as you need.

[Configuration Document →](./CONFIG.md)

The config menu of breeze-shell can be found as you open your `Data Folder` and right-click anywhere inside it.

## Lightweight & Fast

Breeze uses breeze-ui, which is implemented to be a cross-platform, simple,
animation-friendly and fast ui library for modern C++, with the support of both
NanoVG and ThorVG render context. This allowed Breeze to have a delicated user
interface in ~2MiB.

# Try it out!

Download and extract the zip, and Run `breeze.exe`.

![image](https://github.com/user-attachments/assets/e9ab080d-26a2-4d71-b139-31062d79101c)


# Building

Breeze uses xmake. You'd have to install xmake in your computer first. Then,
type `xmake` in the project dir and follow the instructions. Both clang-cl and
MSVC 2019+ can build this project.

# Developing

After building successfully once, you can oprn the project dir in VSCode for
development. Install clangd plugin for full intellisense.

# Credits

#### Third-party libraries
- https://github.com/std-microblock/blook
- https://github.com/quickjs-ng/quickjs
- https://github.com/ftk/quickjspp
- https://github.com/getml/reflect-cpp
- https://github.com/glfw/glfw
- https://github.com/Dav1dde/glad
- https://github.com/memononen/nanovg
- https://github.com/memononen/nanosvg
- https://github.com/freetype/freetype

#### Others
- [@lipraty](https://github.com/lipraty) - Icon Design
- [moudey/Shell](https://github.com/moudey/Shell) - Inspiration
  (All code in this rewrite is ORIGINAL and UNRELATED to moudey/Shell!)
