# Repository instructions

## Credentials and keys

- Keep real Wi-Fi credentials, ESPHome API encryption keys, OTA passwords, access tokens, and private keys out of Git. Use ESPHome `!secret` references and keep their values in the ignored local `secrets.yaml`. Supply GitHub credentials through the credential manager.
- Use placeholders or synthetic test values in documentation, fixtures, and build checks. Treat generated source, compiled firmware, flash backups, captures, and logs as local artifacts because they can contain expanded secrets. The published RF button codes are intentional protocol examples.
- Before each commit, stage files by name. Run `git diff --cached --name-status` and privately inspect the full staged changes, including new files. Check unfamiliar configuration values, binary files, and generated output before committing. Never paste suspected secrets into chat, review comments, or commit messages.
- Use `git check-ignore -v -- secrets.yaml` to verify the ignore rule and `git ls-files -- secrets.yaml` to check the index. The second command should print nothing. Ignore rules do not protect files already tracked. Never force-add local credential files.
- If a credential is staged, unstage it and replace the tracked value with a reference. If one has already been committed, stop the push and tell the owner privately which credential and path were affected, omitting the value. Coordinate rotation or revocation and any history cleanup with the owner.

## Changing the controller

- Put shared wiring and radio settings in [the hardware package](packages/radio.yaml). For a verified RC Switch device, use [the shared transmit action](packages/send-rc-switch.yaml). Keep device-specific frequencies, codes, and repeat counts with the device configuration.
- Preserve leading zeros in remote codes. Use integer Hz for tuning and include units in logs. Keep the startup guard, finish an active frame before retuning, and return the radio to idle after transmission.
- Preserve Home Assistant device and entity identities when refactoring. Check the effect on existing pairings and automations before renaming a device or changing its API configuration.
- Measure optimizations against a recorded baseline. For capture changes, replay the same input and compare the decoded output as well as the performance metric.

## Testing and Git

- Start with `git status --short --branch` and preserve existing local edits. Keep commits focused so a hardware regression can be traced to one change.
- On a fresh checkout, run `make setup`. Use `make check` for configuration and host regression tests. Run `make verify` for firmware, shared-package, dependency, or build changes. Both targets use isolated test credentials. For documentation-only changes, verify referenced commands and paths and run `git diff --cached --check`.
- Derive expected behavior in tests from independent observations. For radio changes, cover ON and OFF, repeat settings, startup silence, and switching to another frequency and back. If a fixture must change, explain which measurement supports the new expectation.
- In the commit or review summary, identify which checks ran, which behaviors were observed on hardware, and what still needs a physical test.

## Working with hardware

- Identify the USB device and chip before uploading. Show the target, firmware, and overwrite impact, and obtain the operator’s approval. Use 3.3 V for the CC1101 and confirm wiring changes with the operator.
- Start a new remote in receive-only mode. Compare quiet background with several independent presses per labeled button. Record the tuned frequency, receiver settings, and repeatable frame or timing pattern before implementing transmission.
- Verify SPI using register reads and write/readback. After an approved controller upload, verify the encrypted local API and have the operator observe the appliance responding. For multiple devices, alternate commands and check both appliances.
