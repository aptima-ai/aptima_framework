//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//

package ten

//#include "common.h"
import "C"

import (
	"unsafe"
)

func convertCArrayToGoSlice(cArray *C.ten_go_handle_array_t) []handle {
	var array []handle

	arrayPtr := unsafe.Pointer(cArray.array)
	length := int(cArray.size)

	array = unsafe.Slice((*handle)(arrayPtr), length)

	return array
}

func convertCByteArrayToGoSlice(
	cByteArray unsafe.Pointer,
	cByteArrayLen int,
) []byte {
	return unsafe.Slice((*byte)(cByteArray), cByteArrayLen)
}
