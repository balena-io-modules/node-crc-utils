const Module = require('./crc32.js');

const ready = new Promise((resolve) => {
	Module.onRuntimeInitialized = resolve;
});
exports.ready = ready;

function crc32_combine(crc1, crc2, len2) {
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
