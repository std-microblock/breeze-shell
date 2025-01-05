import { ClangASTD } from "./clang-ast";
import { existsSync, readFileSync, rmSync, writeFileSync } from "node:fs";
import { join } from "node:path";

const ast = JSON.parse(readFileSync(join(__dirname, "ast.json"), "utf-8")) as ClangASTD;

const targetFile = 'binding_types.h'
const outputFile = 'binding_qjs.h'
// filter out loc.file contains targetFile

let binding = `#pragma once
#include "binding_types.h"
#include "quickjs.h"
#include "quickjspp.hpp"

`

const parseFunctionQualType = (type: string) => {
    // std::variant<int, std::string> (std::string, std::string)
    const match = type.match(/^(.*?)\s*\((.*?)\)$/);
    if (!match) {
        throw new Error(`Invalid function type: ${type}`);
    }
    const [, returnType, args] = match;
    return {
        returnType: returnType.trim(),
        args: args.split(',').map(arg => arg.trim())
    };
}

if (ast.kind !== 'NamespaceDecl') {
    throw new Error('Root node is not a NamespaceDecl');
}

let currentNamespace = ast.name;

const generateForRecordDecl = (node2: ClangASTD) => {
    if (node2.kind !== 'CXXRecordDecl') {
        throw new Error('Node is not a RecordDecl');
    }

    const structName = node2.name;

    const fields: {
        name: string;
        type: string;
    }[] = [];

    const methods: {
        name: string;
        returnType: string;
        args: string[];
    }[] = [];

    for (const node of node2.inner!) {
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
                args: parsed.args
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

    for (const method of methods) {
        binding += `
        JS_SetPropertyStr(
            ctx, obj, "${method.name}",
            JS_NewCFunction(
                ctx,
                [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) -> JSValue {
                    auto obj = js_traits<${currentNamespace}::${structName}>::unwrap(ctx, this_val);
                    if (argc == ${method.args.length}) {
                        return js_traits<${method.returnType}>::wrap(
                            ctx,
                            obj.${method.name}(
                                ${method.args.map((arg, i) => `js_traits<${arg}>::unwrap(ctx, argv[${i}])`).join(', ')}
                            )
                        );
                    } else {
                        return JS_ThrowTypeError(ctx, "Expected ${method.args.length} arguments");
                    }
                },
                "${method.name}", ${method.args.length}));
        `;
    }

    binding += `
        return obj;
    }
};`;
}


for (const node of ast.inner!) {
    if (node.kind === 'CXXRecordDecl') {
        generateForRecordDecl(node);
    }
}

const paths = [
    join(__dirname, 'src/shell/script'),
    join(__dirname, '../src/shell/script'),
    join(__dirname, '../../src/shell/script')
]

for (const path of paths) {
    try {
        if (existsSync(join(path, targetFile))) {
            writeFileSync(join(path, outputFile), binding);
            break;
        }
    } catch (e) {
        console.error(e);
    }
}

rmSync(join(__dirname, 'ast.json'));