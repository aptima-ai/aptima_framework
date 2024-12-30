//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import useSWR, { type SWRResponse, type SWRConfiguration } from "swr";

import logger from "@/logger";
import type { IReqTemplate } from "@/api/endpoints";
import type { ENDPOINT_METHOD } from "@/api/endpoints/constant";

export const prepareReqUrl = (
  reqTemplate: IReqTemplate<ENDPOINT_METHOD, unknown>,
  opts?: {
    query?: Record<string, string>;
    pathParams?: Record<string, string>;
  }
): string => {
  // 1. prepare url
  let url = reqTemplate.url;
  logger.debug({ scope: "api", module: "utils", data: { url } }, "prepare url");
  // 2. append query params
  if (opts?.query) {
    const searchParams = new URLSearchParams();
    Object.entries(opts.query).forEach(([key, value]) => {
      searchParams.append(key, value);
    });
    url = `${url}?${searchParams.toString()}`;
    logger.debug(
      { scope: "api", module: "utils", data: { url } },
      "append query params"
    );
  }
  // 3. validate path params
  if (reqTemplate.pathParams) {
    const missingParams = reqTemplate.pathParams.filter(
      (param) =>
        opts?.pathParams === undefined || opts.pathParams[param] === undefined
    );
    logger.debug(
      { scope: "api", module: "utils", data: { missingParams } },
      "validate path params"
    );
    if (missingParams.length > 0) {
      logger.error(
        { scope: "api", module: "utils", data: { missingParams } },
        "missing required path parameters"
      );
      throw new Error(
        `Missing required path parameters: ${missingParams.join(", ")}`
      );
    }
  }
  // 4. replace path params
  if (opts?.pathParams) {
    Object.entries(opts.pathParams).forEach(([key, value]) => {
      url = url.replace(`:${key}`, value);
    });
    logger.debug(
      { scope: "api", module: "utils", data: { url } },
      "replace path params"
    );
  }
  return url;
};

/**
 * Parse request template and return a fetch request
 * @param reqTemplate - Request template
 * @param opts - Options
 * @param fetchOpts - Fetch options
 * @returns Fetch request
 */
export const parseReq = <T extends ENDPOINT_METHOD>(
  reqTemplate: IReqTemplate<T, unknown>,
  opts?: {
    query?: Record<string, string>;
    pathParams?: Record<string, string>;
    body?: Record<string, unknown>;
  },
  fetchOpts?: RequestInit
) => {
  const url = prepareReqUrl(reqTemplate, opts);
  // 5. prepare fetch options
  const { headers: inputHeaders, ...restInput } = fetchOpts ?? {};
  const headers = {
    "Content-Type": "application/json",
    ...inputHeaders,
  };
  // 6. return fetch
  logger.debug(
    {
      scope: "api",
      module: "utils",
      data: {
        url,
        headers,
        method: reqTemplate.method,
        body: opts?.body,
      },
    },
    "prepare fetch request"
  );
  return fetch(url, {
    headers,
    ...restInput,
    method: reqTemplate.method,
    ...(opts?.body ? { body: JSON.stringify(opts.body) } : {}),
  });
};

export const makeAPIRequest = async <T extends ENDPOINT_METHOD, R = unknown>(
  reqTemplate: IReqTemplate<T, R>,
  opts?: {
    query?: Record<string, string>;
    pathParams?: Record<string, string>;
    body?: Record<string, unknown>;
  }
): Promise<R> => {
  const req = parseReq(reqTemplate, opts);
  const res = await req;
  if (!res.ok) {
    logger.error(
      { scope: "api", module: "request", data: { res } },
      "request failed"
    );
    throw new Error(`Request failed: ${res.status}`);
  }
  const data = await res.json();
  logger.debug(
    { scope: "api", module: "request", data: { data } },
    "request success"
  );
  return data;
};

// https://github.com/vercel/swr/discussions/2330#discussioncomment-4460054
export function useCancelableSWR<T>(
  key: string,
  opts?: SWRConfiguration
): [SWRResponse<T>, AbortController] {
  logger.debug(
    { scope: "api", module: "swr", data: { key, opts } },
    "preparing SWR request"
  );
  const controller = new AbortController();
  return [
    useSWR(
      key,
      (url: string) =>
        fetch(url, { signal: controller.signal }).then((res) => res.json()),
      {
        // revalidateOnFocus: false,
        errorRetryCount: 3,
        refreshInterval: 1000 * 60,
        // dedupingInterval: 30000,
        // focusThrottleInterval: 60000,
        ...opts,
      }
    ),
    controller,
  ];
  // to use it:
  // const [{ data }, controller] = useCancelableSWR('/api')
  // ...
  // controller.abort()
}
