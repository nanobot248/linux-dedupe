/*
   Copyright 2021 Andreas Hubert

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

const bindings = require("bindings");
const dedupe = bindings("linux-dedupe.node");

/**
 * Deduplication was successfull as the two ranges represented the same data.
 */
export const FILE_DEDUPE_RANGE_SAME: number = 0;

/**
 * Deduplication failed because the binary data of two ranges differs.
 */
export const FILE_DEDUPE_RANGE_DIFFERS: number = 1;

/**
 * Used to provide the result of a deduplication attempt.
 */
export interface DeduplicationResult {
  /**
   * The result value of the `ioctl` call.
   */
  ioctlResult: number;

  /**
   * The status of the deduplication. Should be either `FILE_DEDUPE_RANGE_SAME` or `FILE_DEDUPE_RANGE_DIFFERS`.
   * Use `getStatusInfo(status)` to get a description of the value (e.g. for logging).
   */
  status: number;

  /**
   * The number of successfully deduplicated bytes.
   */
  deduplicatedBytes: number;
}

/**
 * Get a description of the status value (e.g. for logging).
 * @param status The status, either `FILE_DEDUPE_RANGE_SAME` or `FILE_DEDUPE_RANGE_DIFFERS`
 */
export function getStatusInfo(status: number) {
  if (status === 0) {
    return "Ranges are the same, dedupe ok.";
  } else if (status === 1) {
    return "Ranges differ, dedupe not possible.";
  } else {
    return "Unknown status value " + status + ".";
  }
}

/**
 * The deduplicator class wraps the deduplication functionality.
 */
export class Deduplicator {

  /**
   * Constructor. At the moment, no initialization is necessary.
   */
  constructor() {
  }

  /**
   * Asynchronously deduplicates the byte ranges in the two files.
   * 
   * @param srcFd The file descriptor of the first file.
   * @param srcOff The offset of the byte range in the first file.
   * @param srcLen The length of the byte range.
   * @param destFd The file descriptor of the second file.
   * @param destOff The offset of the byte range in the second file.
   */
  public async dedupe(srcFd: number, srcOff: number, srcLen: number, destFd: number, destOff: number): Promise<DeduplicationResult> {
    return new Promise((resolve, reject) => {
      const result = dedupe.ioctl_dedupe_range(srcFd, srcOff, srcLen, destFd, destOff, (err: any, ioctlResult: number, status: number, bytes: number) => {
        console.log("dedupe result: err =", err, ", ioctlResult =", ioctlResult, ", status =", status, ", bytes =", bytes);

        if (err != null) {
          reject(err);
        } else {
          resolve({
            ioctlResult: ioctlResult,
            status: status,
            deduplicatedBytes: bytes
          });
        }
      });  
    });
  }
}