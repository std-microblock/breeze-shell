const fs = require('fs');
const spawn = require('child_process').spawn;
const path = require('path');

spawn('cmd', ['/c', 'tsc'], { stdio: 'inherit' });

// copy .d.ts to dist recursively
const listFile = path => {
    const files = fs.readdirSync(path);
    let result = [];
    files.forEach(file => {
        const fullPath = `${path}/${file}`;
        if (fs.statSync(fullPath).isDirectory()) {
            result = result.concat(listFile(fullPath));
        } else if (file.endsWith('.d.ts')) {
            result.push(fullPath);
        }
    });
    return result;
}

const dtsFiles = listFile('./src');
dtsFiles.forEach(file => {
    const dest = file.replace('./src', './dist/types');
    fs.mkdirSync(path.dirname(dest), { recursive: true });
    fs.copyFileSync(file, dest);
});

fs.mkdirSync('./dist/types', { recursive: true });
fs.copyFileSync('../binding_types.d.ts', './dist/types/binding_types.d.ts');