export const splitIntoLines = (str, maxLen) => {
    const lines = []
    // one chinese char = 2 english char
    const maxLenBytes = maxLen * 2
    for (let i = 0; i < str.length; i) {
        let x = 0;
        let line = str.substr(i, maxLenBytes)
        while (x < maxLen && line.length > x) {
            if (line.charCodeAt(x) > 255) {
                x++
            }

            if (line.charAt(x) === '\n') {
                x++;
                break
            }

            x++
        }
        lines.push(line.substr(0, x).trim())
        i += x
    }

    return lines
}