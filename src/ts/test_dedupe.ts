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

import * as fs from "fs";
import * as path from "path";
import * as crypto from "crypto";

import { Deduplicator, getStatusInfo } from "./index";

async function testDedup(basePath: string) {
  const buf1 = crypto.randomBytes(1024 * 1024);
  const buf2 = crypto.randomBytes(1024 * 1024);
  const buf = crypto.randomBytes(5 * 1024 * 1024);

  const filename1 = path.join(basePath, "testFile1.dat");
  const filename2 = path.join(basePath, "testFile2.dat");

  await fs.promises.writeFile(filename1, buf1);
  await fs.promises.writeFile(filename2, buf2);

  await fs.promises.appendFile(filename1, buf);
  await fs.promises.appendFile(filename2, buf);

  const fd1 = await fs.promises.open(filename1, fs.constants.O_RDONLY);
  const fd2 = await fs.promises.open(filename2, fs.constants.O_RDONLY);
  
  const deduplicator = new Deduplicator();
  
  console.log("trying to dedupe whole file ...");
  let result = await deduplicator.dedupe(fd1.fd, 0, buf1.length + buf.length, fd2.fd, 0);
  console.log("Dedupe result:", result);
  console.log("Dedupe status: " + getStatusInfo(result.status));

  console.log("trying to dedupe only safe range ...");
  result = await deduplicator.dedupe(fd1.fd, buf1.length, buf.length, fd2.fd, buf1.length);
  console.log("Dedupe result:", result);
  console.log("Dedupe status: " + getStatusInfo(result.status));
}

testDedup("/media/backup-wd01/dedupe-test");