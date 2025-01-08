
export interface CTypeNode {
    function: boolean;
    template: boolean;
    type: string;
    args: CTypeNode[];
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

    parse() {

        const type: CTypeNode = {
            args: [],
            function: false,
            template: false,
            type: '',
        }

        do {
            const parseCommaList = () => {
                do {
                    const res = this.parse();
                    if (res)
                        type.args.push(res)
                } while (this.eat(','));
            }

            if (this.eat('(')) {
                type.function = true;
                if (!this.next(')'))
                    parseCommaList();
                this.eat(')', true)
            } else if (this.eat('<')) {
                type.template = true;
                if (!this.next('>'))
                    parseCommaList()
                this.eat('>', true)
            } else {
                type.type = this.tokens[this.cursor]
                this.cursor++;
                if (this.next('(') || this.next('<')) {
                    continue;
                }
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
            str += '<' + node.args.map(a => this.formatToC(a)).join(', ') + '>'
        }
        if (node.function) {
            str += '(' + node.args.map(a => this.formatToC(a)).join(', ') + ')'
        }
        return str
    }

    formatToTypeScript(node: CTypeNode) {
        node.type = node.type.split('::').pop() ?? node.type
        const typeMap = {
            'int': 'number',
            'float': 'number',
            'double': 'number',
            'string': 'string',
            'vector': 'Array',
            'bool': 'boolean',
        }

        const ignoreTypes = ['variant', 'shared_ptr', 'function']
        if (
            ignoreTypes.includes(node.type)
        ) {
            return node.args.map(a => this.formatToTypeScript(a)).join(' | ')
        } else if (node.type === 'optional') {
            return `${this.formatToTypeScript(node.args[0])} | undefined`
        }

        if (node.function) {
            return `(${node.args.map(a => this.formatToTypeScript(a)).map((v, i) => `arg${i + 1}: ${v}`).join(', ')}) => ${typeMap[node.type] || node.type}`
        }

        return (typeMap[node.type] ?? node.type) + (node.template ? '<' + node.args.map(a => this.formatToTypeScript(a)).join(', ') + '>' : '')
    }
}

export const cTypeToTypeScript = (str: string) => {
    const parser = new CTypeParser();
    parser.lex(str);
    const res = parser.parse()
    return parser.formatToTypeScript(res);
}

// const parser = new CTypeParser();
// parser.lex('int')
// console.log(parser.parse())
