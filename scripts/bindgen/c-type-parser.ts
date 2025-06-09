
export interface CTypeNode {
    function: boolean;
    template: boolean;
    type: string;
    argsTemplate: CTypeNode[];
    argsFunc: CTypeNode[];
}

export class CTypeParser {
    tokens: string[] = [];
    cursor = 0;

    lex(str: string) {
        this.tokens = []
        let current = ''

        const BREAK_TOKENS = [' ', '(', ')', ',', '<', '>'];

        for (let i = 0; i < str.length; i++) {
            const c = str[i];
            if (BREAK_TOKENS.includes(c)) {
                if (current.length > 0) {
                    this.tokens.push(current);
                    current = '';
                }
                if (c !== ' ') {
                    this.tokens.push(c);
                }
            } else {
                current += c;
            }
        }

        if (current.length > 0) {
            this.tokens.push(current);
        }
    }

    parse(str: string | null = null, typeTransformer: (orig: string) => string = (orig) => orig): CTypeNode {
        if (str) {
            this.lex(str)
            this.cursor = 0
        }

        const type: CTypeNode = {
            argsFunc: [],
            argsTemplate: [],
            function: false,
            template: false,
            type: '',
        }

        do {
            const parseCommaList = (arr) => {
                do {
                    const res = this.parse(null, typeTransformer);
                    if (res)
                        arr.push(res)
                } while (this.eat(','));
            }

            if (this.eat('(')) {
                type.function = true;
                if (!this.next(')'))
                    parseCommaList(type.argsFunc);
                this.eat(')', true)
            } else if (this.eat('<')) {
                type.template = true;
                if (!this.next('>'))
                    parseCommaList(type.argsTemplate)
                this.eat('>', true)
            } else {
                type.type = typeTransformer(this.tokens[this.cursor])
                this.cursor++;
            }

            if (this.next('(') || this.next('<')) {
                continue;
            }

            break;
        } while (true);

        return type
    }

    eat(token, force = false) {
        if (this.next(token)) {
            this.cursor++;
            return true;
        } else {
            if (force) throw new Error(`Excepted: ${token}, found ${this.next()} in ${this.tokens.join(' ')
                }`)
        }
        return false
    }

    next(token: string | null = null) {
        if (this.tokens[this.cursor] === token || !token) {
            return this.tokens[this.cursor];
        }
        return false
    }

    formatToC(node: CTypeNode) {
        let str = node.type;
        if (node.template) {
            str += '<' + node.argsTemplate.map(a => this.formatToC(a)).join(', ') + '>'
        }
        if (node.function) {
            str += '(' + node.argsFunc.map(a => this.formatToC(a)).join(', ') + ')'
        }
        return str
    }

    formatToTypeScript(node: CTypeNode, namespace: string = ''): string {
        if (node.type.startsWith(namespace))
            node.type = node.type.slice(namespace.length);
        node.type = node.type.split('::').filter(Boolean).join('.');
        const typeMap = {
            'int': 'number',
            'float': 'number',
            'double': 'number',
            'std.string': 'string',
            'std.vector': 'Array',
            'bool': 'boolean',
        }

        let tsBasicType = (typeMap[node.type] ?? node.type) + (node.template ? '<' + node.argsTemplate.map(a => this.formatToTypeScript(a, namespace)).join(', ') + '>' : '')

        const ignoreTypes = ['std.variant', 'std.shared_ptr', 'std.function']
        if (
            ignoreTypes.includes(node.type)
        ) {
            tsBasicType = node.argsTemplate.map(a => this.formatToTypeScript(a, namespace)).join(' | ')
        } else if (node.type === 'std.optional') {
            tsBasicType = `${this.formatToTypeScript(node.argsTemplate[0], namespace)} | undefined`
        } else if (node.type === 'std.pair' || node.type === 'std.tuple') {
            tsBasicType = node.argsTemplate.map(a => this.formatToTypeScript(a, namespace)).join(', ')
            tsBasicType = `[${tsBasicType}]`
        }

        if (node.function) {
            return `((${node.argsFunc.map(a => this.formatToTypeScript(a, namespace)).map((v, i) => `arg${i + 1}: ${v}`).join(', ')}) => ${tsBasicType})`
        }

        return tsBasicType;
    }
}

export const cTypeToTypeScript = (str: string, namespace: string) => {
    const parser = new CTypeParser();
    parser.lex(str);
    const res = parser.parse()
    return parser.formatToTypeScript(res, namespace);
}


// Example usage:

// const parser = new CTypeParser();
// const res = parser.parse('std::function<void()>(std::function<void(mb_shell::js::menu_info_basic_js)>)')
// console.log(JSON.stringify(res, null, 2), parser.formatToTypeScript(res))