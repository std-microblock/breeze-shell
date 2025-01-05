import { readFileSync, writeFileSync } from "fs";
import { types } from "util";

type Type = {
    isArr: boolean;
    type: 'string' | 'number' | 'boolean' | string;
}
type TypeMap = {
    [typeName: string]: {
        [fieldName: string]: {
            types: Type[];
            isOptional: boolean;
        }
    };
};

function generateInterfaces(json: any): string {
    const typeMap: TypeMap = {};

    const traverse = (obj: any, k = "Main") => {
        typeMap[k] ??= {
        };

        const typeCur = typeMap[k];

        for (const key in typeCur) {
            if (!obj[key]) {
                typeCur[key].isOptional = true;
            }
        }

        for (const key in obj) {
            const value = obj[key];
            
            typeCur[key] ??= {
                types: [],
                isOptional: false
            };

            const type = typeCur[key];

            const pushIfNotExists = (t: Type) => {
                if (type.types.findIndex(x => x.type === t.type && x.isArr === t.isArr) === -1) {
                    type.types.push(t);
                }
            }

            if (Array.isArray(value)) {
                if (value.length === 0) {
                    pushIfNotExists({
                        isArr: true,
                        type: 'any'
                    });
                } else {
                    for (const val of value) {
                        if (typeof val === 'object') {
                            traverse(val, key);
                            pushIfNotExists({
                                isArr: true,
                                type: key
                            });
                        } else {
                            pushIfNotExists({
                                isArr: true,
                                type: typeof val
                            });
                        }
                    }
                }
            } else if (typeof value === 'object') {
                pushIfNotExists({
                    isArr: false,
                    type: key
                });
                traverse(value, key);
            } else {
                pushIfNotExists({
                    isArr: false,
                    type: typeof value
                });
            }
        }
    }

    traverse(json);

    let ts = '';

    const escapeForType = (type: string) => {
        if (!type.includes(' ')) return type;
        return type.split(' ').map(x => x[0].toUpperCase() + x.slice(1)).join('');
    }

    const escapeForField = (field: string) => {
        const keywords = ['delete', 'export', 'import', 'in', 'instanceof', 'new', 'typeof', 'void', 'yield'];
        if (keywords.includes(field) || field.includes('-') || field.includes(' ')) {
            return `['${field}']`;
        }

        return field;
    }

    for (const type in typeMap) {
        ts += `export interface ${escapeForType(type)} {\n`;

        for (const field in typeMap[type]) {
            const { types, isOptional } = typeMap[type][field];

            ts += `    ${escapeForField(field)}${isOptional ? '?' : ''}: ${types.map(x => `${escapeForType(x.type)}${x.isArr ? '[]' : ''}`).join(' | ')};\n`;
        }

        ts += '}\n\n';
    }

    return ts;
}

const json = JSON.parse(
    readFileSync('ast.json', 'utf-8')
)

const ts = generateInterfaces({
    inner: json
});

writeFileSync('ast.d.ts', ts);