import { ClangASTD } from "./clang-ast";
import { existsSync, readFileSync, rmSync, writeFileSync } from "node:fs";
import { join } from "node:path";
import { cTypeToTypeScript } from "./c-type-parser";

const ast = JSON.parse(readFileSync(join(__dirname, "ast.json"), "utf-8")) as ClangASTD;

const targetFile = 'binding_types.h'
const outputFile = 'binding_qjs.h'
// filter out loc.file contains targetFile

let binding =
    `// This file is generated by scripts/bindgen/index.ts
// Do not modify this file manually!

#pragma once
#include "binding_types.h"
#include "quickjs.h"
#include "quickjspp.hpp"

template <typename T>
struct js_bind {
    static void bind(qjs::Context::Module &mod) {}
};
`

let typescriptDef = `// This file is generated by scripts/bindgen/index.ts
// Do not modify this file manually!

declare module 'mshell' {

`

const parseFunctionQualType = (type: string) => {
    // std::variant<int, std::string> (std::string, std::string)
    // std::function<void(int, std::string)> (int, std::string)

    enum State {
        ReturnType,
        Args,
        Done
    }

    let state = State.ReturnType;
    let returnType = '';
    let currentArg = '';
    let args: string[] = [];
    let depth = 0;
    let angleBracketDepth = 0;

    for (let i = 0; i < type.length; i++) {
        const char = type[i];

        if (char === '<') {
            angleBracketDepth++;
        } else if (char === '>') {
            angleBracketDepth--;
        }

        switch (state) {
            case State.ReturnType:
                if (char === '(' && angleBracketDepth === 0) {
                    state = State.Args;
                    returnType = returnType.trim();
                } else {
                    returnType += char;
                }
                break;

            case State.Args:
                if (char === '(') depth++;
                if (char === ')') {
                    if (depth === 0 && angleBracketDepth === 0) {
                        if (currentArg.trim()) args.push(currentArg.trim());
                        state = State.Done;
                        break;
                    }
                    depth--;
                }
                if (char === ',' && depth === 0 && angleBracketDepth === 0) {
                    args.push(currentArg.trim());
                    currentArg = '';
                } else {
                    currentArg += char;
                }
                break;
        }
    }

    if (state !== State.Done) {
        throw new Error('Invalid function type');
    }

    return {
        returnType,
        args
    };
}

if (ast.kind !== 'NamespaceDecl') {
    throw new Error('Root node is not a NamespaceDecl');
}

let currentNamespace = 'mb_shell::js'

const generateForRecordDecl = (node_struct: ClangASTD) => {
    if (node_struct.kind !== 'CXXRecordDecl') {
        throw new Error('Node is not a RecordDecl');
    }

    const structName = node_struct.name;

    const fields: {
        name: string;
        type: string;
    }[] = [];

    const methods: {
        name: string;
        returnType: string;
        args: string[];
        static: boolean;
    }[] = [];

    if (!node_struct.inner) return;

    for (const node of node_struct.inner) {
        if (node.name?.startsWith('$')) continue;

        if (node.kind === 'FieldDecl') {
            fields.push({
                name: node.name!,
                type: node.type!.qualType
            });
        }

        if (node.kind === 'CXXMethodDecl') {
            const parsed = parseFunctionQualType(node.type!.qualType);

            if (
                ['operator='].includes(node.name!)
            ) continue;

            methods.push({
                name: node.name!,
                returnType: parsed.returnType,
                args: parsed.args,
                static: node.storageClass === 'static'
            });
        }
    }

    console.log({
        structName, fields, methods
    });

    binding += `
template <> struct qjs::js_traits<${currentNamespace}::${structName}> {
    static ${currentNamespace}::${structName} unwrap(JSContext *ctx, JSValueConst v) {
        ${currentNamespace}::${structName} obj;
    `;

    for (const field of fields) {
        binding += `
        obj.${field.name} = js_traits<${field.type}>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "${field.name}"));
        `;
    }

    binding += `
        return obj;
    }

    static JSValue wrap(JSContext *ctx, const ${currentNamespace}::${structName} &val) noexcept {
        JSValue obj = JS_NewObject(ctx);
    `;

    for (const field of fields) {
        binding += `
        JS_SetPropertyStr(ctx, obj, "${field.name}", js_traits<${field.type}>::wrap(ctx, val.${field.name}));
        `;
    }

    // for (const method of methods) {
    //     binding += `
    //     JS_SetPropertyStr(
    //         ctx, obj, "${method.name}",
    //         JS_NewCFunction(
    //             ctx,
    //             [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) -> JSValue {
    //                 auto obj = js_traits<${currentNamespace}::${structName}>::unwrap(ctx, this_val);
    //                 if (argc == ${method.args.length}) {
    //                     ${method.args.length > 0 ? `return js_traits<${method.returnType}>::wrap(
    //                         ctx,
    //                         obj.${method.name}(
    //                             ${method.args.map((arg, i) => `js_traits<${arg}>::unwrap(ctx, argv[${i}])`).join(', ')}
    //                         )
    //                     );` : `obj.${method.name}(); return JS_UNDEFINED;`
    //         }
    //                 } else {
    //                     return JS_ThrowTypeError(ctx, "Expected ${method.args.length} arguments");
    //                 }
    //             },
    //             "${method.name}", ${method.args.length}));
    //     `;
    // }

    binding += `
        return obj;
    }
};`;

    /**
     * 
     *   module.class_<MyClass>("MyClass")
                    .constructor<>()
                    .constructor<std::vector<int>>("MyClassA")
                    .fun<&MyClass::member_variable>("member_variable")
                    .fun<&MyClass::member_function>("member_function")
                    .static_fun<&MyClass::static_function>("static_function")
     */
    binding += `
template<> struct js_bind<${currentNamespace}::${structName}> {
    static void bind(qjs::Context::Module &mod) {
        mod.class_<${currentNamespace}::${structName}>("${structName}")
            .constructor<>()`;
    for (const method of methods) {
        if (method.static) {
            binding += `
                .static_fun<&${currentNamespace}::${structName}::${method.name}>("${method.name}")`;
        } else {
            binding += `
                .fun<&${currentNamespace}::${structName}::${method.name}>("${method.name}")`;
        }
    }

    for (const field of fields) {
        binding += `
                .fun<&${currentNamespace}::${structName}::${field.name}>("${field.name}")`;
    }

    binding += `
            ;
    }

};
    `;

    typescriptDef += `
export class ${structName} {
\t${fields.map(field => `${field.name}: ${cTypeToTypeScript(field.type)}`).join('\n\t')}
\t${methods.map(method => {
    return `${method.static ? 'static ' :''}${method.name}: ${cTypeToTypeScript(`${method.returnType}(${method.args.join(', ')})`)}`
}).join('\n\t')}
}
    `;

}

const structNames: string[] = []
for (const node of ast.inner!) {
    if (node.kind === 'CXXRecordDecl') {
        generateForRecordDecl(node);
        if (node.name && node.inner)
            structNames.push(node.name);
    }
}

binding += `
inline void bindAll(qjs::Context::Module &mod) {
`

for (const structName of structNames) {
    binding += `
    js_bind<${currentNamespace}::${structName}>::bind(mod);
`
}

binding += `
}
`

typescriptDef += `
}
`

const paths = [
    join(__dirname, 'src/shell/script'),
    join(__dirname, '../src/shell/script'),
    join(__dirname, '../../src/shell/script')
]

for (const path of paths) {
    try {
        if (existsSync(join(path, targetFile))) {
            writeFileSync(join(path, outputFile), binding);
            writeFileSync(join(path, 'binding_types.d.ts'), typescriptDef);
            break;
        }
    } catch (e) {
        console.error(e);
    }
}

rmSync(join(__dirname, 'ast.json'));