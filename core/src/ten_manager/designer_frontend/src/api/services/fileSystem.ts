//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import z from "zod";

import {
  makeAPIRequest,
  useCancelableSWR,
  prepareReqUrl,
} from "@/api/services/utils";
import { ENDPOINT_FILE_SYSTEM } from "@/api/endpoints";
import { ENDPOINT_METHOD } from "@/api/endpoints/constant";

// request functions -------------------------------

export const getFileContent = async (path: string) => {
  const encodedPath = encodeURIComponent(path);
  const template = ENDPOINT_FILE_SYSTEM.fileContent.get;
  const req = makeAPIRequest(template, {
    pathParams: {
      path: encodedPath,
    },
  });
  const res = await req;
  return template.responseSchema.parse(res).data;
};

export const putFileContent = async (
  path: string,
  data: { content: string }
) => {
  const encodedPath = encodeURIComponent(path);
  const template = ENDPOINT_FILE_SYSTEM.fileContent[ENDPOINT_METHOD.PUT];
  const req = makeAPIRequest(template, {
    pathParams: {
      path: encodedPath,
    },
    body: data,
  });
  const res = await req;
  return res;
};

export const putBaseDir = async (baseDir: string) => {
  const template = ENDPOINT_FILE_SYSTEM.baseDir[ENDPOINT_METHOD.PUT];
  const req = makeAPIRequest(template, {
    body: { base_dir: baseDir },
  });
  const res = await req;
  return template.responseSchema.parse(res).data;
};

export const getDirList = async (path: string) => {
  const encodedPath = encodeURIComponent(path);
  const template = ENDPOINT_FILE_SYSTEM.dirList[ENDPOINT_METHOD.GET];
  const req = makeAPIRequest(template, {
    pathParams: { path: encodedPath },
  });
  const res = await req;
  return template.responseSchema.parse(res).data;
};

// query hooks -------------------------------

export const useFileContent = (path: string) => {
  const template = ENDPOINT_FILE_SYSTEM.fileContent[ENDPOINT_METHOD.GET];
  const url = prepareReqUrl(template, {
    pathParams: {
      path: encodeURIComponent(path),
    },
  });
  const [{ data, error, isLoading }] = useCancelableSWR<
    z.infer<typeof template.responseSchema>
  >(url, {
    revalidateOnFocus: false,
    refreshInterval: 0,
  });
  return {
    content: data?.data.content,
    error,
    isLoading,
  };
};

export const useDirList = (path: string) => {
  const template = ENDPOINT_FILE_SYSTEM.dirList[ENDPOINT_METHOD.GET];
  const url = prepareReqUrl(template, {
    pathParams: { path: encodeURIComponent(path) },
  });
  const [{ data, error, isLoading, mutate }, controller] = useCancelableSWR<
    z.infer<typeof template.responseSchema>
  >(url, {
    revalidateOnFocus: false,
    refreshInterval: 0,
    errorRetryCount: 0,
  });
  return {
    data: data?.data,
    error,
    isLoading,
    mutate,
    controller,
  };
};
