const Module = require('./crc32.js');

const ready = new Promise((resolve) => {
	Module.onRuntimeInitialized = resolve;
});
exports.ready = ready;

function crc32_combine(crc1, crc2, len2) {
	// 🤖-explanation: 
	// B/c the WASM crc32_combine, only accepts len as a 32-bit integer,
	// we need to reduce the part's len mod T before passing it to it.
	// The CRC32 shift matrix M has period T = 2^32-1 (primitive polynomial),
	// so crc32_combine(crc1, crc2, n) == crc32_combine(crc1, crc2, n mod T).
	const CRC32_PERIOD_NUMBER = 0xffffffff; // T = 2^32-1
	if (len2 > CRC32_PERIOD_NUMBER) {
		// len2 is a 64-bit integer, but crc32_combine only accepts 32-bit integers.
		len2 = len2 % CRC32_PERIOD_NUMBER;
		if (len2 === 0) {
			// Special case: if n mod T = 0 (and n > 0), use T instead of 0 to avoid
			// the len == 0 early-return in crc32_combine that would ignore crc2.
			len2 = CRC32_PERIOD_NUMBER;
		}
	}

	const value = Module.ccall('crc32_combine', 'number', ['number', 'number', 'number'], [crc1, crc2, len2]);
	// value is a signed number here even if it is defined as an uint32_t in crc32.c
	const buffer = Buffer.alloc(4);
	buffer.writeInt32LE(value);
	return buffer;
}
exports.crc32_combine = crc32_combine;

const message = "The argument should be an Array of at least 2 Objects with 'crc' and 'len' keys";
exports.message = message;

function crc32_combine_multi(crcs) {
	if (!Array.isArray(crcs)) {
		throw new Error(message);
	}
	if (crcs.length < 2) {
		throw new Error(message);
	}
	let { crc: result, len: intLength } = crcs[0];
	for (const { crc, len } of crcs.slice(1)) {
		if (!Number.isInteger(crc) || !Number.isInteger(len)) {
			throw new Error(message);
		}
		result = crc32_combine(result, crc, len).readUInt32LE();
		intLength += len;
	}
	const combinedCrc32 = Buffer.alloc(4);
	combinedCrc32.writeUInt32LE(result);
	return { combinedCrc32, intLength };
}
exports.crc32_combine_multi = crc32_combine_multi;
