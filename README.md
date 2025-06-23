Here's the revised README.md with links styled as buttons and all media centered:

```markdown
> [!WARNING]
> This project is still in active development. File a bug report if you meet any!
> This project is still under development. If you encounter any problems, please send an Issue
>
> Both English and Chinese issues are accepted.
> Issue (You can use Chinese or English)
> 对于中国人来说, [中文](./README_zh.md)

<div align="center">
  <a href="./DONATE.md" style="display: inline-block; padding: 10px 20px; margin: 10px; background: #4CAF50; color: white; text-decoration: none; border-radius: 5px;">Donate Me</a>
  <a href="https://discord.gg/MgpHk8pa3d" style="display: inline-block; padding: 10px 20px; margin: 10px; background: #7289DA; color: white; text-decoration: none; border-radius: 5px;">Discord</a>
</div>

<div align="center">
  <img src="./resources/icon.webp" width="300" />
  <h1>Breeze Shell</h1>
  <h5>Bring fluency & delication back to Windows</h5>
  <div align="center">
    <img src="https://github.com/user-attachments/assets/1d0e8b5d-c808-4d3d-8004-0a2490775d96" width="500" />
  </div>
</div>

Breeze is an **alternative context menu** for Windows 10 and Windows 11.

## Fluent
Breeze is designed with animations in mind.

<div align="center">
  <img src="https://github.com/user-attachments/assets/1d0e8b5d-c808-4d3d-8004-0a2490775d96" width="500" />
</div>

All animations are configurable and can be scaled and disabled as you want.

## Extensible
Empowered by the embedded JavaScript script api, Breeze enables you to extend the functionalities of your context menu in a few lines of code.

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

<a href="./src/shell/script/binding_types.d.ts" style="display: inline-block; padding: 8px 16px; background: #03A9F4; color: white; text-decoration: none; border-radius: 5px;">See full bindings</a>

Send pull requests to <a href="https://github.com/breeze-shell/plugins" style="display: inline-block; padding: 8px 16px; background: #FF9800; color: white; text-decoration: none; border-radius: 5px;">this repo</a> to add your script to the plugin market!

## Configurable
Breeze shell exposed a bunch of configurations ranging from item height to background radius method. Customize them as you need.

<a href="./CONFIG.md" style="display: inline-block; padding: 8px 16px; background: #9C27B0; color: white; text-decoration: none; border-radius: 5px;">Configuration Document</a>

The config menu of breeze-shell can be found as you open your `Data Folder` and right-click anywhere inside it.

## Lightweight & Fast
Breeze uses breeze-ui, which is implemented to be a cross-platform, simple, animation-friendly and fast ui library for modern C++, with the support of both NanoVG and ThorVG render context. This allowed Breeze to have a delicated user interface in ~2MiB.

# Try it out!
Download and extract the zip, and Run `breeze.exe`.

<div align="center">
  <img src="https://github.com/user-attachments/assets/e9ab080d-26a2-4d71-b139-31062d79101c" width="500" />
</div>

# Building
Breeze uses xmake. You'd have to install xmake in your computer first. Then, type `xmake` in the project dir and follow the instructions. Both clang-cl and MSVC 2019+ can build this project.

# Developing
After building successfully once, you can open the project dir in VSCode for development. Install clangd plugin for full intellisense.

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
```

Key changes made:
1. Created styled buttons using HTML/CSS with inline styles
2. Centered all images and video placeholders using `div align="center"`
3. Maintained all original functionality and links
4. Added consistent button styling with different colors for different purposes
5. Ensured all media elements are centered in the documentation

Note: GitHub Markdown has limited CSS support, but these changes will render correctly on most platforms while maintaining maximum compatibility.
