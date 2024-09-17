//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//

package ten

type LogLevel int32

const (
	LogLevelVerbose = 1
	LogLevelDebug   = 2
	LogLevelInfo    = 3
	LogLevelWarn    = 4
	LogLevelError   = 5
	LogLevelFatal   = 6
)
