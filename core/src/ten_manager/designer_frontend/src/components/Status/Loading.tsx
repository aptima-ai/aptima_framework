//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import { LoaderCircleIcon } from "lucide-react";

import { cn } from "@/lib/utils";

export function SpinnerLoading(props: { className?: string }) {
  const { className } = props;

  return (
    <div className={cn("flex items-center justify-center", className)}>
      <LoaderCircleIcon className="animate-spin" />
    </div>
  );
}
