const esbuild = require('esbuild');

async function build() {
  try {
    await esbuild.build({
      entryPoints: ['src/entry.ts'],
      bundle: true,
      format: 'esm',
      platform: 'browser',
      outfile: '../script.js',
      external: ['mshell'],
      minify: true,
      banner: {
        js: `// Generated from project in ./ts
// Don't edit this file directly!

import * as __mshell from "mshell";
const setTimeout = __mshell.infra.setTimeout;
const clearTimeout = __mshell.infra.clearTimeout;
`
      },
      alias: {
        react: 'react',
        'react-reconciler': 'react-reconciler',
      }
    });
    console.log('Build completed successfully');
  } catch (error) {
    console.error('Build failed:', error);
    process.exit(1);
  }
}

build();