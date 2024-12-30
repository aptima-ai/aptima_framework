//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import { makeAPIRequest } from "@/api/services/utils";
import { ENDPOINT_ADDONS } from "@/api/endpoints";
import { ENDPOINT_METHOD } from "@/api/endpoints/constant";

export const getExtensionAddonByName = async (name: string) => {
  const template = ENDPOINT_ADDONS.extensionByName[ENDPOINT_METHOD.GET];
  const req = makeAPIRequest(template, {
    pathParams: {
      name,
    },
  });
  const res = await req;
  return template.responseSchema.parse(res).data;
};
