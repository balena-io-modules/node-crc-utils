Node CRC32 Utils
==============

Combines two or more CRC32 checksums into new one.

##How to install:

```
npm install @balena/node-crc-utils
```

##How to build:

Install emscripten, clone this repo then
```
npm run build
```

##Example:

```javascript
const crcUtils = require('@balena/node-crc-utils');

// for crc32 checksum use lib: https://github.com/brianloveswords/buffer-crc32/
const crc32 = require('buffer-crc32');

const foo = Buffer.from('foo');
const bar = Buffer.from('bar');

const fooCrc32 = crc32(foo); // <Buffer 8c 73 65 21>
const barCrc32 = crc32(bar); // <Buffer 76 ff 8c aa>
 
const foobar = Buffer.from('foobar');
const foobarCrc32 = crc32(foobar);

crcUtils.ready.then(() => {
	const foobarCrc32Combined = crcUtils.crc32_combine(
		fooCrc32.readUInt32BE(0), 
		barCrc32.readUInt32BE(0), 
		bar.length
	); 

	// CRC32 are the same but Endianness is prepared for GZIP format
	console.log(foobarCrc32);         // <Buffer 9e f6 1f 95>
	console.log(foobarCrc32Combined); // <Buffer 95 1f f6 9e>
});
```
