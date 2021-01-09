Linux file data deduplication for Node.js
===

Linux VFS provides the `FIDEDUPERANGE` `ioctl` to deduplicate byte ranges in multiple files.
This package provides facilities for calling this `ioctl`.

Usage
---

The package exports the class `Deduplicator`. For now, this class only provides a single method called
`dedupe`, which is able to deduplicate a single byte range in two files.

```typescript
  import * as fs from "fs";
  import { Deduplicator, getStatusInfo } from "@nanobot248/linux-dedupe";

  const fd1 = fs.openSync("/path/to/file1").fd;
  const off1 = 0;
  const len1 = 1024 * 1024;

  const fd2 = fs.openSync("/path/to/file2").fd;
  const off2 = 0;

  const deduplicator = new Deduplicator();
  deduplicator.dedupe(fd1, off1, len1, fd2, off2).then((result) => {
    console.log("result:", result);
    console.log("status: " + getStatusInfo(result.status));
  }).catch((err) => {
    console.error("error:", err);
  });
```