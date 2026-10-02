# Security

## Scope

Hax loads the official HaxBall website in a native CEF runtime. Remote content must never receive a privileged native JavaScript bridge, Node.js integration, arbitrary file access, or direct access to process-control APIs.

## Current hardening boundary

The current development build:

- uses HTTPS for the official HaxBall page;
- exposes no privileged JS/native bridge to the page;
- stores browser cache and measured machine profiles under LocalAppData;
- applies only process-local performance policies;
- verifies downloaded calibration tooling by SHA-256.

The current CEF host uses sandbox-disabled executable mode. This is suitable for development and measurement, but it remains an explicit release-hardening item. A signed public release should migrate to CEF's current Windows bootstrap/sandbox packaging before being presented as a security-hardened production application.

## Reporting

Use GitHub's private security-advisory/reporting mechanism where available. Include the affected commit, Windows version, CEF version, reproduction steps, and whether malicious remote content or local access is required.
