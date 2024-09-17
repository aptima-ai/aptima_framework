//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
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
