<div align=center>
  <img src=./resources/breeze-shell-small.webp?1 />
<h1>Breeze Shell</h1> 
<h5>Bring fluency & delication back to Windows</h5>
<div>
  <img widtb=500 src=./resources/preview1.webp />
</div>
</div>

Breeze is an **alternative context menu** for Windows 10 and Windows 11.

## Fluency
Breeze is designed with animations in mind.

<img src=https://github.com/user-attachments/assets/304fdd08-ef67-4cdb-94cc-83b47d41eb36 height=300 />

## Extensibility
Empowered by the embedded JavaScript script api, Breeze enables you to extend the functionalities of your context menu in a few lines of code.

```javascript
shell.menu_controller.add_menu_listener((a)=>{
        a.menu.add_menu_item_after({
            type: 'button',
            name: 'test',
            action: ()=>{
               shell.println(123)
            }
     }, 0)
 })
```
[See full bindings →](./src/shell/script/binding_types.d.ts)
