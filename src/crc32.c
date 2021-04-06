#include <emscripten.h>
#include <stdint.h>

#define GF2_DIM 32

uint32_t gf2_matrix_times(uint32_t *mat, uint32_t vec) {
	uint32_t sum = 0;
	while (vec) {
		if (vec & 1) {
			sum ^= *mat;
		}
		vec >>= 1;
		mat++;
	}
	return sum;
}

void gf2_matrix_square(uint32_t *square, uint32_t *mat) {
	int n;
	for (n = 0; n < GF2_DIM; n++) {
		square[n] = gf2_matrix_times(mat, mat[n]);
	}
}

uint32_t crc32_combine(uint32_t crc1, uint32_t crc2, unsigned long len2) {
	int n;
	uint32_t row;
	uint32_t even[GF2_DIM];    /* even-power-of-two zeros operator */
	uint32_t odd[GF2_DIM];     /* odd-power-of-two zeros operator */

	/* degenerate case (also disallow negative lengths) */
	if (len2 <= 0) {
		return crc1;
	}

	/* put operator for one zero bit in odd */
	odd[0] = 0xedb88320UL;   /* CRC-32 polynomial */
	row = 1;
	for (n = 1; n < GF2_DIM; n++) {
		odd[n] = row;
		row <<= 1;
	}

	/* put operator for two zero bits in even */
	gf2_matrix_square(even, odd);

	/* put operator for four zero bits in odd */
	gf2_matrix_square(odd, even);

	/* apply len2 zeros to crc1 (first square will put the operator for one
	   zero byte, eight zero bits, in even) */
	do {
		/* apply zeros operator for this bit of len2 */
		gf2_matrix_square(even, odd);
		if (len2 & 1) {
			crc1 = gf2_matrix_times(even, crc1);
		}
		len2 >>= 1;

		/* if no more bits set, then done */
		if (len2 == 0) {
			break;
		}

		/* another iteration of the loop with odd and even swapped */
		gf2_matrix_square(odd, even);
		if (len2 & 1) {
			crc1 = gf2_matrix_times(odd, crc1);
		}
		len2 >>= 1;

		/* if no more bits set, then done */
	} while (len2 != 0);

	/* return combined crc */
	crc1 ^= crc2;
	return crc1;
}
