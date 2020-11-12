const assert = require('assert');
const crc32 = require('buffer-crc32');

const combine = require('../');

const foo = Buffer.from('foo');
const bar = Buffer.from('bar');
const baz = Buffer.from('baz');

const fooCrc32 = crc32(foo); // <Buffer 8c 73 65 21>
const barCrc32 = crc32(bar); // <Buffer 76 ff 8c aa>
const bazCrc32 = crc32(baz); // <Buffer 78 24 04 98>

const foobar = Buffer.from('foobar');
const foobarCrc32 = crc32(foobar); // <Buffer 9e f6 1f 95>

const foobarbaz = Buffer.from('foobarbaz');
const foobarbazCrc32 = crc32(foobarbaz); // <Buffer 1a 78 27 aa>

describe('combine crc32', () => {
	describe('crc32_combine', () => {
		it('should combine 2 crc32 values', () => {
			const foobarCrc32Combined = combine.crc32_combine(
				fooCrc32.readUInt32BE(), 
				barCrc32.readUInt32BE(), 
				bar.length,
			); // <Buffer 95 1f f6 9e> (Little Endian)
			assert.equal(foobarCrc32Combined.readUInt32LE(), foobarCrc32.readUInt32BE());
		});
	});

	describe('crc32_combine_multi', () => {
		it('should combine 2 crc32 values', () => {
			const foobarCrc32Combined = combine.crc32_combine_multi([
				{crc: fooCrc32.readUInt32BE(), len: foo.length},
				{crc: barCrc32.readUInt32BE(), len: bar.length},
			]); 
			assert.equal(foobarCrc32Combined.intLength, foobar.length);
			assert.equal(foobarCrc32Combined.combinedCrc32.readUInt32LE(), foobarCrc32.readUInt32BE());
		});

		it('should combine 3 crc32 values', () => {
			const foobarbazCrc32Combined = combine.crc32_combine_multi([
				{crc: fooCrc32.readUInt32BE(), len: foo.length},
				{crc: barCrc32.readUInt32BE(), len: bar.length},
				{crc: bazCrc32.readUInt32BE(), len: baz.length},
			]); 
			assert.equal(foobarbazCrc32Combined.intLength, foobarbaz.length);
			assert.equal(foobarbazCrc32Combined.combinedCrc32.readUInt32LE(), foobarbazCrc32.readUInt32BE());
		});
	});
});
