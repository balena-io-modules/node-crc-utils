#include <node.h>
#include <node_buffer.h>
#include <v8.h>
#include <stdio.h>
#include <nan.h>

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

uint32_t crc32_combine(uint32_t crc1, uint32_t crc2, long len2) {
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



using namespace v8;
using namespace node;

NAN_METHOD(crc32_combine) {
	Nan::HandleScope scope;

	if (info.Length() < 3) {
		Nan::ThrowTypeError("Wrong number of arguments");
		return;
	}

	if (!info[0]->IsNumber() || !info[1]->IsNumber() || !info[2]->IsNumber()) {
		Nan::ThrowTypeError("Wrong arguments");
		return;
	}

	auto context = Nan::GetCurrentContext();
	uint32_t combine = crc32_combine(
		info[0]->NumberValue(context).FromJust(), // crc32 #1
		info[1]->NumberValue(context).FromJust(), // crc32 #2
		info[2]->NumberValue(context).FromJust()  // len2
	);

	info.GetReturnValue().Set(Nan::CopyBuffer((char *)&combine, sizeof(uint32_t)).ToLocalChecked());
}

uint32_t getUint32Value(Local<Object> obj, const char* key, Local<Context> context) {
	return Nan::Get(obj, Nan::New(key).ToLocalChecked()).ToLocalChecked()->Uint32Value(context).FromJust();
}

const char* crc32_combine_multi_args_error = "The argument should be an Array of at least 2 Objects with 'crc' and 'len' keys";

NAN_METHOD(crc32_combine_multi) {
	Nan::HandleScope scope;

	if (info.Length() < 1) {
		Nan::ThrowTypeError(crc32_combine_multi_args_error);
		return;
	}

	if (!info[0]->IsArray()) {
		Nan::ThrowTypeError(crc32_combine_multi_args_error);
		return;
	}

	Local<Array> arr = Local<Array>::Cast(info[0]);
	uint32_t arLength = arr->Length();

	if (arLength < 2) {
		Nan::ThrowTypeError(crc32_combine_multi_args_error);
		return;
	}

	auto maybeFirstElementCrc = Nan::Get(arr, 0).ToLocalChecked();
	if (!maybeFirstElementCrc->IsObject()) {
		Nan::ThrowTypeError(crc32_combine_multi_args_error);
		return;
	}
	auto firstElementCrc = maybeFirstElementCrc.As<Object>();
	auto context = Nan::GetCurrentContext();
	auto retCrc = getUint32Value(firstElementCrc, "crc", context);
	auto retLen = getUint32Value(firstElementCrc, "len", context);

	uint32_t n;
	for (n = 1; n < arLength; n++){
		auto maybeObj = Nan::Get(arr, n).ToLocalChecked();
		if (!maybeObj->IsObject()) {
			Nan::ThrowTypeError(crc32_combine_multi_args_error);
			return;
		}
		auto obj = maybeObj.As<Object>();
		auto crc1 = getUint32Value(obj, "crc", context);
		auto len2 = getUint32Value(obj, "len", context);
		retCrc = crc32_combine(retCrc, crc1, len2);
		retLen += len2;
	}

	int length = sizeof(uint32_t);
	Local<Object> crcBuffer = Nan::CopyBuffer((char *)&retCrc, length).ToLocalChecked();

	Local<Object> lengthBuffer = Nan::CopyBuffer((char *)&retLen, length).ToLocalChecked();

	Local<Number> numRetLen = Nan::New<Number>(retLen);

	Local<Object> retValObj = Nan::New<Object>();
	Nan::Set(retValObj, Nan::New("combinedCrc32").ToLocalChecked(), crcBuffer);
	Nan::Set(retValObj, Nan::New("intLength").ToLocalChecked(), numRetLen);
	Nan::Set(retValObj, Nan::New("bufferLength").ToLocalChecked(), lengthBuffer);

	info.GetReturnValue().Set(retValObj);
}

NAN_MODULE_INIT(Initialize) {
	NAN_EXPORT(target, crc32_combine);
	NAN_EXPORT(target, crc32_combine_multi);
}

NODE_MODULE(crc32, Initialize);
