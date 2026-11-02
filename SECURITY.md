# Security policy

The runtime uses untrusted gameplay events and externally implemented task sinks. Validate entity IDs, event origin, LOS, distance and target ownership in the host engine. Never execute hook callbacks received from untrusted data. The library does not open network connections or write files.

## Supported releases

The latest `main` branch and the latest tagged release receive security review. Older releases are not guaranteed updates.

## Reporting

For a suspected vulnerability, use GitHub's private vulnerability reporting feature on the repository if enabled; otherwise contact Rootcastle Engineering & Innovation through https://rootcastle.com/. Do not disclose exploit details in a public issue before coordination.

Risk note: The C ABI uses opaque pointers; callers must not pass a freed or foreign world pointer. A malicious native plugin within the same process is outside the isolation boundary.
