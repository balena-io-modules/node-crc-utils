#include <node.h>
#include <node_buffer.h>
#include <v8.h>
#include <stdio.h>
#include <nan.h>

#define GF2_DIM 32

unsigned long gf2_matrix_times(unsigned long *mat, unsigned long vec) {
    unsigned long sum = 0;
    while (vec) {
        if (vec & 1) {
            sum ^= *mat;
        }
        vec >>= 1;
        mat++;
    }
    return sum;
}

void gf2_matrix_square(unsigned long *square, unsigned long *mat) {
    int n;
    for (n = 0; n < GF2_DIM; n++) {
        square[n] = gf2_matrix_times(mat, mat[n]);
    }
}

unsigned long crc32_combine(unsigned long crc1, unsigned long crc2, long len2) {
    int n;
    unsigned long row;
    unsigned long even[GF2_DIM];    /* even-power-of-two zeros operator */
    unsigned long odd[GF2_DIM];     /* odd-power-of-two zeros operator */

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
	unsigned long combine = crc32_combine(
		info[0]->NumberValue(context).ToChecked(), // crc32 #1
		info[1]->NumberValue(context).ToChecked(), // crc32 #2
		info[2]->NumberValue(context).ToChecked()  // len2
	);

	info.GetReturnValue().Set(Nan::CopyBuffer((char *)&combine, sizeof(unsigned long)).ToLocalChecked());
}

NAN_METHOD(crc32_combine_multi) {
	Nan::HandleScope scope;

	if (info.Length() < 1) {
		Nan::ThrowTypeError("Wrong number of arguments");
		return;
	}

	if (!info[0]->IsArray()) {
		Nan::ThrowTypeError("Wrong arguments");
		return;
	}

	Local<Array> arr = Local<Array>::Cast(info[0]);
	uint32_t arLength = arr->Length();

	if (arLength < 2) {
		Nan::ThrowTypeError("Array too small. I need min 2 elements");
		return;
	}

	Local<Object> firstElementCrc = Local<Object>::Cast(arr->Get(0));
	auto context = Nan::GetCurrentContext();
	unsigned int retCrc = firstElementCrc->Get(Nan::New("crc").ToLocalChecked())->Uint32Value(context).ToChecked();
	unsigned int retLen = firstElementCrc->Get(Nan::New("len").ToLocalChecked())->Uint32Value(context).ToChecked();

	uint32_t n;
	for (n = 1; n < arLength; n++){
		Local<Object> obj = Local<Object>::Cast(arr->Get(n));
		unsigned long crc1 = obj->Get(Nan::New("crc").ToLocalChecked())->Uint32Value(context).ToChecked();
		unsigned long len2 = obj->Get(Nan::New("len").ToLocalChecked())->Uint32Value(context).ToChecked();
		retCrc = crc32_combine(retCrc, crc1, len2);
		retLen += len2;
	}

	int length = sizeof(unsigned long);
	Local<Object> crcBuffer = Nan::CopyBuffer((char *)&retCrc, length).ToLocalChecked();

	Local<Object> lengthBuffer = Nan::CopyBuffer((char *)&retLen, length).ToLocalChecked();

	Local<Number> numRetLen = Nan::New<Number>(retLen);

	Local<Object> retValObj = Nan::New<Object>();
	retValObj->Set(Nan::New("combinedCrc32").ToLocalChecked(), crcBuffer);
	retValObj->Set(Nan::New("intLength").ToLocalChecked(), numRetLen);
	retValObj->Set(Nan::New("bufferLength").ToLocalChecked(), lengthBuffer);

	info.GetReturnValue().Set(retValObj);
}

NAN_MODULE_INIT(Initialize) {
	NAN_EXPORT(target, crc32_combine);
	NAN_EXPORT(target, crc32_combine_multi);
}

NODE_MODULE(crc32, Initialize);
