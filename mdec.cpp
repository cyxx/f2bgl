
#include <math.h>
#include "mdec.h"
#include "mdec_coeffs.h"

struct BitStream { // most significant 16 bits
	const uint8_t *_src;
	uint16_t _bits;
	int _len;
	const uint8_t *_end;

	BitStream(const uint8_t *src, int size)
		: _src(src), _len(0), _end(src + size) {
	}

	bool endOfStream() const {
		return _src >= _end && _len == 0;
	}

	void refill() {
		assert(_len == 0);
		assert(_src < _end);
		_bits = READ_LE_UINT16(_src); _src += 2;
		_len = 16;
	}

	int getBits(int len) {
		int value = 0;
		while (len != 0) {
			if (_len == 0) {
				refill();
			}
			const int count = (len < _len) ? len : _len;
			value <<= count;
			value |= _bits >> (16 - count);
			_bits <<= count;
			_len -= count;
			len -= count;
		}
		return value;
	}
	int getSignedBits(int len) {
		const int shift = 32 - len;
		int32_t value = getBits(len);
		return (value << shift) >> shift;
	}
	bool getBit() {
		if (_len == 0) {
			refill();
		}
		const bool bit = (_bits & 0x8000) != 0;
		_bits <<= 1;
		--_len;
		return bit;
	}
};

static int readDC(BitStream *bs, int version, const DcHuff *dcHuffTree, int *prevDC) {
	if (version == 2) {
		return bs->getSignedBits(10);
	}
	assert(version == 3);
	int node = 0;
	while (dcHuffTree[node].value < 0 && !bs->endOfStream()) {
		if (bs->getBit()) {
			node = dcHuffTree[node].right;
		} else {
			node = dcHuffTree[node].left;
		}
	}
	const int count = dcHuffTree[node].value;
	int dc = 0;
	if (count != 0) {
		if (bs->getBit()) {
			dc = bs->getBits(count - 1) + (1 << (count - 1));
		} else {
			dc = bs->getBits(count - 1) - ((1 << count) - 1);
		}
		*prevDC += dc * 4;
	}
	return *prevDC;
}

static void readAC(BitStream *bs, int *coefficients) {
	int count = 0;
	int node = 0;
	while (!bs->endOfStream()) {
		const uint16_t value = _acHuffTree[node].value;
		switch (value) {
		case kAcHuff_EscapeCode: {
				const int zeroes = bs->getBits(6);
				count += zeroes + 1;
				assert(count < 63);
				coefficients += zeroes;
				*coefficients++ = bs->getSignedBits(10);
			}
			break;
		case kAcHuff_EndOfBlock:
			return;
		case 0:
			if (bs->getBit()) {
				node = _acHuffTree[node].right;
			} else {
				node = _acHuffTree[node].left;
			}
			continue;
		default: {
				const int zeroes = value >> 8;
				count += zeroes + 1;
				assert(count < 63);
				coefficients += zeroes;
				const int nonZeroes = value & 255;
				*coefficients++ = bs->getBit() ? -nonZeroes : nonZeroes;
			}
			break;
		}
		node = 0; // root
	}
}

static const uint8_t _zigZagTable[8 * 8] = {
	 0,  1,  5,  6, 14, 15, 27, 28,
	 2,  4,  7, 13, 16, 26, 29, 42,
	 3,  8, 12, 17, 25, 30, 41, 43,
	 9, 11, 18, 24, 31, 40, 44, 53,
	10, 19, 23, 32, 39, 45, 52, 54,
	20, 22, 33, 38, 46, 51, 55, 60,
	21, 34, 37, 47, 50, 56, 59, 61,
	35, 36, 48, 49, 57, 58, 62, 63
};

static const uint8_t _quantizationTable[8 * 8] = {
	 2, 16, 19, 22, 26, 27, 29, 34,
	16, 16, 22, 24, 27, 29, 34, 37,
	19, 22, 26, 27, 29, 34, 34, 38,
	22, 22, 26, 27, 29, 34, 37, 40,
	22, 26, 27, 29, 32, 35, 40, 48,
	26, 27, 29, 32, 35, 40, 48, 58,
	26, 27, 29, 34, 38, 46, 56, 69,
	27, 29, 35, 38, 46, 56, 69, 83
};

static void dequantizeBlock(int *coefficients, float *block, int scale) {
	block[0] = coefficients[0] * _quantizationTable[0]; // DC
	for (int i = 1; i < 8 * 8; i++) {
		block[i] = coefficients[_zigZagTable[i]] * _quantizationTable[i] * scale / 8.f;
	}
}

static const double _idct8x8[8][8] = {
	{ 0.353553390593274,  0.490392640201615,  0.461939766255643,  0.415734806151273,  0.353553390593274,  0.277785116509801,  0.191341716182545,  0.097545161008064 },
	{ 0.353553390593274,  0.415734806151273,  0.191341716182545, -0.097545161008064, -0.353553390593274, -0.490392640201615, -0.461939766255643, -0.277785116509801 },
	{ 0.353553390593274,  0.277785116509801, -0.191341716182545, -0.490392640201615, -0.353553390593274,  0.097545161008064,  0.461939766255643,  0.415734806151273 },
	{ 0.353553390593274,  0.097545161008064, -0.461939766255643, -0.277785116509801,  0.353553390593274,  0.415734806151273, -0.191341716182545, -0.490392640201615 },
	{ 0.353553390593274, -0.097545161008064, -0.461939766255643,  0.277785116509801,  0.353553390593274, -0.415734806151273, -0.191341716182545,  0.490392640201615 },
	{ 0.353553390593274, -0.277785116509801, -0.191341716182545,  0.490392640201615, -0.353553390593273, -0.097545161008064,  0.461939766255643, -0.415734806151273 },
	{ 0.353553390593274, -0.415734806151273,  0.191341716182545,  0.097545161008064, -0.353553390593274,  0.490392640201615, -0.461939766255643,  0.277785116509801 },
	{ 0.353553390593274, -0.490392640201615,  0.461939766255643, -0.415734806151273,  0.353553390593273, -0.277785116509801,  0.191341716182545, -0.097545161008064 }
};

static void idct(float *dequantData, float *result) {
	float tmp[8 * 8];
	// 1D IDCT rows
	for (int y = 0; y < 8; y++) {
		for (int x = 0; x < 8; x++) {
			float p = 0;
			for (int i = 0; i < 8; ++i) {
				p += dequantData[i] * _idct8x8[x][i];
			}
			tmp[y + x * 8] = p;
		}
		dequantData += 8;
	}
	// 1D IDCT columns
	for (int x = 0; x < 8; x++) {
		const float *u = tmp + x * 8;
		for (int y = 0; y < 8; y++) {
			float p = 0;
			for (int i = 0; i < 8; ++i) {
				p += u[i] * _idct8x8[y][i];
			}
                        result[y * 8 + x] = p;
		}
	}
}

static void decodeBlock(BitStream *bs, int x8, int y8, uint8_t *dst, int dstPitch, int scale, int version, bool luma, int *prevDC) {
	int coefficients[8 * 8];
	memset(coefficients, 0, sizeof(coefficients));
	coefficients[0] = readDC(bs, version, luma ? _dcLumaHuffTree : _dcChromaHuffTree, prevDC);
	readAC(bs, &coefficients[1]);

	float dequantData[8 * 8];
	dequantizeBlock(coefficients, dequantData, scale);

	float idctData[8 * 8];
	idct(dequantData, idctData);

	dst += (y8 * dstPitch + x8) * 8;
	for (int y = 0; y < 8; y++) {
		for (int x = 0; x < 8; x++) {
			const int val = (int)round(idctData[y * 8 + x]); // (-128,127) range
			dst[x] = (val < -128) ? 0 : ((val > 127) ? 255 : (128 + val));
		}
		dst += dstPitch;
	}
}

enum {
	kOutputPlaneY = 0,
	kOutputPlaneCb = 1,
	kOutputPlaneCr = 2
};

int decodeMDEC(const uint8_t *src, int len, int w, int h, void *userdata, void (*output)(const MdecOutput *, void *)) {
	BitStream bs(src, len);
	bs.getBits(16);
	const uint16_t vlc = bs.getBits(16);
	assert(vlc == 0x3800);
	const uint16_t qscale = bs.getBits(16);
	const uint16_t version = bs.getBits(16);
	// fprintf(stdout, "mdec qscale %d version %d\n", qscale, version);
	assert(version == 2 || version == 3);

	const int blockW = (w + 15) / 16;
	const int blockH = (h + 15) / 16;

	MdecOutput mdecOut;
	mdecOut.w = blockW * 16;
	mdecOut.h = blockH * 16;
	mdecOut.planes[kOutputPlaneY].ptr = (uint8_t *)malloc(blockW * 16 * blockH * 16);
	mdecOut.planes[kOutputPlaneY].pitch = blockW * 16;
	mdecOut.planes[kOutputPlaneCb].ptr = (uint8_t *)malloc(blockW * 8 * blockH * 8);
	mdecOut.planes[kOutputPlaneCb].pitch = blockW * 8;
	mdecOut.planes[kOutputPlaneCr].ptr = (uint8_t *)malloc(blockW * 8 * blockH * 8);
	mdecOut.planes[kOutputPlaneCr].pitch = blockW * 8;

	int prevDC[3];
	prevDC[0] = prevDC[1] = prevDC[2] = 0;

	for (int x = 0; x < blockW; ++x) {
		for (int y = 0; y < blockH; ++y) {
			decodeBlock(&bs, x, y, mdecOut.planes[kOutputPlaneCr].ptr, mdecOut.planes[kOutputPlaneCr].pitch, qscale, version, false, &prevDC[kOutputPlaneCr]);
			decodeBlock(&bs, x, y, mdecOut.planes[kOutputPlaneCb].ptr, mdecOut.planes[kOutputPlaneCb].pitch, qscale, version, false, &prevDC[kOutputPlaneCb]);
			decodeBlock(&bs, 2 * x,     2 * y,     mdecOut.planes[kOutputPlaneY].ptr, mdecOut.planes[kOutputPlaneY].pitch, qscale, version, true, &prevDC[kOutputPlaneY]);
			decodeBlock(&bs, 2 * x + 1, 2 * y,     mdecOut.planes[kOutputPlaneY].ptr, mdecOut.planes[kOutputPlaneY].pitch, qscale, version, true, &prevDC[kOutputPlaneY]);
			decodeBlock(&bs, 2 * x,     2 * y + 1, mdecOut.planes[kOutputPlaneY].ptr, mdecOut.planes[kOutputPlaneY].pitch, qscale, version, true, &prevDC[kOutputPlaneY]);
			decodeBlock(&bs, 2 * x + 1, 2 * y + 1, mdecOut.planes[kOutputPlaneY].ptr, mdecOut.planes[kOutputPlaneY].pitch, qscale, version, true, &prevDC[kOutputPlaneY]);
		}
	}

	output(&mdecOut, userdata);

	free(mdecOut.planes[kOutputPlaneY].ptr);
	free(mdecOut.planes[kOutputPlaneCb].ptr);
	free(mdecOut.planes[kOutputPlaneCr].ptr);

	return 0;
}
